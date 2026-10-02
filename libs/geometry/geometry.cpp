#include "geometry.h"
#include "library/model.h"
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
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <cmath>
#include <gp_Circ.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf.hxx>
#include <numbers>
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
        block(0, 0, 0, w, d, h);
        return out;
    }
    throw std::invalid_argument("Geometria não suportada");
}
std::vector<Part> buildScene(const Document &d) {
    std::vector<Part> all;
    for (const auto &e : d.entities)
        if (e.visible) {
            bool hidden = false;
            auto parent = e.parent;
            while (!parent.empty()) {
                const auto &p = d.at(parent);
                hidden |= !p.visible;
                parent = p.parent;
            }
            if (hidden)
                continue;
            auto parts = buildEntity(d, e);
            all.insert(all.end(), parts.begin(), parts.end());
        }
    return all;
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
Json meshSnapshot(const Document &d) {
    d.validate();
    Json meshes = Json::array();
    for (const auto &part : buildScene(d)) {
        if (d.at(part.owner).type != "MeshObject") {
            BRepMesh_IncrementalMesh mesher(part.shape, 1.0, false, 0.35, true);
            if (!mesher.IsDone())
                throw std::runtime_error("Falha de tesselação");
        }
        Json vertices = Json::array(), triangles = Json::array(), uvs = Json::array(),
             normals = Json::array();
        for (TopExp_Explorer it(part.shape, TopAbs_FACE); it.More(); it.Next()) {
            auto face = TopoDS::Face(it.Current());
            TopLoc_Location loc;
            auto mesh = BRep_Tool::Triangulation(face, loc);
            if (mesh.IsNull())
                continue;
            int offset = static_cast<int>(vertices.size());
            for (int i = 1; i <= mesh->NbNodes(); ++i) {
                auto p = mesh->Node(i).Transformed(loc.Transformation());
                vertices.push_back({p.X() / 1000, p.Y() / 1000, p.Z() / 1000});
                if (d.at(part.owner).type == "MeshObject") {
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
        meshes.push_back({{"owner", part.owner},
                          {"kind", d.at(part.owner).type},
                          {"material", part.material},
                          {"vertices", vertices},
                          {"triangles", triangles}});
        if (uvs.size() == vertices.size() && !uvs.empty())
            meshes.back()["uvs"] = std::move(uvs);
        if (normals.size() == vertices.size() && !normals.empty())
            meshes.back()["normals"] = std::move(normals);
    }
    Json lights = Json::array(), cameras = Json::array();
    for (const auto &e : d.entities)
        if (e.visible && (e.type == "Light" || e.type == "Camera")) {
            Json j = {{"id", e.id},
                      {"name", e.name},
                      {"position", {e.transform.x / 1000, e.transform.y / 1000, e.transform.z / 1000}},
                      {"parameters", e.parameters}};
            (e.type == "Light" ? lights : cameras).push_back(j);
        }
    return {{"schema", 1},
            {"materials", d.materials},
            {"meshes", meshes},
            {"lights", lights},
            {"cameras", cameras},
            {"assets", d.serialize()["embeddedAssets"]},
            {"renderSettings", d.renderSettings}};
}
} // namespace lmx
