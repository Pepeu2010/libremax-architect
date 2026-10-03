#include "commands/editor.h"
#include "document/examples.h"
#include "persistence/project_store.h"
#include "rendering/environment_map.h"
#include "rendering/high_dynamic_image.h"
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
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif
using namespace lmx;
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
