#include "model_pack.h"
#include "model.h"
#include <QCryptographicHash>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <memory>
#include <set>
#include <stdexcept>
#include <zip.h>
namespace lmx {
namespace {
void save(const QString &path, const QByteArray &bytes) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        throw std::runtime_error("Não foi possível instalar a coleção. Verifique o espaço livre.");
}
void savePayload(const QString &path, const QByteArray &bytes) {
    if (QFile::exists(path)) {
        QFile existing(path);
        if (!existing.open(QIODevice::ReadOnly) || existing.readAll() != bytes)
            throw std::runtime_error("Há um arquivo de modelo alterado na biblioteca.");
        return;
    }
    save(path, bytes);
}
} // namespace
ModelPack readModelPack(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 128 * 1024 * 1024)
        throw std::runtime_error("Coleção indisponível ou grande demais.");
    auto archiveBytes = file.readAll();
    zip_error_t error;
    zip_error_init(&error);
    auto *source = zip_source_buffer_create(archiveBytes.constData(), archiveBytes.size(), 0, &error);
    auto *raw = source ? zip_open_from_source(source, ZIP_RDONLY, &error) : nullptr;
    if (!raw) {
        if (source)
            zip_source_free(source);
        zip_error_fini(&error);
        throw std::runtime_error("A coleção não é um arquivo válido.");
    }
    zip_error_fini(&error);
    std::unique_ptr<zip_t, decltype(&zip_discard)> archive(raw, zip_discard);
    std::map<std::string, QByteArray> files;
    zip_uint64_t total = 0;
    const auto count = zip_get_num_entries(raw, 0);
    if (count < 2 || count > 10000)
        throw std::runtime_error("Quantidade de arquivos da coleção inválida.");
    for (zip_uint64_t i = 0; i < static_cast<zip_uint64_t>(count); ++i) {
        zip_stat_t st;
        zip_stat_init(&st);
        if (zip_stat_index(raw, i, 0, &st) != 0 || !st.name || st.size > 4 * 1024 * 1024 ||
            st.encryption_method != ZIP_EM_NONE)
            throw std::runtime_error("Arquivo da coleção fora do limite.");
        const std::string name = st.name;
        if (!QRegularExpression("^[A-Za-z0-9_-]+\\.(json|png)$")
                 .match(QString::fromStdString(name))
                 .hasMatch() ||
            files.contains(name))
            throw std::runtime_error("Caminho ou arquivo repetido na coleção.");
        total += st.size;
        if (total > 128 * 1024 * 1024)
            throw std::runtime_error("A coleção ocupa mais de 128 MB.");
        std::unique_ptr<zip_file_t, decltype(&zip_fclose)> entry(zip_fopen_index(raw, i, 0), zip_fclose);
        if (!entry)
            throw std::runtime_error("Não foi possível ler a coleção.");
        QByteArray bytes(static_cast<qsizetype>(st.size), Qt::Uninitialized);
        qsizetype offset = 0;
        while (offset < bytes.size()) {
            auto n = zip_fread(entry.get(), bytes.data() + offset, bytes.size() - offset);
            if (n <= 0)
                throw std::runtime_error("Arquivo da coleção truncado.");
            offset += n;
        }
        char extra;
        if (zip_fread(entry.get(), &extra, 1) != 0)
            throw std::runtime_error("Integridade da coleção inválida.");
        files[name] = std::move(bytes);
    }
    auto parse = [&](const char *name) {
        return Json::parse(files.at(name).toStdString(), [](int depth, Json::parse_event_t, Json &) {
            if (depth > 16)
                throw std::runtime_error("Dados da coleção profundos demais.");
            return true;
        });
    };
    auto manifest = parse("manifest.json"), catalog = parse("catalog.json");
    if (manifest.at("schema") != 1 || !catalog.is_array() || catalog.empty() || catalog.size() > 1000)
        throw std::runtime_error("Formato de coleção não suportado.");
    ModelPack pack{
        QString::fromStdString(manifest.at("id")),
        QString::fromStdString(manifest.at("name")),
        manifest.at("version").get<int>(),
        {},
        QString::fromLatin1(QCryptographicHash::hash(archiveBytes, QCryptographicHash::Sha256).toHex())};
    if (!QRegularExpression("^[A-Za-z0-9_-]{1,48}$").match(pack.id).hasMatch() || pack.name.isEmpty() ||
        pack.name.size() > 200 || pack.version < 1)
        throw std::runtime_error("Identidade da coleção inválida.");
    std::set<std::string> expected{"manifest.json", "catalog.json"}, ids;
    for (const auto &j : catalog) {
        auto recipe = j.at("recipe");
        const auto id = j.at("id").get<std::string>();
        if (!QRegularExpression("^[A-Za-z0-9_-]{1,64}$").match(QString::fromStdString(id)).hasMatch() ||
            !ids.insert(id).second || recipe.at("type") != "MeshObject")
            throw std::runtime_error("Identidade ou tipo de modelo da coleção inválido.");
        const auto hash = recipe.at("parameters").at("meshAsset").get<std::string>();
        if (!QRegularExpression("^[a-f0-9]{64}$").match(QString::fromStdString(hash)).hasMatch())
            throw std::runtime_error("Hash do modelo inválido.");
        const auto filename = hash + ".json";
        expected.insert(filename);
        recipe["modelFile"] = filename;
        recipe["userModel"] = true;
        recipe["collection"] = "pack-" + pack.id.toStdString();
        recipe["license"] = j.at("license");
        recipe["author"] = j.at("author");
        recipe["source"] = j.at("origin");
        for (auto key : {"license", "author", "source"})
            if (!recipe.at(key).is_string() || recipe.at(key).get<std::string>().size() > 1024)
                throw std::runtime_error("Procedência do modelo inválida.");
        Asset asset{"pack-" + pack.id + "-" + QString::fromStdString(id),
                    QString::fromStdString(j.at("name")),
                    QString::fromStdString(j.at("category")),
                    j.at("width"),
                    j.at("height"),
                    j.at("depth"),
                    recipe,
                    false,
                    files.at(filename),
                    {}};
        if (recipe.at("parameters").contains("editMeshAsset")) {
            const auto low = recipe.at("parameters").at("editMeshAsset").get<std::string>();
            if (!QRegularExpression("^[a-f0-9]{64}$").match(QString::fromStdString(low)).hasMatch())
                throw std::runtime_error("Hash da malha leve inválido.");
            expected.insert(low + ".json");
            asset.editModel = files.at(low + ".json");
            asset.recipe["lodFile"] = low + ".json";
        }
        const auto model = readModel(asset.model);
        for (const auto &mat : model.value("materials", Json::array()))
            for (const auto *key : {"baseColorTexture", "roughnessTexture", "normalTexture"})
                if (mat.contains(key)) {
                    const auto textureHash = mat.at(key).get<std::string>();
                    if (!QRegularExpression("^[a-f0-9]{64}$")
                             .match(QString::fromStdString(textureHash))
                             .hasMatch())
                        throw std::runtime_error("Hash de textura inválido.");
                    expected.insert(textureHash + ".png");
                    asset.textures[textureHash] = files.at(textureHash + ".png");
                }
        Document candidate;
        Library::attachModel(candidate, asset);
        candidate.entities.push_back(Library::instantiate(asset, 0, 0));
        candidate.validate();
        pack.assets.push_back(std::move(asset));
    }
    if (expected.size() != files.size())
        throw std::runtime_error("A coleção contém arquivos não declarados.");
    return pack;
}
void installModelPack(Library &library, const QString &directory, const ModelPack &pack) {
    const auto receipts = QDir(directory).filePath("collections");
    QDir().mkpath(receipts);
    const auto receipt = QDir(receipts).filePath(pack.id + ".json");
    if (QFile::exists(receipt)) {
        QFile old(receipt);
        if (!old.open(QIODevice::ReadOnly))
            throw std::runtime_error("Histórico da coleção indisponível.");
        auto installed = Json::parse(old.readAll().toStdString());
        if (installed.at("version").get<int>() > pack.version)
            throw std::runtime_error("Uma versão mais recente desta coleção já está instalada.");
        if (installed.at("version") == pack.version && installed.at("sha256") != pack.sha256.toStdString())
            throw std::runtime_error("Esta versão da coleção tem conteúdo diferente. Use uma nova versão.");
    }
    Json entries = Json::array();
    // Validate everything before writing. Content-addressed files preserve previous versions.
    for (const auto &asset : pack.assets) {
        Document candidate;
        Library::attachModel(candidate, asset);
        candidate.entities.push_back(Library::instantiate(asset, 0, 0));
        candidate.validate();
        const auto hash = asset.recipe.at("parameters").at("meshAsset").get<std::string>();
        savePayload(QDir(directory).filePath(QString::fromStdString(hash) + ".json"), asset.model);
        if (!asset.editModel.isEmpty()) {
            const auto low = asset.recipe.at("parameters").at("editMeshAsset").get<std::string>();
            savePayload(QDir(directory).filePath(QString::fromStdString(low) + ".json"), asset.editModel);
        }
        for (const auto &[key, bytes] : asset.textures)
            savePayload(QDir(directory).filePath(QString::fromStdString(key) + ".png"), bytes);
        entries.push_back({{"id", asset.id.toStdString()},
                           {"name", asset.name.toStdString()},
                           {"category", asset.category.toStdString()},
                           {"width", asset.width},
                           {"depth", asset.depth},
                           {"height", asset.height},
                           {"recipe", asset.recipe},
                           {"license", asset.recipe.at("license")},
                           {"author", asset.recipe.at("author")},
                           {"origin", asset.recipe.at("source")},
                           {"tags", "coleção " + pack.name.toStdString() + " " + asset.name.toStdString()},
                           {"date", QDate::currentDate().toString(Qt::ISODate).toStdString()}});
    }
    library.seed(entries);
    save(receipt, QByteArray::fromStdString(Json{{"id", pack.id.toStdString()},
                                                 {"name", pack.name.toStdString()},
                                                 {"version", pack.version},
                                                 {"sha256", pack.sha256.toStdString()},
                                                 {"items", pack.assets.size()}}
                                                .dump(2)));
}
} // namespace lmx
