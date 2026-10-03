#include "render_snapshot.h"
namespace lmx {
RenderSnapshot::RenderSnapshot(const Document &document, Json options, const std::string &camera)
    : scene(document), settings(std::move(options)) {
    validateRenderOptions(settings);
    if (!scene.contains(camera) || scene.at(camera).type != "Camera" || !scene.at(camera).visible)
        throw std::invalid_argument("Escolha uma câmera disponível");
    scene.renderSettings["camera"] = camera;
    scene.renderSettings["cycles"] = settings;
    if (settings.at("format") == "EXR")
        scene.version = std::max(2, scene.version);
    scene.renderSettings["denoise"] = settings.at("denoise");
}
} // namespace lmx
