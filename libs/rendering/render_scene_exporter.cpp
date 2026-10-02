#include "render_scene_exporter.h"
#include "geometry/geometry.h"
#include <QSaveFile>
namespace lmx {
void RenderSceneExporter::exportScene(const Document &document, const QString &filename) {
    const auto bytes = meshSnapshot(document).dump();
    QSaveFile file(filename);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(bytes.data(), static_cast<qint64>(bytes.size())) != static_cast<qint64>(bytes.size()) ||
        !file.commit())
        throw std::runtime_error("Não foi possível preparar a cena para renderizar");
}
} // namespace lmx
