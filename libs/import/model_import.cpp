#include "model_import.h"
#include "library/model.h"
#include <QCryptographicHash>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <stdexcept>
namespace lmx {
namespace {
void write(const QString &path, const QByteArray &bytes) {
    if (QFile::exists(path)) {
        QFile existing(path);
        if (!existing.open(QIODevice::ReadOnly) || existing.readAll() != bytes)
            throw std::runtime_error("Um arquivo da coleção foi alterado. Use outra pasta de biblioteca.");
        return;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        throw std::runtime_error("Não foi possível salvar o modelo na biblioteca.");
}
} // namespace
ModelImporter::ModelImporter(QObject *parent) : QObject(parent) {
    timeout.setSingleShot(true);
    connect(&timeout, &QTimer::timeout, this, [this] {
        fail(tr("O modelo demorou demais para abrir. Exporte uma versão mais leve."));
        process.kill();
    });
    process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&process, &QProcess::readyReadStandardOutput, this, [this] {
        log += process.readAllStandardOutput();
        if (log.size() > 32768)
            log = log.right(32768);
    });
    connect(&process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            fail(tr("O renderizador não abriu. Reinstale o LibreMax para restaurar o Blender."));
    });
    connect(&process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                timeout.stop();
                if (complete)
                    return;
                if (code != 0 || status != QProcess::NormalExit) {
                    fail(tr("Não foi possível abrir o modelo. Verifique o formato e as texturas.\n%1")
                             .arg(QString::fromUtf8(log.right(2000))));
                    return;
                }
                try {
                    QFile file(temporary.filePath("model.json")), result(temporary.filePath("result.json"));
                    if (!file.open(QIODevice::ReadOnly) || file.size() > 4 * 1024 * 1024 ||
                        !result.open(QIODevice::ReadOnly) || result.size() > 8192)
                        throw std::runtime_error("Modelo convertido incompleto.");
                    auto bytes = file.readAll();
                    auto model = readModel(bytes);
                    const auto details = Json::parse(result.readAll().toStdString());
                    const auto dims = details.at("dimensions").get<std::vector<double>>();
                    if (dims.size() != 3)
                        throw std::runtime_error("Medidas do modelo inválidas.");
                    const auto hash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
                    Asset asset{"import-" + QString::fromLatin1(hash.left(24)),
                                QFileInfo(source).completeBaseName(),
                                tr("Meus modelos"),
                                dims[0],
                                dims[2],
                                dims[1],
                                {{"type", "MeshObject"},
                                 {"material", "white"},
                                 {"modelFile", hash.toStdString() + ".json"},
                                 {"userModel", true},
                                 {"license", "user-provided"},
                                 {"source", QFileInfo(source).fileName().toStdString()},
                                 {"author", "Modelo fornecido pelo usuário"},
                                 {"collection", "user-imports"},
                                 {"parameters",
                                  {{"meshAsset", hash.toStdString()},
                                   {"originalMaterials", true},
                                   {"placement", "floor"}}}},
                                false,
                                bytes,
                                {}};
                    for (const auto &mat : model.value("materials", Json::array()))
                        for (const auto *key : {"baseColorTexture", "roughnessTexture", "normalTexture"}) {
                            if (!mat.contains(key))
                                continue;
                            const auto textureHash = mat.at(key).get<std::string>();
                            QFile texture(temporary.filePath(QString::fromStdString(textureHash) + ".png"));
                            if (!texture.open(QIODevice::ReadOnly) || texture.size() > 4 * 1024 * 1024)
                                throw std::runtime_error("Textura convertida indisponível.");
                            asset.textures[textureHash] = texture.readAll();
                        }
                    Document candidate;
                    Library::attachModel(candidate, asset);
                    candidate.entities.push_back(Library::instantiate(asset, 0, 0));
                    candidate.validate();
                    complete = true;
                    emit ready(asset, details);
                } catch (const std::exception &error) {
                    fail(QString::fromUtf8(error.what()));
                }
            });
}
void ModelImporter::fail(const QString &message) {
    if (complete)
        return;
    complete = true;
    timeout.stop();
    emit failed(message);
}
void ModelImporter::start(const QString &file, const QString &blender, const QString &script) {
    const auto suffix = QFileInfo(file).suffix().toLower();
    if (!temporary.isValid() || !QStringList{"glb", "gltf", "obj", "fbx", "stl", "ply"}.contains(suffix) ||
        QFileInfo(file).size() > 128 * 1024 * 1024) {
        fail(tr("Escolha um modelo GLB, glTF, OBJ, FBX, STL ou PLY com até 128 MB."));
        return;
    }
    source = file;
    timeout.start(120000);
    process.start(blender,
                  {"--background", "--factory-startup", "--disable-autoexec", "--python-exit-code", "1",
                   "--python", script, "--", QFileInfo(file).absoluteFilePath(), temporary.path()});
}
void ModelImporter::cancel() {
    fail(tr("Importação cancelada. O projeto foi preservado."));
    process.kill();
}
void storeUserAsset(Library &library, const QString &directory, const Asset &a) {
    Document candidate;
    Library::attachModel(candidate, a);
    candidate.entities.push_back(Library::instantiate(a, 0, 0));
    candidate.validate();
    if (!QDir().mkpath(directory))
        throw std::runtime_error("Não foi possível criar a pasta de modelos.");
    const auto hash = a.recipe.at("parameters").at("meshAsset").get<std::string>();
    write(QDir(directory).filePath(QString::fromStdString(hash) + ".json"), a.model);
    for (const auto &[key, bytes] : a.textures)
        write(QDir(directory).filePath(QString::fromStdString(key) + ".png"), bytes);
    auto recipe = a.recipe;
    recipe["modelFile"] = hash + ".json";
    recipe["userModel"] = true;
    library.seed(Json::array({{{"id", a.id.toStdString()},
                               {"name", a.name.toStdString()},
                               {"category", a.category.toStdString()},
                               {"width", a.width},
                               {"height", a.height},
                               {"depth", a.depth},
                               {"recipe", recipe},
                               {"license", recipe.value("license", std::string("user-provided"))},
                               {"author", recipe.value("author", std::string("Usuário"))},
                               {"origin", recipe.value("source", std::string("Importação local"))},
                               {"date", QDate::currentDate().toString(Qt::ISODate).toStdString()},
                               {"tags", "importado meus modelos " + a.name.toStdString()}}}));
}
} // namespace lmx
