#include "model.h"
#include <QCryptographicHash>
#include <QMutex>
#include <QMutexLocker>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
namespace lmx {
std::vector<std::string> validatedModelMaterials(const QByteArray &bytes) {
    static QMutex mutex;
    static std::map<QByteArray, std::vector<std::string>> cache;
    const auto digest = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
    {
        QMutexLocker lock(&mutex);
        auto found = cache.find(digest);
        if (found != cache.end())
            return found->second;
    }
    const auto model = readModel(bytes);
    std::set<std::string> used;
    for (const auto &part : model.at("parts"))
        used.insert(part.at("material").get<std::string>());
    std::vector<std::string> result(used.begin(), used.end());
    QMutexLocker lock(&mutex);
    if (cache.size() >= 128)
        cache.clear();
    cache[digest] = result;
    return result;
}
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
    if (model.contains("materials") &&
        (!model.at("materials").is_array() || model.at("materials").size() > 64))
        throw std::invalid_argument("Acabamentos do modelo inválidos");
    for (const auto &part : model.at("parts")) {
        const auto &vertices = part.at("vertices"), &triangles = part.at("triangles");
        if (!vertices.is_array() || vertices.size() < 3 || vertices.size() > 100000 ||
            !triangles.is_array() || triangles.empty() || triangles.size() > 200000 ||
            !part.at("material").is_string())
            throw std::invalid_argument("Malha 3D inválida");
        if (part.at("material").get<std::string>().empty() ||
            part.at("material").get<std::string>().size() > 128)
            throw std::invalid_argument("Nome de acabamento da malha inválido");
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
        for (const auto *attribute : {"uvs", "normals"}) {
            if (!part.contains(attribute))
                continue;
            const auto &values = part.at(attribute);
            const bool normal = std::string(attribute) == "normals";
            if (!values.is_array() || values.size() != vertices.size())
                throw std::invalid_argument("Atributos da malha incompletos");
            for (const auto &value : values) {
                if (!value.is_array() || value.size() != (normal ? 3u : 2u))
                    throw std::invalid_argument("Atributo da malha inválido");
                double length = 0;
                for (const auto &axis : value) {
                    if (!axis.is_number() || !std::isfinite(axis.get<double>()) ||
                        std::abs(axis.get<double>()) > (normal ? 1.001 : 1e6))
                        throw std::invalid_argument("Atributo da malha fora do limite");
                    length += axis.get<double>() * axis.get<double>();
                }
                if (normal && std::abs(length - 1) > 0.002)
                    throw std::invalid_argument("Normal da malha inválida");
            }
        }
    }
    return model;
}
} // namespace lmx
