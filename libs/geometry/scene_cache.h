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
    std::size_t builds = 0;

  public:
    std::vector<Part> scene(const Document &document);
    std::size_t buildCount() const { return builds; }
};
} // namespace lmx
