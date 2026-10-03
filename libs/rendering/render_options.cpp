#include "render_options.h"
#include <cmath>
#include <stdexcept>
#include <tuple>
namespace lmx {
Json renderPresets() {
    Json modes = Json::object();
    for (const auto &[name, samples, bounces] :
         {std::tuple{"rapid", 32, 4}, std::tuple{"normal", 128, 8}, std::tuple{"final", 512, 12}}) {
        modes[name] = {{"version", 1},
                       {"preset", name},
                       {"samples", samples},
                       {"width", name == std::string("rapid") ? 640 : 1920},
                       {"height", name == std::string("rapid") ? 360 : 1080},
                       {"maxBounces", bounces},
                       {"diffuseBounces", bounces},
                       {"glossyBounces", bounces},
                       {"transmissionBounces", bounces},
                       {"transparentBounces", bounces},
                       {"denoise", true},
                       {"device", "AUTO"},
                       {"clamp", 5.0},
                       {"noiseThreshold", 0.008},
                       {"format", "PNG"},
                       {"transparent", false}};
    }
    return {{"version", 1}, {"presets", modes}};
}
Json renderPreset(const QString &name) {
    auto id = name.toStdString();
    auto options = renderPresets().at("presets").at(id == "custom" ? "normal" : id);
    options["preset"] = id;
    return options;
}
void validateRenderOptions(const Json &o) {
    if (!o.is_object() || o.at("version") != 1)
        throw std::invalid_argument("Configuração de render não suportada");
    for (const auto *key : {"width", "height", "samples"}) {
        if (!o.at(key).is_number_integer())
            throw std::invalid_argument("Tamanho ou qualidade inválidos");
        const auto v = o.at(key).get<int>();
        const bool sample = std::string(key) == "samples";
        if (v < (sample ? 1 : 16) || v > (sample ? 4096 : 8192))
            throw std::invalid_argument("Tamanho ou qualidade fora do limite");
    }
    for (const auto *key :
         {"maxBounces", "diffuseBounces", "glossyBounces", "transmissionBounces", "transparentBounces"})
        if (!o.at(key).is_number_integer() || o.at(key).get<int>() < 0 || o.at(key).get<int>() > 64)
            throw std::invalid_argument("Reflexões fora do limite");
    for (const auto *key : {"clamp", "noiseThreshold"}) {
        auto value = o.at(key).get<double>();
        if (!std::isfinite(value) || value < 0 || value > (std::string(key) == "clamp" ? 100.0 : 1.0))
            throw std::invalid_argument("Controle de ruído inválido");
    }
    const auto preset = o.at("preset").get<std::string>();
    if ((preset != "rapid" && preset != "normal" && preset != "final" && preset != "custom") ||
        (o.at("device") != "CPU" && o.at("device") != "AUTO") ||
        (o.at("format") != "PNG" && o.at("format") != "JPEG" && o.at("format") != "EXR") ||
        !o.at("denoise").is_boolean() || !o.at("transparent").is_boolean() ||
        (o.at("transparent") == true && o.at("format") == "JPEG") ||
        (o.at("format") == "EXR" && preset != "custom"))
        throw std::invalid_argument("Modo ou formato de render inválido");
}
QString renderStateLabel(const QString &state) {
    static const std::map<QString, QString> labels{{"Queued", "Na fila"},
                                                   {"Preparing", "Preparando"},
                                                   {"Exporting", "Preparando a cena"},
                                                   {"Rendering", "Renderizando"},
                                                   {"Denoising", "Reduzindo ruído"},
                                                   {"Saving", "Salvando imagem"},
                                                   {"Completed", "Concluído"},
                                                   {"Failed", "Falhou"},
                                                   {"Cancelled", "Cancelado"},
                                                   {"Interrupted", "Interrompido"}};
    auto found = labels.find(state);
    return found == labels.end() ? state : found->second;
}
} // namespace lmx
