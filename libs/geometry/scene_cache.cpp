#include "scene_cache.h"
#include <cmath>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>
#include <numbers>
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
    std::set<std::string> usedPrototypes;
    for (const auto &e : d.entities) {
        bool visible = e.visible;
        auto parent = e.parent;
        while (!parent.empty()) {
            const auto &p = d.at(parent);
            visible &= p.visible;
            parent = p.parent;
        }
        if (!visible)
            continue;
        alive.insert(e.id);
        const bool canShare = e.type != "Wall" && e.type != "HalfWall" && e.type != "Door" &&
                              e.type != "Window" && !e.metadata.contains("automation");
        std::string prototypeKey;
        if (canShare) {
            auto state = geometryState(e);
            state.erase("transform");
            state.erase("parent");
            state["mirrored"] = e.transform.mirrored;
            if (e.parameters.value("roomOutline", false))
                state["outline"] = d.at(e.parent).parameters.at("outline");
            prototypeKey = state.dump();
            usedPrototypes.insert(prototypeKey);
        }
        Json key = Json::array({geometryState(e)});
        if (e.parameters.value("roomOutline", false))
            key.push_back(geometryState(d.at(e.parent)));
        if (e.metadata.contains("roomEdge"))
            key.push_back(geometryState(d.at(e.parent)));
        if (e.type == "Wall" || e.type == "HalfWall") {
            for (const auto &opening : d.entities)
                if (opening.parent == e.id && (opening.type == "Door" || opening.type == "Window"))
                    key.push_back(geometryState(opening));
            if (!e.metadata.contains("roomEdge")) {
                const auto angle = e.transform.yaw * std::numbers::pi / 180;
                for (const auto &other : d.entities) {
                    if (other.id == e.id || (other.type != "Wall" && other.type != "HalfWall"))
                        continue;
                    const auto a = other.transform.yaw * std::numbers::pi / 180;
                    bool touches = false;
                    for (int i = 0; i < 2; ++i)
                        for (int j = 0; j < 2; ++j)
                            touches |= std::hypot(e.transform.x + i * e.width * std::cos(angle) -
                                                      other.transform.x - j * other.width * std::cos(a),
                                                  e.transform.y + i * e.width * std::sin(angle) -
                                                      other.transform.y - j * other.width * std::sin(a)) < .1;
                    if (touches)
                        key.push_back(geometryState(other));
                }
            }
        }
        if (e.type == "Door" || e.type == "Window")
            key.push_back(geometryState(d.at(e.parent)));
        if (e.metadata.contains("automation"))
            for (const auto &source : e.metadata.at("sources"))
                key.push_back(geometryState(d.at(source.get<std::string>())));
        const auto fingerprint = key.dump();
        auto &entry = entries[e.id];
        if (entry.key != fingerprint) {
            std::vector<Part> parts;
            if (canShare) {
                auto prototype = prototypes.find(prototypeKey);
                if (prototype == prototypes.end()) {
                    auto local = e;
                    local.transform.x = local.transform.y = local.transform.z = local.transform.yaw = 0;
                    prototype = prototypes.emplace(prototypeKey, buildEntity(d, local)).first;
                    ++prototypeBuilds;
                }
                gp_Trsf placement;
                placement.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)),
                                      e.transform.yaw * std::numbers::pi / 180);
                placement.SetTranslationPart(gp_Vec(e.transform.x, e.transform.y, e.transform.z));
                for (const auto &part : prototype->second)
                    parts.push_back({part.shape.Moved(TopLoc_Location(placement)), part.material, e.id});
            } else
                parts = buildEntity(d, e);
            entry = {fingerprint, std::move(parts)};
            ++builds;
        }
        result.insert(result.end(), entry.parts.begin(), entry.parts.end());
    }
    std::erase_if(entries, [&](const auto &entry) { return !alive.contains(entry.first); });
    std::erase_if(prototypes, [&](const auto &entry) { return !usedPrototypes.contains(entry.first); });
    return result;
}
} // namespace lmx
