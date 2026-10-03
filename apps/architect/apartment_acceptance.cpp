#include "apartment_acceptance.h"
#include "commands/arrangement.h"
#include "document/room_outline.h"
#include "library/model.h"
#include "library/model_pack.h"
#include "main_window.h"
#include "persistence/project_store.h"
#include "rendering/room_look.h"
#include "resource_paths.h"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
#include <iostream>
namespace lmx {
namespace {
void ensure(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
void startApartmentAcceptance(MainWindow &window, QApplication &app, const QString &output,
                              const QString &preferredBlender) {
    auto directory = QDir(output).absolutePath();
    QDir().mkpath(directory);
    const auto blender = BlenderBridge::findExecutable(preferredBlender);
    struct State {
        int phase = 0;
        std::string room, first, second;
        Json beforeFailedImport;
        QStringList jobs;
        QElapsedTimer elapsed;
        bool importDialog = false;
    };
    auto state = std::make_shared<State>();
    state->elapsed.start();
    auto *poll = new QTimer(&window);
    poll->setInterval(50);
    QObject::connect(poll, &QTimer::timeout, &window, [&window, &app, directory, blender, state, poll] {
        poll->stop();
        try {
            ensure(state->elapsed.elapsed() < 600000, "Apartment tools acceptance timeout");
            auto *cad = window.cad();
            auto &editor = window.editor();
            if (state->phase == 0) {
                Document reference;
                addRectangularRoom(reference, 6000, 6000, 2700, 120);
                editor.load(reference);
                cad->setTop(true);
                cad->frame();
                QTest::qWait(150);
                editor.load(Document{});
                cad->setTool("room");
                for (auto p : Outline{{0, 0}, {5000, 0}, {5000, 2000}, {2000, 2000}, {2000, 5000}, {0, 5000}})
                    QTest::mouseClick(cad, Qt::LeftButton, Qt::NoModifier, cad->project(p[0], p[1]));
                QTest::mouseClick(cad, Qt::RightButton, Qt::NoModifier, cad->project(0, 5000));
                const auto &doc = editor.document();
                auto room = std::find_if(doc.entities.begin(), doc.entities.end(),
                                         [](const auto &e) { return e.type == "Room"; });
                ensure(room != doc.entities.end() && roomOutline(*room).size() == 6,
                       "Native contour drawing did not create L room");
                state->room = room->id;
                window.selectIds({QString::fromStdString(state->room)});
                std::string error;
                QTimer::singleShot(50, &window, [&window, &error] {
                    try {
                        auto *dialog = window.findChild<QDialog *>("roomOutlineDialog");
                        ensure(dialog, "Room editing dialog missing");
                        auto *points = dialog->findChild<QTableWidget *>("roomOutlinePoints");
                        points->item(1, 0)->setText("5.5");
                        points->item(2, 0)->setText("5.5");
                        dialog->findChild<QDoubleSpinBox *>("roomOutlineHeight")->setValue(2.8);
                        QTest::mouseClick(
                            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save),
                            Qt::LeftButton);
                    } catch (const std::exception &e) {
                        error = e.what();
                        if (auto *dialog = window.findChild<QDialog *>("roomOutlineDialog"))
                            dialog->reject();
                    }
                });
                ensure(QMetaObject::invokeMethod(&window, "editRoomOutline", Qt::DirectConnection),
                       "Room edit action unavailable");
                ensure(error.empty(), error.c_str());
                ensure(editor.document().at(state->room).width == 5500,
                       "Edited contour did not update width");
                for (int i = 0; i < 2; ++i) {
                    auto object = entity("GeometryObject", "Banco de teste");
                    object.width = 500;
                    object.depth = 500;
                    object.height = 600;
                    object.transform = {1000.0 + i * 1200, 600.0 + i * 250, 0, 0, false};
                    editor.apply("Fixture furniture", [&](Document &d) { d.entities.push_back(object); });
                    (i == 0 ? state->first : state->second) = object.id;
                }
                window.selectIds(
                    {QString::fromStdString(state->first), QString::fromStdString(state->second)});
                ensure(QMetaObject::invokeMethod(&window, "arrangeSelection", Qt::DirectConnection,
                                                 Q_ARG(QString, "group")),
                       "Group action unavailable");
                auto group = editor.document().at(state->first).parent;
                ensure(!group.empty(), "Group was not created");
                cad->frame();
                QTest::qWait(100);
                auto start = cad->project(1250, 850, 600), end = cad->project(1450, 1050, 600);
                QTest::mousePress(cad, Qt::LeftButton, Qt::NoModifier, start);
                QTest::mouseMove(cad, end, 150);
                QTest::mouseRelease(cad, Qt::LeftButton, Qt::NoModifier, end);
                ensure(std::abs(editor.document().at(state->first).transform.x - 1000) > 50,
                       "Native grouped furniture drag did not move");
                const auto &a = editor.document().at(state->first), &b = editor.document().at(state->second);
                ensure(std::abs(b.transform.x - a.transform.x - 1200) < 1 &&
                           std::abs(b.transform.y - a.transform.y - 250) < 1,
                       "Group drag changed relative positions");
                editor.history.undo();
                ensure(editor.document().at(state->first).transform.x == 1000, "Group drag undo failed");
                editor.history.redo();
                window.selectIds({QString::fromStdString(group)});
                ensure(QMetaObject::invokeMethod(&window, "arrangeSelection", Qt::DirectConnection,
                                                 Q_ARG(QString, "back")),
                       "Align action unavailable");
                ensure(editor.document().at(state->first).transform.y ==
                           editor.document().at(state->second).transform.y,
                       "Native alignment failed");
                const auto arranged = editor.document().serialize();
                auto *rotate = window.findChild<QAction *>("rotateFurniture");
                auto *mirror = window.findChild<QAction *>("mirrorSelection");
                ensure(rotate && mirror, "Furniture transformation actions missing");
                rotate->trigger();
                ensure(editor.document().at(state->first).transform.yaw == 90 &&
                           editor.document().at(state->second).transform.yaw == 90,
                       "Native rotation ignored the furniture set");
                mirror->trigger();
                ensure(editor.document().at(state->first).transform.mirrored &&
                           editor.document().at(state->second).transform.mirrored,
                       "Native reflection ignored group members");
                editor.history.undo();
                editor.history.undo();
                ensure(editor.document().serialize() == arranged,
                       "Set transform undo changed the arrangement");
                window.resize(900, 700);
                cad->setTop(true);
                cad->frame();
                QTest::qWait(150);
                ensure(window.screen()->grabWindow(window.winId()).save(directory + "/room-tools-900.png"),
                       "Compact tool screenshot failed");
                ProjectStore::save(directory + "/contorno-e-conjunto.lmx", editor.document(), false);
                ensure(ProjectStore::open(directory + "/contorno-e-conjunto.lmx").serialize() ==
                           editor.document().serialize(),
                       "New apartment workflow is not portable");
                std::cout << "APARTMENT_TOOLS_PASS: native free contour, L floor, corner editing, "
                             "dimensions, grouped mouse drag, alignment, rotation, reflection, undo and "
                             "portable project\n";
                QFile fixture(directory + "/imported-chair.obj");
                ensure(fixture.open(QIODevice::WriteOnly), "Import fixture unavailable");
                fixture.write("v 0 0 0\nv .5 0 0\nv .5 .5 0\nv 0 .5 0\nv 0 0 .7\nv .5 0 .7\nv .5 .5 .7\nv 0 "
                              ".5 .7\nf 1 4 3 2\nf 5 6 7 8\nf 1 2 6 5\nf 2 3 7 6\nf 3 4 8 7\nf 4 1 5 8\n");
                fixture.close();
                if (!blender.isEmpty())
                    window.findChild<QLineEdit *>("blenderPath")->setText(blender);
                ensure(!blender.isEmpty(), "Bundled or selected Blender is required for actual model import");
                const auto file = directory + "/imported-chair.obj";
                ensure(QMetaObject::invokeMethod(&window, "importModel", Qt::DirectConnection,
                                                 Q_ARG(QString, file)),
                       "Native model import unavailable");
                state->phase = 1;
            } else if (state->phase == 1) {
                if (auto *dialog = window.findChild<QDialog *>("importModelDetails");
                    dialog && dialog->isVisible() && !state->importDialog) {
                    state->importDialog = true;
                    dialog->findChild<QLineEdit *>("importModelName")->setText("Modelo importado de teste");
                    dialog->findChild<QDoubleSpinBox *>("importModelWidth")->setValue(50);
                    dialog->findChild<QDoubleSpinBox *>("importModelDepth")->setValue(50);
                    dialog->findChild<QDoubleSpinBox *>("importModelHeight")->setValue(70);
                    QTest::mouseClick(dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok),
                                      Qt::LeftButton);
                }
                auto *assets = window.findChild<QListWidget *>("assetList");
                if (state->importDialog && assets && assets->count() == 1 &&
                    assets->item(0)->data(Qt::UserRole).toString().startsWith("import-")) {
                    const auto before = editor.document().entities.size();
                    auto pixel = cad->project(1000, 3500);
                    QTest::mouseMove(cad, pixel);
                    QTest::mouseClick(cad, Qt::LeftButton, Qt::NoModifier, pixel);
                    ensure(editor.document().entities.size() == before + 1,
                           "Imported model could not be placed");
                    ensure(editor.document().entities.back().metadata.at("license") == "user-provided",
                           "Imported model was incorrectly marked as CC0");
                    ProjectStore::save(directory + "/importado.lmx", editor.document(), false);
                    ensure(ProjectStore::open(directory + "/importado.lmx").serialize() ==
                               editor.document().serialize(),
                           "Imported model was not embedded");
                    std::cout << "MODEL_IMPORT_PASS: actual OBJ via Blender, native size and placement "
                                 "dialog, catalog insertion, original licensing and portable project\n";
                    state->phase = 2;
                }
            } else if (state->phase == 2) {
                auto pack = readModelPack(resourcePath("collections/Apartamento-contemporaneo-1.lmaxpack"));
                ensure(pack.assets.size() == 28, "Offline model collection missing");
                const auto root = directory + "/pack-library";
                Library library(root + "/catalog.db", {}, root + "/models");
                installModelPack(library, root + "/models", pack);
                ensure(library.search().size() == 28, "Collection installation failed");
                // Furnish the L room with actual shipped meshes and use a window for daylight.
                auto doc = editor.document();
                std::erase_if(doc.entities, [](const auto &e) { return movable(e) || e.type == "Group"; });
                for (const auto &[needle, x, y] :
                     std::vector<std::tuple<QString, double, double>>{{"home-desk", 1000, 500},
                                                                      {"home-chair", 1000, 1500},
                                                                      {"home-wall-shelf", 1000, 0}}) {
                    auto found = std::find_if(pack.assets.begin(), pack.assets.end(),
                                              [&](const auto &a) { return a.id.endsWith(needle); });
                    ensure(found != pack.assets.end(), "New furnishing missing");
                    Library::attachModel(doc, *found);
                    auto placement = placeObject(doc, Library::instantiate(*found, 0, 0), x, y, true);
                    ensure(placement.allowed, "New furniture does not fit");
                    doc.entities.push_back(placement.object);
                }
                auto windowWall = std::find_if(doc.entities.begin(), doc.entities.end(), [](const auto &e) {
                    return e.type == "Wall" && e.width > 4000;
                });
                auto opening = entity("Window", "Janela");
                opening.parent = windowWall->id;
                opening.width = 1600;
                opening.height = 1400;
                opening.parameters = {{"offset", 2800}, {"sill", 900}};
                doc.entities.push_back(opening);
                auto camera = entity("Camera", "Foto do cômodo em L");
                camera.transform = {1650, 4400, 1500, 0, false};
                camera.parameters = {{"target", {1300, 1200, 1000}}, {"lens", 20}, {"fstop", 8}};
                doc.entities.push_back(camera);
                doc.renderSettings["camera"] = camera.id;
                auto options = renderPreset("rapid");
                options["width"] = 320;
                options["height"] = 180;
                options["samples"] = 8;
                options["device"] = "CPU";
                for (auto style : {"natural", "bright", "evening"}) {
                    applyRoomLook(doc, state->room, style);
                    doc.validate();
                    state->jobs << window.renderQueue().enqueue(RenderSnapshot(doc, options, camera.id),
                                                                blender,
                                                                resourcePath("scripts/cycles_render.py"));
                    ProjectStore::save(directory + "/foto-" + style + ".lmx", doc, false);
                }
                editor.load(doc);
                cad->setTop(false);
                cad->frame();
                QTest::qWait(200);
                ensure(
                    window.screen()->grabWindow(window.winId()).save(directory + "/apartment-tools-3d.png"),
                    "New furnished room capture failed");
                std::cout << "MODEL_PACK_PASS: 28 real models with editing meshes, offline installation and "
                             "real furnished room\n";
                state->phase = 3;
            } else if (state->phase == 3) {
                auto &queue = window.renderQueue();
                if (queue.busy()) {
                    poll->start();
                    return;
                }
                const auto entries = queue.entries();
                for (const auto &id : state->jobs) {
                    auto found = std::find_if(entries.begin(), entries.end(), [&](const auto &entry) {
                        return entry.at("id") == id.toStdString();
                    });
                    ensure(found != entries.end() && found->at("state") == "Completed",
                           "Room look render did not complete");
                    ensure(QImage(queue.imagePath(id)).size() == QSize(320, 180), "Room look image invalid");
                    QFile::copy(queue.imagePath(id), directory + "/look-" + id + ".png");
                }
                ensure(QImage(queue.imagePath(state->jobs[0])) != QImage(queue.imagePath(state->jobs[2])),
                       "Lighting styles produced identical images");
                QFile report(directory + "/report.json");
                const auto pack =
                    readModelPack(resourcePath("collections/Apartamento-contemporaneo-1.lmaxpack"));
                const auto asset = std::find_if(pack.assets.begin(), pack.assets.end(),
                                                [](const auto &a) { return a.id.endsWith("home-curtain"); });
                ensure(asset != pack.assets.end() && !asset->editModel.isEmpty(),
                       "Benchmark editing mesh missing");
                Document benchmark;
                Library::attachModel(benchmark, *asset);
                std::string moved;
                for (int i = 0; i < 48; ++i) {
                    auto e = Library::instantiate(*asset, (i % 8) * 2800, (i / 8) * 800);
                    if (i == 0)
                        moved = e.id;
                    benchmark.entities.push_back(e);
                }
                cad->setTop(false);
                cad->setPerformanceMode(0);
                QElapsedTimer cold;
                cold.start();
                editor.load(benchmark);
                cad->frame();
                const auto coldMs = cold.elapsed();
                std::vector<double> latencies;
                for (int i = 0; i < 20; ++i) {
                    QElapsedTimer edit;
                    edit.start();
                    editor.apply("Benchmark move", [&](Document &d) { d.at(moved).transform.x = i * 10; });
                    latencies.push_back(edit.nsecsElapsed() / 1e6);
                }
                std::sort(latencies.begin(), latencies.end());
                auto full = readModel(asset->model), low = readModel(asset->editModel);
                size_t highCount = 0, lowCount = 0;
                for (auto p : full.at("parts"))
                    highCount += p.at("triangles").size();
                for (auto p : low.at("parts"))
                    lowCount += p.at("triangles").size();
                const Json measurements = {{"objects", 48},
                                           {"fullTriangles", highCount * 48},
                                           {"editorTriangles", lowCount * 48},
                                           {"coldDisplayMs", coldMs},
                                           {"editP50Ms", latencies[10]},
                                           {"editP95Ms", latencies[18]},
                                           {"actualViewportDevice", cad->deviceDiagnostics()}};
                ensure(report.open(QIODevice::WriteOnly), "Acceptance report unavailable");
                report.write(QByteArray::fromStdString(Json{
                    {"elapsedMs", state->elapsed.elapsed()},
                    {"renderJobs", state->jobs.size()},
                    {"models", 28},
                    {"benchmark", measurements},
                    {"minimumHardwareVerified", false}}.dump(2)));
                std::cout << "EDITOR_BENCHMARK: " << measurements.dump() << '\n';
                std::cout << "ROOM_LOOK_RENDER_PASS: 3 distinct real Cycles CPU styles, generated lights, "
                             "actual apartment meshes and intact queue snapshots\n";
                state->beforeFailedImport = editor.document().serialize();
                QFile invalid(directory + "/invalid-model.obj");
                ensure(invalid.open(QIODevice::WriteOnly), "Invalid import fixture unavailable");
                invalid.write("f 1 2 3\n");
                invalid.close();
                const auto file = directory + "/invalid-model.obj";
                ensure(QMetaObject::invokeMethod(&window, "importModel", Qt::DirectConnection,
                                                 Q_ARG(QString, file)),
                       "Failed model import action unavailable");
                state->phase = 4;
            } else {
                if (auto *error = window.findChild<QMessageBox *>("modelImportError");
                    error && error->isVisible()) {
                    ensure(error->text().size() < 200 && !error->text().contains("Traceback") &&
                               !error->detailedText().isEmpty(),
                           "Import error exposes a raw log or loses diagnostic details");
                    ensure(editor.document().serialize() == state->beforeFailedImport,
                           "Failed model import changed the project");
                    error->accept();
                    std::cout << "MODEL_IMPORT_ERROR_PASS: actual malformed OBJ, simple warning, separate "
                                 "diagnostics and unchanged project\n";
                    app.exit(0);
                    return;
                }
            }
            poll->start();
        } catch (const std::exception &error) {
            std::cerr << "APARTMENT_TOOLS_FAIL: " << error.what() << '\n';
            app.exit(1);
        }
    });
    QTimer::singleShot(1500, poll, [poll] { poll->start(); });
}
} // namespace lmx
