#include "texture.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
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
} // namespace lmx
