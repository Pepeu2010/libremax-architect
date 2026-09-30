#include "texture.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <algorithm>
#include <stdexcept>
namespace lmx {
ImportedTexture readTexture(const QString &filename) {
    if (QFileInfo(filename).size() > 16 * 1024 * 1024)
        throw std::runtime_error("Textura maior que 16 MiB");
    QImageReader reader(filename);
    auto dimensions = reader.size();
    if (!dimensions.isValid() || dimensions.width() > 8192 || dimensions.height() > 8192 ||
        static_cast<qint64>(dimensions.width()) * dimensions.height() > 16000000)
        throw std::runtime_error("Dimensões da textura inválidas ou grandes demais");
    auto image = reader.read();
    if (image.isNull())
        throw std::runtime_error(reader.errorString().toStdString());
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG") || png.size() > 16 * 1024 * 1024)
        throw std::runtime_error("Falha ao incorporar textura");
    auto hash = QCryptographicHash::hash(png, QCryptographicHash::Sha256).toHex().toStdString();
    return {hash, QFileInfo(filename).baseName().toStdString(), png};
}
std::string attachTexture(Document &d, const ImportedTexture &texture) {
    d.embeddedAssets[texture.hash] = texture.png;
    std::string id = "texture-" + texture.hash;
    for (const auto &m : d.materials)
        if (m.at("id") == id)
            return id;
    auto m = materialPresets()[1];
    m["id"] = id;
    m["name"] = texture.name;
    m["baseColorTexture"] = texture.hash;
    m["textureScale"] = 1000.0;
    m["textureRotation"] = 0.0;
    m["textureOffset"] = {0, 0, 0};
    d.materials.push_back(m);
    return id;
}
PbrMaterialPack readPbrMaterials(const QString &directory) {
    QFile catalog(QDir(directory).filePath("catalog.json"));
    if (!catalog.open(QIODevice::ReadOnly) || catalog.size() > 1024 * 1024)
        throw std::runtime_error("Biblioteca de materiais PBR indisponível");
    auto manifest = Json::parse(catalog.readAll().toStdString());
    if (manifest.at("schema") != 1 || !manifest.at("materials").is_array() ||
        manifest.at("materials").size() > 20)
        throw std::runtime_error("Catálogo PBR inválido");
    PbrMaterialPack pack;
    for (const auto &entry : manifest.at("materials")) {
        auto presets = materialPresets();
        auto preset = std::find_if(presets.begin(), presets.end(),
                                   [&](const auto &m) { return m.at("id") == entry.at("id"); });
        if (preset == presets.end() || entry.at("license") != "CC0-1.0")
            throw std::runtime_error("Material PBR inválido");
        auto material = *preset;
        for (auto key : {"name", "source", "license", "authors", "textureScale"})
            material[key] = entry.at(key);
        material["normalStrength"] = entry.at("id") == "oak" ? 0.55 : 0.25;
        material["textureRotation"] = entry.at("id") == "oak" ? 90.0 : 0.0;
        material["procedural"] = "none";
        for (auto channel : {"baseColorTexture", "roughnessTexture", "normalTexture"}) {
            const auto &map = entry.at("maps").at(channel);
            auto name = QString::fromStdString(map.at("file").get<std::string>());
            if (name.isEmpty() || QFileInfo(name).fileName() != name || name.contains('\\'))
                throw std::runtime_error("Caminho de textura PBR inválido");
            auto path = QDir(directory).filePath(name);
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly) || file.size() > 16 * 1024 * 1024 ||
                QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex().toStdString() !=
                    map.at("sha256").get<std::string>())
                throw std::runtime_error("Integridade da textura PBR inválida");
            auto texture = readTexture(path);
            material[channel] = texture.hash;
            pack.assets[texture.hash] = texture.png;
        }
        pack.materials.push_back(material);
    }
    return pack;
}
void attachPbrMaterials(Document &document, const PbrMaterialPack &pack) {
    auto next = document;
    for (const auto &[hash, png] : pack.assets)
        next.embeddedAssets[hash] = png;
    for (const auto &material : pack.materials) {
        auto existing = std::find_if(next.materials.begin(), next.materials.end(),
                                     [&](const auto &m) { return m.at("id") == material.at("id"); });
        if (existing == next.materials.end())
            next.materials.push_back(material);
        else
            *existing = material;
    }
    next.validate();
    document = std::move(next);
}
} // namespace lmx
