#include "project_store.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <zip.h>
#ifdef Q_OS_UNIX
#include <fcntl.h>
#include <unistd.h>
#else
#include <io.h>
#endif

namespace lmx {
namespace {
constexpr zip_uint64_t maxEntry = 16 * 1024 * 1024, maxTotal = 64 * 1024 * 1024;
const std::set<std::string> required = {"manifest.json",  "scene.json",    "project.json",
                                        "materials.json", "lighting.json", "cameras.json"};
using Archive = std::unique_ptr<zip_t, decltype(&zip_discard)>;
std::string readRaw(zip_t *archive, zip_uint64_t index) {
    zip_stat_t st;
    zip_stat_init(&st);
    if (zip_stat_index(archive, index, 0, &st) != 0 || st.size > maxEntry ||
        st.encryption_method != ZIP_EM_NONE)
        throw std::runtime_error("Entrada ZIP inválida");
    std::unique_ptr<zip_file_t, decltype(&zip_fclose)> file(zip_fopen_index(archive, index, 0), zip_fclose);
    if (!file)
        throw std::runtime_error("Não foi possível ler ZIP");
    std::string data(static_cast<std::size_t>(st.size), '\0');
    std::size_t readBytes = 0;
    while (readBytes < data.size()) {
        auto n = zip_fread(file.get(), data.data() + readBytes, data.size() - readBytes);
        if (n <= 0)
            throw std::runtime_error("ZIP truncado ou CRC inválido");
        readBytes += static_cast<std::size_t>(n);
    }
    char extra;
    if (zip_fread(file.get(), &extra, 1) != 0)
        throw std::runtime_error("Tamanho ZIP divergente");
    return data;
}
Json read(zip_t *archive, zip_uint64_t index) {
    auto j = Json::parse(readRaw(archive, index), [](int depth, Json::parse_event_t, Json &) {
        if (depth > 64)
            throw std::runtime_error("JSON profundo demais");
        return true;
    });
    if (!j.is_object() && !j.is_array())
        throw std::runtime_error("JSON inválido");
    return j;
}
void atomicWrite(const QString &path, const QByteArray &bytes) {
    QSaveFile f(path);
    f.setDirectWriteFallback(false);
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.flush())
        throw std::runtime_error("Falha ao gravar projeto temporário");
#ifdef Q_OS_UNIX
    if (::fsync(f.handle()) != 0)
        throw std::runtime_error("Falha ao sincronizar projeto");
#else
    if (::_commit(f.handle()) != 0)
        throw std::runtime_error("Falha ao sincronizar projeto");
#endif
    if (!f.commit())
        throw std::runtime_error("Falha no salvamento atômico");
#ifdef Q_OS_UNIX
    auto parent = QFile::encodeName(QFileInfo(path).absolutePath());
    int fd = ::open(parent.constData(), O_RDONLY | O_DIRECTORY);
    if (fd < 0)
        throw std::runtime_error("Projeto salvo; falha ao abrir diretório para sincronização");
    int syncResult = ::fsync(fd);
    ::close(fd);
    if (syncResult != 0)
        throw std::runtime_error("Projeto salvo; diretório não sincronizado");
#endif
}
} // namespace
Document ProjectStore::open(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 128 * 1024 * 1024 || file.size() < 22)
        throw std::runtime_error("Arquivo inexistente, grande demais ou incompleto");
    const auto bytes = file.readAll();
    zip_error_t error;
    zip_error_init(&error);
    auto *source =
        zip_source_buffer_create(bytes.constData(), static_cast<zip_uint64_t>(bytes.size()), 0, &error);
    if (!source) {
        zip_error_fini(&error);
        throw std::runtime_error("Não foi possível ler container");
    }
    auto *raw = zip_open_from_source(source, ZIP_RDONLY | ZIP_CHECKCONS, &error);
    if (!raw) {
        zip_source_free(source);
        zip_error_fini(&error);
        throw std::runtime_error("Container ZIP inválido");
    }
    zip_error_fini(&error);
    Archive archive(raw, zip_discard);
    auto count = zip_get_num_entries(raw, 0);
    if (count < static_cast<zip_int64_t>(required.size()) || count > 128)
        throw std::runtime_error("Entradas inesperadas no projeto v1");
    std::map<std::string, Json> data;
    Json assets = Json::object();
    std::set<std::string> names;
    zip_uint64_t total = 0;
    for (zip_uint64_t i = 0; i < static_cast<zip_uint64_t>(count); ++i) {
        zip_stat_t st;
        zip_stat_init(&st);
        if (zip_stat_index(raw, i, 0, &st) != 0 || !st.name || !names.insert(st.name).second)
            throw std::runtime_error("Nome ZIP não permitido");
        bool asset = QRegularExpression("^custom-assets/[a-f0-9]{64}\\.png$")
                         .match(QString::fromUtf8(st.name))
                         .hasMatch();
        if (!asset && !required.contains(st.name))
            throw std::runtime_error("Nome ZIP não permitido");
        zip_uint8_t os = 0;
        zip_uint32_t attrs = 0;
        zip_file_get_external_attributes(raw, i, 0, &os, &attrs);
        if (os == ZIP_OPSYS_UNIX && ((attrs >> 16) & 0170000) == 0120000)
            throw std::runtime_error("Symlink não permitido");
        if (st.size > maxEntry || total > maxTotal - st.size)
            throw std::runtime_error("Projeto excede limites");
        total += st.size;
        if (asset) {
            std::string name = st.name;
            auto bytes = readRaw(raw, i);
            assets[name.substr(14, 64)] = QByteArray::fromStdString(bytes).toBase64().toStdString();
        } else
            data.emplace(st.name, read(raw, i));
    }
    for (const auto &name : required)
        if (!data.contains(name))
            throw std::runtime_error("Entrada essencial ausente");
    const auto &manifest = data.at("manifest.json");
    if (manifest.at("format") != "LibreMax" || manifest.at("version") != 1)
        throw std::runtime_error("Versão de projeto não suportada; arquivo preservado");
    Json j = data.at("project.json");
    j["entities"] = data.at("scene.json").at("entities");
    j["materials"] = data.at("materials.json");
    j["embeddedAssets"] = assets;
    auto d = Document::deserialize(j);
    if (manifest.at("project") != d.id)
        throw std::runtime_error("Identidade do container divergente");
    Json hashes = Json::array();
    for (const auto &[hash, bytes] : d.embeddedAssets) {
        hashes.push_back(hash);
        (void)bytes;
    }
    if (manifest.value("assets", Json::array()) != hashes)
        throw std::runtime_error("Manifest de assets divergente");
    Json lights = Json::array(), cameras = Json::array();
    for (const auto &e : j["entities"]) {
        if (e["type"] == "Light")
            lights.push_back(e);
        if (e["type"] == "Camera")
            cameras.push_back(e);
    }
    if (lights != data.at("lighting.json") || cameras != data.at("cameras.json"))
        throw std::runtime_error("Índices do container divergentes");
    return d;
}
void ProjectStore::save(const QString &path, const Document &d, bool backup) {
    d.validate();
    QFileInfo info(path);
    if (!QDir(info.absolutePath()).exists())
        throw std::runtime_error("Diretório de destino não existe");
    auto j = d.serialize();
    Json lights = Json::array(), cameras = Json::array();
    for (const auto &e : j["entities"]) {
        if (e["type"] == "Light")
            lights.push_back(e);
        if (e["type"] == "Camera")
            cameras.push_back(e);
    }
    auto project = j;
    project.erase("entities");
    project.erase("materials");
    project.erase("embeddedAssets");
    Json hashes = Json::array();
    for (const auto &[hash, bytes] : d.embeddedAssets) {
        hashes.push_back(hash);
        (void)bytes;
    }
    std::map<std::string, std::string> entries = {
        {"manifest.json",
         Json({{"format", "LibreMax"}, {"version", 1}, {"project", d.id}, {"assets", hashes}}).dump()},
        {"scene.json", Json({{"entities", j["entities"]}}).dump()},
        {"project.json", project.dump()},
        {"materials.json", d.materials.dump()},
        {"lighting.json", lights.dump()},
        {"cameras.json", cameras.dump()}};
    for (const auto &[hash, bytes] : d.embeddedAssets)
        entries["custom-assets/" + hash + ".png"] = bytes.toStdString();
    QTemporaryDir temp(info.absolutePath() + "/.libremax-XXXXXX");
    if (!temp.isValid())
        throw std::runtime_error("Não foi possível criar temporário");
    auto filename = temp.filePath("project.zip");
    int error = 0;
    auto encoded = QFile::encodeName(filename);
    Archive archive(zip_open(encoded.constData(), ZIP_CREATE | ZIP_TRUNCATE, &error), zip_discard);
    if (!archive)
        throw std::runtime_error("Falha ao criar ZIP");
    for (const auto &[name, data] : entries) {
        if (data.size() > maxEntry)
            throw std::runtime_error("Projeto grande demais");
        auto *source = zip_source_buffer(archive.get(), data.data(), data.size(), 0);
        if (!source)
            throw std::runtime_error("Falha de memória ZIP");
        if (zip_file_add(archive.get(), name.c_str(), source, ZIP_FL_ENC_UTF_8) < 0) {
            zip_source_free(source);
            throw std::runtime_error("Falha ao montar ZIP");
        }
    }
    auto *raw = archive.release();
    if (zip_close(raw) != 0) {
        std::string message = zip_strerror(raw);
        zip_discard(raw);
        throw std::runtime_error("Falha ao finalizar ZIP: " + message);
    }
    if (open(filename).serialize() != j)
        throw std::runtime_error("Validação do temporário falhou");
    QFile ready(filename);
    if (!ready.open(QIODevice::ReadOnly))
        throw std::runtime_error("Falha ao ler temporário");
    auto bytes = ready.readAll();
    ready.close();
    if (backup && QFile::exists(path)) {
        open(path);
        QFile previous(path);
        if (!previous.open(QIODevice::ReadOnly))
            throw std::runtime_error("Falha ao criar backup");
        atomicWrite(path + ".bak", previous.readAll());
    }
    atomicWrite(path, bytes);
}
} // namespace lmx
