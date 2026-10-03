#include "camera_acceptance.h"
#include "document/room_outline.h"
#include "main_window.h"
#include "persistence/project_store.h"
#include "rendering/camera_model.h"
#include "resource_paths.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QImage>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSpinBox>
#include <QTest>
#include <QTimer>
#include <QWheelEvent>
#include <iostream>
namespace lmx {
namespace {
void ensure(bool result, const char *message) {
    if (!result)
        throw std::runtime_error(message);
}
} // namespace
void startCameraAcceptance(MainWindow &window, QApplication &app, const QString &output,
                           const QString &preferred) {
    const auto directory = QDir(output).absolutePath();
    QDir().mkpath(directory);
    struct State {
        QStringList jobs;
        int ticks = 0;
        bool initialized = false, projectionFinished = false, projectionPassed = false;
    };
    auto state = std::make_shared<State>();
    auto *timer = new QTimer(&window);
    timer->setInterval(100);
    QObject::connect(timer, &QTimer::timeout, &window, [&window, &app, directory, preferred, state, timer] {
        timer->stop();
        try {
            ensure(++state->ticks < 1200, "Camera acceptance timeout");
            auto &editor = window.editor();
            auto *cad = window.cad();
            if (!state->initialized) {
                state->initialized = true;
                Document document;
                addPolygonRoom(document, {{0, 0}, {6000, 0}, {6000, 6000}, {0, 6000}});
                std::vector<std::array<double, 3>> points;
                for (int column = 0; column < 3; ++column)
                    for (int row = 0; row < 3; ++row) {
                        auto object = entity("GeometryObject", "Referência de enquadramento");
                        object.width = object.depth = 250;
                        object.height = 600 + row * 400;
                        object.transform = {1700.0 + column * 1200, 1000.0 + row * 900, 0, 0, false};
                        points.push_back({object.transform.x + 125, object.transform.y + 125, object.height});
                        document.entities.push_back(object);
                    }
                auto camera = entity("Camera", "Foto com quadro calibrado");
                camera.transform = {3000, 5200, 1600, 0, false};
                camera.parameters = {{"target", {3000, 1300, 1100}}, {"lens", 20}, {"fstop", 8}};
                document.entities.push_back(camera);
                document.renderSettings["camera"] = camera.id;
                editor.load(document);
                window.resize(1280, 850);
                QTest::qWait(100);
                Json cases = Json::array();
                for (int index = 0; index < 4; ++index) {
                    const QSize image = index == 0   ? QSize(320, 180)
                                        : index == 1 ? QSize(180, 320)
                                        : index == 2 ? QSize(240, 240)
                                                     : QSize(320, 240);
                    auto doc = document;
                    auto &c = doc.at(camera.id);
                    if (index) {
                        c.parameters["sensorWidth"] = index == 2 ? 24 : 36;
                        c.parameters["lens"] = index == 2 ? 35 : 20;
                        c.parameters["shiftX"] = .12;
                        c.parameters["shiftY"] = -.06;
                        c.parameters["clipNear"] = 25;
                        c.parameters["clipFar"] = 80000;
                        c.parameters["up"] = {.12, 0, 1};
                    }
                    editor.load(doc);
                    cad->showCameraFrame(c, image);
                    QTest::qWait(50);
                    Json evidence = Json::array();
                    const auto model = cameraModel(c);
                    const auto frame = cad->photographFrame();
                    for (const auto &world : points) {
                        const auto expected = cameraPoint(model, image, world);
                        const auto pixel = cad->cameraScreenPoint(world[0], world[1], world[2]);
                        const double dx = pixel.x() - frame.left() - expected[0] * frame.width(),
                                     dy = pixel.y() - frame.top() - (1 - expected[1]) * frame.height();
                        // Compare continuous native projection, before V3d_View::Convert
                        // truncates pixels and subtracts one from its top-left Y index.
                        ensure(std::hypot(dx, dy) <= 1e-6,
                               "Native frame differs from persistent camera projection");
                        evidence.push_back({{"world", world},
                                            {"normalized", expected},
                                            {"nativePixelError", std::hypot(dx, dy)}});
                    }
                    cases.push_back(
                        {{"name", "frame-" + std::to_string(index)},
                         {"image", {image.width(), image.height()}},
                         {"camera",
                          {{"id", c.id},
                           {"name", c.name},
                           {"position", {c.transform.x / 1000, c.transform.y / 1000, c.transform.z / 1000}},
                           {"parameters", c.parameters}}},
                         {"points", evidence}});
                    ensure(window.screen()
                               ->grabWindow(window.winId())
                               .save(directory + QString("/frame-%1.png").arg(index)),
                           "Camera screenshot unavailable");
                    auto options = renderPreset("rapid");
                    options["width"] = image.width();
                    options["height"] = image.height();
                    options["samples"] = 8;
                    options["device"] = "CPU";
                    const auto blender = BlenderBridge::findExecutable(preferred);
                    ensure(!blender.isEmpty(), "Blender required for camera parity");
                    state->jobs << window.renderQueue().enqueue(RenderSnapshot(doc, options, c.id), blender,
                                                                resourcePath("scripts/cycles_render.py"));
                }
                QFile proof(directory + "/projection.json");
                ensure(proof.open(QIODevice::WriteOnly), "Camera evidence file unavailable");
                proof.write(QByteArray::fromStdString(cases.dump(2)));
                proof.close();
                auto *process = new QProcess(&window);
                process->setProcessChannelMode(QProcess::MergedChannels);
                QObject::connect(process, &QProcess::errorOccurred, &window,
                                 [state](QProcess::ProcessError error) {
                                     if (error == QProcess::FailedToStart)
                                         state->projectionFinished = true;
                                 });
                QObject::connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), &window,
                                 [state, process](int code, QProcess::ExitStatus status) {
                                     state->projectionFinished = true;
                                     state->projectionPassed = code == 0 && status == QProcess::NormalExit;
                                     std::cout << process->readAllStandardOutput().constData() << std::flush;
                                     process->deleteLater();
                                 });
                process->start(
                    BlenderBridge::findExecutable(preferred),
                    {"--background", "--factory-startup", "--disable-autoexec", "--python-exit-code", "1",
                     "--python", resourcePath("scripts/verify-camera-projection.py"), "--", "--input",
                     directory + "/projection.json", "--output", directory + "/blender-projection.json"});
                // Actual mouse controls alter the pending frame, then the native Save button commits one
                // undoable change.
                window.resize(900, 700);
                auto *dock = window.findChild<QDockWidget *>("renderDock");
                dock->show();
                dock->raise();
                auto *quality = window.findChild<QComboBox *>("renderQuality");
                quality->setCurrentIndex(quality->findData("custom"));
                window.findChild<QSpinBox *>("render_width")->setValue(320);
                window.findChild<QSpinBox *>("render_height")->setValue(240);
                QMetaObject::invokeMethod(window.findChild<QSpinBox *>("render_height"), "editingFinished",
                                          Qt::DirectConnection);
                auto *scroll = window.findChild<QScrollArea *>("renderScroll");
                auto *show = window.findChild<QPushButton *>("showCameraFrame");
                scroll->ensureWidgetVisible(show);
                QTest::qWait(100);
                ensure(show->isVisible() && show->width() <= dock->width(),
                       "Camera action clipped in compact UI");
                QTest::mouseClick(show, Qt::LeftButton);
                ensure(cad->hasCameraFrame(), "Native camera frame button failed");
                const auto before = editor.document().serialize();
                const auto center = cad->rect().center();
                QWheelEvent wheel(center, cad->mapToGlobal(center), QPoint{}, QPoint(0, 120), Qt::NoButton,
                                  Qt::NoModifier, Qt::NoScrollPhase, false);
                QApplication::sendEvent(cad, &wheel);
                ensure(cad->cameraFrameChanged(), "Camera wheel did not change framing");
                QTest::mousePress(cad, Qt::MiddleButton, Qt::NoModifier, center);
                QTest::mouseMove(cad, center + QPoint(25, 15), 40);
                QTest::mouseRelease(cad, Qt::MiddleButton, Qt::NoModifier, center + QPoint(25, 15));
                QTest::mousePress(cad, Qt::RightButton, Qt::NoModifier, center);
                QTest::mouseMove(cad, center + QPoint(20, 8), 40);
                QTest::mouseRelease(cad, Qt::RightButton, Qt::NoModifier, center + QPoint(20, 8));
                auto *save = window.findChild<QPushButton *>("saveCameraFrame");
                scroll->ensureWidgetVisible(save);
                ensure(save->isVisible() && save->isEnabled(), "Native framing save action unavailable");
                QTest::mouseClick(save, Qt::LeftButton);
                ensure(editor.document().serialize() != before && !cad->cameraFrameChanged(),
                       "Native framing save failed");
                const auto saved = editor.document().serialize();
                editor.history.undo();
                ensure(editor.document().serialize() == before,
                       "Camera framing undo did not restore exact scene");
                editor.history.redo();
                ensure(editor.document().serialize() == saved, "Camera framing redo failed");
                ProjectStore::save(directory + "/camera-portable.lmx", editor.document(), false);
                ensure(ProjectStore::open(directory + "/camera-portable.lmx").serialize() == saved,
                       "Camera up/shift/sensor lost on save");
                ensure(window.screen()->grabWindow(window.winId()).save(directory + "/compact.png"),
                       "Compact camera screenshot unavailable");
                QApplication::sendEvent(cad, &wheel);
                editor.apply("Ajustar lente pelas propriedades",
                             [&](Document &d) { d.at(camera.id).parameters["lens"] = 27; });
                ensure(!cad->cameraFrameChanged() &&
                           cad->framedCamera()->parameters == editor.document().at(camera.id).parameters,
                       "Property edit left an obsolete pending camera frame");
                editor.history.undo();
                QApplication::sendEvent(cad, &wheel);
                const auto pending = *cad->framedCamera();
                ensure(cad->cameraFrameChanged(), "Second framing adjustment was not pending");
                window.findChild<QSpinBox *>("render_samples")->setValue(8);
                auto *device = window.findChild<QComboBox *>("renderDevice");
                device->setCurrentIndex(device->findData("CPU"));
                window.findChild<QLineEdit *>("blenderPath")
                    ->setText(BlenderBridge::findExecutable(preferred));
                auto *start = window.findChild<QPushButton *>("startRender");
                scroll->ensureWidgetVisible(start);
                QTest::mouseClick(start, Qt::LeftButton);
                ensure(editor.document().at(camera.id).parameters == pending.parameters &&
                           !cad->cameraFrameChanged(),
                       "Create image did not save the adjusted camera before taking its snapshot");
                const auto queued = window.renderQueue().entries();
                ensure(queued.size() == static_cast<std::size_t>(state->jobs.size()) + 1,
                       "Native render action did not enqueue a photo");
                state->jobs << QString::fromStdString(queued.back().at("id").get<std::string>());
            } else if (state->projectionFinished && !window.renderQueue().busy()) {
                ensure(state->projectionPassed, "Actual Blender camera projection differs from the editor");
                const auto entries = window.renderQueue().entries();
                Json rendered = Json::array();
                for (const auto &id : state->jobs) {
                    const auto entry = std::find_if(entries.begin(), entries.end(), [&](const auto &e) {
                        return e.at("id") == id.toStdString();
                    });
                    ensure(entry != entries.end() && entry->at("state") == "Completed",
                           "Camera parity render failed");
                    const QImage photo(window.renderQueue().imagePath(id));
                    ensure(!photo.isNull(), "Camera render image missing");
                    const auto &options = entry->at("options");
                    ensure(photo.size() ==
                               QSize(options.at("width").get<int>(), options.at("height").get<int>()),
                           "Camera render did not preserve the requested photo dimensions");
                    int minimum = 255, maximum = 0;
                    for (int y = 0; y < photo.height(); ++y)
                        for (int x = 0; x < photo.width(); ++x) {
                            const auto gray = qGray(photo.pixel(x, y));
                            minimum = std::min(minimum, gray);
                            maximum = std::max(maximum, gray);
                        }
                    ensure(maximum - minimum >= 20, "Camera render lacks visible scene variation");
                    rendered.push_back(
                        {{"id", id.toStdString()}, {"options", options}, {"grayRange", maximum - minimum}});
                    QFile::copy(window.renderQueue().imagePath(id), directory + "/render-" + id + ".png");
                }
                QFile report(directory + "/acceptance.json");
                ensure(report.open(QIODevice::WriteOnly), "Camera acceptance report unavailable");
                report.write(
                    QByteArray::fromStdString(Json({{"passed", true}, {"renders", rendered}}).dump(2)));
                std::cout << "CAMERA_ACCEPTANCE_PASS: 4 actual native perspective frames, Blender projection "
                             "parity, compact UI, mouse controls, history, portable camera and 5 real Cycles "
                             "CPU images\n";
                app.exit(0);
                return;
            }
            timer->start();
        } catch (const std::exception &error) {
            std::cerr << "CAMERA_ACCEPTANCE_FAIL: " << error.what() << '\n';
            window.screen()->grabWindow(window.winId()).save(directory + "/failed.png");
            app.exit(1);
        }
    });
    timer->start();
}
} // namespace lmx
