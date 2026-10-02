#pragma once
#include "render_options.h"
namespace lmx {
class RenderSnapshot {
    Document scene;
    Json settings;

  public:
    RenderSnapshot(const Document &document, Json options, const std::string &camera);
    const Document &document() const { return scene; }
    const Json &options() const { return settings; }
};
} // namespace lmx
