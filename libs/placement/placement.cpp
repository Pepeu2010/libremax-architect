#include "placement.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
namespace lmx {
namespace {
struct Point {
    double x, y;
};
Point local(const Transform &t, double x, double y) {
    const double a = t.yaw * std::numbers::pi / 180, c = std::cos(a), s = std::sin(a);
    return {(x - t.x) * c + (y - t.y) * s, -(x - t.x) * s + (y - t.y) * c};
}
Point world(const Transform &t, double x, double y) {
    const double a = t.yaw * std::numbers::pi / 180;
    return {t.x + x * std::cos(a) - y * std::sin(a), t.y + x * std::sin(a) + y * std::cos(a)};
}
std::array<Point, 4> corners(const Entity &e) {
    return {world(e.transform, 0, 0), world(e.transform, e.width, 0), world(e.transform, e.width, e.depth),
            world(e.transform, 0, e.depth)};
}
bool overlap(const Entity &a, const Entity &b) {
    if (a.transform.z + a.height <= b.transform.z + 1 || b.transform.z + b.height <= a.transform.z + 1)
        return false;
    const auto ac = corners(a), bc = corners(b);
    for (const auto *e : {&a, &b}) {
        const double angle = e->transform.yaw * std::numbers::pi / 180;
        for (Point axis :
             {Point{std::cos(angle), std::sin(angle)}, Point{-std::sin(angle), std::cos(angle)}}) {
            double amin = 1e30, amax = -1e30, bmin = 1e30, bmax = -1e30;
            for (auto p : ac) {
                auto d = p.x * axis.x + p.y * axis.y;
                amin = std::min(amin, d);
                amax = std::max(amax, d);
            }
            for (auto p : bc) {
                auto d = p.x * axis.x + p.y * axis.y;
                bmin = std::min(bmin, d);
                bmax = std::max(bmax, d);
            }
            if (amax <= bmin + 1 || bmax <= amin + 1)
                return false;
        }
    }
    return true;
}
std::string obstruction(const Document &d, const Entity &object) {
    for (const auto &other : d.entities) {
        if (other.id == object.id || !other.visible || other.type == "Room" || other.type == "Group" ||
            other.type == "Floor" || other.type == "Ceiling" || other.type == "Camera" ||
            other.type == "Light" || other.type == "Door" || other.type == "Window" ||
            other.metadata.contains("automation") ||
            other.parameters.value("family", std::string{}) == "rug" ||
            other.parameters.value("placement", std::string{}) == "rug")
            continue;
        auto obstacle = other;
        if (obstacle.type == "Wall" || obstacle.type == "HalfWall") {
            const auto p = world(obstacle.transform, 0, -obstacle.depth / 2);
            obstacle.transform.x = p.x;
            obstacle.transform.y = p.y;
        }
        if (overlap(object, obstacle))
            return "Não cabe aqui: há uma parede ou outro móvel no caminho.";
    }
    // Reserve the opening itself; the swing area is not simulated.
    for (const auto &opening : d.entities) {
        if (!opening.visible || (opening.type != "Door" && opening.type != "Window") ||
            opening.id == object.id)
            continue;
        const auto &wall = d.at(opening.parent);
        Entity space = opening;
        auto p = world(wall.transform, opening.parameters.value("offset", 0.0), -wall.depth / 2 - 80);
        space.transform = {p.x, p.y, wall.transform.z + opening.parameters.value("sill", 0.0),
                           wall.transform.yaw, false};
        space.depth = wall.depth + 160;
        if (overlap(object, space))
            return "Deixe a porta ou janela livre.";
    }
    return {};
}
bool contained(const Document &d, const Entity &e) {
    bool rooms = false;
    for (const auto &r : d.entities) {
        if (r.type != "Room")
            continue;
        rooms = true;
        bool inside = true;
        for (auto p : corners(e)) {
            auto q = local(r.transform, p.x, p.y);
            if (q.x < -1 || q.y < -1 || q.x > r.width + 1 || q.y > r.depth + 1)
                inside = false;
        }
        if (inside)
            return true;
    }
    return !rooms;
}
} // namespace
bool movable(const Entity &e) {
    return !e.locked && !e.metadata.contains("automation") &&
           (e.type == "FurnitureModule" || e.type == "DecorativeObject" || e.type == "MeshObject" ||
            e.type == "GeometryObject");
}
Placement placeObject(const Document &d, Entity object, double x, double y, bool assist,
                      const std::string &wallHint) {
    millimeters(x);
    millimeters(y);
    Placement result{object, false, {}, {}};
    auto &e = result.object;
    const bool opening = e.type == "Door" || e.type == "Window";
    const auto center = world(e.transform, e.width / 2, e.depth / 2);
    e.transform.x = millimeters(e.transform.x + x - center.x);
    e.transform.y = millimeters(e.transform.y + y - center.y);
    const auto mode = e.parameters.value("placement", std::string("floor"));
    const bool wallOnly = opening || mode == "wall";
    const Entity *nearest = nullptr;
    Point hit{};
    double best = std::numeric_limits<double>::max();
    if (assist || wallOnly) {
        for (const auto &wall : d.entities) {
            if ((wall.type != "Wall" && wall.type != "HalfWall") || !wall.visible || wall.locked)
                continue;
            if (!wallHint.empty() && wall.id != wallHint)
                continue;
            auto p = local(wall.transform, x, y);
            auto distance = std::hypot(std::max({0.0, -p.x, p.x - wall.width}),
                                       std::max(0.0, std::abs(p.y) - wall.depth / 2));
            if (distance < best) {
                nearest = &wall;
                hit = p;
                best = distance;
            }
        }
    }
    const double reach = wallOnly ? 600 : 350;
    if (nearest && (best <= reach || !wallHint.empty()) && mode != "surface" && mode != "rug") {
        const auto &wall = *nearest;
        if (e.width > wall.width + 0.1) {
            result.message = "Este móvel é mais largo que a parede.";
            return result;
        }
        double side = hit.y >= 0 ? 1 : -1;
        if (!wall.parent.empty() && d.contains(wall.parent) && d.at(wall.parent).type == "Room") {
            const auto &room = d.at(wall.parent);
            auto center = world(room.transform, room.width / 2, room.depth / 2);
            side = local(wall.transform, center.x, center.y).y >= 0 ? 1 : -1;
        }
        for (const auto &room : d.entities)
            if (room.type == "Room") {
                auto inside = local(room.transform, x, y);
                if (inside.x > wall.depth / 2 && inside.y > wall.depth / 2 &&
                    inside.x < room.width - wall.depth / 2 && inside.y < room.depth - wall.depth / 2) {
                    auto middle = world(room.transform, room.width / 2, room.depth / 2);
                    side = local(wall.transform, middle.x, middle.y).y >= 0 ? 1 : -1;
                    break;
                }
            }
        const double desired = std::clamp(hit.x - e.width / 2, 0.0, wall.width - e.width);
        std::vector<double> starts{desired, 0, wall.width - e.width};
        for (const auto &other : d.entities) {
            if (other.id == e.id || !other.visible)
                continue;
            if (movable(other) || other.type == "FurnitureModule" || other.type == "MeshObject" ||
                other.type == "DecorativeObject") {
                for (auto p : corners(other)) {
                    auto q = local(wall.transform, p.x, p.y);
                    if (std::abs(q.y) < wall.depth / 2 + e.depth + other.depth + 80) {
                        starts.push_back(q.x + 2);
                        starts.push_back(q.x - e.width - 2);
                    }
                }
            }
        }
        // Small magnet to a neighbor or corner; collision fallback is limited to 40 cm.
        double magnet = desired, magnetDistance = 61;
        for (double start : starts)
            if (start >= 0 && start + e.width <= wall.width && std::abs(start - desired) < magnetDistance) {
                if (start != desired) {
                    magnet = start;
                    magnetDistance = std::abs(start - desired);
                }
            }
        starts[0] = magnet;
        std::stable_sort(starts.begin() + 1, starts.end(),
                         [&](double a, double b) { return std::abs(a - desired) < std::abs(b - desired); });
        for (double start : starts) {
            if (start < 0 || start + e.width > wall.width || std::abs(start - desired) > 400)
                continue;
            if (opening) {
                e.parent = wall.id;
                e.depth = wall.depth;
                e.parameters["offset"] = millimeters(start);
                e.parameters["sill"] =
                    e.parameters.value("sill", opening && e.type == "Window" ? 1000.0 : 0.0);
                bool free = e.parameters.at("sill").get<double>() + e.height <= wall.height;
                for (const auto &other : d.entities)
                    if (other.id != e.id && other.parent == wall.id &&
                        (other.type == "Door" || other.type == "Window")) {
                        const double offset = other.parameters.value("offset", 0.0);
                        if (start < offset + other.width && offset < start + e.width)
                            free = false;
                    }
                if (!free)
                    continue;
            } else {
                const auto p = side > 0 ? world(wall.transform, start, wall.depth / 2 + 2)
                                        : world(wall.transform, start + e.width, -wall.depth / 2 - 2);
                e.transform.x = millimeters(p.x);
                e.transform.y = millimeters(p.y);
                e.transform.yaw = wall.transform.yaw + (side < 0 ? 180 : 0);
                if (!contained(d, e) || !obstruction(d, e).empty())
                    continue;
            }
            result.wall = wall.id;
            e.metadata["placementWall"] = wall.id;
            result.allowed = true;
            result.message = "Encostado na parede · solte para colocar";
            return result;
        }
        result.message = "Não há espaço livre nesta parte da parede.";
        return result;
    }
    if (wallOnly) {
        result.message = "Aproxime o mouse de uma parede para colocar este item.";
        return result;
    }
    if (mode == "surface") {
        const Entity *support = nullptr;
        for (const auto &other : d.entities) {
            if (other.id == e.id || !other.visible || !movable(other) ||
                other.parameters.value("placement", std::string{}) == "surface")
                continue;
            auto p = local(other.transform, x, y);
            if (p.x >= 0 && p.y >= 0 && p.x <= other.width && p.y <= other.depth &&
                (!support || other.transform.z + other.height > support->transform.z + support->height))
                support = &other;
        }
        if (support)
            e.transform.z = support->transform.z + support->height + 2;
        else
            e.transform.z = e.parameters.value("defaultElevation", 0.0);
    }
    e.metadata.erase("placementWall");
    if (!contained(d, e)) {
        result.message = "Coloque o móvel dentro de um cômodo.";
        return result;
    }
    if (mode != "rug")
        result.message = obstruction(d, e);
    if (!result.message.empty())
        return result;
    result.allowed = true;
    result.message = mode == "surface" && e.transform.z > 0 ? "Sobre o móvel · solte para colocar"
                                                            : "No piso · solte para colocar";
    return result;
}
} // namespace lmx
