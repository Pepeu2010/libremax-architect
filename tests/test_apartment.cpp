#include "commands/arrangement.h"
#include "commands/editor.h"
#include "document/room_outline.h"
#include "geometry/geometry.h"
#include "geometry/scene_cache.h"
#include "import/model_import.h"
#include "library/model.h"
#include "library/model_pack.h"
#include "persistence/project_store.h"
#include "placement/placement.h"
#include "rendering/room_look.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <zip.h>
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif
using namespace lmx;
TEST_CASE("Connected wall endpoints form clean miters and invalidate their neighbors", "[apartment][joins]") {
    for (double angle : {30.0, 90.0, 135.0}) {
        const auto radians = angle * 3.141592653589793 / 180;
        Document d;
        d.entities.push_back(wall(0, 0, 4000, 0));
        d.entities.push_back(wall(4000, 0, 4000 + 3000 * std::cos(radians), 3000 * std::sin(radians)));
        auto first = compound(buildEntity(d, d.entities[0])),
             second = compound(buildEntity(d, d.entities[1]));
        REQUIRE(BRepCheck_Analyzer(first).IsValid());
        REQUIRE(BRepCheck_Analyzer(second).IsValid());
        BRepAlgoAPI_Common common(first, second);
        common.Build();
        REQUIRE(common.IsDone());
        REQUIRE(std::abs(volume(common.Shape())) < 1);
        REQUIRE(volume(first) + volume(second) == Catch::Approx(7000 * 120.0 * 2700));
        SceneGeometryCache cache;
        cache.scene(d);
        auto builds = cache.buildCount();
        d.entities[1].height = 3000;
        cache.scene(d);
        REQUIRE(cache.buildCount() == builds + 2);
    }
}
TEST_CASE("Cached mesh validation still rejects rehashed malformed models", "[apartment][lod][cache]") {
    Json model = {{"schema", 1}};
    model["parts"] = Json::array({Json{
        {"material", "white"}, {"vertices", {{0, 0, 0}, {1, 0, 0}, {0, 1, 1}}}, {"triangles", {{0, 1, 2}}}}});
    auto bytes = QByteArray::fromStdString(model.dump());
    REQUIRE(validatedModelMaterials(bytes) == std::vector<std::string>{"white"});
    REQUIRE(validatedModelMaterials(bytes) == std::vector<std::string>{"white"});
    model["parts"][0]["triangles"][0][2] = 8;
    REQUIRE_THROWS(validatedModelMaterials(QByteArray::fromStdString(model.dump())));
    Document d;
    auto hash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex().toStdString();
    d.embeddedAssets[hash] = bytes;
    auto object = entity("MeshObject", "Cached model");
    object.parameters["meshAsset"] = hash;
    d.entities.push_back(object);
    d.validate();
    d.embeddedAssets[hash].append('x');
    REQUIRE_THROWS(d.validate());
}
TEST_CASE("Contemporary collection retains provenance and distinct editing meshes",
          "[apartment][packs][lod]") {
    const auto pack =
        readModelPack(QStringLiteral(LMX_SOURCE_DIR) + "/collections/Apartamento-contemporaneo-1.lmaxpack");
    REQUIRE(pack.assets.size() == 28);
    QTemporaryDir temp;
    const auto models = temp.filePath("models");
    Library library(temp.filePath("catalog.db"), {}, models);
    installModelPack(library, models, pack);
    REQUIRE(library.search().size() == 28);
    const auto assets = library.search();
    size_t reduced = 0;
    for (const auto &asset : assets) {
        INFO(asset.id.toStdString());
        Document d;
        Library::attachModel(d, asset);
        auto e = Library::instantiate(asset, 0, 0);
        d.entities.push_back(e);
        d.validate();
        REQUIRE(e.metadata["license"] == "CC0-1.0");
        REQUIRE_FALSE(meshSnapshot(d)["meshes"].empty());
        if (!asset.editModel.isEmpty()) {
            auto full = readModel(asset.model), low = readModel(asset.editModel);
            size_t fullCount = 0, lowCount = 0;
            for (auto p : full["parts"])
                fullCount += p["triangles"].size();
            for (auto p : low["parts"])
                lowCount += p["triangles"].size();
            REQUIRE(lowCount < fullCount);
            REQUIRE(lowCount <= 2250);
            ++reduced;
            auto display = d;
            display.entities.back().parameters["meshAsset"] = e.parameters.at("editMeshAsset");
            REQUIRE(meshSnapshot(display)["meshes"] != meshSnapshot(d)["meshes"]);
        }
    }
    REQUIRE(reduced == 18);
    const auto favorite = assets.front().id;
    library.favorite(favorite, true);
    installModelPack(library, models, pack);
    REQUIRE(library.search().size() == 28);
    REQUIRE(library.search({}, {}, true).size() == 1);
    auto updated = pack;
    updated.version = 2;
    updated.sha256 = QString(64, 'a');
    updated.assets.front().name = "Peça atualizada";
    installModelPack(library, models, updated);
    REQUIRE(library.search("Peça atualizada").size() == 1);
    REQUIRE_THROWS(installModelPack(library, models, pack));
    REQUIRE(library.search().size() == 28);
    auto bad = updated;
    bad.assets.back().model.append('x');
    bad.version = 3;
    REQUIRE_THROWS(installModelPack(library, models, bad));
    REQUIRE(library.search().size() == 28);
    int error = 0;
    auto *archive = zip_open(QFile::encodeName(temp.filePath("bad.lmaxpack")).constData(),
                             ZIP_CREATE | ZIP_TRUNCATE, &error);
    REQUIRE(archive);
    const std::string bytes = "{}";
    for (auto name : {"../escape.json", "manifest.json"}) {
        auto *source = zip_source_buffer(archive, bytes.data(), bytes.size(), 0);
        REQUIRE(source);
        REQUIRE(zip_file_add(archive, name, source, 0) >= 0);
    }
    REQUIRE(zip_close(archive) == 0);
    REQUIRE_THROWS(readModelPack(temp.filePath("bad.lmaxpack")));
    ProjectStore::save(
        temp.filePath("portable.lmx"),
        [&] {
            Document d;
            Library::attachModel(d, assets.front());
            d.entities.push_back(Library::instantiate(assets.front(), 0, 0));
            return d;
        }(),
        false);
    REQUIRE(ProjectStore::open(temp.filePath("portable.lmx")).entities.size() == 1);
}
TEST_CASE("Room photo styles replace only their generated lights and support undo", "[apartment][looks]") {
    Document d;
    addPolygonRoom(d, {{0, 0}, {5000, 0}, {5000, 2000}, {2000, 2000}, {2000, 5000}, {0, 5000}});
    const auto room = d.entities.front();
    auto user = entity("Light", "Luz do usuário");
    d.entities.push_back(user);
    Editor editor;
    editor.load(d);
    for (auto style : {"natural", "bright", "evening"}) {
        editor.apply("Estilo", [&](Document &doc) { applyRoomLook(doc, room.id, style); });
        const auto &doc = editor.document();
        REQUIRE_NOTHROW(doc.validate());
        REQUIRE(doc.contains(user.id));
        auto ceiling = std::find_if(doc.entities.begin(), doc.entities.end(),
                                    [](const auto &e) { return e.type == "Ceiling"; });
        REQUIRE(ceiling != doc.entities.end());
        REQUIRE(ceiling->visible);
        REQUIRE(volume(compound(buildEntity(doc, *ceiling))) == Catch::Approx(16000000 * 25));
        size_t generated = 0;
        for (auto e : doc.entities)
            if (e.metadata.value("lookRoom", std::string{}) == room.id) {
                ++generated;
                REQUIRE(insideOutline(roomOutline(room, true), {e.transform.x, e.transform.y}, -.1));
            }
        REQUIRE(generated == 1);
    }
    editor.history.undo();
    editor.history.undo();
    editor.history.undo();
    REQUIRE(editor.document().serialize() == d.serialize());
}
TEST_CASE("Furniture sets move atomically and align without entering walls", "[apartment][arrangement]") {
    Document d;
    addRectangularRoom(d, 6000, 5000, 2700, 120);
    std::vector<std::string> ids;
    for (int i = 0; i < 3; ++i) {
        auto e = entity("GeometryObject", "Banco");
        e.width = 500;
        e.depth = 500;
        e.height = 500;
        e.transform = {1000.0 + i * 1500, 1000.0 + i * 300, 0, 0, false};
        ids.push_back(e.id);
        d.entities.push_back(e);
    }
    Editor editor;
    editor.load(d);
    std::string group;
    editor.apply("Juntar", [&](Document &doc) { group = groupObjects(doc, ids); });
    REQUIRE(arrangementMembers(editor.document(), {group}).size() == 3);
    editor.apply("Mover", [&](Document &doc) { moveObjects(doc, {group}, 200, 300); });
    for (auto id : ids) {
        REQUIRE(editor.document().at(id).transform.x == d.at(id).transform.x + 200);
        REQUIRE(editor.document().at(id).transform.y == d.at(id).transform.y + 300);
    }
    const auto moved = editor.document().serialize();
    REQUIRE_THROWS(
        editor.apply("Mover para fora", [&](Document &doc) { moveObjects(doc, {group}, 5000, 0); }));
    REQUIRE(editor.document().serialize() == moved);
    editor.apply("Alinhar", [&](Document &doc) { arrangeObjects(doc, {group}, "back"); });
    for (auto id : ids)
        REQUIRE(editor.document().at(id).transform.y == 1300);
    editor.apply("Separar", [&](Document &doc) { ungroupObjects(doc, {group}); });
    REQUIRE_FALSE(editor.document().contains(group));
    for (auto id : ids)
        REQUIRE(editor.document().at(id).parent.empty());
    editor.history.undo();
    REQUIRE(editor.document().contains(group));
    REQUIRE_NOTHROW(editor.document().validate());
}
TEST_CASE("L rooms retain their actual footprint and associative surfaces", "[apartment][outline]") {
    Document d;
    const Outline l{{0, 0}, {5000, 0}, {5000, 2000}, {2000, 2000}, {2000, 5000}, {0, 5000}};
    addPolygonRoom(d, l);
    REQUIRE_NOTHROW(d.validate());
    const auto room = d.entities.front();
    REQUIRE(insideOutline(l, roomInteriorPoint(room), -.1));
    REQUIRE_FALSE(insideOutline(l, {4000, 4000}));
    auto table = entity("GeometryObject", "Mesa");
    table.width = 500;
    table.depth = 500;
    table.height = 700;
    REQUIRE(placeObject(d, table, 1000, 1000, false).allowed);
    REQUIRE_FALSE(placeObject(d, table, 4000, 4000, false).allowed);
    // Corners alone are insufficient for a shape bridging a concave notch.
    REQUIRE_FALSE(footprintInside(l, {{1000, 3000}, {3000, 1000}, {4000, 2000}, {2000, 4000}}));
    const auto floor = d.entities[d.entities.size() - 2];
    REQUIRE(volume(compound(buildEntity(d, floor))) == Catch::Approx(16000000 * 25));
    for (const auto &wall : d.entities)
        if (wall.type == "Wall")
            REQUIRE(BRepCheck_Analyzer(compound(buildEntity(d, wall))).IsValid());
    QTemporaryDir dir;
    ProjectStore::save(dir.filePath("l-room.lmx"), d, false);
    auto reopened = ProjectStore::open(dir.filePath("l-room.lmx"));
    REQUIRE(reopened.serialize() == d.serialize());
    Editor editor;
    editor.load(reopened);
    SceneGeometryCache cache;
    cache.scene(d);
    auto modified = l;
    modified[1][0] = 6000;
    modified[2][0] = 6000;
    editor.apply("Ajustar", [&](Document &doc) { editPolygonRoom(doc, room.id, modified, 3000); });
    auto edited = editor.document();
    REQUIRE(volume(compound(buildEntity(edited, edited.at(floor.id)))) == Catch::Approx(18000000 * 25));
    const auto parts = cache.scene(edited);
    double cachedFloor = 0;
    for (auto part : parts)
        if (part.owner == floor.id)
            cachedFloor += volume(part.shape);
    REQUIRE(cachedFloor == Catch::Approx(18000000 * 25));
    editor.history.undo();
    REQUIRE(editor.document().serialize() == d.serialize());
    editor.history.redo();
    REQUIRE(editor.document().serialize() == edited.serialize());
}
TEST_CASE("Room outlines reject crossings overlaps and opening loss", "[apartment][outline]") {
    Document neighbors;
    addPolygonRoom(neighbors, {{0, 0}, {4000, 0}, {4000, 4000}, {0, 4000}});
    const auto firstRoom = neighbors.entities.front().id;
    addPolygonRoom(neighbors, {{4000, 0}, {8000, 0}, {8000, 4000}, {4000, 4000}});
    size_t walls = 0;
    for (const auto &e : neighbors.entities)
        walls += e.type == "Wall";
    REQUIRE(walls == 7);
    const auto before = neighbors.serialize();
    REQUIRE_THROWS(editPolygonRoom(neighbors, firstRoom, {{0, 0}, {3500, 0}, {3500, 4000}, {0, 4000}}, 2700));
    REQUIRE(neighbors.serialize() == before);
    REQUIRE_THROWS(validateOutline({{0, 0}, {4000, 4000}, {0, 4000}, {4000, 0}}));
    REQUIRE_THROWS(validateOutline({{0, 0}, {4000, 0}, {2000, 0}, {2000, 3000}, {0, 3000}}));
    Document d;
    const Outline square{{0, 0}, {4000, 0}, {4000, 4000}, {0, 4000}};
    addPolygonRoom(d, square);
    REQUIRE_THROWS(addPolygonRoom(d, square));
    REQUIRE_THROWS(addPolygonRoom(d, {{3000, 3000}, {5000, 3000}, {5000, 5000}, {3000, 5000}}));
    const auto room = d.entities.front();
    auto door = entity("Door", "Porta");
    door.parent = d.entities[1].id;
    door.width = 900;
    door.height = 2100;
    door.parameters = {{"offset", 3000}, {"sill", 0}};
    d.entities.push_back(door);
    d.validate();
    Editor editor;
    editor.load(d);
    REQUIRE_THROWS(editor.apply("Diminuir", [&](Document &doc) {
        editPolygonRoom(doc, room.id, {{0, 0}, {3500, 0}, {3500, 4000}, {0, 4000}}, 2700);
    }));
    REQUIRE(editor.document().serialize() == d.serialize());
    REQUIRE(editor.history.count() == 0);
}
