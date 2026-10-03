#include "light_model.h"
#include <cmath>
#include <set>
#include <stdexcept>
namespace lmx {
Entity lightEntity(const std::string &kind) {
    auto light = entity("Light", kind == "led" ? "Fita LED" : kind == "sun" ? "Sol" : "Luz");
    light.transform.z = 2500;
    light.parameters = {{"kind", kind},
                        {"power", kind == "led"   ? 10.0
                                  : kind == "sun" ? 1.5
                                                  : 100.0},
                        {"colorMode", "kelvin"},
                        {"temperature", kind == "sun" ? 5500 : 3000},
                        {"color", {1.0, 1.0, 1.0}},
                        {"size", kind == "led" ? 1200 : 1000},
                        {"sizeY", kind == "led" ? 12 : 1000},
                        {"shape", "RECTANGLE"},
                        {"radius", 50},
                        {"angle", 45.0},
                        {"sunAngle", 0.526},
                        {"blend", 0.3},
                        {"target", {0, 0, 0}}};
    validateLight(light);
    return light;
}
bool advancedLight(const Entity &light) {
    const auto &p = light.parameters;
    return p.value("kind", "area") == "led" || p.value("kind", "area") == "sun" || p.contains("colorMode") ||
           p.contains("temperature") || p.contains("shape") || p.contains("sizeY") || p.contains("radius") ||
           p.contains("sunAngle");
}
void validateLight(const Entity &light) {
    const auto &p = light.parameters;
    const auto target = p.value("target", Json::array({2000, 1500, 0}));
    if (!target.is_array() || target.size() != 3)
        throw std::invalid_argument("Direção da luz inválida");
    for (const auto &coordinate : target)
        millimeters(coordinate.get<double>());
    const std::set<std::string> kinds{"area", "point", "spot", "led", "sun"};
    if (!kinds.contains(p.value("kind", "area")))
        throw std::invalid_argument("Tipo de luz inválido");
    auto range = [&](const char *key, double fallback, double low, double high) {
        const auto value = p.value(key, fallback);
        if (!std::isfinite(value) || value < low || value > high)
            throw std::invalid_argument("Valor de iluminação fora do limite");
    };
    range("power", 500, 0, 100000);
    range("size", 1000, 1, 100000);
    range("sizeY", 1000, 1, 100000);
    range("radius", 0, 0, 10000);
    range("angle", 45, 1, 179);
    range("blend", 0.3, 0, 1);
    range("sunAngle", 0.526, 0.001, 180);
    range("temperature", 3000, 1000, 12000);
    const auto mode = p.value("colorMode", "custom");
    const auto shape = p.value("shape", "DISK");
    if ((mode != "custom" && mode != "kelvin") ||
        (shape != "DISK" && shape != "RECTANGLE" && shape != "SQUARE"))
        throw std::invalid_argument("Cor ou formato de luz inválido");
    const auto color = p.value("color", Json::array({1.0, 0.89, 0.73}));
    if (!color.is_array() || color.size() != 3)
        throw std::invalid_argument("Cor de luz inválida");
    for (const auto &value : color)
        if (!value.is_number() || !std::isfinite(value.get<double>()) || value.get<double>() < 0 ||
            value.get<double>() > 1)
            throw std::invalid_argument("Cor de luz inválida");
    if (p.value("kind", "area") != "point" && advancedLight(light)) {
        const auto dx = target[0].get<double>() - light.transform.x;
        const auto dy = target[1].get<double>() - light.transform.y;
        const auto dz = target[2].get<double>() - light.transform.z;
        if (dx * dx + dy * dy + dz * dz < 0.01)
            throw std::invalid_argument("Escolha uma direção diferente da posição da luz");
    }
}
} // namespace lmx
