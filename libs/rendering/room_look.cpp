#include "room_look.h"
#include "document/room_outline.h"
#include "light_model.h"
#include <algorithm>
#include <stdexcept>
namespace lmx {
void applyRoomLook(Document &d, const std::string &id, const std::string &style) {
    const auto room = d.at(id);
    if (room.type != "Room" || (style != "natural" && style != "bright" && style != "evening"))
        throw std::invalid_argument("Escolha um cômodo e um estilo de iluminação.");
    const bool evening = style == "evening", bright = style == "bright";
    const auto wallId = "look-" + style + "-wall", floorId = "look-" + style + "-floor";
    auto add = [&](const std::string &mid, const std::string &name, const Json &color, double roughness,
                   const std::string &procedure) {
        if (std::any_of(d.materials.begin(), d.materials.end(),
                        [&](const auto &m) { return m.at("id") == mid; }))
            return;
        d.materials.push_back({{"id", mid},
                               {"name", name},
                               {"baseColor", color},
                               {"roughness", roughness},
                               {"metallic", 0},
                               {"transmission", 0},
                               {"opacity", 1},
                               {"ior", 1.45},
                               {"procedural", procedure},
                               {"textureScale", 1000}});
    };
    add(wallId,
        bright    ? "Parede clara"
        : evening ? "Parede areia ao anoitecer"
                  : "Parede de tom natural",
        bright    ? Json{.82, .82, .79}
        : evening ? Json{.50, .43, .35}
                  : Json{.70, .67, .60},
        .82, "paint");
    add(floorId,
        bright    ? "Pedra clara acetinada"
        : evening ? "Madeira escura natural"
                  : "Madeira clara natural",
        bright    ? Json{.68, .67, .62}
        : evening ? Json{.16, .075, .038}
                  : Json{.39, .23, .11},
        bright ? .38 : .48, bright ? "stone" : "wood");
    for (auto &e : d.entities)
        if (e.parent == id) {
            if (e.type == "Wall" || e.type == "HalfWall")
                e.material = wallId;
            if (e.type == "Floor")
                e.material = floorId;
            if (e.type == "Ceiling") {
                e.visible = true;
                e.material = wallId;
            }
        }
    std::erase_if(d.entities, [&](const auto &e) {
        return e.type == "Light" && e.metadata.value("lookRoom", std::string{}) == id;
    });
    const auto center = roomInteriorPoint(room);
    auto light = entity("Light", evening ? "Luz aconchegante do cômodo" : "Luz suave do cômodo");
    light.transform = {center[0], center[1], room.transform.z + room.height - 100, 0, false};
    light.parameters = {{"kind", "area"},
                        {"shape", "DISK"},
                        {"size", std::min({room.width * .45, room.depth * .45, 1800.0})},
                        {"sizeY", 1000},
                        {"power", evening ? 35 : 80},
                        {"target", {center[0], center[1], room.transform.z + 800}},
                        {"colorMode", "kelvin"},
                        {"temperature", evening  ? 2700
                                        : bright ? 5000
                                                 : 3500},
                        {"color", {1, 1, 1}}};
    light.metadata["lookRoom"] = id;
    d.entities.push_back(light);
    d.version = 3;
    d.renderSettings["environmentMode"] = evening ? "studio" : "sky";
    d.renderSettings["environmentStrength"] = evening ? .06 : bright ? .35 : .20;
    d.renderSettings["sunElevation"] = bright ? 45 : 25;
    d.renderSettings["sunRotation"] = 35;
    d.renderSettings["exposure"] = evening ? .3 : 0;
    d.renderSettings["roomLook"] = style;
}
} // namespace lmx
