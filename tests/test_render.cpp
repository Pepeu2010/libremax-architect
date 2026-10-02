#include "document/examples.h"
#include "persistence/project_store.h"
#include "rendering/render_queue.h"
#include "rendering/render_snapshot.h"
#include <QDir>
#include <QFile>
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
