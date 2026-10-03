#pragma once
#include "geometry.h"
#include <map>
namespace lmx {
class SceneGeometryCache {
    struct Entry {
        std::string key;
        std::vector<Part> parts;
    };
    std::map<std::string, Entry> entries;
    std::map<std::string, std::vector<Part>> prototypes;
    std::size_t builds = 0;
    std::size_t prototypeBuilds = 0;

  public:
    std::vector<Part> scene(const Document &document);
    std::size_t buildCount() const { return builds; }
    std::size_t prototypeBuildCount() const { return prototypeBuilds; }
};
} // namespace lmx
