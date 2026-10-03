#include "arrangement.h"
#include "placement/placement.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
#include <stdexcept>
namespace lmx {
std::vector<std::string> arrangementMembers(const Document &d, const std::vector<std::string> &ids) {
    std::set<std::string> requested(ids.begin(), ids.end()), result;
    for (const auto &e : d.entities) {
        auto parent = e.id;
        while (!parent.empty()) {
            if (requested.contains(parent) && movable(e)) {
                result.insert(e.id);
                break;
            }
            parent = d.at(parent).parent;
        }
    }
    for (const auto &id : ids) {
        if (d.at(id).locked || (!movable(d.at(id)) && d.at(id).type != "Group"))
            throw std::invalid_argument("Selecione móveis ou conjuntos para organizar.");
        auto parent = d.at(id).parent;
        while (!parent.empty()) {
            if (d.at(parent).locked)
                throw std::invalid_argument(
                    "O móvel pertence a um conjunto bloqueado. Desbloqueie antes de mover.");
            parent = d.at(parent).parent;
        }
    }
    for (const auto &e : d.entities)
        if (e.locked) {
            auto p = e.parent;
            while (!p.empty()) {
                if (requested.contains(p))
                    throw std::invalid_argument(
                        "O conjunto contém um móvel bloqueado. Desbloqueie antes de mover.");
                p = d.at(p).parent;
            }
        }
    return {result.begin(), result.end()};
}
std::string groupObjects(Document &d, const std::vector<std::string> &ids) {
    auto members = arrangementMembers(d, ids);
    if (members.size() < 2)
        throw std::invalid_argument("Selecione pelo menos dois móveis para juntar.");
    for (const auto &id : members)
        if (!d.at(id).parent.empty() && d.at(d.at(id).parent).type == "Group")
            throw std::invalid_argument("Separe o conjunto atual antes de juntar novamente.");
    auto group = entity("Group", "Conjunto de móveis");
    for (const auto &id : members) {
        auto &e = d.at(id);
        e.metadata["beforeGroupParent"] = e.parent;
        e.parent = group.id;
    }
    d.entities.push_back(group);
    return group.id;
}
void ungroupObjects(Document &d, const std::vector<std::string> &ids) {
    std::set<std::string> groups;
    for (const auto &id : ids) {
        const auto &e = d.at(id);
        if (e.type == "Group")
            groups.insert(id);
        else if (!e.parent.empty() && d.at(e.parent).type == "Group")
            groups.insert(e.parent);
    }
    if (groups.empty())
        throw std::invalid_argument("Selecione um conjunto para separar.");
    arrangementMembers(d, {groups.begin(), groups.end()});
    for (auto &e : d.entities)
        if (groups.contains(e.parent)) {
            auto parent = e.metadata.value("beforeGroupParent", std::string{});
            e.parent = d.contains(parent) ? parent : std::string{};
            e.metadata.erase("beforeGroupParent");
        }
    std::erase_if(d.entities, [&](const auto &e) { return groups.contains(e.id); });
}
std::vector<Entity> placedTogether(const Document &d, const std::vector<Entity> &objects, double dx,
                                   double dy) {
    auto obstacles = d;
    std::set<std::string> selected;
    for (const auto &e : objects)
        selected.insert(e.id);
    std::erase_if(obstacles.entities, [&](const auto &e) { return selected.contains(e.id); });
    std::vector<Entity> placed;
    for (auto e : objects) {
        const auto a = e.transform.yaw * std::numbers::pi / 180;
        const double x = e.transform.x + (e.width * std::cos(a) - e.depth * std::sin(a)) / 2 + dx,
                     y = e.transform.y + (e.width * std::sin(a) + e.depth * std::cos(a)) / 2 + dy;
        // Keep relative elevations and positions; only the whole set may move.
        auto check = e;
        check.parameters["placement"] = "floor";
        auto result = placeObject(obstacles, check, x, y, false);
        if (!result.allowed)
            throw std::invalid_argument(result.message);
        // Placement validates against the room and obstacles. Do not round each member independently:
        // that would change relative distances and make repeated transforms drift.
        e.transform.x += dx;
        e.transform.y += dy;
        e.metadata.erase("placementWall");
        placed.push_back(e);
    }
    return placed;
}
void moveObjects(Document &d, const std::vector<std::string> &ids, double dx, double dy) {
    auto members = arrangementMembers(d, ids);
    std::vector<Entity> objects;
    if (members.empty())
        throw std::invalid_argument("Selecione os móveis para mover.");
    for (const auto &id : members)
        objects.push_back(d.at(id));
    auto result = placedTogether(d, objects, dx, dy);
    for (const auto &e : result)
        d.at(e.id) = e;
}
namespace {
std::array<double, 4> bounds(const Entity &e) {
    std::array<double, 4> b{1e30, 1e30, -1e30, -1e30};
    const auto a = e.transform.yaw * std::numbers::pi / 180;
    for (auto p :
         std::array<std::array<double, 2>, 4>{{{0, 0}, {e.width, 0}, {e.width, e.depth}, {0, e.depth}}}) {
        auto x = e.transform.x + p[0] * std::cos(a) - p[1] * std::sin(a),
             y = e.transform.y + p[0] * std::sin(a) + p[1] * std::cos(a);
        b[0] = std::min(b[0], x);
        b[1] = std::min(b[1], y);
        b[2] = std::max(b[2], x);
        b[3] = std::max(b[3], y);
    }
    return b;
}
} // namespace
namespace {
void transformTogether(Document &d, const std::vector<std::string> &ids, double degrees, bool mirror) {
    if (!std::isfinite(degrees))
        throw std::invalid_argument("Informe um ângulo válido para girar os móveis.");
    const auto members = arrangementMembers(d, ids);
    if (members.empty())
        throw std::invalid_argument("Selecione móveis ou um conjunto para transformar.");
    std::array<double, 4> extent{1e30, 1e30, -1e30, -1e30};
    for (const auto &id : members) {
        const auto b = bounds(d.at(id));
        for (int axis = 0; axis < 2; ++axis) {
            extent[axis] = std::min(extent[axis], b[axis]);
            extent[axis + 2] = std::max(extent[axis + 2], b[axis + 2]);
        }
    }
    const double pivotX = (extent[0] + extent[2]) / 2, pivotY = (extent[1] + extent[3]) / 2;
    const double angle = std::remainder(degrees, 360.0) * std::numbers::pi / 180;
    const auto c = std::cos(angle), s = std::sin(angle);
    std::vector<Entity> objects;
    for (const auto &id : members) {
        auto e = d.at(id);
        const auto old = bounds(e);
        const double x = (old[0] + old[2]) / 2 - pivotX, y = (old[1] + old[3]) / 2 - pivotY;
        const double centerX = pivotX + (mirror ? -x : c * x - s * y),
                     centerY = pivotY + (mirror ? y : s * x + c * y);
        e.transform.yaw = std::remainder(mirror ? -e.transform.yaw : e.transform.yaw + degrees, 360.0);
        if (mirror)
            e.transform.mirrored = !e.transform.mirrored;
        const auto a = e.transform.yaw * std::numbers::pi / 180;
        e.transform.x = centerX - (e.width * std::cos(a) - e.depth * std::sin(a)) / 2;
        e.transform.y = centerY - (e.width * std::sin(a) + e.depth * std::cos(a)) / 2;
        objects.push_back(std::move(e));
    }
    // Validate the entire rigid transformation against external obstacles before writing any member.
    // Internal overlaps (a vase on a table, for example) remain exactly as they were.
    const auto placed = placedTogether(d, objects, 0, 0);
    for (const auto &e : placed)
        d.at(e.id) = e;
}
} // namespace
void rotateObjects(Document &d, const std::vector<std::string> &ids, double degrees) {
    transformTogether(d, ids, degrees, false);
}
void mirrorObjects(Document &d, const std::vector<std::string> &ids) {
    transformTogether(d, ids, 0, true);
}
void arrangeObjects(Document &d, const std::vector<std::string> &ids, const std::string &mode) {
    auto members = arrangementMembers(d, ids);
    if (members.size() < 2)
        throw std::invalid_argument("Selecione dois ou mais móveis com Ctrl + clique.");
    const bool horizontal = mode == "left" || mode == "right" || mode == "centerX" || mode == "spaceX";
    const bool distribute = mode == "spaceX" || mode == "spaceY";
    if (!horizontal && mode != "back" && mode != "front" && mode != "centerY" && mode != "spaceY")
        throw std::invalid_argument("Alinhamento desconhecido.");
    const int axis = horizontal ? 0 : 1;
    std::sort(members.begin(), members.end(),
              [&](const auto &a, const auto &b) { return bounds(d.at(a))[axis] < bounds(d.at(b))[axis]; });
    const double start = bounds(d.at(members.front()))[axis];
    double end = start, total = 0;
    for (auto id : members) {
        auto b = bounds(d.at(id));
        end = std::max(end, b[axis + 2]);
        total += b[axis + 2] - b[axis];
    }
    const double gap = (end - start - total) / (members.size() - 1);
    if (distribute && gap < 0)
        throw std::invalid_argument("Afaste os móveis para distribuir com espaço entre eles.");
    auto candidate = d;
    double cursor = start;
    for (auto id : members) {
        auto &e = candidate.at(id);
        auto b = bounds(e);
        double delta = distribute                                 ? cursor - b[axis]
                       : (mode == "right" || mode == "front")     ? end - b[axis + 2]
                       : (mode == "centerX" || mode == "centerY") ? (start + end - b[axis] - b[axis + 2]) / 2
                                                                  : start - b[axis];
        if (axis == 0)
            e.transform.x += delta;
        else
            e.transform.y += delta;
        cursor += b[axis + 2] - b[axis] + gap;
    }
    for (auto id : members) {
        auto obstacles = candidate;
        std::erase_if(obstacles.entities, [&](const auto &e) { return e.id == id; });
        placedTogether(obstacles, {candidate.at(id)}, 0, 0);
    }
    for (auto id : members) {
        candidate.at(id).metadata.erase("placementWall");
        d.at(id) = candidate.at(id);
    }
}
} // namespace lmx
