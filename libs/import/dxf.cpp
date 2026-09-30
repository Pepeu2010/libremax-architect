#include "dxf.h"
#include <QFile>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace lmx {
namespace {
struct Pair {
    int code;
    std::string value;
};
double number(const std::string &value) {
    std::size_t used = 0;
    auto result = std::stod(value, &used);
    if (used != value.size() || !std::isfinite(result))
        throw std::invalid_argument("Número DXF inválido");
    return result;
}
double value(const std::vector<Pair> &record, int code, double fallback = 0) {
    for (const auto &p : record)
        if (p.code == code)
            return number(p.value);
    return fallback;
}
std::string text(const std::vector<Pair> &record, int code, const std::string &fallback) {
    for (const auto &p : record)
        if (p.code == code)
            return p.value;
    return fallback;
}
} // namespace
DxfDrawing readDxf(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 8 * 1024 * 1024)
        throw std::runtime_error("DXF não encontrado ou maior que 8 MiB");
    auto bytes = file.readAll();
    if (bytes.startsWith("AutoCAD Binary DXF"))
        throw std::runtime_error("DXF binário ainda não suportado. Exporte DXF ASCII.");
    auto lines = QString::fromUtf8(bytes).split('\n');
    while (!lines.empty() && lines.last().trimmed().isEmpty())
        lines.removeLast();
    if (lines.size() % 2 || lines.size() > 400000)
        throw std::runtime_error("Pares DXF incompletos ou muitos registros");
    std::vector<Pair> pairs;
    for (int i = 0; i < lines.size(); i += 2) {
        bool ok = false;
        int code = lines[i].trimmed().toInt(&ok);
        if (!ok || code < 0 || code > 1071)
            throw std::runtime_error("Código DXF inválido");
        pairs.push_back({code, lines[i + 1].trimmed().toStdString()});
    }
    DxfDrawing drawing;
    bool entities = false;
    std::set<std::string> unsupported;
    std::string polyLayer;
    Json polyPoints = Json::array();
    bool polyClosed = false;
    for (std::size_t i = 0; i + 1 < pairs.size(); ++i)
        if (pairs[i].code == 9 && pairs[i].value == "$INSUNITS") {
            int unit = static_cast<int>(number(pairs[i + 1].value));
            drawing.millimeterScale = unit == 1   ? 25.4
                                      : unit == 2 ? 304.8
                                      : unit == 5 ? 10
                                      : unit == 6 ? 1000
                                                  : 1;
        }
    auto insert = [&](const std::string &layer, const Json &item) {
        if (layer.size() > 256)
            throw std::runtime_error("Layer DXF inválido");
        if (!drawing.layers.contains(layer))
            drawing.layers[layer] = Json::array();
        if (drawing.layers[layer].size() >= 20000)
            throw std::runtime_error("Layer DXF grande demais");
        drawing.layers[layer].push_back(item);
    };
    auto polyline = [&](const std::string &layer, const Json &points, bool closed) {
        if (points.size() < 2 || points.size() > 20000)
            throw std::runtime_error("Polyline DXF inválida");
        for (std::size_t i = 1; i < points.size(); ++i)
            insert(layer, {{"kind", "line"}, {"points", {points[i - 1], points[i]}}});
        if (closed)
            insert(layer, {{"kind", "line"}, {"points", {points.back(), points.front()}}});
    };
    for (std::size_t index = 0; index < pairs.size();) {
        if (pairs[index].code == 9 && pairs[index].value == "$INSUNITS" && index + 1 < pairs.size()) {
            int unit = static_cast<int>(number(pairs[index + 1].value));
            drawing.millimeterScale = unit == 1   ? 25.4
                                      : unit == 2 ? 304.8
                                      : unit == 5 ? 10
                                      : unit == 6 ? 1000
                                                  : 1;
        }
        if (pairs[index].code != 0) {
            ++index;
            continue;
        }
        auto type = pairs[index].value;
        std::size_t end = index + 1;
        while (end < pairs.size() && pairs[end].code != 0)
            ++end;
        std::vector<Pair> record(pairs.begin() + static_cast<std::ptrdiff_t>(index + 1),
                                 pairs.begin() + static_cast<std::ptrdiff_t>(end));
        index = end;
        if (type == "SECTION") {
            entities = text(record, 2, "") == "ENTITIES";
            continue;
        }
        if (type == "ENDSEC") {
            entities = false;
            continue;
        }
        if (type == "EOF")
            break;
        if (!entities)
            continue;
        auto layer = text(record, 8, "0");
        if (type == "LINE")
            insert(layer,
                   {{"kind", "line"},
                    {"points",
                     {{value(record, 10), value(record, 20)}, {value(record, 11), value(record, 21)}}}});
        else if (type == "CIRCLE" || type == "ARC") {
            double radius = value(record, 40);
            if (radius <= 0)
                throw std::runtime_error("Raio DXF inválido");
            insert(layer, {{"kind", type == "ARC" ? "arc" : "circle"},
                           {"center", {value(record, 10), value(record, 20)}},
                           {"radius", radius},
                           {"start", value(record, 50)},
                           {"end", value(record, 51, 360)}});
        } else if (type == "LWPOLYLINE") {
            Json points = Json::array();
            double x = 0;
            bool haveX = false;
            for (const auto &p : record) {
                if (p.code == 42 && number(p.value) != 0)
                    throw std::runtime_error("Polyline com bulge não suportada; converta curvas em ARC");
                if (p.code == 10) {
                    x = number(p.value);
                    haveX = true;
                }
                if (p.code == 20) {
                    if (!haveX)
                        throw std::runtime_error("Polyline sem coordenada X");
                    points.push_back({x, number(p.value)});
                    haveX = false;
                }
            }
            polyline(layer, points, (static_cast<int>(value(record, 70)) & 1) != 0);
        } else if (type == "POLYLINE") {
            polyLayer = layer;
            polyPoints = Json::array();
            polyClosed = (static_cast<int>(value(record, 70)) & 1) != 0;
        } else if (type == "VERTEX") {
            if (polyLayer.empty())
                throw std::runtime_error("VERTEX sem POLYLINE");
            if (value(record, 42) != 0)
                throw std::runtime_error("Bulge DXF não suportado");
            polyPoints.push_back({value(record, 10), value(record, 20)});
        } else if (type == "SEQEND") {
            if (!polyLayer.empty()) {
                polyline(polyLayer, polyPoints, polyClosed);
                polyLayer.clear();
            }
        } else
            unsupported.insert(type);
    }
    if (!polyLayer.empty())
        throw std::runtime_error("POLYLINE incompleta");
    if (drawing.layers.empty())
        throw std::runtime_error("DXF sem entidades 2D suportadas");
    drawing.unsupported.assign(unsupported.begin(), unsupported.end());
    return drawing;
}
std::vector<Entity> dxfEntities(const DxfDrawing &drawing, double scale,
                                const std::vector<std::string> &layers) {
    if (!std::isfinite(scale) || scale <= 0 || scale > 1e6)
        throw std::invalid_argument("Escala DXF inválida");
    std::vector<Entity> result;
    for (const auto &name : layers) {
        auto primitives = drawing.layers.at(name);
        for (auto &primitive : primitives) {
            auto key = primitive.at("kind") == "line" ? "points" : "center";
            if (std::string(key) == "points") {
                for (auto &point : primitive[key])
                    for (auto &coordinate : point)
                        coordinate = millimeters(coordinate.get<double>() * scale);
            } else {
                for (auto &coordinate : primitive[key])
                    coordinate = millimeters(coordinate.get<double>() * scale);
                primitive["radius"] = millimeters(primitive["radius"].get<double>() * scale);
            }
        }
        auto e = entity("GeometryObject", "DXF · " + name);
        e.parameters = {{"primitives", primitives}};
        e.metadata = {{"format", "DXF"}, {"layer", name}, {"scale", scale}};
        e.locked = true;
        e.material = "stone";
        result.push_back(e);
    }
    return result;
}
} // namespace lmx
