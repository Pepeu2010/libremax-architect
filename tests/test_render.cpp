#include "commands/editor.h"
#include "document/examples.h"
#include "geometry/geometry.h"
#include "persistence/project_store.h"
#include "rendering/environment_map.h"
#include "rendering/high_dynamic_image.h"
#include "rendering/light_model.h"
#include "rendering/render_progress.h"
#include "rendering/render_queue.h"
#include "rendering/render_result.h"
#include "rendering/render_snapshot.h"
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfOutputFile.h>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <limits>
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif
using namespace lmx;
TEST_CASE("Cycles progress keeps tile completion separate from current samples and final publication",
          "[progress]") {
    CyclesProgress tracker;
    auto update = tracker.consume("LIBREMAX_STATS Remaining: 06:27.79 | Rendered 0/4 Tiles, Sample 96/512");
    REQUIRE(update);
    REQUIRE(update->percent == 4);
    REQUIRE(update->remainingMs == 387790);
    REQUIRE(tracker.consume("Rendered 0/4 Tiles, Sample 512/512")->percent == 25);
    REQUIRE(tracker.consume("Rendered 1/4 Tiles, Sample 512/512")->percent == 25);
    REQUIRE(tracker.consume("Rendered 1/4 Tiles, Sample 1/512")->percent == 25);
    REQUIRE(tracker.consume("Rendered 1/4 Tiles, Sample 256/512")->percent == 37);
    REQUIRE(tracker.consume("Rendered 1/4 Tiles, Sample 512/512")->percent == 50);
    REQUIRE(tracker.consume("Rendered 2/4 Tiles, Sample 512/512")->percent == 50);
    REQUIRE(tracker.consume("Rendered 3/4 Tiles, Sample 1/512")->percent == 75);
    REQUIRE(tracker.consume("Rendered 3/4 Tiles, Sample 512/512")->percent == 99);
    REQUIRE(tracker.consume("Rendered 3/4 Tiles, Sample 0/512")->percent == 99);
    REQUIRE_FALSE(tracker.consume("LIBREMAX_STATS ViewLayer | Denoising"));
    REQUIRE_FALSE(isCyclesDenoising("LIBREMAX_STATS Mem: 56M | Loading denoising kernels"));
    REQUIRE(isCyclesDenoising("LIBREMAX_STATS Mem: 226M | ViewLayer | Denoising"));
    REQUIRE_FALSE(isCyclesDenoising("LIBREMAX_STATS Mem: 238M | Sample 1/512"));
    tracker = {};
    REQUIRE(tracker.consume("Rendering 1/128")->percent == 0);
    REQUIRE(tracker.consume("Rendering 128/128")->percent == 99);
}
TEST_CASE("Engine estimates parse hours and milliseconds and never synthesize time from sample count",
          "[progress]") {
    CyclesProgress tracker;
    REQUIRE(tracker.consume("Remaining: 01:02:03.25 | Sample 2/8")->remainingMs == 3723250);
    REQUIRE(tracker.consume("Remaining: 00:00.00 | Sample 3/8")->remainingMs == 0);
    REQUIRE(tracker.consume("Sample 4/8")->remainingMs == -1);
    REQUIRE(tracker.consume("Remaining: 00:99.9 | Sample 5/8")->remainingMs == -1);
    REQUIRE(tracker.consume("Remaining: 999:00:00 | Sample 6/8")->remainingMs == -1);
    REQUIRE_FALSE(tracker.consume("Sample 9/8"));
    REQUIRE_FALSE(tracker.consume("Sample 0/0"));
    REQUIRE_FALSE(tracker.consume("Rendered 5/4 Tiles, Sample 1/8"));
    REQUIRE(renderDuration(3723250) == "1h 2min 3s");
}
TEST_CASE("Render timing hides stale predictions and finalization estimates and freezes completed durations",
          "[progress]") {
    Json entry = {{"state", "Queued"}};
    REQUIRE(renderTimingText(entry).contains("Aguardando"));
    entry = {{"state", "Rendering"},
             {"started", "2026-10-03T14:00:00.000Z"},
             {"elapsedMs", 10000},
             {"remainingMs", 90000},
             {"remainingUpdatedElapsedMs", 5000},
             {"estimatedFinish", "2026-10-03T14:01:35.000Z"}};
    REQUIRE(renderTimingText(entry).contains("Decorrido: 10s"));
    REQUIRE(renderTimingText(entry).contains("restante estimado: 1min 25s"));
    REQUIRE(renderTimingText(entry).contains("aproximadamente"));
    entry["elapsedMs"] = 36000;
    REQUIRE_FALSE(renderTimingText(entry).contains("restante estimado"));
    entry["state"] = "Denoising";
    REQUIRE(renderTimingText(entry).contains("finalizando"));
    REQUIRE_FALSE(renderTimingText(entry).contains("restante estimado"));
    entry["state"] = "Completed";
    REQUIRE(renderTimingText(entry) == "Tempo em execução: 36s");
    entry.erase("elapsedMs");
    REQUIRE(renderTimingText(entry).contains("não registrado"));
}
TEST_CASE("Five light types retain Kelvin, physical dimensions, orientation and versioned snapshots",
          "[lighting]") {
    QTemporaryDir directory;
    auto document = kitchenExample();
    document.version = 3;
    std::erase_if(document.entities, [](const Entity &e) { return e.type == "Light"; });
    for (const auto &kind : {"point", "spot", "area", "led", "sun"}) {
        auto light = lightEntity(kind);
        light.transform = {1200, 1500, 2400, 35, false};
        light.parameters["target"] = {1200, 1500, 0};
        light.parameters["temperature"] = 3400;
        document.entities.push_back(light);
    }
    REQUIRE_NOTHROW(document.validate());
    const auto filename = directory.filePath("lighting.lmx");
    ProjectStore::save(filename, document, false);
    REQUIRE(ProjectStore::open(filename).serialize() == document.serialize());
    auto options = renderPreset("custom");
    options["format"] = "EXR";
    RenderSnapshot snapshot(document, options, document.renderSettings.at("camera").get<std::string>());
    REQUIRE(snapshot.document().version == 3);
    const auto exported = meshSnapshot(snapshot.document());
    REQUIRE(exported.at("lights").size() == 5);
    for (const auto &light : exported.at("lights")) {
        REQUIRE(light.at("position") == Json::array({1.2, 1.5, 2.4}));
        REQUIRE(light.at("rotationZ") == 35);
        REQUIRE(light.at("parameters").at("temperature") == 3400);
    }
    document.entities.back().parameters["temperature"] = 6500;
    REQUIRE(snapshot.document().entities.back().parameters.at("temperature") == 3400);
    attachEnvironment(document, importEnvironment(QStringLiteral(
                                    LMX_SOURCE_DIR "/starter-environments/kiara_1_dawn_1k.hdr")));
    REQUIRE(document.version == 3);
    REQUIRE_NOTHROW(document.validate());
    const auto legacy = kitchenExample();
    REQUIRE_NOTHROW(legacy.validate());
    auto legacyZeroDirection = legacy;
    auto oldLight = entity("Light", "Luz antiga");
    oldLight.parameters = {{"kind", "area"}, {"target", {0, 0, 0}}};
    legacyZeroDirection.entities.push_back(oldLight);
    REQUIRE_NOTHROW(legacyZeroDirection.validate());
    auto downgraded = document;
    downgraded.version = 2;
    REQUIRE_THROWS(downgraded.validate());
}
TEST_CASE("Lighting rejects invalid temperature, geometry, direction and non-finite intensity",
          "[lighting]") {
    auto document = kitchenExample();
    document.version = 3;
    document.entities.push_back(lightEntity("led"));
    const auto good = document;
    for (const auto &[key, bad] :
         std::vector<std::pair<std::string, Json>>{{"temperature", 500},
                                                   {"temperature", 13000},
                                                   {"power", -1},
                                                   {"power", 100001},
                                                   {"size", 0},
                                                   {"sizeY", 0},
                                                   {"radius", -1},
                                                   {"sunAngle", 0},
                                                   {"blend", 2},
                                                   {"angle", 180},
                                                   {"shape", "INVALID"},
                                                   {"colorMode", "INVALID"},
                                                   {"kind", "LASER"},
                                                   {"target", Json::array({0, 0})},
                                                   {"target", Json::array({0, 0, 2500})},
                                                   {"color", Json::array({1, 0, -1})}}) {
        INFO(key);
        document = good;
        document.entities.back().parameters[key] = bad;
        REQUIRE_THROWS(document.validate());
    }
    document = good;
    document.entities.back().parameters["power"] = std::numeric_limits<double>::quiet_NaN();
    REQUIRE_THROWS(document.validate());
}
TEST_CASE("Render modes preserve distinct user quality contracts and reject unsafe settings") {
    const auto rapid = renderPreset("rapid"), normal = renderPreset("normal"), final = renderPreset("final");
    REQUIRE(rapid.at("samples") == 32);
    REQUIRE(normal.at("samples") == 128);
    REQUIRE(final.at("samples") == 512);
    REQUIRE(rapid.at("maxBounces") < normal.at("maxBounces"));
    REQUIRE(normal.at("maxBounces") < final.at("maxBounces"));
    for (const auto &options : {rapid, normal, final, renderPreset("custom")})
        REQUIRE_NOTHROW(validateRenderOptions(options));
    auto bad = final;
    bad["width"] = 9000;
    REQUIRE_THROWS(validateRenderOptions(bad));
    bad = final;
    bad["width"] = 640.5;
    REQUIRE_THROWS(validateRenderOptions(bad));
    bad = final;
    bad["format"] = "JPEG";
    bad["transparent"] = true;
    REQUIRE_THROWS(validateRenderOptions(bad));
    bad = final;
    bad["noiseThreshold"] = 2;
    REQUIRE_THROWS(validateRenderOptions(bad));
    bad = final;
    bad["diffuseBounces"] = -1;
    REQUIRE_THROWS(validateRenderOptions(bad));
    auto advanced = renderPreset("custom");
    advanced["format"] = "EXR";
    advanced["transparent"] = true;
    REQUIRE_NOTHROW(validateRenderOptions(advanced));
    advanced["preset"] = "normal";
    REQUIRE_THROWS(validateRenderOptions(advanced));
}

TEST_CASE("Local HDRI is floating point, portable, versioned and immutable in queued snapshots") {
    QTemporaryDir directory;
    const auto source = directory.filePath("daylight.hdr");
    REQUIRE(QFile::copy(QStringLiteral(LMX_SOURCE_DIR "/starter-environments/kiara_1_dawn_1k.hdr"), source));
    const auto environment = importEnvironment(source);
    const auto decoded = inspectHighDynamicImage(environment.bytes, true);
    REQUIRE(decoded.size == QSize(1024, 512));
    REQUIRE(decoded.peak > 1.0f);
    auto document = kitchenExample();
    attachEnvironment(document, environment);
    document.renderSettings["hdri"]["rotation"] = 90;
    REQUIRE(document.version == 2);
    REQUIRE_NOTHROW(document.validate());
    RenderSnapshot captured(document, renderPreset("rapid"),
                            document.renderSettings.at("camera").get<std::string>());
    REQUIRE(QFile::remove(source));
    const auto filename = directory.filePath("portable.lmx");
    ProjectStore::save(filename, document, false);
    const auto reopened = ProjectStore::open(filename);
    REQUIRE(reopened.version == 2);
    REQUIRE(reopened.embeddedAssets == document.embeddedAssets);
    REQUIRE(reopened.renderSettings == document.renderSettings);
    REQUIRE(reopened.serialize(false).at("embeddedAssets").empty());
    Editor editor;
    editor.load(reopened);
    editor.apply("No change", [](Document &) {});
    REQUIRE(editor.history.count() == 0);
    editor.apply("Move furniture", [](Document &d) { d.entities.front().transform.x += 25; });
    REQUIRE(editor.history.count() == 1);
    REQUIRE(editor.document().embeddedAssets == reopened.embeddedAssets);
    editor.history.undo();
    REQUIRE(editor.document().serialize(false) == reopened.serialize(false));
    editor.history.redo();
    REQUIRE(editor.document().embeddedAssets == reopened.embeddedAssets);
    document.renderSettings["hdri"]["rotation"] = 180;
    document.embeddedAssets.clear();
    REQUIRE(captured.document().renderSettings.at("hdri").at("rotation") == 90);
    REQUIRE(captured.document().embeddedAssets.at(environment.hash) == environment.bytes);
    REQUIRE_THROWS(document.validate());
    auto bad = reopened;
    bad.version = 1;
    REQUIRE_THROWS(bad.validate());
    bad = reopened;
    bad.renderSettings["hdri"]["rotation"] = 999;
    REQUIRE_THROWS(bad.validate());
    REQUIRE_THROWS(inspectHighDynamicImage("invalid"));
    REQUIRE_THROWS(inspectHighDynamicImage(environment.bytes, true, 100));
    REQUIRE_THROWS(inspectHighDynamicImage(environment.bytes.left(70), true));
}

TEST_CASE("EXR originals preserve floating point while safe publication provides portable previews") {
    QTemporaryDir directory;
    const auto raw = directory.filePath("render.exr"), png = directory.filePath("render.png"),
               output = directory.filePath("result.exr");
    {
        std::array<std::array<float, 4>, 32> pixels;
        for (auto &pixel : pixels)
            pixel = {8.0f, 0.5f, 0.1f, 0.5f};
        Imf::Header header(8, 4);
        Imf::FrameBuffer frame;
        int i = 0;
        for (const auto *name : {"R", "G", "B", "A"}) {
            header.channels().insert(name, Imf::Channel(Imf::FLOAT));
            frame.insert(name, Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(&pixels.front()[i++]),
                                          sizeof(pixels.front()), 8 * sizeof(pixels.front())));
        }
        Imf::OutputFile image(raw.toUtf8().constData(), header);
        image.setFrameBuffer(frame);
        image.writePixels(4);
    }
    QFile bytes(raw);
    REQUIRE(bytes.open(QIODevice::ReadOnly));
    const auto original = bytes.readAll();
    bytes.close();
    const auto hdr = inspectHighDynamicImage(original, true);
    REQUIRE(hdr.size == QSize(8, 4));
    REQUIRE(hdr.peak == 8.0f);
    const auto environment = importEnvironment(raw);
    auto portable = kitchenExample();
    attachEnvironment(portable, environment);
    REQUIRE_NOTHROW(portable.validate());
    ProjectStore::save(directory.filePath("exr-environment.lmx"), portable, false);
    REQUIRE(ProjectStore::open(directory.filePath("exr-environment.lmx")).embeddedAssets ==
            portable.embeddedAssets);
    REQUIRE_THROWS(inspectHighDynamicImage(original.left(10), true));
    QImage preview(8, 4, QImage::Format_ARGB32);
    preview.fill(QColor(200, 100, 50, 128));
    REQUIRE(preview.save(png));
    std::atomic_bool cancelled{false};
    const auto first = publishRender(raw, png, output, {8, 4}, cancelled);
    REQUIRE(first.error.isEmpty());
    REQUIRE(QImage(first.preview).size() == QSize(8, 4));
    QFile saved(output);
    REQUIRE(saved.open(QIODevice::ReadOnly));
    REQUIRE(saved.readAll() == original);
    saved.close();
    const auto failed = publishRender(raw, png, output, {9, 4}, cancelled);
    REQUIRE_FALSE(failed.error.isEmpty());
    REQUIRE(QFileInfo::exists(first.preview));
    cancelled = true;
    REQUIRE_FALSE(publishRender(raw, png, output, {8, 4}, cancelled).error.isEmpty());
    REQUIRE(saved.open(QIODevice::ReadOnly));
    REQUIRE(saved.readAll() == original);
    saved.close();
    cancelled = false;
    preview.fill(Qt::red);
    REQUIRE(preview.save(png));
    const auto count = QDir(directory.path()).entryList({"preview-*.png"}, QDir::Files).size();
    REQUIRE_FALSE(
        publishRender(raw, png, directory.filePath("missing/result.exr"), {8, 4}, cancelled).error.isEmpty());
    REQUIRE(QDir(directory.path()).entryList({"preview-*.png"}, QDir::Files).size() == count);
    REQUIRE_FALSE(
        publishRender(raw, directory.filePath("missing.png"), output, {8, 4}, cancelled).error.isEmpty());
}

TEST_CASE("Large HDRI has its own budget without losing an apartment's existing assets") {
    QTemporaryDir directory;
    QFile file(directory.filePath("large.hdr"));
    REQUIRE(file.open(QIODevice::WriteOnly));
    file.write("#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y 2560 +X 5120\n");
    const auto row = QByteArray::fromHex("80402082").repeated(5120);
    qint64 written = 0;
    for (int y = 0; y < 2560; ++y)
        written += file.write(row);
    REQUIRE(written == 5120LL * 2560 * 4);
    file.close();
    const auto environment = importEnvironment(file.fileName());
    REQUIRE(environment.bytes.size() > 16 * 1024 * 1024);
    auto apartment = ProjectStore::open(QStringLiteral(LMX_SOURCE_DIR "/examples/apartamento-moderno.lmx"));
    const auto existing = apartment.embeddedAssets;
    attachEnvironment(apartment, environment);
    REQUIRE_NOTHROW(apartment.validate());
    REQUIRE(apartment.embeddedAssets.size() == existing.size() + 1);
    ProjectStore::save(directory.filePath("large-portable.lmx"), apartment, false);
    const auto portable = ProjectStore::open(directory.filePath("large-portable.lmx"));
    REQUIRE(portable.embeddedAssets == apartment.embeddedAssets);
    for (const auto &[hash, bytes] : existing)
        REQUIRE(portable.embeddedAssets.at(hash) == bytes);
    REQUIRE(inspectHighDynamicImage(portable.embeddedAssets.at(environment.hash), true).size ==
            QSize(5120, 2560));
}
TEST_CASE("Queued snapshot retains geometry and material edits independently of editor document") {
    auto document = kitchenExample();
    auto camera = document.renderSettings.at("camera").get<std::string>();
    RenderSnapshot frozen(document, renderPreset("final"), camera);
    const auto captured = frozen.document().serialize();
    document.entities.front().transform.x += 180;
    document.materials.front()["roughness"] = 0.13;
    document.renderSettings["exposure"] = 2.0;
    REQUIRE(frozen.document().serialize() == captured);
    REQUIRE(frozen.document().serialize() != document.serialize());
    REQUIRE(frozen.options().at("samples") == 512);
    REQUIRE_THROWS(RenderSnapshot(document, renderPreset("normal"), uuid()));
    document.at(camera).visible = false;
    REQUIRE_THROWS(RenderSnapshot(document, renderPreset("normal"), camera));
}
TEST_CASE("Render history recovers incomplete jobs without automatically launching processes") {
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto id = QString::fromStdString(uuid());
    REQUIRE(QDir().mkpath(directory.filePath(id + "/scene")));
    auto document = kitchenExample();
    ProjectStore::save(directory.filePath(id + "/scene/snapshot.lmx"), document, false);
    Json record{{"version", 1},
                {"id", id.toStdString()},
                {"project", document.id},
                {"projectName", document.name},
                {"camera", document.renderSettings.at("camera")},
                {"cameraName", "Cozinha"},
                {"created", "2026-10-02T10:00:00.000Z"},
                {"state", "Rendering"},
                {"options", renderPreset("normal")},
                {"blender", "missing"}};
    QFile file(directory.filePath(id + "/job.json"));
    REQUIRE(file.open(QIODevice::WriteOnly));
    file.write(QByteArray::fromStdString(record.dump()));
    file.close();
    const auto corrupt = QString::fromStdString(uuid());
    REQUIRE(QDir().mkpath(directory.filePath(corrupt)));
    QFile broken(directory.filePath(corrupt + "/job.json"));
    REQUIRE(broken.open(QIODevice::WriteOnly));
    broken.write("{ invalid json");
    broken.close();
    RenderQueue recovered(directory.path());
    auto entries = recovered.entries(document.id);
    REQUIRE(entries.size() == 1);
    REQUIRE(entries.front().at("state") == "Interrupted");
    REQUIRE_FALSE(recovered.busy());
    REQUIRE(QFileInfo::exists(broken.fileName()));
    REQUIRE_THROWS(recovered.retry(id));
    recovered.remove(id);
    REQUIRE(recovered.entries().empty());
    REQUIRE(QFileInfo::exists(recovered.snapshotPath(id)));
}
