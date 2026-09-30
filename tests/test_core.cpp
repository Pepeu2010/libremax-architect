#include "commands/editor.h"
#include "core/expression.h"
#include "document/document.h"
#include "document/examples.h"
#include "geometry/geometry.h"
#include "import/dxf.h"
#include "library/library.h"
#include "library/thumbnails.h"
#include "materials/texture.h"
#include "persistence/project_store.h"
#include "persistence/recovery_store.h"
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Bnd_Box.hxx>
#include <QApplication>
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
    static int argc = 1;
    static char name[] = "lmx-tests";
    static char *argv[] = {name, nullptr};
    static QApplication app(argc, argv);
    return app;
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
    REQUIRE(entries.size() == 25);
    library.seed(entries);
    auto assets = library.search({});
    REQUIRE(assets.size() == 25);
    for (const auto &asset : assets) {
        INFO(asset.name.toStdString());
        Document document;
        document.entities.push_back(Library::instantiate(asset, 0, 0));
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
            geometryPixels += first.pixelColor(x, y) != QColor("#111a22");
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
