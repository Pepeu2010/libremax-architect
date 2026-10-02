#include "scene_cache.h"
#include <set>
namespace lmx {
namespace {
Json geometryState(const Entity &e) {
    return {
        {"type", e.type},
        {"parent", e.parent},
        {"material", e.material},
        {"dimensions", {e.width, e.depth, e.height}},
        {"transform", {e.transform.x, e.transform.y, e.transform.z, e.transform.yaw, e.transform.mirrored}},
        {"parameters", e.parameters},
        {"metadata", e.metadata}};
}
} // namespace
std::vector<Part> SceneGeometryCache::scene(const Document &d) {
    std::vector<Part> result;
    std::set<std::string> alive;
    for (const auto &e : d.entities) {
        alive.insert(e.id);
        bool visible = e.visible;
        auto parent = e.parent;
        while (!parent.empty()) {
            const auto &p = d.at(parent);
            visible &= p.visible;
            parent = p.parent;
        }
        if (!visible)
            continue;
        Json key = Json::array({geometryState(e)});
        if (e.type == "Wall" || e.type == "HalfWall")
            for (const auto &opening : d.entities)
                if (opening.parent == e.id && (opening.type == "Door" || opening.type == "Window"))
                    key.push_back(geometryState(opening));
        if (e.type == "Door" || e.type == "Window")
            key.push_back(geometryState(d.at(e.parent)));
        if (e.metadata.contains("automation"))
            for (const auto &source : e.metadata.at("sources"))
                key.push_back(geometryState(d.at(source.get<std::string>())));
        const auto fingerprint = key.dump();
        auto &entry = entries[e.id];
        if (entry.key != fingerprint) {
            auto parts = buildEntity(d, e);
            entry = {fingerprint, std::move(parts)};
            ++builds;
        }
        result.insert(result.end(), entry.parts.begin(), entry.parts.end());
    }
    std::erase_if(entries, [&](const auto &entry) { return !alive.contains(entry.first); });
    return result;
}
} // namespace lmx
