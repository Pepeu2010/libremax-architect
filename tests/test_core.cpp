#include "commands/editor.h"
#include "core/expression.h"
#include "document/document.h"
#include "document/examples.h"
#include "geometry/geometry.h"
#include "geometry/scene_cache.h"
#include "import/dxf.h"
#include "library/library.h"
#include "library/model.h"
#include "library/thumbnails.h"
#include "materials/texture.h"
#include "persistence/project_library.h"
#include "persistence/project_store.h"
#include "persistence/recovery_store.h"
#include "placement/placement.h"
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Bnd_Box.hxx>
#include <QApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QThread>
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif
#include <iostream>
#include <zip.h>

namespace {
QApplication &application() {
    return *qApp;
}
using namespace lmx;
Document simple() {
    Document d;
    d.entities.push_back(wall(0, 0, 4000, 0));
    return d;
}
void maliciousZip(const QString &path, const std::string &name, const std::string &body) {
    int error = 0;
    auto *z = zip_open(QFile::encodeName(path).constData(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    REQUIRE(z);
    auto *source = zip_source_buffer(z, body.data(), body.size(), 0);
    REQUIRE(source);
    REQUIRE(zip_file_add(z, name.c_str(), source, 0) >= 0);
    REQUIRE(zip_close(z) == 0);
}
} // namespace

TEST_CASE("Furniture placement fits wall faces, corners and free gaps", "[placement]") {
    Document d;
    auto w = wall(0, 0, 3000, 0);
    d.entities.push_back(w);
    auto e = entity("DecorativeObject", "Mesa");
    e.width = 600;
    e.depth = 500;
    e.height = 700;
    e.parameters = {{"family", "table"}};
    auto north = placeObject(d, e, 1000, 100);
    REQUIRE(north.allowed);
    REQUIRE(north.wall == w.id);
    REQUIRE(north.object.transform.x == Catch::Approx(700));
    REQUIRE(north.object.transform.y == Catch::Approx(62));
    REQUIRE(north.object.transform.yaw == 0);
    auto south = placeObject(d, e, 1000, -100);
    REQUIRE(south.allowed);
    REQUIRE(south.object.transform.y == Catch::Approx(-62));
    REQUIRE(south.object.transform.x == Catch::Approx(1300));
    REQUIRE(south.object.transform.yaw == 180);
    auto corner = placeObject(d, e, 320, 100);
    REQUIRE(corner.allowed);
    REQUIRE(corner.object.transform.x == 0);
    d.entities.push_back(north.object);
    auto next = e;
    next.id = uuid();
    auto adjacent = placeObject(d, next, 1420, 100);
    REQUIRE(adjacent.allowed);
    REQUIRE(adjacent.object.transform.x == Catch::Approx(1302));
    auto tooWide = e;
    tooWide.width = 4000;
    REQUIRE_FALSE(placeObject(d, tooWide, 1000, 100).allowed);
    auto blocked = next;
    blocked.width = 3000;
    REQUIRE_FALSE(placeObject(d, blocked, 1000, 100).allowed);
    auto rotated = d;
    rotated.entities.clear();
    auto angle = wall(1000, 1000, 1000, 4000);
    rotated.entities.push_back(angle);
    auto vertical = placeObject(rotated, e, 900, 2000);
    REQUIRE(vertical.allowed);
    REQUIRE(vertical.object.transform.yaw == Catch::Approx(90));
    REQUIRE(vertical.object.transform.x == Catch::Approx(938));
    angle.locked = true;
    rotated.entities[0] = angle;
    REQUIRE(placeObject(rotated, e, 900, 2000).wall.empty());
}

TEST_CASE("Apartment placement stays inside, preserves height and reserves openings", "[placement]") {
    Document d;
    addRectangularRoom(d, 4000, 3000, 2700, 120, 1000, 2000, "Sala");
    auto e = entity("FurnitureModule", "Balcão");
    e.width = 600;
    e.depth = 500;
    e.height = 700;
    auto floor = placeObject(d, e, 2600, 3500);
    REQUIRE(floor.allowed);
    REQUIRE(floor.object.transform.x == Catch::Approx(2300));
    REQUIRE(floor.object.transform.y == Catch::Approx(3250));
    REQUIRE_FALSE(placeObject(d, e, 10000, 10000).allowed);
    auto wallPlace = placeObject(d, e, 2500, 2100);
    REQUIRE(wallPlace.allowed);
    REQUIRE(wallPlace.object.transform.y == 2062);
    d.entities.push_back(wallPlace.object);
    auto upper = e;
    upper.id = uuid();
    upper.transform.z = 1400;
    upper.parameters = {{"placement", "wall"}};
    auto over = placeObject(d, upper, 2500, 2100);
    REQUIRE(over.allowed);
    REQUIRE(over.object.transform.z == 1400);
    auto door = entity("Door", "Porta");
    door.width = 800;
    door.height = 2100;
    door.parameters = {{"placement", "wall"}, {"sill", 0}, {"openAngle", 0}};
    REQUIRE_FALSE(placeObject(d, door, 2800, 3500).allowed);
    auto opening = placeObject(d, door, 3900, 2100);
    REQUIRE(opening.allowed);
    REQUIRE(opening.object.parent == opening.wall);
    d.entities.push_back(opening.object);
    REQUIRE_NOTHROW(d.validate());
    auto reserved = placeObject(d, e, 3900, 2100);
    REQUIRE_FALSE(reserved.allowed);
    auto vase = entity("DecorativeObject", "Vaso");
    vase.width = 200;
    vase.depth = 200;
    vase.height = 250;
    vase.parameters = {{"family", "vase"}, {"placement", "surface"}};
    auto onTop = placeObject(d, vase, wallPlace.object.transform.x + 300, wallPlace.object.transform.y + 250);
    REQUIRE(onTop.allowed);
    REQUIRE(onTop.object.transform.z == 702);
    auto turning = e;
    turning.transform.yaw = 90;
    auto free = placeObject(d, turning, 3600, 3800, false);
    REQUIRE(free.allowed);
    REQUIRE(free.object.transform.x == 3850);
    REQUIRE(free.object.transform.y == 3500);
    Editor editor;
    editor.load(d);
    auto count = d.entities.size();
    editor.apply("Place vase", [&](Document &next) { next.entities.push_back(onTop.object); });
    REQUIRE(editor.document().entities.size() == count + 1);
    editor.history.undo();
    REQUIRE(editor.document().entities.size() == count);
    editor.history.redo();
    REQUIRE(editor.document().entities.back().transform.z == 702);
    auto rug = entity("MeshObject", "Tapete pronto");
    rug.width = 1600;
    rug.depth = 1200;
    rug.height = 8;
    rug.transform = {1800, 2900, 0, 0, false};
    rug.parameters = {{"placement", "rug"}};
    d.entities.push_back(rug);
    auto onRug = placeObject(d, e, 2600, 3500, false);
    REQUIRE(onRug.allowed);
    REQUIRE(onRug.object.transform.z == 0);
}

TEST_CASE("Adjacent named rooms have distinct floors and share matching walls", "[placement][document]") {
    Document d;
    addRectangularRoom(d, 4000, 3000, 2700, 120, 0, 0, "Sala");
    addRectangularRoom(d, 2000, 3000, 2700, 120, 4000, 0, "Quarto");
    d.validate();
    REQUIRE(std::count_if(d.entities.begin(), d.entities.end(),
                          [](const auto &e) { return e.type == "Wall"; }) == 7);
    auto room =
        std::find_if(d.entities.begin(), d.entities.end(), [](const auto &e) { return e.name == "Quarto"; });
    REQUIRE(room->transform.x == 4000);
    auto sofa = entity("DecorativeObject", "Sofá");
    sofa.parameters = {{"family", "sofa"}};
    sofa.width = 1500;
    sofa.depth = 800;
    sofa.height = 850;
    auto placed = placeObject(d, sofa, 4100, 1500);
    REQUIRE(placed.allowed);
    REQUIRE(placed.object.transform.x == 4062);
    addRectangularRoom(d, 1000, 1500, 2700, 120, 6000, 0, "Banheiro");
    REQUIRE(std::count_if(d.entities.begin(), d.entities.end(),
                          [](const auto &e) { return e.type == "Wall"; }) == 10);
    auto count = d.entities.size();
    REQUIRE_THROWS(addRectangularRoom(d, 1000, 1000, 2700, 120, 4500, 500, "Sobreposto"));
    REQUIRE(d.entities.size() == count);
}

TEST_CASE("All 52 ready furniture meshes embed, persist and render independently", "[models][library]") {
    application();
    QTemporaryDir dir;
    Library library(dir.filePath("models.db"), QStringLiteral(LMX_SOURCE_DIR) + "/starter-models");
    QFile file(QStringLiteral(LMX_SOURCE_DIR) + "/starter-models/catalog.json");
    REQUIRE(file.open(QIODevice::ReadOnly));
    auto entries = Json::parse(file.readAll().toStdString());
    REQUIRE(entries.size() == 52);
    library.seed(entries);
    auto all = library.search();
    REQUIRE(all.size() == 52);
    for (const auto &asset : all) {
        INFO(asset.name.toStdString());
        REQUIRE_FALSE(asset.model.isEmpty());
        auto model = readModel(asset.model);
        REQUIRE_FALSE(model.at("parts").empty());
        Document d;
        Library::attachModel(d, asset);
        auto object = Library::instantiate(asset, 100, 200);
        d.entities.push_back(object);
        REQUIRE_NOTHROW(d.validate());
        auto snapshot = meshSnapshot(d);
        REQUIRE_FALSE(snapshot.at("meshes").empty());
        for (const auto &mesh : snapshot.at("meshes")) {
            REQUIRE_FALSE(mesh.at("vertices").empty());
            REQUIRE_FALSE(mesh.at("triangles").empty());
        }
        REQUIRE_FALSE(renderAssetThumbnail(asset).isNull());
    }
    Document persisted;
    Library::attachModel(persisted, all[0]);
    persisted.entities.push_back(Library::instantiate(all[0], 100, 200));
    auto filename = dir.filePath("model.lmx");
    ProjectStore::save(filename, persisted, false);
    auto restored = ProjectStore::open(filename);
    REQUIRE(restored.embeddedAssets == persisted.embeddedAssets);
    REQUIRE(meshSnapshot(restored).at("meshes").size() == meshSnapshot(persisted).at("meshes").size());
    auto bad = all[0];
    bad.model.append('x');
    REQUIRE_THROWS(Library::attachModel(persisted, bad));
    auto invalid = readModel(all[0].model);
    invalid["parts"][0]["triangles"][0][0] = 999999;
    REQUIRE_THROWS(readModel(QByteArray::fromStdString(invalid.dump())));
    library.favorite(all[0].id, true);
    auto changed = entries;
    changed[0]["name"] = "Novo nome do catálogo";
    library.seed(changed);
    REQUIRE(library.search("Novo nome").size() == 1);
    REQUIRE(library.search({}, {}, true).size() == 1);
}
TEST_CASE("Modern CC0 furniture retains authored UVs, normals, textures and provenance", "[modern][models]") {
    QTemporaryDir dir;
    Library library(dir.filePath("modern.db"), QStringLiteral(LMX_SOURCE_DIR) + "/starter-models");
    QFile catalog(QStringLiteral(LMX_SOURCE_DIR) + "/starter-models/modern-catalog.json");
    REQUIRE(catalog.open(QIODevice::ReadOnly));
    library.seed(Json::parse(catalog.readAll().toStdString()));
    const auto all = library.search();
    REQUIRE(all.size() == 24);
    const auto shelf = library.search("Estante de madeira e metal");
    REQUIRE(shelf.size() == 1);
    REQUIRE(shelf.front().width == 900);
    REQUIRE(shelf.front().height < 2000);
    for (const auto &asset : all) {
        INFO(asset.id.toStdString());
        REQUIRE_FALSE(asset.textures.empty());
        Document d;
        Library::attachModel(d, asset);
        auto object = Library::instantiate(asset, 100, 200);
        object.transform.yaw = 37;
        object.transform.mirrored = true;
        d.entities.push_back(object);
        REQUIRE(object.metadata.at("author") != "Kenney");
        REQUIRE(object.metadata.at("source").get<std::string>().starts_with("https://polyhaven.com/a/"));
        REQUIRE_NOTHROW(d.validate());
        auto snapshot = meshSnapshot(d);
        REQUIRE_FALSE(snapshot.at("meshes").empty());
        for (const auto &mesh : snapshot.at("meshes")) {
            REQUIRE(mesh.at("uvs").size() == mesh.at("vertices").size());
            REQUIRE(mesh.at("normals").size() == mesh.at("vertices").size());
        }
        REQUIRE_FALSE(renderAssetThumbnail(asset).isNull());
        const auto filename = dir.filePath("self-contained.lmx");
        ProjectStore::save(filename, d, false);
        const auto restored = ProjectStore::open(filename);
        REQUIRE(restored.serialize() == d.serialize());
        REQUIRE(meshSnapshot(restored) == snapshot);
        // An additional copy preserves the user's changed material finish.
        const auto id = readModel(asset.model).at("materials")[0].at("id");
        for (auto &material : d.materials)
            if (material.at("id") == id)
                material["roughness"] = 0.123;
        Library::attachModel(d, asset);
        for (const auto &material : d.materials)
            if (material.at("id") == id)
                REQUIRE(material.at("roughness") == 0.123);
    }
    auto bad = all.front();
    bad.textures.begin()->second.append('x');
    Document unchanged;
    const auto before = unchanged.serialize();
    REQUIRE_THROWS(Library::attachModel(unchanged, bad));
    REQUIRE(unchanged.serialize() == before);
    bad = all.front();
    bad.textures.clear();
    REQUIRE_THROWS(Library::attachModel(unchanged, bad));
    REQUIRE(unchanged.serialize() == before);
    auto invalid = readModel(all.front().model);
    invalid["parts"][0]["uvs"].erase(invalid["parts"][0]["uvs"].begin());
    REQUIRE_THROWS(readModel(QByteArray::fromStdString(invalid.dump())));
    invalid = readModel(all.front().model);
    invalid["parts"][0]["normals"][0] = {0, 0, 0};
    REQUIRE_THROWS(readModel(QByteArray::fromStdString(invalid.dump())));
    invalid = readModel(all.front().model);
    invalid["parts"][0]["uvs"][0] = {1e20, 0};
    REQUIRE_THROWS(readModel(QByteArray::fromStdString(invalid.dump())));
}
TEST_CASE("Thumbnails resolve crossing surfaces per pixel", "[thumbnails][occlusion]") {
    auto red = materialPresets().front();
    red["id"] = "thumbnail-red";
    red["baseColor"] = {1, 0, 0};
    auto blue = red;
    blue["id"] = "thumbnail-blue";
    blue["baseColor"] = {0, 0, 1};
    // Identical projected triangles cross along the depth axis. Sorting their
    // average depths incorrectly hides the whole blue triangle behind the red.
    Json model = {{"schema", 1},
                  {"materials", {red, blue}},
                  {"parts",
                   {{{"material", "thumbnail-red"},
                     {"vertices", {{.2, .3, .3}, {.6, .3, .3}, {.2, .7, .3}}},
                     {"triangles", {{0, 1, 2}}}},
                    {{"material", "thumbnail-blue"},
                     {"vertices", {{.1, .4, .372}, {.5, .4, .372}, {.5, .4, .084}}},
                     {"triangles", {{0, 1, 2}}}}}}};
    Asset asset;
    asset.id = "crossing-surfaces";
    asset.name = "Crossing surfaces";
    asset.category = "Decoração";
    asset.width = asset.height = asset.depth = 1000;
    asset.model = QByteArray::fromStdString(model.dump());
    asset.recipe = {
        {"type", "MeshObject"},
        {"material", "white"},
        {"parameters",
         {{"meshAsset",
           QCryptographicHash::hash(asset.model, QCryptographicHash::Sha256).toHex().toStdString()},
          {"originalMaterials", true}}}};
    const auto image = renderAssetThumbnail(asset);
    REQUIRE(image.pixelColor(83, 60).blue() > 100);
    REQUIRE(image.pixelColor(83, 60).red() == 0);
    REQUIRE(image.pixelColor(150, 113).red() > 100);
    REQUIRE(image.pixelColor(150, 113).blue() == 0);
}
TEST_CASE("Original contemporary furniture remains portable and follows mounting rules",
          "[current][models]") {
    QTemporaryDir dir;
    Library library(dir.filePath("current.db"), QStringLiteral(LMX_SOURCE_DIR) + "/starter-models");
    QFile catalog(QStringLiteral(LMX_SOURCE_DIR) + "/starter-models/current-catalog.json");
    REQUIRE(catalog.open(QIODevice::ReadOnly));
    library.seed(Json::parse(catalog.readAll().toStdString()));
    const auto assets = library.search();
    REQUIRE(assets.size() == 12);
    for (const auto &asset : assets) {
        INFO(asset.id.toStdString());
        REQUIRE(asset.width > 100);
        REQUIRE(asset.width <= 2800);
        REQUIRE(asset.height < 2000);
        Document d;
        addRectangularRoom(d, 7000, 6000, 2700, 120);
        Library::attachModel(d, asset);
        auto object = Library::instantiate(asset, 0, 0);
        const bool mounted = object.parameters.at("placement") == "wall";
        auto result = placeObject(d, object, 3500, mounted ? 90 : 3000, true);
        REQUIRE(result.allowed);
        if (mounted) {
            REQUIRE_FALSE(result.wall.empty());
            REQUIRE(result.object.transform.z == object.parameters.at("defaultElevation").get<double>());
            REQUIRE_FALSE(placeObject(d, object, 3500, 3000, true).allowed);
        }
        d.entities.push_back(result.object);
        REQUIRE_NOTHROW(d.validate());
        const auto model = readModel(asset.model);
        std::size_t triangles = 0;
        for (const auto &part : model.at("parts")) {
            triangles += part.at("triangles").size();
            REQUIRE(part.at("normals").size() == part.at("vertices").size());
        }
        REQUIRE(triangles > 100);
        REQUIRE_FALSE(renderAssetThumbnail(asset).isNull());
        const auto snapshot = meshSnapshot(d);
        const auto path = dir.filePath("portable.lmx");
        ProjectStore::save(path, d, false);
        const auto restored = ProjectStore::open(path);
        REQUIRE(restored.serialize() == d.serialize());
        REQUIRE(meshSnapshot(restored) == snapshot);
    }
    REQUIRE(library.search("sofa").size() == 2);
    REQUIRE(library.search("sofa").front().name.startsWith(QString::fromUtf8("Sofá")));
    REQUIRE(library.search("luminaria").size() == 1);
    REQUIRE(library.search("boucle").size() == 2);
    REQUIRE(library.search({}, "Decoração").size() == 2);
    REQUIRE(library.search("cama queen").size() == 1);
    REQUIRE(library.search({}, "Dormitório").size() == 3);
    REQUIRE(library.search("ripad").size() == 3);
}
TEST_CASE("Ceiling fixtures follow room height and reject outside or obstructed placement",
          "[modern][placement]") {
    Document d;
    addRectangularRoom(d, 4000, 4000, 2700, 120);
    auto lamp = entity("DecorativeObject", "Pendente");
    lamp.parameters = {{"family", "lamp"}, {"placement", "ceiling"}};
    lamp.width = lamp.depth = 400;
    lamp.height = 950;
    const auto placed = placeObject(d, lamp, 2000, 2000, true);
    REQUIRE(placed.allowed);
    REQUIRE(placed.wall.empty());
    REQUIRE(placed.object.transform.z == 1730);
    REQUIRE(placed.message.starts_with("No teto"));
    REQUIRE_FALSE(placeObject(d, lamp, -1000, 2000, true).allowed);
    lamp.height = 2800;
    REQUIRE_FALSE(placeObject(d, lamp, 2000, 2000, true).allowed);
    lamp.height = 950;
    d.entities.push_back(placed.object);
    lamp.id = uuid();
    REQUIRE_FALSE(placeObject(d, lamp, 2000, 2000, true).allowed);
}
TEST_CASE("numeric expressions are bounded and do not execute code", "[units]") {
    REQUIRE(evaluate("800+20") == 820);
    REQUIRE(evaluate("1200/2") == 600);
    REQUIRE(evaluate("2*450") == 900);
    REQUIRE(evaluate("812,5") == 812.5);
    REQUIRE(evaluate("(50+25)*2") == 150);
    REQUIRE_THROWS(evaluate("1/0"));
    REQUIRE_THROWS(evaluate("1.2.3"));
    REQUIRE_THROWS(evaluate("system(1)"));
    REQUIRE_THROWS(evaluate("999999999"));
    REQUIRE(millimeters(812.54) == 812.5);
}
TEST_CASE("Project home index persists previews, deduplicates paths and preserves project files",
          "[experience][persistence]") {
    QTemporaryDir directory;
    auto filename = directory.filePath("Apartamento.lmx");
    auto document = kitchenExample();
    ProjectStore::save(filename, document, false);
    QFile original(filename);
    REQUIRE(original.open(QIODevice::ReadOnly));
    const auto originalBytes = original.readAll();
    original.close();
    ProjectLibrary library(directory.filePath("library"));
    REQUIRE(library.projects().empty());
    QImage preview(200, 100, QImage::Format_RGB32);
    preview.fill(Qt::red);
    library.remember(filename, document, preview);
    library.remember(filename, document);
    ProjectLibrary reopened(directory.filePath("library"));
    auto projects = reopened.projects();
    REQUIRE(projects.size() == 1);
    REQUIRE(projects[0].name.toStdString() == document.name);
    REQUIRE(projects[0].available);
    REQUIRE_FALSE(QImage(projects[0].thumbnail).isNull());
    REQUIRE(original.open(QIODevice::ReadOnly));
    REQUIRE(original.readAll() == originalBytes);
    original.close();
    REQUIRE(QFile::rename(filename, directory.filePath("Movido.lmx")));
    REQUIRE_FALSE(reopened.projects()[0].available);
    reopened.forget(filename);
    REQUIRE(reopened.projects().empty());
    REQUIRE(QFile::exists(directory.filePath("Movido.lmx")));
}
TEST_CASE("Scene cache reuses solids and invalidates walls, openings and associated finishes correctly",
          "[performance][geometry]") {
    auto document = kitchenExample();
    SceneGeometryCache cache;
    QElapsedTimer timer;
    timer.start();
    const auto first = cache.scene(document);
    const auto cold = timer.nsecsElapsed();
    const auto count = cache.buildCount();
    timer.restart();
    const auto second = cache.scene(document);
    const auto warm = timer.nsecsElapsed();
    REQUIRE(second.size() == first.size());
    REQUIRE(cache.buildCount() == count);
    for (std::size_t i = 0; i < first.size(); ++i)
        REQUIRE(first[i].shape.TShape() == second[i].shape.TShape());
    std::cout << "SCENE_CACHE_BENCHMARK: cold " << cold / 1e6 << " ms; warm " << warm / 1e6 << " ms\n";
    document.name = "Nome alterado";
    document.entities[0].name = "Outro nome";
    document.materials[0]["baseColor"] = {0.1, 0.2, 0.3};
    cache.scene(document);
    REQUIRE(cache.buildCount() == count);
    auto module = std::find_if(document.entities.begin(), document.entities.end(),
                               [](const auto &e) { return e.type == "FurnitureModule"; });
    module->width += 10;
    cache.scene(document);
    REQUIRE(cache.buildCount() > count + 1); // Source module and its associated countertop/plinth.
    const auto afterModule = cache.buildCount();
    auto door = std::find_if(document.entities.begin(), document.entities.end(),
                             [](const auto &e) { return e.type == "Door"; });
    door->width += 10;
    cache.scene(document);
    REQUIRE(cache.buildCount() == afterModule + 2); // Opening geometry and its wall cut.
    door->visible = false;
    REQUIRE(cache.scene(document).size() == buildScene(document).size());
}
TEST_CASE("wall vertical workflow keeps UUID across commands and project reopening",
          "[persistence][commands]") {
    application();
    Editor editor;
    auto w = wall(0, 0, 4000, 0);
    editor.apply("CreateWall", [&](Document &d) { d.entities.push_back(w); });
    REQUIRE(editor.document().at(w.id).width == 4000);
    editor.apply("ResizeWall", [&](Document &d) { d.at(w.id).width = 4123.5; });
    editor.history.undo();
    REQUIRE(editor.document().at(w.id).width == 4000);
    editor.history.redo();
    REQUIRE(editor.document().at(w.id).width == 4123.5);
    QTemporaryDir dir;
    auto path = dir.filePath("test.lmx");
    ProjectStore::save(path, editor.document());
    auto restored = ProjectStore::open(path);
    REQUIRE(restored.serialize() == editor.document().serialize());
    REQUIRE(restored.at(w.id).id == w.id);
    auto next = simple();
    ProjectStore::save(path, next);
    REQUIRE(ProjectStore::open(path + ".bak").serialize() == restored.serialize());
    auto invalid = simple();
    invalid.entities[0].width = -1;
    REQUIRE_THROWS(ProjectStore::save(path, invalid));
    REQUIRE(ProjectStore::open(path).serialize() == next.serialize());
}
TEST_CASE("failed edits preserve both scene and undo history", "[commands]") {
    application();
    Editor e;
    e.load(simple());
    auto before = e.document().serialize();
    REQUIRE_THROWS(e.apply("Invalid", [](Document &d) { d.entities[0].height = -30; }));
    REQUIRE(e.document().serialize() == before);
    REQUIRE(e.history.count() == 0);
}
TEST_CASE("B-rep wall dimensions and true opening volume", "[geometry]") {
    auto d = simple();
    auto wallId = d.entities[0].id;
    auto parts = buildEntity(d, d.entities[0]);
    REQUIRE(BRepCheck_Analyzer(parts[0].shape).IsValid());
    REQUIRE(volume(parts[0].shape) == Catch::Approx(4000.0 * 120 * 2700));
    auto door = entity("Door", "Porta");
    door.parent = wallId;
    door.width = 800;
    door.height = 2100;
    door.parameters = {{"offset", 200}, {"sill", 0}};
    d.entities.push_back(door);
    d.validate();
    auto withDoor = buildEntity(d, d.entities[0]);
    REQUIRE(BRepCheck_Analyzer(withDoor[0].shape).IsValid());
    REQUIRE(volume(withDoor[0].shape) == Catch::Approx(4000.0 * 120 * 2700 - 800.0 * 120 * 2100));
    auto window = entity("Window", "Janela");
    window.parent = wallId;
    window.width = 1200;
    window.height = 1000;
    window.parameters = {{"offset", 1600}, {"sill", 1000}};
    d.entities.push_back(window);
    d.validate();
    REQUIRE(volume(buildEntity(d, d.entities[0])[0].shape) ==
            Catch::Approx(4000.0 * 120 * 2700 - 800.0 * 120 * 2100 - 1200.0 * 120 * 1000));
    d.at(wallId).transform.x = 500;
    d.at(wallId).transform.yaw = 90;
    auto opening = buildEntity(d, d.at(window.id));
    Bnd_Box bounds;
    BRepBndLib::Add(compound(opening), bounds);
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bounds.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    REQUIRE(xmin == Catch::Approx(440).margin(0.01));
    REQUIRE(ymin == Catch::Approx(1600).margin(0.01));
}
TEST_CASE("openings cannot overlap or exceed their parent", "[geometry][validation]") {
    auto d = simple();
    auto o = entity("Door", "Porta");
    o.parent = d.entities[0].id;
    o.parameters = {{"offset", 3500}, {"sill", 0}};
    d.entities.push_back(o);
    REQUIRE_THROWS(d.validate());
    d.entities.back().parameters["offset"] = 100;
    d.validate();
    auto copy = d.entities.back();
    copy.id = uuid();
    d.entities.push_back(copy);
    REQUIRE_THROWS(d.validate());
}
TEST_CASE("parametric cabinet preserves panel thickness after a nonpreset resize", "[modules][geometry]") {
    Document d;
    auto m = entity("FurnitureModule", "Balcão");
    m.parameters = {
        {"family", "cabinet"}, {"doors", 2}, {"legs", 100}, {"handle", "bar"}, {"carcass", "oak"}};
    m.width = 947;
    d.entities.push_back(m);
    auto parts = buildEntity(d, m);
    REQUIRE(parts.size() >= 12);
    REQUIRE(volume(parts[0].shape) == Catch::Approx(18.0 * 550 * 620));
    REQUIRE(volume(parts[1].shape) == Catch::Approx(18.0 * 550 * 620));
    for (const auto &p : parts)
        REQUIRE(BRepCheck_Analyzer(p.shape).IsValid());
}
TEST_CASE("associated countertop recalculates from resized sources and cascades on deletion",
          "[automation]") {
    Document d;
    auto a = entity("FurnitureModule", "A");
    auto b = a;
    b.id = uuid();
    b.name = "B";
    b.transform.x = 800;
    d.entities = {a, b};
    auto top = automation(d, {a.id, b.id}, "countertop", 30, 0);
    d.entities.push_back(top);
    auto first = volume(buildEntity(d, top)[0].shape);
    REQUIRE(first == Catch::Approx(1600.0 * 568 * 30));
    d.at(b.id).width = 947;
    REQUIRE(volume(buildEntity(d, top)[0].shape) == Catch::Approx(1747.0 * 568 * 30));
    eraseCascade(d, {a.id});
    REQUIRE(!d.contains(top.id));
    REQUIRE(d.contains(b.id));
}
TEST_CASE("malicious and future containers are rejected without extraction", "[security][persistence]") {
    application();
    QTemporaryDir dir;
    auto bad = dir.filePath("evil.lmx");
    maliciousZip(bad, "../outside.txt", "attacker");
    REQUIRE_THROWS(ProjectStore::open(bad));
    REQUIRE(!QFile::exists(dir.filePath("../outside.txt")));
    auto good = dir.filePath("good.lmx");
    auto d = simple();
    ProjectStore::save(good, d);
    QFile original(good);
    REQUIRE(original.open(QIODevice::ReadOnly));
    auto bytes = original.readAll();
    bytes.truncate(bytes.size() / 2);
    QFile broken(bad);
    REQUIRE(broken.open(QIODevice::WriteOnly));
    broken.write(bytes);
    broken.close();
    REQUIRE_THROWS(ProjectStore::open(bad));
    REQUIRE(ProjectStore::open(good).serialize() == d.serialize());
}
TEST_CASE("stable graph rejects cycles duplicate UUIDs and orphan parents", "[validation]") {
    auto d = simple();
    auto clone = d.entities[0];
    d.entities.push_back(clone);
    REQUIRE_THROWS(d.validate());
    d.entities.pop_back();
    d.entities[0].parent = uuid();
    REQUIRE_THROWS(d.validate());
    d.entities[0].parent = d.entities[0].id;
    REQUIRE_THROWS(d.validate());
}
TEST_CASE("local SQLite FTS supports accent independent search favorites and recent", "[library]") {
    application();
    QTemporaryDir dir;
    Library lib(dir.filePath("library.db"));
    Json entries = Json::array();
    for (int i = 0; i < 10000; ++i)
        entries.push_back({{"id", "fixture-" + std::to_string(i)},
                           {"name", i == 0 ? "Armário Carvalho" : "Fixture " + std::to_string(i)},
                           {"category", "Cozinha"},
                           {"width", 800},
                           {"height", 720},
                           {"depth", 550},
                           {"recipe", {{"type", "FurnitureModule"}, {"parameters", {{"family", "cabinet"}}}}},
                           {"license", "CC0-1.0"},
                           {"author", "Test fixture"},
                           {"origin", "Synthetic test data, not product catalog"},
                           {"date", "2026-09-29"},
                           {"tags", "carvalho"}});
    lib.seed(entries);
    QElapsedTimer timer;
    timer.start();
    auto found = lib.search("armario");
    auto elapsed = timer.nsecsElapsed() / 1e6;
    std::cout << "FTS_BENCHMARK_10000: " << elapsed << " ms\n";
    REQUIRE(found.size() == 1);
    REQUIRE(found[0].name == QString::fromUtf8("Armário Carvalho"));
    REQUIRE(elapsed < 100);
    lib.favorite(found[0].id, true);
    REQUIRE(lib.search({}, {}, true).size() == 1);
    lib.used(found[0].id);
    REQUIRE(lib.search({}, {}, false, true).size() == 1);
    REQUIRE_NOTHROW(lib.search("\" OR * "));
    REQUIRE(lib.search("missing").empty());
}
TEST_CASE("scene snapshot exports actual geometry meters materials lights and camera", "[render]") {
    auto d = simple();
    auto camera = entity("Camera", "Camera");
    camera.parameters = {{"target", {2000, 0, 1000}}, {"lens", 28}};
    d.entities.push_back(camera);
    auto j = meshSnapshot(d);
    REQUIRE(j["meshes"].size() == 1);
    REQUIRE(j["meshes"][0]["triangles"].size() == 12);
    REQUIRE(j["cameras"].size() == 1);
    REQUIRE(j["materials"].size() >= 8);
}
TEST_CASE("DXF lines polylines circles arcs and units persist as locked layers", "[import][dxf]") {
    application();
    QTemporaryDir dir;
    auto path = dir.filePath("plan.dxf");
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    file.write("0\nSECTION\n2\nHEADER\n9\n$"
               "INSUNITS\n70\n6\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nLINE\n8\nWalls\n10\n0\n20\n0\n11\n4\n"
               "21\n0\n0\nLWPOLYLINE\n8\nWalls\n70\n1\n10\n0\n20\n0\n10\n4\n20\n0\n10\n4\n20\n3\n0\nCIRCLE\n8"
               "\nFurniture\n10\n2\n20\n1.5\n40\n0.5\n0\nARC\n8\nFurniture\n10\n0\n20\n0\n40\n0."
               "8\n50\n0\n51\n90\n0\nPOLYLINE\n8\nOld\n70\n0\n0\nVERTEX\n10\n0\n20\n0\n0\nVERTEX\n10\n2\n20\n"
               "1\n0\nSEQEND\n0\nENDSEC\n0\nEOF\n");
    file.close();
    auto drawing = readDxf(path);
    REQUIRE(drawing.millimeterScale == 1000);
    REQUIRE(drawing.layers.size() == 3);
    Document d;
    d.entities = dxfEntities(drawing, 1000, {"Walls", "Furniture", "Old"});
    d.validate();
    REQUIRE(d.entities[0].locked);
    REQUIRE(d.entities[1].parameters["primitives"][0]["radius"] == 500);
    auto parts = buildScene(d);
    REQUIRE(parts.size() == 7);
    auto archive = dir.filePath("dxf.lmx");
    ProjectStore::save(archive, d);
    REQUIRE(ProjectStore::open(archive).serialize() == d.serialize());
}
TEST_CASE("imported JPEG survives removal of original with hash deduplication", "[materials][persistence]") {
    application();
    QTemporaryDir dir;
    auto filename = dir.filePath("wood.jpg");
    QImage image(32, 32, QImage::Format_RGB32);
    image.fill(QColor(90, 70, 35));
    REQUIRE(image.save(filename, "JPEG"));
    auto texture = readTexture(filename);
    Document d;
    d.entities.push_back(wall(0, 0, 4000, 0));
    auto id = attachTexture(d, texture);
    REQUIRE(attachTexture(d, texture) == id);
    REQUIRE(d.embeddedAssets.size() == 1);
    d.entities[0].material = id;
    d.validate();
    auto project = dir.filePath("textured.lmx");
    ProjectStore::save(project, d);
    REQUIRE(QFile::remove(filename));
    auto restored = ProjectStore::open(project);
    REQUIRE(restored.serialize() == d.serialize());
    REQUIRE(restored.embeddedAssets.at(texture.hash) == texture.png);
    auto snapshot = meshSnapshot(restored);
    REQUIRE(snapshot["assets"].contains(texture.hash));
    restored.embeddedAssets[texture.hash] = "tampered";
    REQUIRE_THROWS(restored.validate());
}
TEST_CASE("recovery retains five versions per project and skips corrupted snapshots", "[recovery]") {
    application();
    QTemporaryDir dir;
    RecoveryStore store(dir.path());
    auto first = simple(), second = simple();
    for (int i = 0; i < 7; ++i) {
        first.entities[0].width = 4000 + i * 0.1;
        store.save(first);
        QThread::msleep(3);
    }
    store.save(second);
    REQUIRE(store.entries().size() == 6);
    auto corrupt = dir.filePath("corrupted.lmx");
    QFile file(corrupt);
    REQUIRE(file.open(QIODevice::WriteOnly));
    file.write("truncated ZIP");
    file.close();
    QStringList errors;
    auto entries = store.entries(&errors);
    REQUIRE(entries.size() == 6);
    REQUIRE(errors.size() == 1);
    REQUIRE(QFile::exists(corrupt));
    auto latest = std::find_if(entries.begin(), entries.end(),
                               [&](const auto &entry) { return entry.projectId == first.id; });
    REQUIRE(latest != entries.end());
    REQUIRE(ProjectStore::open(latest->file.absoluteFilePath()).entities[0].width == Catch::Approx(4000.6));
    store.clear(first.id);
    REQUIRE(store.entries().size() == 1);
    REQUIRE(store.entries()[0].projectId == second.id);
    REQUIRE_THROWS(store.clear("../not-a-project"));
}
TEST_CASE("every shipped starter recipe creates valid solids; invalid edits preserve state",
          "[library][geometry]") {
    application();
    QTemporaryDir dir;
    Library library(dir.filePath("starter.db"));
    QFile catalog(QStringLiteral(LMX_SOURCE_DIR) + "/starter-library/catalog.json");
    REQUIRE(catalog.open(QIODevice::ReadOnly));
    auto entries = Json::parse(catalog.readAll().toStdString());
    REQUIRE(entries.size() == 27);
    library.seed(entries);
    auto assets = library.search({});
    REQUIRE(assets.size() == 27);
    for (const auto &asset : assets) {
        INFO(asset.name.toStdString());
        Document document;
        auto object = Library::instantiate(asset, 0, 0);
        if (object.type == "Door" || object.type == "Window") {
            auto supporting = wall(0, 0, 5000, 0);
            object.parent = supporting.id;
            document.entities.push_back(supporting);
        }
        document.entities.push_back(object);
        REQUIRE_NOTHROW(document.validate());
        const auto parts = buildScene(document);
        REQUIRE_FALSE(parts.empty());
        for (const auto &part : parts)
            REQUIRE(BRepCheck_Analyzer(part.shape).IsValid());
    }
    Editor editor;
    Document sofa;
    auto object = entity("DecorativeObject", "Sofá");
    object.parameters = {{"family", "sofa"}};
    object.width = 2100;
    object.depth = 850;
    object.height = 900;
    sofa.entities.push_back(object);
    editor.load(sofa);
    REQUIRE_THROWS(editor.apply("Invalid sofa", [](Document &d) { d.entities[0].height = 100; }));
    REQUIRE(editor.document().serialize() == sofa.serialize());
    REQUIRE(editor.history.count() == 0);
    REQUIRE_THROWS(
        editor.apply("Unknown family", [](Document &d) { d.entities[0].parameters["family"] = "unknown"; }));
    auto badMaterial = sofa;
    badMaterial.materials[0].erase("name");
    REQUIRE_THROWS(badMaterial.validate());
}
TEST_CASE("render camera exposure environment survive ZIP and camera removal undo", "[render][persistence]") {
    application();
    QTemporaryDir dir;
    auto document = simple();
    auto first = entity("Camera", "Principal"), second = entity("Camera", "Detalhe");
    document.entities.push_back(first);
    document.entities.push_back(second);
    document.renderSettings = {
        {"camera", second.id}, {"exposure", -1.2}, {"environmentStrength", 0.08}, {"denoise", false}};
    auto path = dir.filePath("presentation.lmx");
    ProjectStore::save(path, document);
    auto reopened = ProjectStore::open(path);
    REQUIRE(reopened.serialize() == document.serialize());
    REQUIRE(meshSnapshot(reopened)["renderSettings"] == document.renderSettings);
    auto legacy = document.serialize();
    legacy.erase("renderSettings");
    REQUIRE(Document::deserialize(legacy).renderSettings["camera"] == "");
    Editor editor;
    editor.load(document);
    editor.apply("Delete selected camera", [&](Document &d) { eraseCascade(d, {second.id}); });
    REQUIRE(editor.document().renderSettings["camera"] == "");
    editor.history.undo();
    REQUIRE(editor.document().serialize() == document.serialize());
    REQUIRE_THROWS(editor.apply("Invalid exposure", [](Document &d) { d.renderSettings["exposure"] = 20; }));
    REQUIRE_THROWS(editor.apply("Invalid camera", [](Document &d) { d.renderSettings["camera"] = uuid(); }));
    auto light = entity("Light", "Spot");
    light.parameters = {{"kind", "spot"}, {"angle", 50}, {"blend", 0.4}, {"color", {1.0, 0.9, 0.8}}};
    reopened.entities.push_back(light);
    REQUIRE_NOTHROW(reopened.validate());
    reopened.entities.back().parameters["color"] = {2.0, 0.9, 0.8};
    REQUIRE_THROWS(reopened.validate());
}
TEST_CASE("library thumbnails show real furniture geometry and distinct decorative shapes",
          "[library][thumbnails]") {
    application();
    Asset cabinet{"cabinet",
                  "Armário",
                  "Cozinha",
                  800,
                  720,
                  550,
                  {{"type", "FurnitureModule"}, {"parameters", {{"family", "cabinet"}}}},
                  false};
    Asset vase{"vase",
               "Vaso",
               "Decoração",
               200,
               450,
               200,
               {{"type", "DecorativeObject"}, {"parameters", {{"family", "vase"}}}},
               false};
    auto first = renderAssetThumbnail(cabinet), second = renderAssetThumbnail(vase);
    REQUIRE(first.size() == QSize(192, 144));
    REQUIRE(second.size() == first.size());
    REQUIRE(first != second);
    int geometryPixels = 0;
    for (int y = 0; y < first.height(); ++y)
        for (int x = 0; x < first.width(); ++x)
            geometryPixels += first.pixelColor(x, y) != first.pixelColor(0, 0);
    REQUIRE(geometryPixels > 1000);
}
TEST_CASE("bundled PBR maps remain self contained and reject missing normals or tampered source",
          "[materials][persistence][pbr]") {
    application();
    auto pack = readPbrMaterials(QStringLiteral(LMX_SOURCE_DIR) + "/starter-materials");
    REQUIRE(pack.materials.size() == 2);
    REQUIRE(pack.assets.size() == 6);
    auto document = kitchenExample();
    attachPbrMaterials(document, pack);
    REQUIRE(document.embeddedAssets.size() == 6);
    QTemporaryDir directory;
    ProjectStore::save(directory.filePath("photo.lmx"), document, false);
    auto reopened = ProjectStore::open(directory.filePath("photo.lmx"));
    REQUIRE(reopened.serialize() == document.serialize());
    REQUIRE(meshSnapshot(reopened)["assets"].size() == 6);
    REQUIRE(reopened.renderSettings["environmentMode"] == "sky");
    auto bad = reopened;
    auto material = std::find_if(bad.materials.begin(), bad.materials.end(),
                                 [](const auto &m) { return m.at("id") == "oak"; });
    material->at("normalTexture") = std::string(64, '0');
    REQUIRE_THROWS(bad.validate());
    bad = reopened;
    bad.renderSettings["sunElevation"] = 95;
    REQUIRE_THROWS(bad.validate());
    bad.renderSettings["sunElevation"] = 35;
    bad.renderSettings["environmentMode"] = "invented";
    REQUIRE_THROWS(bad.validate());
    QFile source(QStringLiteral(LMX_SOURCE_DIR) + "/starter-materials/catalog.json");
    REQUIRE(source.open(QIODevice::ReadOnly));
    auto manifest = Json::parse(source.readAll().toStdString());
    manifest["materials"][0]["maps"]["baseColorTexture"]["file"] = "../escape.png";
    QFile corrupt(directory.filePath("catalog.json"));
    REQUIRE(corrupt.open(QIODevice::WriteOnly));
    corrupt.write(QByteArray::fromStdString(manifest.dump()));
    corrupt.close();
    REQUIRE_THROWS(readPbrMaterials(directory.path()));
    source.seek(0);
    manifest = Json::parse(source.readAll().toStdString());
    auto &map = manifest["materials"][0]["maps"]["baseColorTexture"];
    const auto filename = QString::fromStdString(map.at("file").get<std::string>());
    REQUIRE(QFile::copy(QStringLiteral(LMX_SOURCE_DIR) + "/starter-materials/" + filename,
                        directory.filePath(filename)));
    map["sha256"] = std::string(64, '0');
    REQUIRE(corrupt.open(QIODevice::WriteOnly | QIODevice::Truncate));
    corrupt.write(QByteArray::fromStdString(manifest.dump()));
    corrupt.close();
    REQUIRE_THROWS(readPbrMaterials(directory.path()));
    Editor editor;
    auto original = kitchenExample();
    editor.load(original);
    editor.apply("PBR", [&](Document &d) { attachPbrMaterials(d, pack); });
    editor.history.undo();
    REQUIRE(editor.document().serialize() == original.serialize());
    editor.history.redo();
    REQUIRE(editor.document().embeddedAssets.size() == 6);
}
