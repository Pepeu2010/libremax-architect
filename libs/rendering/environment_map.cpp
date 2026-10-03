#include "environment_map.h"
#include "high_dynamic_image.h"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <stdexcept>
namespace lmx {
ImportedEnvironment importEnvironment(const QString &filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly) || file.size() < 32 || file.size() > 64 * 1024 * 1024)
        throw std::invalid_argument("Escolha um HDR ou EXR de até 64 MB");
    ImportedEnvironment result;
    result.bytes = file.read(64 * 1024 * 1024 + 1);
    if (result.bytes.size() > 64 * 1024 * 1024 || file.error() != QFile::NoError)
        throw std::invalid_argument("Escolha um HDR ou EXR de até 64 MB");
    const auto image = inspectHighDynamicImage(result.bytes, true);
    result.hash = QCryptographicHash::hash(result.bytes, QCryptographicHash::Sha256).toHex().toStdString();
    result.settings = {{"asset", result.hash},
                       {"format", image.format.toStdString()},
                       {"name", QFileInfo(filename).fileName().toStdString()},
                       {"width", image.size.width()},
                       {"height", image.size.height()},
                       {"rotation", 0.0},
                       {"visible", true}};
    return result;
}
void attachEnvironment(Document &document, const ImportedEnvironment &environment) {
    const auto previous = document.renderSettings.value("hdri", Json::object());
    if (!previous.empty() && previous.at("asset") != environment.hash)
        document.embeddedAssets.erase(previous.at("asset").get<std::string>());
    document.version = std::max(2, document.version);
    document.embeddedAssets[environment.hash] = environment.bytes;
    document.renderSettings["hdri"] = environment.settings;
    document.renderSettings["environmentMode"] = "hdri";
}
} // namespace lmx
