#include "geometry.h"
#include "document/room_outline.h"
#include "library/model.h"
#include "scene_cache.h"
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <algorithm>
#include <cmath>
#include <gp_Circ.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf.hxx>
#include <map>
#include <numbers>
#include <set>
#include <stdexcept>

namespace lmx {
namespace {
TopoDS_Shape box(double x, double y, double z, double w, double d, double h) {
    if (w <= 0 || d <= 0 || h <= 0)
        throw std::invalid_argument("Volume degenerado");
    return BRepPrimAPI_MakeBox(gp_Pnt(x, y, z), w, d, h).Shape();
}
TopoDS_Shape place(const TopoDS_Shape &shape, const Transform &t, double width = 0) {
    TopoDS_Shape result = shape;
    if (t.mirrored) {
        gp_Trsf mirror;
        mirror.SetMirror(gp_Ax2(gp_Pnt(width / 2, 0, 0), gp_Dir(1, 0, 0)));
        result = BRepBuilderAPI_Transform(result, mirror, true).Shape();
    }
    gp_Trsf rotation;
    rotation.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), t.yaw * std::numbers::pi / 180);
    rotation.SetTranslationPart(gp_Vec(t.x, t.y, t.z));
    return BRepBuilderAPI_Transform(result, rotation, true).Shape();
}
TopoDS_Shape unite(const TopoDS_Shape &a, const TopoDS_Shape &b) {
    if (a.IsNull())
        return b;
    BRepAlgoAPI_Fuse fuse(a, b);
    fuse.Build();
    if (!fuse.IsDone())
        throw std::runtime_error("Falha de união geométrica");
    return fuse.Shape();
}
TopoDS_Shape cut(const TopoDS_Shape &a, const TopoDS_Shape &b) {
    BRepAlgoAPI_Cut op(a, b);
    op.Build();
    if (!op.IsDone())
        throw std::runtime_error("Falha no recorte geométrico");
    return op.Shape();
}
} // namespace
TopoDS_Shape compound(const std::vector<Part> &parts) {
    BRep_Builder b;
    TopoDS_Compound c;
    b.MakeCompound(c);
    for (const auto &p : parts)
        if (!p.shape.IsNull())
            b.Add(c, p.shape);
    return c;
}
double volume(const TopoDS_Shape &s) {
    GProp_GProps p;
    BRepGProp::VolumeProperties(s, p);
    return p.Mass();
}
std::vector<Part> buildEntity(const Document &doc, const Entity &e) {
    std::vector<Part> out;
    auto transform = e.transform;
    auto add = [&](const TopoDS_Shape &s, const std::string &mat = "") {
        out.push_back({place(s, transform, e.width), mat.empty() ? e.material : mat, e.id});
    };
    auto block = [&](double x, double y, double z, double w, double d, double h,
                     const std::string &mat = "") { add(box(x, y, z, w, d, h), mat); };
    const double w = e.width, h = e.height, d = e.depth;
    if (e.type == "MeshObject") {
        const auto model = readModel(doc.embeddedAssets.at(e.parameters.at("meshAsset").get<std::string>()));
        const double angle = e.transform.yaw * std::numbers::pi / 180;
        for (const auto &part : model.at("parts")) {
            const auto &vertices = part.at("vertices"), &triangles = part.at("triangles");
            Handle(Poly_Triangulation) mesh = new Poly_Triangulation(
                static_cast<int>(vertices.size()), static_cast<int>(triangles.size()), part.contains("uvs"));
            if (part.contains("normals"))
                mesh->AddNormals();
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                double x = vertices[i][0].get<double>() * w, y = vertices[i][1].get<double>() * d,
                       z = vertices[i][2].get<double>() * h;
                if (e.transform.mirrored)
                    x = w - x;
                mesh->SetNode(static_cast<int>(i) + 1,
                              gp_Pnt(e.transform.x + x * std::cos(angle) - y * std::sin(angle),
                                     e.transform.y + x * std::sin(angle) + y * std::cos(angle),
                                     e.transform.z + z));
                if (part.contains("uvs")) {
                    const auto &uv = part.at("uvs")[i];
                    mesh->SetUVNode(static_cast<int>(i) + 1,
                                    gp_Pnt2d(uv[0].get<double>(), uv[1].get<double>()));
                }
                if (part.contains("normals")) {
                    const auto &normal = part.at("normals")[i];
                    double nx = normal[0].get<double>() / w, ny = normal[1].get<double>() / d,
                           nz = normal[2].get<double>() / h;
                    if (e.transform.mirrored)
                        nx = -nx;
                    mesh->SetNormal(static_cast<int>(i) + 1,
                                    gp_Dir(nx * std::cos(angle) - ny * std::sin(angle),
                                           nx * std::sin(angle) + ny * std::cos(angle), nz));
                }
            }
            for (std::size_t i = 0; i < triangles.size(); ++i) {
                int a = triangles[i][0].get<int>() + 1, b = triangles[i][1].get<int>() + 1,
                    c = triangles[i][2].get<int>() + 1;
                if (e.transform.mirrored)
                    std::swap(b, c);
                mesh->SetTriangle(static_cast<int>(i) + 1, Poly_Triangle(a, b, c));
            }
            if (!part.contains("normals"))
                mesh->ComputeNormals();
            TopoDS_Face face;
            BRep_Builder builder;
            builder.MakeFace(face, mesh);
            out.push_back({face,
                           e.parameters.value("originalMaterials", true)
                               ? part.at("material").get<std::string>()
                               : e.material,
                           e.id});
        }
        return out;
    }
    if (e.type == "Room" || e.type == "Group" || e.type == "Camera" || e.type == "Light")
        return out;
    if (e.type == "Wall" || e.type == "HalfWall") {
        auto s = box(0, -d / 2, 0, w, d, h);
        if (!e.metadata.contains("roomEdge")) {
            const auto angle = e.transform.yaw * std::numbers::pi / 180;
            std::array<PlanPoint, 4> vertices{{{0, d / 2}, {w, d / 2}, {w, -d / 2}, {0, -d / 2}}};
            bool joined = false;
            for (int end = 0; end < 2; ++end) {
                const PlanPoint joint{e.transform.x + end * w * std::cos(angle),
                                      e.transform.y + end * w * std::sin(angle)};
                std::vector<std::pair<PlanPoint, double>> neighbors;
                for (const auto &other : doc.entities) {
                    if (other.id == e.id || (other.type != "Wall" && other.type != "HalfWall") ||
                        std::abs(other.transform.z - e.transform.z) > .1 || std::abs(other.height - h) > .1)
                        continue;
                    const auto a = other.transform.yaw * std::numbers::pi / 180;
                    for (int endpoint = 0; endpoint < 2; ++endpoint) {
                        PlanPoint p{other.transform.x + endpoint * other.width * std::cos(a),
                                    other.transform.y + endpoint * other.width * std::sin(a)};
                        if (std::hypot(p[0] - joint[0], p[1] - joint[1]) > .1)
                            continue;
                        const auto sign = endpoint == 0 ? 1.0 : -1.0;
                        neighbors.push_back(
                            {{sign * std::cos(a - angle), sign * std::sin(a - angle)}, other.depth});
                    }
                }
                if (neighbors.size() != 1)
                    continue;
                const auto [direction, thickness] = neighbors.front();
                if (std::abs(direction[1]) < .05)
                    continue;
                const auto inward = end == 0 ? 1.0 : -1.0;
                for (double side : {1.0, -1.0}) {
                    const double bx = side * inward * direction[1] * thickness / 2,
                                 by = -side * inward * direction[0] * thickness / 2;
                    const double x = end * w + bx + (side * d / 2 - by) * direction[0] / direction[1];
                    if (std::abs(x - end * w) > 4 * std::max(d, thickness))
                        continue;
                    const int index = end == 0 ? (side > 0 ? 0 : 3) : (side > 0 ? 1 : 2);
                    vertices[index] = {x, side * d / 2};
                    joined = true;
                }
            }
            if (joined) {
                BRepBuilderAPI_MakePolygon wire;
                for (auto p : vertices)
                    wire.Add(gp_Pnt(p[0], p[1], 0));
                wire.Close();
                s = BRepPrimAPI_MakePrism(BRepBuilderAPI_MakeFace(wire.Wire()).Face(), gp_Vec(0, 0, h))
                        .Shape();
            }
        }
        if (e.metadata.contains("roomEdge")) {
            const auto &room = doc.at(e.parent);
            const auto p = roomOutline(room);
            const auto edge = e.metadata.at("roomEdge").get<size_t>();
            auto offset = [&](size_t i) {
                const auto prev = p[(i + p.size() - 1) % p.size()], v = p[i], next = p[(i + 1) % p.size()];
                const double a = std::hypot(v[0] - prev[0], v[1] - prev[1]),
                             b = std::hypot(next[0] - v[0], next[1] - v[1]);
                PlanPoint n1{-(v[1] - prev[1]) / a, (v[0] - prev[0]) / a},
                    n2{-(next[1] - v[1]) / b, (next[0] - v[0]) / b};
                const double denominator = 1 + n1[0] * n2[0] + n1[1] * n2[1];
                if (denominator < .02)
                    throw std::invalid_argument("Este canto é estreito demais para a parede.");
                const double x = (n1[0] + n2[0]) * d / 2 / denominator,
                             y = (n1[1] + n2[1]) * d / 2 / denominator;
                const double angle = (room.transform.yaw - e.transform.yaw) * std::numbers::pi / 180;
                return PlanPoint{x * std::cos(angle) - y * std::sin(angle),
                                 x * std::sin(angle) + y * std::cos(angle)};
            };
            auto start = offset(edge), end = offset((edge + 1) % p.size());
            const auto originalLength = std::hypot(p[(edge + 1) % p.size()][0] - p[edge][0],
                                                   p[(edge + 1) % p.size()][1] - p[edge][1]);
            if (e.metadata.value("edgeStart", 0.0) > .1)
                start = {0, d / 2};
            if (e.metadata.value("edgeEnd", originalLength) < originalLength - .1)
                end = {0, d / 2};
            BRepBuilderAPI_MakePolygon wire;
            for (auto v : Outline{{start[0], start[1]},
                                  {w + end[0], end[1]},
                                  {w - end[0], -end[1]},
                                  {-start[0], -start[1]}})
                wire.Add(gp_Pnt(v[0], v[1], 0));
            wire.Close();
            s = BRepPrimAPI_MakePrism(BRepBuilderAPI_MakeFace(wire.Wire()).Face(), gp_Vec(0, 0, h)).Shape();
        }
        for (const auto &o : doc.entities)
            if (o.parent == e.id && (o.type == "Door" || o.type == "Window"))
                s = cut(s, box(o.parameters.value("offset", 0.0), -d / 2 - 1, o.parameters.value("sill", 0.0),
                               o.width, d + 2, o.height));
        add(s);
        return out;
    }
    if (e.type == "Door" || e.type == "Window") {
        const auto &wall = doc.at(e.parent);
        const double angle = wall.transform.yaw * std::numbers::pi / 180,
                     offset = e.parameters.value("offset", 0.0);
        transform = wall.transform;
        transform.x += std::cos(angle) * offset;
        transform.y += std::sin(angle) * offset;
        transform.z += e.parameters.value("sill", 0.0);
        const double frame = std::min(45.0, std::min(w, h) / 4), depth = wall.depth;
        block(0, -depth / 2, 0, frame, depth, h, "oak");
        block(w - frame, -depth / 2, 0, frame, depth, h, "oak");
        block(frame, -depth / 2, h - frame, w - 2 * frame, depth, frame, "oak");
        if (e.type == "Window") {
            block(frame, -depth / 2, 0, w - 2 * frame, depth, frame, "oak");
            block(frame, -3, frame, w - 2 * frame, 6, h - 2 * frame, "glass");
        } else {
            const bool right = e.parameters.value("hinge", std::string("left")) == "right";
            Transform leaf;
            leaf.x = right ? w - frame : frame;
            leaf.y = 0;
            leaf.z = 0;
            leaf.yaw = e.parameters.value("openAngle", 30.0) * (right ? -1 : 1);
            auto s = place(box(right ? -(w - 2 * frame) : 0, -18, 0, w - 2 * frame, 36, h - frame), leaf);
            add(s, "oak");
        }
        return out;
    }
    if (e.type == "FurnitureModule") {
        const double t = 18, leg = e.parameters.value("legs", 100.0);
        const auto family = e.parameters.value("family", std::string("cabinet"));
        if (leg < 0 || leg >= h - 50)
            throw std::invalid_argument("Altura do pé inválida");
        const std::string carcass = e.parameters.value("carcass", std::string("oak"));
        block(0, 0, leg, t, d, h - leg, carcass);
        block(w - t, 0, leg, t, d, h - leg, carcass);
        block(t, 0, leg, w - 2 * t, d, t, carcass);
        block(t, 0, h - t, w - 2 * t, d, t, carcass);
        block(t, 0, leg + t, w - 2 * t, 6, h - leg - 2 * t, carcass);
        if (family == "shelf" || family == "niche") {
            if (family == "shelf")
                for (int i = 1; i <= 3; ++i)
                    block(t, 6, leg + (h - leg) * i / 4, w - 2 * t, d - 6, t, carcass);
        } else {
            int doors = std::clamp(e.parameters.value("doors", 2), 1, 6);
            bool drawer = family == "drawer";
            int count = drawer ? 3 : doors;
            for (int i = 0; i < count; ++i) {
                double fx = drawer ? 2 : i * w / count + 2,
                       fz = drawer ? leg + (h - leg) * i / count + 2 : leg + 2;
                double fw = drawer ? w - 4 : w / count - 4, fh = drawer ? (h - leg) / count - 4 : h - leg - 4;
                bool glass = e.parameters.value("glass", false);
                auto front = box(fx, d, fz, fw, t, fh);
                if (glass) {
                    front = cut(front, box(fx + 35, d - 1, fz + 35, fw - 70, t + 2, fh - 70));
                    block(fx + 35, d + 5, fz + 35, fw - 70, 6, fh - 70, "glass");
                }
                add(front);
                std::string handle = e.parameters.value("handle", std::string("bar"));
                if (handle == "point")
                    add(BRepPrimAPI_MakeSphere(gp_Pnt(fx + fw - 35, d + t + 12, fz + fh * 0.65), 10).Shape(),
                        "metal");
                else if (handle == "profile")
                    block(fx + 10, d + t, fz + fh - 15, fw - 20, 12, 10, "metal");
                else if (handle == "bar") {
                    const auto length = std::min(130.0, fh / 3);
                    const auto hx = fx + fw - 27, hz = fz + fh * 0.60;
                    add(BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(hx, d + t + 24, hz), gp_Dir(0, 0, 1)), 5,
                                                 length)
                            .Shape(),
                        "metal");
                    for (double z : {hz + 8, hz + length - 8})
                        add(BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(hx, d + t, z), gp_Dir(0, 1, 0)), 3.5, 24)
                                .Shape(),
                            "metal");
                }
            }
            block(t, 6, leg + (h - leg) / 2, w - 2 * t, d - 6, t, carcass);
        }
        if (leg > 0)
            for (double x : {35.0, w - 65})
                for (double y : {35.0, d - 65})
                    block(x, y, 0, 30, 30, leg, "metal");
        return out;
    }
    if (e.type == "Stair") {
        int steps = e.parameters.value("steps", 15);
        for (int i = 0; i < steps; ++i)
            block(0, d * i / steps, 0, w, d / steps, h * (i + 1) / steps);
        return out;
    }
    if (e.type == "GeometryObject" && e.metadata.value("format", std::string{}) == "DXF") {
        for (const auto &primitive : e.parameters.at("primitives")) {
            auto kind = primitive.at("kind").get<std::string>();
            if (kind == "line") {
                auto points = primitive.at("points");
                gp_Pnt a(points[0][0].get<double>(), points[0][1].get<double>(), 0),
                    b(points[1][0].get<double>(), points[1][1].get<double>(), 0);
                if (a.Distance(b) > 0.01)
                    add(BRepBuilderAPI_MakeEdge(a, b).Shape());
            } else {
                auto c = primitive.at("center");
                gp_Circ circle(gp_Ax2(gp_Pnt(c[0].get<double>(), c[1].get<double>(), 0), gp_Dir(0, 0, 1)),
                               primitive.at("radius").get<double>());
                double start = primitive.at("start").get<double>() * std::numbers::pi / 180,
                       end = primitive.at("end").get<double>() * std::numbers::pi / 180;
                if (end <= start)
                    end += 2 * std::numbers::pi;
                if (kind == "circle")
                    add(BRepBuilderAPI_MakeEdge(circle).Shape());
                else
                    add(BRepBuilderAPI_MakeEdge(circle, start, end).Shape());
            }
        }
        return out;
    }
    if (e.type == "GeometryObject" && e.metadata.contains("automation")) {
        const auto kind = e.metadata.at("automation").get<std::string>();
        TopoDS_Shape result;
        const double thick = e.parameters.value("thickness", 30.0),
                     over = e.parameters.value("overhang", 20.0);
        for (const auto &source : e.metadata.at("sources")) {
            const auto &m = doc.at(source.get<std::string>());
            TopoDS_Shape local;
            if (kind == "countertop")
                local = box(-over, -over, m.height, m.width + 2 * over, m.depth + 18 + 2 * over, thick);
            else if (kind == "backsplash")
                local = box(-over, -thick, m.height, m.width + 2 * over, thick, 100);
            else if (kind == "plinth")
                local = box(0, m.depth - 60, 0, m.width, thick,
                            std::max(10.0, m.parameters.value("legs", 100.0)));
            else if (kind == "cornice")
                local = box(0, m.depth, m.height, m.width, thick, 100);
            else if (kind == "envelope") {
                local = box(-thick, -over, 0, thick, m.depth + 18 + over, m.height + thick);
                local = unite(local, box(m.width, -over, 0, thick, m.depth + 18 + over, m.height + thick));
                local = unite(local, box(0, -over, m.height, m.width, m.depth + 18 + over, thick));
            } else if (kind == "closure")
                local = box(m.width, 0, 0, thick, m.depth + 18, m.height);
            else
                throw std::invalid_argument("Automação desconhecida");
            result = unite(result, place(local, m.transform, m.width));
        }
        out.push_back({result, e.material, e.id});
        return out;
    }
    if (e.type == "DecorativeObject") {
        auto family = e.parameters.value("family", std::string("table"));
        if (family == "table" || family == "chair" || family == "desk") {
            const double top = family == "chair" ? h * 0.5 : h - 35;
            block(0, 0, top, w, d, 35);
            for (double x : {15.0, w - 50})
                for (double y : {15.0, d - 50})
                    block(x, y, 0, 35, 35, top, "oak");
            if (family == "chair")
                block(0, 0, top + 35, w, 35, h - top - 35);
        } else if (family == "bed") {
            block(0, 0, 100, w, d, 100, "oak");
            block(10, 10, 200, w - 20, d - 20, std::max(50.0, h - 200), "white");
            block(0, 0, 0, w, 45, h + 450, "oak");
            block(40, d * 0.15, h, w - 80, d * 0.55, 12, "fabric");
            for (int i = 0; i < 2; ++i)
                block(60 + i * w / 2, d * 0.04, h, w / 2 - 100, 350, 60, "white");
        } else if (family == "sofa") {
            block(0, 0, 80, w, d, 180, "oak");
            block(0, 0, 260, w, 160, h - 260);
            block(0, 160, 260, 140, d - 160, h * 0.7 - 260);
            block(w - 140, 160, 260, 140, d - 160, h * 0.7 - 260);
            for (int i = 0; i < 3; ++i)
                block(145 + i * (w - 290) / 3, 170, 260, (w - 300) / 3, d - 180, 140);
        } else if (family == "fridge") {
            block(0, 0, 0, w, d, h, "metal");
            block(3, d, 3, w - 6, 30, h * 0.65 - 6);
            block(3, d, h * 0.65, w - 6, 30, h * 0.35 - 3);
            block(w - 60, d + 30, h * 0.2, 12, 25, 350, "metal");
            block(w - 60, d + 30, h * 0.75, 12, 25, 250, "metal");
        } else if (family == "oven") {
            block(0, 0, 0, w, d, h, "metal");
            block(25, d, 60, w - 50, 8, h - 150, "glass");
            block(45, d + 8, h - 100, w - 90, 20, 12, "metal");
        } else if (family == "vase" || family == "plant") {
            double radius = std::min(w, d) / 2;
            auto pot = BRepPrimAPI_MakeCone(radius * 0.65, radius, h * 0.6).Shape();
            pot =
                cut(pot, BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(0, 0, 15), gp_Dir(0, 0, 1)), radius * 0.55, h)
                             .Shape());
            Transform center;
            center.x = w / 2;
            center.y = d / 2;
            add(place(pot, center));
            if (family == "plant") {
                add(BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(w / 2, d / 2, h * 0.4), gp_Dir(0, 0, 1)), 8,
                                             h * 0.6)
                        .Shape(),
                    "oak");
                for (int i = 0; i < 7; ++i) {
                    double a = i * 2.4;
                    add(BRepPrimAPI_MakeSphere(gp_Pnt(w / 2 + std::cos(a) * radius * 0.55,
                                                      d / 2 + std::sin(a) * radius * 0.55,
                                                      h * 0.6 + i * h * 0.05),
                                               radius * 0.4)
                            .Shape(),
                        "fabric");
                }
            }
        } else if (family == "rug")
            block(0, 0, 0, w, d, 8);
        else if (family == "picture") {
            block(0, 0, 0, w, 30, h, "oak");
            block(25, 30, 25, w - 50, 3, h - 50, "fabric");
        } else
            throw std::invalid_argument("Objeto decorativo desconhecido");
        return out;
    }
    if (e.type == "Floor" || e.type == "Ceiling" || e.type == "GeometryObject") {
        if (e.parameters.value("roomOutline", false)) {
            BRepBuilderAPI_MakePolygon wire;
            for (auto p : roomOutline(doc.at(e.parent)))
                wire.Add(gp_Pnt(p[0], p[1], 0));
            wire.Close();
            add(BRepPrimAPI_MakePrism(BRepBuilderAPI_MakeFace(wire.Wire()).Face(), gp_Vec(0, 0, h)).Shape());
            return out;
        }
        block(0, 0, 0, w, d, h);
        return out;
    }
    throw std::invalid_argument("Geometria não suportada");
}
std::vector<Part> buildScene(const Document &d) {
    SceneGeometryCache cache;
    return cache.scene(d);
}
Entity automation(const Document &d, const std::vector<std::string> &ids, const std::string &kind,
                  double thickness, double overhang) {
    if (ids.empty() || thickness < 1 || thickness > 300 || overhang < 0 || overhang > 300)
        throw std::invalid_argument("Selecione módulos e parâmetros válidos");
    Json sources = Json::array();
    for (const auto &id : ids) {
        const auto &e = d.at(id);
        if (e.type != "FurnitureModule")
            throw std::invalid_argument("Selecione apenas módulos editáveis");
        sources.push_back(id);
    }
    auto e = entity("GeometryObject", kind);
    e.material = kind == "countertop" || kind == "backsplash" ? "stone" : "oak";
    e.parameters = {{"thickness", thickness}, {"overhang", overhang}};
    e.metadata = {{"automation", kind}, {"sources", sources}};
    return e;
}
Json meshSnapshot(const Document &d, bool instances) {
    d.validate();
    Json meshes = Json::array();
    Json placements = Json::array();
    std::map<std::string, std::string> definitions;
    for (const auto &part : buildScene(d)) {
        const auto &owner = d.at(part.owner);
        const auto material = std::find_if(d.materials.begin(), d.materials.end(),
                                           [&](const auto &m) { return m.at("id") == part.material; });
        if (material == d.materials.end())
            throw std::invalid_argument("Material de componente não encontrado");
        const bool mapped = material->contains("baseColorTexture") ||
                            material->contains("roughnessTexture") || material->contains("normalTexture");
        bool authoredUV = owner.type == "MeshObject" && material->value("modelUV", false);
        if (authoredUV)
            for (TopExp_Explorer it(part.shape, TopAbs_FACE); it.More(); it.Next()) {
                TopLoc_Location location;
                const auto mesh = BRep_Tool::Triangulation(TopoDS::Face(it.Current()), location);
                authoredUV &= !mesh.IsNull() && mesh->HasUVNodes();
            }
        // Legacy generated UVs depend on world position. Keep those meshes expanded.
        const bool share = instances && (!mapped || authoredUV);
        const auto shape = share ? part.shape.Located(TopLoc_Location()) : part.shape;
        const auto key = std::to_string(reinterpret_cast<std::uintptr_t>(shape.TShape().get())) + "/" +
                         part.material + "/" + owner.type;
        if (instances) {
            const auto transform = share ? part.shape.Location().Transformation() : gp_Trsf{};
            Json matrix = Json::array();
            for (int row = 1; row <= 4; ++row) {
                Json values = Json::array();
                for (int column = 1; column <= 4; ++column)
                    values.push_back(row == 4 ? (column == 4 ? 1.0 : 0.0)
                                              : transform.Value(row, column) / (column == 4 ? 1000.0 : 1.0));
                matrix.push_back(std::move(values));
            }
            auto definition = share ? definitions.find(key) : definitions.end();
            const auto meshId = definition == definitions.end() ? "mesh-" + std::to_string(meshes.size())
                                                                : definition->second;
            placements.push_back({{"mesh", meshId}, {"owner", part.owner}, {"matrix", std::move(matrix)}});
            if (definition != definitions.end())
                continue;
            if (share)
                definitions[key] = meshId;
        }
        if (owner.type != "MeshObject") {
            BRepMesh_IncrementalMesh mesher(shape, 1.0, false, 0.35, true);
            if (!mesher.IsDone())
                throw std::runtime_error("Falha de tesselação");
        }
        Json vertices = Json::array(), triangles = Json::array(), uvs = Json::array(),
             normals = Json::array();
        for (TopExp_Explorer it(shape, TopAbs_FACE); it.More(); it.Next()) {
            auto face = TopoDS::Face(it.Current());
            TopLoc_Location loc;
            auto mesh = BRep_Tool::Triangulation(face, loc);
            if (mesh.IsNull())
                continue;
            int offset = static_cast<int>(vertices.size());
            for (int i = 1; i <= mesh->NbNodes(); ++i) {
                auto p = mesh->Node(i).Transformed(loc.Transformation());
                vertices.push_back({p.X() / 1000, p.Y() / 1000, p.Z() / 1000});
                if (owner.type == "MeshObject") {
                    if (mesh->HasUVNodes()) {
                        const auto uv = mesh->UVNode(i);
                        uvs.push_back({uv.X(), uv.Y()});
                    }
                    if (mesh->HasNormals()) {
                        const auto normal = mesh->Normal(i).Transformed(loc.Transformation());
                        normals.push_back({normal.X(), normal.Y(), normal.Z()});
                    }
                }
            }
            for (int i = 1; i <= mesh->NbTriangles(); ++i) {
                int a, b, c;
                mesh->Triangle(i).Get(a, b, c);
                if (face.Orientation() == TopAbs_REVERSED)
                    std::swap(b, c);
                triangles.push_back({offset + a - 1, offset + b - 1, offset + c - 1});
            }
        }
        const auto vertexCount = vertices.size();
        meshes.push_back({{"owner", part.owner}, {"kind", owner.type}, {"material", part.material}});
        meshes.back()["vertices"] = std::move(vertices);
        meshes.back()["triangles"] = std::move(triangles);
        if (instances)
            meshes.back()["id"] = "mesh-" + std::to_string(meshes.size() - 1);
        if (uvs.size() == vertexCount && !uvs.empty())
            meshes.back()["uvs"] = std::move(uvs);
        if (normals.size() == vertexCount && !normals.empty())
            meshes.back()["normals"] = std::move(normals);
    }
    Json lights = Json::array(), cameras = Json::array();
    for (const auto &e : d.entities)
        if (e.visible && (e.type == "Light" || e.type == "Camera")) {
            Json j = {{"id", e.id},
                      {"name", e.name},
                      {"position", {e.transform.x / 1000, e.transform.y / 1000, e.transform.z / 1000}},
                      {"rotationZ", e.transform.yaw},
                      {"parameters", e.parameters}};
            (e.type == "Light" ? lights : cameras).push_back(j);
        }
    Json assets = Json::object();
    if (instances) {
        std::set<std::string> required;
        for (const auto &material : d.materials)
            for (const auto *channel : {"baseColorTexture", "roughnessTexture", "normalTexture"})
                if (material.contains(channel))
                    required.insert(material.at(channel).get<std::string>());
        if (d.renderSettings.value("environmentMode", "studio") == "hdri")
            required.insert(d.renderSettings.at("hdri").at("asset").get<std::string>());
        for (const auto &hash : required)
            assets[hash] = d.embeddedAssets.at(hash).toBase64().toStdString();
    } else
        assets = d.serialize()["embeddedAssets"];
    Json result = {{"schema", instances ? 2 : 1},
                   {"materials", d.materials},
                   {"lights", lights},
                   {"cameras", cameras},
                   {"renderSettings", d.renderSettings}};
    result["meshes"] = std::move(meshes);
    result["assets"] = std::move(assets);
    if (instances)
        result["instances"] = std::move(placements);
    return result;
}
} // namespace lmx
