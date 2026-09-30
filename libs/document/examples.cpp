#include "examples.h"
#include "geometry/geometry.h"
namespace lmx {
Document kitchenExample() {
    Document d;
    d.name = "Cozinha de referência";
    for (auto &material : d.materials)
        if (material.at("id") == "paint")
            material["baseColor"] = {0.76, 0.76, 0.73};
    addRectangularRoom(d, 4000, 3000, 2700, 120);
    for (auto &e : d.entities) {
        if (e.type == "Floor")
            e.material = "porcelain";
        if (e.type == "Ceiling")
            e.visible = true;
    }
    auto wallId = d.entities[3].id;
    auto door = entity("Door", "Porta 800 × 2100");
    door.parent = wallId;
    door.width = 800;
    door.height = 2100;
    door.parameters = {{"offset", 200}, {"sill", 0}, {"openAngle", 30}};
    d.entities.push_back(door);
    auto window = entity("Window", "Janela 1200 × 1000");
    window.parent = d.entities[2].id;
    window.width = 1200;
    window.height = 1000;
    window.parameters = {{"offset", 900}, {"sill", 1000}};
    d.entities.push_back(window);
    std::vector<std::string> bases;
    double x = 250;
    for (double width : {600.0, 947.0, 600.0, 800.0}) {
        auto e = entity("FurnitureModule", width == 947 ? "Balcão redimensionado 947 mm" : "Balcão");
        e.width = width;
        e.material = "graphite";
        e.transform = {x, 80, 0, 0, false};
        e.parameters = {{"family", bases.size() == 2 ? "drawer" : "cabinet"},
                        {"doors", 2},
                        {"legs", 100},
                        {"carcass", "oak"},
                        {"handle", "bar"},
                        {"glass", false}};
        d.entities.push_back(e);
        bases.push_back(e.id);
        x += width;
    }
    d.entities.push_back(automation(d, bases, "countertop"));
    d.entities.back().material = "quartz";
    auto backsplash = entity("GeometryObject", "Revestimento em pedra");
    backsplash.width = 3000;
    backsplash.height = 550;
    backsplash.depth = 12;
    backsplash.transform = {230, 63, 750, 0, false};
    backsplash.material = "porcelain";
    d.entities.push_back(backsplash);
    d.entities.push_back(automation(d, bases, "plinth"));
    auto upper = entity("FurnitureModule", "Aéreo com vidro");
    upper.width = 1200;
    upper.height = 700;
    upper.depth = 320;
    upper.material = "oak";
    upper.transform = {1300, 80, 1550, 0, false};
    upper.parameters = {{"family", "cabinet"}, {"doors", 2},          {"legs", 0},
                        {"glass", true},       {"handle", "profile"}, {"carcass", "oak"}};
    d.entities.push_back(upper);
    auto fridge = entity("DecorativeObject", "Geladeira duplex");
    fridge.width = 700;
    fridge.height = 1800;
    fridge.depth = 650;
    fridge.transform = {3250, 80, 0, 0, false};
    fridge.parameters = {{"family", "fridge"}};
    fridge.material = "graphite";
    d.entities.push_back(fridge);
    auto plant = entity("DecorativeObject", "Planta");
    plant.width = 350;
    plant.height = 900;
    plant.depth = 350;
    plant.transform = {3400, 1800, 0, 0, false};
    plant.parameters = {{"family", "plant"}};
    d.entities.push_back(plant);
    auto light = entity("Light", "Luz de área");
    light.transform = {1800, 1700, 2450, 0, false};
    light.parameters = {{"kind", "area"},
                        {"power", 0},
                        {"size", 1500},
                        {"target", {1800, 600, 700}},
                        {"color", {1.0, 0.9, 0.77}}};
    d.entities.push_back(light);
    auto fill = entity("Light", "Luz da janela");
    fill.transform = {3700, 1800, 2000, 0, false};
    fill.parameters = {{"kind", "area"},
                       {"power", 0},
                       {"size", 1200},
                       {"target", {1800, 300, 900}},
                       {"color", {0.8, 0.89, 1.0}}};
    d.entities.push_back(fill);
    auto vase = entity("DecorativeObject", "Cerâmica da bancada");
    vase.width = 140;
    vase.depth = 140;
    vase.height = 230;
    vase.transform = {700, 400, 750, 0, false};
    vase.parameters = {{"family", "vase"}};
    vase.material = "ceramic";
    d.entities.push_back(vase);
    for (int i = 0; i < 2; ++i) {
        auto ceramic = vase;
        ceramic.id = uuid();
        ceramic.name = "Cerâmica no aéreo";
        ceramic.width = ceramic.depth = 110;
        ceramic.height = 190 + i * 35;
        ceramic.transform = {1450.0 + i * 600, 180, 1568, 0, false};
        d.entities.push_back(ceramic);
    }
    auto camera = entity("Camera", "Vista da cozinha");
    camera.transform = {1100, 2970, 1300, 0, false};
    camera.parameters = {{"target", {1950, 350, 1300}}, {"lens", 19}, {"fstop", 8}};
    d.entities.push_back(camera);
    d.renderSettings = {{"camera", camera.id}, {"exposure", 1.6},          {"environmentStrength", 0.12},
                        {"denoise", true},     {"environmentMode", "sky"}, {"sunElevation", 35.0},
                        {"sunRotation", 30.0}};
    auto detail = camera;
    detail.id = uuid();
    detail.name = "Detalhe de materiais";
    detail.transform = {2400, 1700, 1400, 0, false};
    detail.parameters = {{"target", {1700, 500, 1000}}, {"lens", 35}};
    d.entities.push_back(detail);
    d.validate();
    return d;
}
Document bedroomExample() {
    Document d;
    d.name = "Dormitório de referência";
    addRectangularRoom(d, 4500, 4000, 2700, 120);
    auto wardrobe = entity("FurnitureModule", "Roupeiro 1812,5 mm");
    wardrobe.width = 1812.5;
    wardrobe.height = 2200;
    wardrobe.depth = 600;
    wardrobe.transform = {200, 80, 0, 0, false};
    wardrobe.parameters = {{"family", "cabinet"}, {"doors", 3},       {"legs", 80},
                           {"handle", "point"},   {"carcass", "oak"}, {"glass", false}};
    d.entities.push_back(wardrobe);
    auto bed = entity("DecorativeObject", "Cama");
    bed.width = 1600;
    bed.height = 450;
    bed.depth = 2000;
    bed.transform = {2300, 300, 0, 0, false};
    bed.parameters = {{"family", "bed"}};
    d.entities.push_back(bed);
    auto light = entity("Light", "Luz de área");
    light.transform = {2200, 2000, 2500, 0, false};
    light.parameters = {{"kind", "area"}, {"power", 300}, {"target", {2200, 2000, 0}}};
    d.entities.push_back(light);
    auto camera = entity("Camera", "Dormitório");
    camera.transform = {2100, 3600, 1800, 0, false};
    camera.parameters = {{"target", {2000, 500, 1000}}, {"lens", 20}};
    d.entities.push_back(camera);
    d.validate();
    return d;
}
} // namespace lmx
