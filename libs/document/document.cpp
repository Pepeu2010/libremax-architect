#include "document.h"
#include "library/model.h"
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
    add("oak", "Carvalho", {0.32, 0.19, 0.095}, 0.5, 0);
    add("stone", "Pedra escura", {0.12, 0.14, 0.15}, 0.3, 0);
    add("glass", "Vidro", {0.88, 0.96, 0.98}, 0.05, 0, 1);
    add("metal", "Inox", {0.62, 0.65, 0.68}, 0.25, 1);
    add("mirror", "Espelho", {0.96, 0.96, 0.96}, 0.01, 1);
    add("fabric", "Tecido verde", {0.21, 0.34, 0.27}, 0.9, 0);
    add("graphite", "Grafite acetinado", {0.045, 0.065, 0.075}, 0.38, 0);
    add("porcelain", "Porcelanato areia", {0.55, 0.51, 0.43}, 0.55, 0);
    add("quartz", "Quartzo claro", {0.62, 0.59, 0.52}, 0.22, 0);
    add("ceramic", "Cerâmica esmaltada", {0.7, 0.69, 0.65}, 0.18, 0);
    for (auto &material : materials) {
        const auto id = material.at("id").get<std::string>();
        material["procedural"] = id == "oak"                                            ? "wood"
                                 : id == "stone" || id == "porcelain" || id == "quartz" ? "stone"
                                 : id == "fabric"                                       ? "fabric"
                                 : id == "paint" || id == "graphite"                    ? "paint"
                                                                                        : "none";
    }
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
    static const std::set<std::string> types = {
        "Wall",           "HalfWall",   "Room",  "Floor",           "Ceiling",
        "Door",           "Window",     "Stair", "FurnitureModule", "DecorativeObject",
        "GeometryObject", "MeshObject", "Light", "Camera",          "Group"};
    if (version != 1 || QUuid(QString::fromStdString(id)).isNull() || name.empty() || name.size() > 512 ||
        entities.size() > 10000)
        throw std::invalid_argument("Documento inválido ou versão não suportada");
    if (!materials.is_array() || materials.size() > 1000)
        throw std::invalid_argument("Materiais inválidos");
    if (!renderSettings.is_object())
        throw std::invalid_argument("Configuração de render inválida");
    for (auto key : {"exposure", "environmentStrength"})
        if (!std::isfinite(renderSettings.at(key).get<double>()))
            throw std::invalid_argument("Configuração de render inválida");
    if (std::abs(renderSettings.at("exposure").get<double>()) > 8 ||
        renderSettings.at("environmentStrength").get<double>() < 0 ||
        renderSettings.at("environmentStrength").get<double>() > 5 ||
        !renderSettings.at("denoise").is_boolean() || !renderSettings.at("camera").is_string())
        throw std::invalid_argument("Configuração de render fora do limite");
    auto selectedCamera = renderSettings.at("camera").get<std::string>();
    if (!selectedCamera.empty() && (!contains(selectedCamera) || at(selectedCamera).type != "Camera"))
        throw std::invalid_argument("Câmera de render não encontrada");
    const auto environment = renderSettings.value("environmentMode", std::string("studio"));
    const auto elevation = renderSettings.value("sunElevation", 35.0);
    const auto rotation = renderSettings.value("sunRotation", 30.0);
    if ((environment != "studio" && environment != "sky") || !std::isfinite(elevation) ||
        !std::isfinite(rotation) || elevation < 1 || elevation > 89 || rotation < 0 || rotation > 360)
        throw std::invalid_argument("Configuração de céu inválida");
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
        const std::set<std::string> procedures = {"none", "wood", "stone", "fabric", "paint"};
        if (!procedures.contains(m.value("procedural", std::string("none"))))
            throw std::invalid_argument("Material procedural inválido");
        if (mid.empty() || mid.size() > 128 || materialName.empty() || materialName.size() > 512 ||
            !materialIds.insert(mid).second || !m.at("baseColor").is_array() || m.at("baseColor").size() != 3)
            throw std::invalid_argument("Material inválido");
        for (const auto &c : m.at("baseColor"))
            if (!c.is_number() || !std::isfinite(c.get<double>()) || c.get<double>() < 0 ||
                c.get<double>() > 1)
                throw std::invalid_argument("Cor inválida");
        for (auto channel : {"baseColorTexture", "roughnessTexture", "normalTexture"})
            if (m.contains(channel) &&
                (!m.at(channel).is_string() || !embeddedAssets.contains(m.at(channel).get<std::string>()) ||
                 !std::isfinite(m.value("textureScale", 1000.0)) || m.value("textureScale", 1000.0) < 1 ||
                 m.value("textureScale", 1000.0) > 1e7))
                throw std::invalid_argument("Mapa ou escala de material inválido");
        if (!std::isfinite(m.value("normalStrength", 1.0)) || m.value("normalStrength", 1.0) < 0 ||
            m.value("normalStrength", 1.0) > 2)
            throw std::invalid_argument("Intensidade de relevo inválida");
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
        if (e.type == "MeshObject") {
            const auto hash = e.parameters.at("meshAsset").get<std::string>();
            if (!embeddedAssets.contains(hash))
                throw std::invalid_argument("Modelo 3D incorporado ausente");
            const auto model = readModel(embeddedAssets.at(hash));
            for (const auto &part : model.at("parts"))
                if (!materialIds.contains(part.at("material").get<std::string>()))
                    throw std::invalid_argument("Acabamento do modelo 3D ausente");
        }
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
                auto color = e.parameters.value("color", Json::array({1.0, 0.89, 0.73}));
                if (!color.is_array() || color.size() != 3)
                    throw std::invalid_argument("Cor de luz inválida");
                for (const auto &value : color)
                    if (!value.is_number() || !std::isfinite(value.get<double>()) ||
                        value.get<double>() < 0 || value.get<double>() > 1)
                        throw std::invalid_argument("Cor de luz inválida");
                auto kind = e.parameters.value("kind", std::string("area"));
                if ((kind != "area" && kind != "point" && kind != "spot") ||
                    e.parameters.value("power", 500.0) < 0 || e.parameters.value("power", 500.0) > 100000 ||
                    e.parameters.value("size", 1000.0) < 1 || e.parameters.value("angle", 45.0) < 1 ||
                    e.parameters.value("angle", 45.0) > 179 || e.parameters.value("blend", 0.3) < 0 ||
                    e.parameters.value("blend", 0.3) > 1)
                    throw std::invalid_argument("Luz inválida");
            }
            if (e.type == "Camera" &&
                (e.parameters.value("lens", 28.0) < 1 || e.parameters.value("lens", 28.0) > 1000 ||
                 e.parameters.value("fstop", 8.0) < 1 || e.parameters.value("fstop", 8.0) > 64 ||
                 e.parameters.value("focusDistance", 2500.0) < 100 ||
                 e.parameters.value("focusDistance", 2500.0) > 1e7))
                throw std::invalid_argument("Lente, abertura ou foco inválidos");
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
    return {{"version", version},
            {"uuid", id},
            {"name", name},
            {"units", "mm"},
            {"entities", list},
            {"materials", materials},
            {"embeddedAssets", assets},
            {"renderSettings", renderSettings}};
}
Document Document::deserialize(const Json &j) {
    Document d;
    if (j.at("units") != "mm")
        throw std::invalid_argument("Unidade interna deve ser mm");
    d.id = j.at("uuid");
    d.name = j.at("name");
    d.version = j.at("version");
    d.materials = j.at("materials");
    d.renderSettings = j.value("renderSettings", d.renderSettings);
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
void addRectangularRoom(Document &d, double w, double depth, double h, double t, double x, double y,
                        const std::string &name) {
    if (w < 500 || depth < 500 || h < 100 || t < 10 || t > w / 4 || t > depth / 4)
        throw std::invalid_argument("Medidas do ambiente inválidas");
    for (const auto &room : d.entities)
        if (room.type == "Room" && std::abs(room.transform.yaw) < 0.001 &&
            x < room.transform.x + room.width - 0.1 && room.transform.x < x + w - 0.1 &&
            y < room.transform.y + room.depth - 0.1 && room.transform.y < y + depth - 0.1)
            throw std::invalid_argument(
                "Este cômodo ocupa o espaço de outro. Escolha outro lado ou ajuste as medidas.");
    auto r = entity("Room", name);
    r.transform.x = millimeters(x);
    r.transform.y = millimeters(y);
    r.width = w;
    r.depth = depth;
    r.height = h;
    d.entities.push_back(r);
    const double points[4][2] = {{0, 0}, {w, 0}, {w, depth}, {0, depth}};
    for (int i = 0; i < 4; ++i) {
        auto e = wall(points[i][0] + x, points[i][1] + y, points[(i + 1) % 4][0] + x,
                      points[(i + 1) % 4][1] + y, h, t);
        std::vector<std::pair<double, double>> segments{{0, e.width}};
        const double newAngle = e.transform.yaw * std::numbers::pi / 180;
        for (const auto &existing : d.entities)
            if (existing.type == "Wall" && std::abs(existing.depth - t) < 0.1 &&
                std::abs(existing.height - h) < 0.1) {
                const double a = existing.transform.yaw * std::numbers::pi / 180;
                const double ex = existing.transform.x + existing.width * std::cos(a),
                             ey = existing.transform.y + existing.width * std::sin(a);
                auto cross = [&](double px, double py) {
                    return -(px - e.transform.x) * std::sin(newAngle) +
                           (py - e.transform.y) * std::cos(newAngle);
                };
                if (std::abs(cross(existing.transform.x, existing.transform.y)) > 0.1 ||
                    std::abs(cross(ex, ey)) > 0.1)
                    continue;
                auto along = [&](double px, double py) {
                    return (px - e.transform.x) * std::cos(newAngle) +
                           (py - e.transform.y) * std::sin(newAngle);
                };
                const double left =
                    std::min(along(existing.transform.x, existing.transform.y), along(ex, ey));
                const double right =
                    std::max(along(existing.transform.x, existing.transform.y), along(ex, ey));
                std::vector<std::pair<double, double>> remaining;
                for (const auto &[start, end] : segments) {
                    if (right <= start + 0.1 || left >= end - 0.1)
                        remaining.emplace_back(start, end);
                    else {
                        if (left > start + 0.1)
                            remaining.emplace_back(start, left);
                        if (right < end - 0.1)
                            remaining.emplace_back(right, end);
                    }
                }
                segments = std::move(remaining);
            }
        e.name = "Parede " + std::to_string(i + 1);
        e.parent = r.id;
        for (const auto &[start, end] : segments) {
            auto piece = e;
            piece.id = uuid();
            piece.transform.x += start * std::cos(newAngle);
            piece.transform.y += start * std::sin(newAngle);
            piece.width = millimeters(end - start);
            d.entities.push_back(piece);
        }
    }
    auto floor = entity("Floor", "Piso");
    floor.width = w - t;
    floor.depth = depth - t;
    floor.height = 25;
    floor.transform = {x + t / 2, y + t / 2, -25, 0, false};
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
    if (ids.contains(d.renderSettings.at("camera").get<std::string>()))
        d.renderSettings["camera"] = "";
}
} // namespace lmx
