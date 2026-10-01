#include "model.h"
#include <cmath>
#include <stdexcept>
namespace lmx {
Json readModel(const QByteArray &bytes) {
    if (bytes.isEmpty() || bytes.size() > 4 * 1024 * 1024)
        throw std::invalid_argument("Modelo 3D fora do limite");
    auto model = Json::parse(bytes.toStdString(), [](int depth, Json::parse_event_t, Json &) {
        if (depth > 12)
            throw std::invalid_argument("Modelo 3D inválido");
        return true;
    });
    if (model.at("schema") != 1 || !model.at("parts").is_array() || model.at("parts").empty() ||
        model.at("parts").size() > 64)
        throw std::invalid_argument("Modelo 3D inválido");
    std::size_t total = 0;
    for (const auto &part : model.at("parts")) {
        const auto &vertices = part.at("vertices"), &triangles = part.at("triangles");
        if (!vertices.is_array() || vertices.size() < 3 || vertices.size() > 100000 ||
            !triangles.is_array() || triangles.empty() || triangles.size() > 200000 ||
            !part.at("material").is_string())
            throw std::invalid_argument("Malha 3D inválida");
        total += triangles.size();
        if (total > 200000)
            throw std::invalid_argument("Modelo 3D grande demais");
        for (const auto &v : vertices) {
            if (!v.is_array() || v.size() != 3)
                throw std::invalid_argument("Ponto 3D inválido");
            for (const auto &axis : v)
                if (!axis.is_number() || !std::isfinite(axis.get<double>()) || axis.get<double>() < -0.0001 ||
                    axis.get<double>() > 1.0001)
                    throw std::invalid_argument("Ponto 3D fora do limite");
        }
        for (const auto &triangle : triangles) {
            if (!triangle.is_array() || triangle.size() != 3)
                throw std::invalid_argument("Face 3D inválida");
            for (const auto &i : triangle)
                if (!i.is_number_integer() || i.get<long long>() < 0 ||
                    i.get<std::size_t>() >= vertices.size())
                    throw std::invalid_argument("Face 3D fora do limite");
        }
    }
    return model;
}
} // namespace lmx
