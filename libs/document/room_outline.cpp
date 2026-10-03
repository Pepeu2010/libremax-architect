#include "room_outline.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace lmx {
namespace {
double cross(PlanPoint a, PlanPoint b, PlanPoint c) {
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
}
double area(const Outline &p) {
    double a = 0;
    for (size_t i = 0; i < p.size(); ++i) {
        auto b = p[(i + 1) % p.size()];
        a += p[i][0] * b[1] - b[0] * p[i][1];
    }
    return a / 2;
}
bool on(PlanPoint a, PlanPoint b, PlanPoint p, double t = 0.1) {
    return std::abs(cross(a, b, p)) <= t * std::hypot(b[0] - a[0], b[1] - a[1]) &&
           p[0] >= std::min(a[0], b[0]) - t && p[0] <= std::max(a[0], b[0]) + t &&
           p[1] >= std::min(a[1], b[1]) - t && p[1] <= std::max(a[1], b[1]) + t;
}
bool crosses(PlanPoint a, PlanPoint b, PlanPoint c, PlanPoint d) {
    const auto ab1 = cross(a, b, c), ab2 = cross(a, b, d), cd1 = cross(c, d, a), cd2 = cross(c, d, b);
    return ((ab1 > 0.01 && ab2 < -.01) || (ab1 < -.01 && ab2 > .01)) &&
           ((cd1 > .01 && cd2 < -.01) || (cd1 < -.01 && cd2 > .01));
}
bool overlaps(const Outline &a, const Outline &b) {
    for (size_t i = 0; i < a.size(); ++i)
        for (size_t j = 0; j < b.size(); ++j)
            if (crosses(a[i], a[(i + 1) % a.size()], b[j], b[(j + 1) % b.size()]))
                return true;
    for (const auto *p : {&a, &b}) {
        const auto &other = p == &a ? b : a;
        for (size_t i = 0; i < p->size(); ++i) {
            auto v = (*p)[i], next = (*p)[(i + 1) % p->size()];
            PlanPoint mid{(v[0] + next[0]) / 2, (v[1] + next[1]) / 2};
            if (insideOutline(other, v, -0.1) || insideOutline(other, mid, -0.1))
                return true;
            const auto length = std::hypot(next[0] - v[0], next[1] - v[1]);
            const auto sign = area(*p) > 0 ? 1.0 : -1.0;
            PlanPoint inset{mid[0] - sign * (next[1] - v[1]) / length,
                            mid[1] + sign * (next[0] - v[0]) / length};
            if (insideOutline(other, inset, -0.1))
                return true;
        }
    }
    return false;
}
} // namespace
void validateOutline(const Outline &p) {
    if (p.size() < 3 || p.size() > 64)
        throw std::invalid_argument("Use de 3 a 64 cantos para o cômodo.");
    for (size_t i = 0; i < p.size(); ++i) {
        millimeters(p[i][0]);
        millimeters(p[i][1]);
        const auto next = p[(i + 1) % p.size()];
        if (std::hypot(next[0] - p[i][0], next[1] - p[i][1]) < 100)
            throw std::invalid_argument("Cada trecho da parede precisa ter pelo menos 10 cm.");
        for (size_t j = i + 1; j < p.size(); ++j) {
            if (j == i + 1 || (i == 0 && j == p.size() - 1))
                continue;
            auto end = p[(j + 1) % p.size()];
            if (crosses(p[i], next, p[j], end) || on(p[i], next, p[j]) || on(p[i], next, end) ||
                on(p[j], end, p[i]) || on(p[j], end, next))
                throw std::invalid_argument("As paredes do contorno não podem se cruzar.");
        }
        const auto prev = p[(i + p.size() - 1) % p.size()];
        if (std::abs(cross(prev, p[i], next)) < 0.01 && on(prev, p[i], next))
            throw std::invalid_argument("O contorno não pode voltar sobre a mesma parede.");
    }
    if (std::abs(area(p)) < 250000)
        throw std::invalid_argument("O cômodo precisa ter pelo menos 0,25 m².");
}
bool roomFootprintsOverlap(const Outline &first, const Outline &second) {
    return overlaps(first, second);
}
Outline roomOutline(const Entity &r, bool world) {
    Outline p = r.parameters.contains("outline")
                    ? r.parameters.at("outline").get<Outline>()
                    : Outline{{0, 0}, {r.width, 0}, {r.width, r.depth}, {0, r.depth}};
    if (world) {
        const auto a = r.transform.yaw * std::numbers::pi / 180;
        for (auto &v : p) {
            const auto x = v[0], y = v[1];
            v = {r.transform.x + x * std::cos(a) - y * std::sin(a),
                 r.transform.y + x * std::sin(a) + y * std::cos(a)};
        }
    }
    return p;
}
bool insideOutline(const Outline &p, PlanPoint v, double tolerance) {
    bool inside = false;
    for (size_t i = 0, j = p.size() - 1; i < p.size(); j = i++) {
        if (on(p[j], p[i], v, std::abs(tolerance)))
            return tolerance >= 0;
        if ((p[i][1] > v[1]) != (p[j][1] > v[1]) &&
            v[0] < (p[j][0] - p[i][0]) * (v[1] - p[i][1]) / (p[j][1] - p[i][1]) + p[i][0])
            inside = !inside;
    }
    return inside;
}
bool footprintInside(const Outline &p, const Outline &footprint) {
    for (size_t i = 0; i < footprint.size(); ++i) {
        auto a = footprint[i], b = footprint[(i + 1) % footprint.size()];
        if (!insideOutline(p, a))
            return false;
        for (size_t j = 0; j < p.size(); ++j)
            if (crosses(a, b, p[j], p[(j + 1) % p.size()]))
                return false;
        if (!insideOutline(p, {(a[0] + b[0]) / 2, (a[1] + b[1]) / 2}))
            return false;
    }
    return true;
}
PlanPoint roomInteriorPoint(const Entity &r) {
    auto p = roomOutline(r, true);
    // Test centers of horizontal strips; also works when the bounding center is in an L-shaped void.
    PlanPoint result = p[0];
    double best = 0;
    std::vector<double> ys;
    for (auto v : p)
        ys.push_back(v[1]);
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    for (size_t i = 0; i + 1 < ys.size(); ++i) {
        auto y = (ys[i] + ys[i + 1]) / 2;
        std::vector<double> xs;
        for (size_t j = 0; j < p.size(); ++j) {
            auto a = p[j], b = p[(j + 1) % p.size()];
            if ((a[1] > y) != (b[1] > y))
                xs.push_back(a[0] + (y - a[1]) * (b[0] - a[0]) / (b[1] - a[1]));
        }
        std::sort(xs.begin(), xs.end());
        for (size_t j = 0; j + 1 < xs.size(); j += 2)
            if (xs[j + 1] - xs[j] > best) {
                best = xs[j + 1] - xs[j];
                result = {(xs[j] + xs[j + 1]) / 2, y};
            }
    }
    return result;
}
void addPolygonRoom(Document &d, Outline p, double h, double t, const std::string &name) {
    validateOutline(p);
    if (h < 500 || h > 10000 || t < 10 || t > 500)
        throw std::invalid_argument("Altura ou espessura da parede inválida.");
    if (area(p) < 0)
        std::reverse(p.begin(), p.end());
    for (const auto &r : d.entities)
        if (r.type == "Room" && overlaps(p, roomOutline(r, true)))
            throw std::invalid_argument("Este cômodo ocupa o espaço de outro.");
    auto r = entity("Room", name);
    auto minx = p[0][0], miny = p[0][1], maxx = minx, maxy = miny;
    for (auto v : p) {
        minx = std::min(minx, v[0]);
        miny = std::min(miny, v[1]);
        maxx = std::max(maxx, v[0]);
        maxy = std::max(maxy, v[1]);
    }
    r.transform.x = minx;
    r.transform.y = miny;
    r.width = maxx - minx;
    r.depth = maxy - miny;
    r.height = h;
    Outline local = p;
    for (auto &v : local) {
        v[0] -= minx;
        v[1] -= miny;
    }
    r.parameters = {{"outline", local}, {"wallThickness", t}};
    d.entities.push_back(r);
    for (size_t i = 0; i < p.size(); ++i) {
        auto a = p[i], b = p[(i + 1) % p.size()];
        auto w = wall(a[0], a[1], b[0], b[1], h, t);
        w.parent = r.id;
        w.name = "Parede " + std::to_string(i + 1);
        w.metadata["roomEdge"] = i;
        std::vector<std::pair<double, double>> segments{{0, w.width}};
        const auto angle = w.transform.yaw * std::numbers::pi / 180;
        for (const auto &existing : d.entities)
            if (existing.type == "Wall" && std::abs(existing.depth - t) < .1 &&
                std::abs(existing.height - h) < .1) {
                const auto a = existing.transform.yaw * std::numbers::pi / 180;
                PlanPoint start{existing.transform.x, existing.transform.y},
                    end{start[0] + existing.width * std::cos(a), start[1] + existing.width * std::sin(a)};
                auto across = [&](PlanPoint p) {
                    return -(p[0] - w.transform.x) * std::sin(angle) +
                           (p[1] - w.transform.y) * std::cos(angle);
                };
                if (std::abs(across(start)) > .1 || std::abs(across(end)) > .1)
                    continue;
                auto along = [&](PlanPoint p) {
                    return (p[0] - w.transform.x) * std::cos(angle) +
                           (p[1] - w.transform.y) * std::sin(angle);
                };
                const auto low = std::min(along(start), along(end)),
                           high = std::max(along(start), along(end));
                std::vector<std::pair<double, double>> remaining;
                for (auto [left, right] : segments) {
                    if (high <= left + .1 || low >= right - .1)
                        remaining.emplace_back(left, right);
                    else {
                        if (low > left + .1)
                            remaining.emplace_back(left, low);
                        if (high < right - .1)
                            remaining.emplace_back(high, right);
                    }
                }
                segments = std::move(remaining);
            }
        for (auto [start, end] : segments) {
            auto piece = w;
            piece.id = uuid();
            piece.width = end - start;
            piece.transform.x += start * std::cos(angle);
            piece.transform.y += start * std::sin(angle);
            piece.metadata["edgeStart"] = start;
            piece.metadata["edgeEnd"] = end;
            d.entities.push_back(piece);
        }
    }
    auto floor = entity("Floor", "Piso");
    floor.parent = r.id;
    floor.width = r.width;
    floor.depth = r.depth;
    floor.height = 25;
    floor.transform = {minx, miny, -25, 0, false};
    floor.material = "oak";
    floor.parameters["roomOutline"] = true;
    d.entities.push_back(floor);
    auto ceiling = floor;
    ceiling.id = uuid();
    ceiling.type = "Ceiling";
    ceiling.name = "Forro";
    ceiling.transform.z = h;
    ceiling.visible = false;
    ceiling.material = "white";
    d.entities.push_back(ceiling);
}
namespace {
void editRoom(Document &d, const std::string &id, const Outline &p, double h) {
    validateOutline(p);
    const auto original = d.at(id);
    const auto originalWorld = roomOutline(original, true);
    for (const auto &other : d.entities)
        if (other.type == "Room" && other.id != id) {
            const auto neighbor = roomOutline(other, true);
            for (size_t i = 0; i < originalWorld.size(); ++i)
                for (size_t j = 0; j < neighbor.size(); ++j) {
                    auto a = originalWorld[i], b = originalWorld[(i + 1) % originalWorld.size()],
                         c = neighbor[j], end = neighbor[(j + 1) % neighbor.size()];
                    const auto length = std::hypot(b[0] - a[0], b[1] - a[1]);
                    if (std::abs(cross(a, b, c)) > .1 * length || std::abs(cross(a, b, end)) > .1 * length)
                        continue;
                    auto along = [&](PlanPoint v) {
                        return ((v[0] - a[0]) * (b[0] - a[0]) + (v[1] - a[1]) * (b[1] - a[1])) / length;
                    };
                    if (std::min(length, std::max(along(c), along(end))) -
                            std::max(0.0, std::min(along(c), along(end))) >
                        1)
                        throw std::invalid_argument("Este cômodo compartilha uma parede. O ajuste conjunto "
                                                    "dos dois cômodos ainda não está disponível.");
                }
        }
    if (!original.parameters.contains("outline") || p.size() != roomOutline(original).size() ||
        area(p) <= 0 || h < 500 || h > 10000)
        throw std::invalid_argument("Mantenha a quantidade e a ordem dos cantos ao editar.");
    auto candidate = original;
    candidate.parameters["outline"] = p;
    candidate.height = h;
    for (const auto &r : d.entities)
        if (r.type == "Room" && r.id != id && overlaps(roomOutline(candidate, true), roomOutline(r, true)))
            throw std::invalid_argument("Este contorno invade outro cômodo.");
    double w = 0, depth = 0;
    for (auto v : p) {
        if (v[0] < 0 || v[1] < 0)
            throw std::invalid_argument("Os cantos precisam ficar acima de zero.");
        w = std::max(w, v[0]);
        depth = std::max(depth, v[1]);
    }
    auto world = roomOutline(candidate, true);
    const auto previous = roomOutline(original, true);
    for (size_t i = 0; i < previous.size(); ++i) {
        const auto a = previous[i], b = previous[(i + 1) % previous.size()];
        double length = 0;
        for (const auto &wall : d.entities)
            if (wall.parent == id && wall.type == "Wall" && wall.metadata.value("roomEdge", size_t(-1)) == i)
                length += wall.width;
        if (std::abs(length - std::hypot(b[0] - a[0], b[1] - a[1])) > .1)
            throw std::invalid_argument("Este cômodo compartilha uma parede. O ajuste conjunto dos dois "
                                        "cômodos ainda não está disponível.");
    }
    for (auto &e : d.entities)
        if (e.parent == id) {
            if (e.type == "Wall" && e.metadata.contains("roomEdge")) {
                auto i = e.metadata.at("roomEdge").get<size_t>();
                auto a = world.at(i), b = world[(i + 1) % world.size()];
                auto replacement = wall(a[0], a[1], b[0], b[1], h, e.depth);
                e.transform = replacement.transform;
                e.width = replacement.width;
                e.height = h;
                e.metadata["edgeStart"] = 0;
                e.metadata["edgeEnd"] = e.width;
                for (const auto &o : d.entities)
                    if (o.parent == e.id && (o.type == "Door" || o.type == "Window") &&
                        (o.parameters.value("offset", 0.0) + o.width > e.width ||
                         o.parameters.value("sill", 0.0) + o.height > h))
                        throw std::invalid_argument("A nova parede deixa uma porta ou janela sem espaço.");
            } else if (e.type == "Floor" || e.type == "Ceiling") {
                e.width = w;
                e.depth = depth;
                if (e.type == "Ceiling")
                    e.transform.z = original.transform.z + h;
            }
        }
    auto &room = d.at(id);
    room.parameters["outline"] = p;
    room.width = w;
    room.depth = depth;
    room.height = h;
}
} // namespace
void editPolygonRoom(Document &d, const std::string &id, const Outline &p, double h) {
    auto candidate = d;
    editRoom(candidate, id, p, h);
    candidate.validate();
    d = std::move(candidate);
}
} // namespace lmx
