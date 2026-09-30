#include "document.h"
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
#include <stdexcept>

namespace lmx {
std::string uuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
}
double millimeters(double value) {
    if (!std::isfinite(value) || std::abs(value) > 1e7)
        throw std::invalid_argument("Medida fora do limite (10 km)");
    return std::round(value * 10.0) / 10.0;
}
Entity entity(std::string type, std::string name) {
    Entity e;
    e.id = uuid();
    e.type = std::move(type);
    e.name = std::move(name);
    return e;
}
Json materialPresets() {
    Json materials = Json::array();
    auto add = [&](const char *id, const char *name, Json color, double roughness, double metallic,
                   double transmission = 0.0) {
        materials.push_back({{"id", id},
                             {"name", name},
                             {"baseColor", color},
                             {"roughness", roughness},
                             {"metallic", metallic},
                             {"transmission", transmission},
                             {"ior", 1.45},
                             {"opacity", 1.0}});
    };
    add("paint", "Pintura areia", {0.82, 0.78, 0.70}, 0.8, 0);
    add("white", "Branco fosco", {0.92, 0.92, 0.88}, 0.5, 0);
    add("oak", "Carvalho", {0.48, 0.29, 0.14}, 0.5, 0);
    add("stone", "Pedra escura", {0.12, 0.14, 0.15}, 0.3, 0);
    add("glass", "Vidro", {0.88, 0.96, 0.98}, 0.05, 0, 1);
    add("metal", "Inox", {0.62, 0.65, 0.68}, 0.25, 1);
    add("mirror", "Espelho", {0.96, 0.96, 0.96}, 0.01, 1);
    add("fabric", "Tecido verde", {0.21, 0.34, 0.27}, 0.9, 0);
    return materials;
}
Document::Document() : id(uuid()), materials(materialPresets()) {
}
Entity &Document::at(const std::string &target) {
    auto i = std::find_if(entities.begin(), entities.end(), [&](const auto &e) { return e.id == target; });
    if (i == entities.end())
        throw std::invalid_argument("Objeto não encontrado");
    return *i;
}
const Entity &Document::at(const std::string &target) const {
    return const_cast<Document *>(this)->at(target);
}
bool Document::contains(const std::string &target) const {
    return std::any_of(entities.begin(), entities.end(), [&](const auto &e) { return e.id == target; });
}
void Document::validate() const {
    static const std::set<std::string> types = {"Wall",
                                                "HalfWall",
                                                "Room",
                                                "Floor",
                                                "Ceiling",
                                                "Door",
                                                "Window",
                                                "Stair",
                                                "FurnitureModule",
                                                "DecorativeObject",
                                                "GeometryObject",
                                                "Light",
                                                "Camera",
                                                "Group"};
    if (version != 1 || QUuid(QString::fromStdString(id)).isNull() || name.empty() || name.size() > 512 ||
        entities.size() > 10000)
        throw std::invalid_argument("Documento inválido ou versão não suportada");
    if (!materials.is_array() || materials.size() > 1000)
        throw std::invalid_argument("Materiais inválidos");
    qint64 assetTotal = 0;
    for (const auto &[hash, bytes] : embeddedAssets) {
        assetTotal += bytes.size();
        if (!QRegularExpression("^[a-f0-9]{64}$").match(QString::fromStdString(hash)).hasMatch() ||
            bytes.size() > 16 * 1024 * 1024 || assetTotal > 48 * 1024 * 1024 ||
            QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex().toStdString() != hash)
            throw std::invalid_argument("Asset incorporado inválido");
    }
    std::set<std::string> materialIds, ids;
    for (const auto &m : materials) {
        const auto mid = m.at("id").get<std::string>();
        const auto materialName = m.at("name").get<std::string>();
        if (mid.empty() || mid.size() > 128 || materialName.empty() || materialName.size() > 512 ||
            !materialIds.insert(mid).second || !m.at("baseColor").is_array() || m.at("baseColor").size() != 3)
            throw std::invalid_argument("Material inválido");
        for (const auto &c : m.at("baseColor"))
            if (!c.is_number() || !std::isfinite(c.get<double>()) || c.get<double>() < 0 ||
                c.get<double>() > 1)
                throw std::invalid_argument("Cor inválida");
        if (m.contains("baseColorTexture") &&
            (!m.at("baseColorTexture").is_string() ||
             !embeddedAssets.contains(m.at("baseColorTexture").get<std::string>()) ||
             !std::isfinite(m.value("textureScale", 1000.0)) || m.value("textureScale", 1000.0) < 1 ||
             m.value("textureScale", 1000.0) > 1e7))
            throw std::invalid_argument("Textura ou escala de material inválida");
        if (!std::isfinite(m.value("ior", 1.45)) || m.value("ior", 1.45) < 1 || m.value("ior", 1.45) > 3)
            throw std::invalid_argument("Índice de refração inválido");
        for (auto key : {"roughness", "metallic", "transmission", "opacity"}) {
            double v = m.value(key, 0.0);
            if (!std::isfinite(v) || v < 0 || v > 1)
                throw std::invalid_argument("Propriedade PBR inválida");
        }
    }
    for (const auto &e : entities) {
        if (QUuid(QString::fromStdString(e.id)).isNull() || !ids.insert(e.id).second ||
            !types.contains(e.type) || e.name.empty() || e.name.size() > 512)
            throw std::invalid_argument("Identidade de objeto inválida");
        for (double v :
             {e.transform.x, e.transform.y, e.transform.z, e.transform.yaw, e.width, e.height, e.depth})
            millimeters(v);
        if (e.width < 0.1 || e.height < 0.1 || e.depth < 0.1 || !materialIds.contains(e.material))
            throw std::invalid_argument("Dimensão ou material inválido");
        if (!e.parameters.is_object() || !e.metadata.is_object() || e.parameters.dump().size() > 65536 ||
            e.metadata.dump().size() > 65536)
            throw std::invalid_argument("Parâmetros inválidos");
        if (e.metadata.value("format", std::string{}) == "DXF") {
            const auto &primitives = e.parameters.at("primitives");
            if (!primitives.is_array() || primitives.empty() || primitives.size() > 20000)
                throw std::invalid_argument("Referência DXF inválida");
            for (const auto &primitive : primitives) {
                auto kind = primitive.at("kind").get<std::string>();
                if (kind == "line") {
                    auto points = primitive.at("points");
                    if (!points.is_array() || points.size() != 2)
                        throw std::invalid_argument("Linha DXF inválida");
                    for (const auto &point : points) {
                        if (!point.is_array() || point.size() != 2)
                            throw std::invalid_argument("Ponto DXF inválido");
                        for (const auto &coordinate : point)
                            millimeters(coordinate.get<double>());
                    }
                } else if (kind == "arc" || kind == "circle") {
                    auto center = primitive.at("center");
                    if (!center.is_array() || center.size() != 2 ||
                        primitive.at("radius").get<double>() < 0.1)
                        throw std::invalid_argument("Curva DXF inválida");
                    for (const auto &coordinate : center)
                        millimeters(coordinate.get<double>());
                    millimeters(primitive.at("radius").get<double>());
                    for (const auto *key : {"start", "end"})
                        if (std::abs(primitive.at(key).get<double>()) > 3600)
                            throw std::invalid_argument("Ângulo DXF inválido");
                } else
                    throw std::invalid_argument("Primitiva DXF não suportada");
            }
        }
        for (const auto &[key, val] : e.parameters.items()) {
            if (val.is_number())
                millimeters(val.get<double>());
            if (val.is_string() && val.get<std::string>().size() > 512)
                throw std::invalid_argument("Texto longo demais");
            (void)key;
        }
        if (!e.parent.empty() && (!contains(e.parent) || e.parent == e.id))
            throw std::invalid_argument("Referência de pai inválida");
        if (e.type == "Door" || e.type == "Window") {
            if (e.parent.empty())
                throw std::invalid_argument("Abertura exige parede");
            const auto &w = at(e.parent);
            const auto hinge = e.parameters.value("hinge", std::string("left"));
            if ((hinge != "left" && hinge != "right") || e.parameters.value("openAngle", 30.0) < 0 ||
                e.parameters.value("openAngle", 30.0) > 180)
                throw std::invalid_argument("Dobradiça ou ângulo de abertura inválido");
            double offset = e.parameters.value("offset", 0.0), sill = e.parameters.value("sill", 0.0);
            if ((w.type != "Wall" && w.type != "HalfWall") || offset < 0 || offset + e.width > w.width ||
                sill < 0 || sill + e.height > w.height)
                throw std::invalid_argument("Abertura fora da parede");
            for (const auto &other : entities)
                if (other.id != e.id && other.parent == e.parent &&
                    (other.type == "Door" || other.type == "Window")) {
                    double o = other.parameters.value("offset", 0.0), s = other.parameters.value("sill", 0.0);
                    if (offset < o + other.width && o < offset + e.width && sill < s + other.height &&
                        s < sill + e.height)
                        throw std::invalid_argument("Aberturas sobrepostas");
                }
        }
        if (e.type == "FurnitureModule") {
            auto family = e.parameters.value("family", std::string("cabinet"));
            auto handle = e.parameters.value("handle", std::string("bar"));
            const std::set<std::string> families = {"cabinet", "drawer", "shelf", "niche"},
                                        handles = {"bar", "profile", "point", "none"};
            int doors = e.parameters.value("doors", 2);
            double legs = e.parameters.value("legs", 100.0);
            if (!families.contains(family) || !handles.contains(handle) || doors < 1 || doors > 6 ||
                e.width < 100 || e.depth < 100 || e.height < 100 || legs < 0 || legs >= e.height - 75 ||
                e.width < doors * 75)
                throw std::invalid_argument("Parâmetros do módulo inválidos");
            if (!materialIds.contains(e.parameters.value("carcass", std::string("oak"))))
                throw std::invalid_argument("Material do corpo não encontrado");
            if (e.parameters.value("glass", false) &&
                (family == "drawer" ? (e.height - legs) / 3 - 4 <= 70 : e.height - legs - 4 <= 70))
                throw std::invalid_argument("Frente pequena demais para moldura de vidro");
        }
        if (e.type == "DecorativeObject") {
            auto family = e.parameters.value("family", std::string("table"));
            const std::set<std::string> families = {"table", "chair", "desk",  "bed", "sofa",   "fridge",
                                                    "oven",  "vase",  "plant", "rug", "picture"};
            bool valid = families.contains(family);
            if (family == "table" || family == "chair" || family == "desk")
                valid = e.width >= 100 && e.depth >= 100 && e.height >= 80;
            else if (family == "bed")
                valid = e.width > 200 && e.depth > 200 && e.height >= 200;
            else if (family == "sofa")
                valid = e.width >= 350 && e.depth >= 200 && e.height >= 400;
            else if (family == "fridge")
                valid = e.width >= 75 && e.height >= 100;
            else if (family == "oven")
                valid = e.width >= 100 && e.height >= 175;
            else if (family == "vase" || family == "plant")
                valid = e.width >= 20 && e.depth >= 20 && e.height >= 25;
            else if (family == "picture")
                valid = e.width > 50 && e.height > 50;
            if (!valid)
                throw std::invalid_argument("Família ou dimensões do objeto decorativo inválidas");
        }
        if (e.type == "Light" || e.type == "Camera") {
            auto target = e.parameters.value("target", Json::array({2000, 1500, 1000}));
            if (!target.is_array() || target.size() != 3)
                throw std::invalid_argument("Alvo inválido");
            for (const auto &v : target)
                millimeters(v.get<double>());
            if (e.type == "Light") {
                auto kind = e.parameters.value("kind", std::string("area"));
                if ((kind != "area" && kind != "point" && kind != "spot") ||
                    e.parameters.value("power", 500.0) < 0 || e.parameters.value("power", 500.0) > 100000 ||
                    e.parameters.value("size", 1000.0) < 1)
                    throw std::invalid_argument("Luz inválida");
            }
            if (e.type == "Camera" &&
                (e.parameters.value("lens", 28.0) < 1 || e.parameters.value("lens", 28.0) > 1000))
                throw std::invalid_argument("Lente inválida");
        }
        if (e.type == "Stair") {
            int steps = e.parameters.value("steps", 15);
            if (steps < 2 || steps > 200)
                throw std::invalid_argument("Número de degraus inválido");
        }
        if (e.metadata.contains("sources")) {
            const auto &sources = e.metadata.at("sources");
            if (!sources.is_array() || sources.empty() || sources.size() > 500)
                throw std::invalid_argument("Fontes de automação inválidas");
            for (const auto &source : sources)
                if (!source.is_string() || !contains(source.get<std::string>()) || source == e.id ||
                    at(source.get<std::string>()).type != "FurnitureModule")
                    throw std::invalid_argument("Fonte de automação inválida");
            auto kind = e.metadata.value("automation", std::string{});
            const std::set<std::string> kinds = {"countertop", "backsplash", "plinth",
                                                 "cornice",    "closure",    "envelope"};
            if (!kinds.contains(kind) || e.parameters.value("thickness", 30.0) < 1 ||
                e.parameters.value("thickness", 30.0) > 300 || e.parameters.value("overhang", 20.0) < 0 ||
                e.parameters.value("overhang", 20.0) > 300)
                throw std::invalid_argument("Automação inválida");
        }
        std::set<std::string> ancestors{e.id};
        auto p = e.parent;
        while (!p.empty()) {
            if (!ancestors.insert(p).second)
                throw std::invalid_argument("Ciclo na hierarquia");
            p = at(p).parent;
        }
    }
}
Json Document::serialize() const {
    Json list = Json::array();
    for (const auto &e : entities) {
        Json children = Json::array();
        for (const auto &c : entities)
            if (c.parent == e.id)
                children.push_back(c.id);
        list.push_back({{"uuid", e.id},
                        {"type", e.type},
                        {"name", e.name},
                        {"parent", e.parent},
                        {"children", children},
                        {"transform",
                         {{"x", e.transform.x},
                          {"y", e.transform.y},
                          {"z", e.transform.z},
                          {"yaw", e.transform.yaw},
                          {"mirrored", e.transform.mirrored}}},
                        {"visibility", e.visible},
                        {"locked", e.locked},
                        {"dimensions", {e.width, e.height, e.depth}},
                        {"material", e.material},
                        {"parameters", e.parameters},
                        {"metadata", e.metadata}});
    }
    Json assets = Json::object();
    for (const auto &[hash, bytes] : embeddedAssets)
        assets[hash] = bytes.toBase64().toStdString();
    return {{"version", version},      {"uuid", id},       {"name", name},
            {"units", "mm"},           {"entities", list}, {"materials", materials},
            {"embeddedAssets", assets}};
}
Document Document::deserialize(const Json &j) {
    Document d;
    if (j.at("units") != "mm")
        throw std::invalid_argument("Unidade interna deve ser mm");
    d.id = j.at("uuid");
    d.name = j.at("name");
    d.version = j.at("version");
    d.materials = j.at("materials");
    const auto storedAssets = j.value("embeddedAssets", Json::object());
    if (!storedAssets.is_object() || storedAssets.size() > 120)
        throw std::invalid_argument("Lista de assets inválida");
    for (const auto &[hash, value] : storedAssets.items()) {
        auto text = value.get<std::string>();
        auto decoded = QByteArray::fromBase64Encoding(QByteArray::fromStdString(text),
                                                      QByteArray::AbortOnBase64DecodingErrors);
        if (!decoded)
            throw std::invalid_argument("Asset base64 inválido");
        d.embeddedAssets[hash] = decoded.decoded;
    }
    if (!j.at("entities").is_array() || j.at("entities").size() > 10000)
        throw std::invalid_argument("Lista de entidades inválida");
    for (const auto &o : j.at("entities")) {
        Entity e;
        e.id = o.at("uuid");
        e.type = o.at("type");
        e.name = o.at("name");
        e.parent = o.at("parent");
        auto t = o.at("transform");
        e.transform = {t.at("x"), t.at("y"), t.at("z"), t.at("yaw"), t.value("mirrored", false)};
        e.visible = o.at("visibility");
        e.locked = o.at("locked");
        auto dims = o.at("dimensions");
        if (!dims.is_array() || dims.size() != 3)
            throw std::invalid_argument("Dimensões inválidas");
        e.width = dims[0];
        e.height = dims[1];
        e.depth = dims[2];
        e.material = o.at("material");
        e.parameters = o.at("parameters");
        e.metadata = o.at("metadata");
        d.entities.push_back(e);
    }
    d.validate();
    return d;
}
Entity wall(double x1, double y1, double x2, double y2, double height, double thickness, bool half) {
    auto e = entity(half ? "HalfWall" : "Wall", half ? "Mureta" : "Parede");
    e.transform.x = millimeters(x1);
    e.transform.y = millimeters(y1);
    e.transform.yaw = std::atan2(y2 - y1, x2 - x1) * 180 / std::numbers::pi;
    e.width = millimeters(std::hypot(x2 - x1, y2 - y1));
    e.height = millimeters(height);
    e.depth = millimeters(thickness);
    e.material = "paint";
    return e;
}
void addRectangularRoom(Document &d, double w, double depth, double h, double t) {
    if (w < 500 || depth < 500 || h < 100 || t < 10 || t > w / 4 || t > depth / 4)
        throw std::invalid_argument("Medidas do ambiente inválidas");
    auto r = entity("Room", "Ambiente");
    r.width = w;
    r.depth = depth;
    r.height = h;
    d.entities.push_back(r);
    const double points[4][2] = {{0, 0}, {w, 0}, {w, depth}, {0, depth}};
    for (int i = 0; i < 4; ++i) {
        auto e = wall(points[i][0], points[i][1], points[(i + 1) % 4][0], points[(i + 1) % 4][1], h, t);
        e.name = "Parede " + std::to_string(i + 1);
        e.parent = r.id;
        d.entities.push_back(e);
    }
    auto floor = entity("Floor", "Piso");
    floor.width = w - t;
    floor.depth = depth - t;
    floor.height = 25;
    floor.transform = {t / 2, t / 2, -25, 0, false};
    floor.material = "oak";
    floor.parent = r.id;
    d.entities.push_back(floor);
    auto ceiling = floor;
    ceiling.id = uuid();
    ceiling.name = "Forro";
    ceiling.type = "Ceiling";
    ceiling.transform.z = h;
    ceiling.material = "white";
    ceiling.visible = false;
    d.entities.push_back(ceiling);
}
void eraseCascade(Document &d, const std::vector<std::string> &requested) {
    std::set<std::string> ids(requested.begin(), requested.end());
    bool more = true;
    while (more) {
        more = false;
        for (const auto &e : d.entities)
            if (!ids.contains(e.id)) {
                bool dependent = ids.contains(e.parent);
                if (e.metadata.contains("sources"))
                    for (const auto &source : e.metadata["sources"])
                        dependent |= ids.contains(source.get<std::string>());
                if (dependent) {
                    ids.insert(e.id);
                    more = true;
                }
            }
    }
    std::erase_if(d.entities, [&](const auto &e) { return ids.contains(e.id); });
}
} // namespace lmx
