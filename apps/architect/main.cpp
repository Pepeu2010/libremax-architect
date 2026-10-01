#include "main_window.h"
#include "materials/texture.h"
#include "persistence/project_store.h"
#include "studio_theme.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    QApplication::setApplicationName("libremax");
    QApplication::setOrganizationName("LibreMax");
    QApplication::setApplicationVersion("0.4.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("LibreMax Architect — native interior design");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"ui-smoke", "Run native UI acceptance and save real screenshots", "directory"});
    parser.addOption(
        {"assembly-smoke", "Verify native apartment mounting and real furniture placement", "directory"});
    parser.addOption({"examples", "Generate valid sample .lmx projects", "directory"});
    parser.addOption({"render-smoke", "Run real Cycles through asynchronous QProcess pipeline", "directory"});
    parser.addOption({"render-size", "Acceptance render size WxH", "size", "320x180"});
    parser.addOption({"render-samples", "Acceptance Cycles samples", "samples", "16"});
    parser.addOption({"render-project", "Use a saved .lmx for render acceptance", "project"});
    parser.addOption({"preview-image", "Exercise native image viewer with an existing real render", "image"});
    parser.addOption({"recovery-smoke", "Kill a child process and verify recovery in a fresh process"});
    parser.addOption({"recovery-fixture", "Internal crash acceptance writer", "directory"});
    parser.addOption({"recovery-verify", "Internal crash acceptance reader", "directory"});
    parser.addOption({"blender", "Blender executable for render acceptance", "executable"});
    parser.addPositionalArgument("project", ".lmx project to open");
    parser.process(app);
    bool test = parser.isSet("assembly-smoke") || parser.isSet("ui-smoke") || parser.isSet("examples") ||
                parser.isSet("render-smoke") || parser.isSet("recovery-smoke") ||
                parser.isSet("recovery-fixture") || parser.isSet("recovery-verify");
    if (test)
        QStandardPaths::setTestModeEnabled(true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    auto logs = QStandardPaths::writableLocation(QStandardPaths::StateLocation) + "/logs";
#else
    auto logs = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/state/logs";
#endif
    QDir().mkpath(logs);
    auto logger =
        spdlog::rotating_logger_mt("libremax", (logs + "/libremax.log").toStdString(), 1024 * 1024, 3);
    spdlog::set_default_logger(logger);
    try {
        if (parser.isSet("recovery-smoke")) {
            QTemporaryDir directory;
            if (!directory.isValid())
                throw std::runtime_error("Unable to isolate crash acceptance");
            QProcess writer;
            writer.setProcessChannelMode(QProcess::MergedChannels);
            writer.start(QApplication::applicationFilePath(), {"--recovery-fixture", directory.path()});
            if (!writer.waitForStarted(10000))
                throw std::runtime_error("Crash fixture did not start");
            QByteArray output;
            for (int i = 0; i < 30 && !output.contains("RECOVERY_READY"); ++i) {
                writer.waitForReadyRead(1000);
                output += writer.readAll();
                if (writer.state() == QProcess::NotRunning)
                    break;
            }
            if (!output.contains("RECOVERY_READY")) {
                writer.kill();
                writer.waitForFinished(5000);
                std::cerr << output.toStdString();
                throw std::runtime_error("Crash fixture did not produce a durable autosave");
            }
            writer.kill();
            if (!writer.waitForFinished(5000) || writer.exitStatus() != QProcess::CrashExit)
                throw std::runtime_error("Fixture was not forcibly terminated");
            QProcess reader;
            reader.setProcessChannelMode(QProcess::MergedChannels);
            reader.start(QApplication::applicationFilePath(), {"--recovery-verify", directory.path()});
            if (!reader.waitForStarted(10000) || !reader.waitForFinished(30000)) {
                reader.kill();
                reader.waitForFinished(5000);
                throw std::runtime_error("Recovery acceptance timed out");
            }
            auto verification = reader.readAll();
            std::cout << verification.toStdString();
            if (reader.exitCode() != 0 || !verification.contains("RECOVERY_RESTART_PASS"))
                throw std::runtime_error("Recovery acceptance failed");
            std::cout << "CRASH_RECOVERY_PASS: killed writer, fresh process, native dialog, "
                         "UUID and 812.5 mm edit preserved\n";
            return 0;
        }
        if (parser.isSet("examples")) {
            QDir dir(parser.value("examples"));
            QDir().mkpath(dir.absolutePath());
            auto pack = lmx::readPbrMaterials(QStringLiteral(LMX_SOURCE_DIR) + "/starter-materials");
            auto kitchen = lmx::kitchenExample(), bedroom = lmx::bedroomExample();
            lmx::attachPbrMaterials(kitchen, pack);
            lmx::attachPbrMaterials(bedroom, pack);
            lmx::ProjectStore::save(dir.filePath("cozinha.lmx"), kitchen, false);
            lmx::ProjectStore::save(dir.filePath("dormitorio.lmx"), bedroom, false);
            return 0;
        }
        if (parser.isSet("render-smoke")) {
            const auto size = parser.value("render-size").split('x');
            if (size.size() != 2)
                throw std::runtime_error("Use render size WxH");
            const int renderWidth = size[0].toInt(), renderHeight = size[1].toInt(),
                      renderSamples = parser.value("render-samples").toInt();
            const auto directory = QDir(parser.value("render-smoke")).absolutePath();
            QDir().mkpath(directory);
            auto document = parser.isSet("render-project")
                                ? lmx::ProjectStore::open(parser.value("render-project"))
                                : lmx::kitchenExample();
            lmx::attachPbrMaterials(
                document, lmx::readPbrMaterials(QStringLiteral(LMX_SOURCE_DIR) + "/starter-materials"));
            lmx::ProjectStore::save(directory + "/render-project.lmx", document, false);
            lmx::RenderJob job;
            bool checkingFailure = false;
            QByteArray successfulHash;
            QObject::connect(&job, &lmx::RenderJob::log, &app,
                             [](const QString &text) { std::cout << text.toStdString() << std::flush; });
            QObject::connect(&job, &lmx::RenderJob::state, &app, [&](const QString &state) {
                std::cout << "RENDER_STATE: " << state.toStdString() << '\n';
                if (state == QString::fromUtf8("Falhou")) {
                    QFile previous(directory + "/cycles-kitchen.png");
                    if (checkingFailure && previous.open(QIODevice::ReadOnly) &&
                        QCryptographicHash::hash(previous.readAll(), QCryptographicHash::Sha256) ==
                            successfulHash) {
                        std::cout << "RENDER_FAILURE_PRESERVES_PREVIOUS_PASS: missing-camera Python "
                                     "exception is detected\n";
                        app.exit(0);
                    } else
                        app.exit(1);
                }
            });
            QObject::connect(&job, &lmx::RenderJob::completed, &app, [&](const QString &path) {
                QImage image(path);
                if (image.size() != QSize(renderWidth, renderHeight)) {
                    std::cerr << "Invalid render dimensions\n";
                    app.exit(1);
                    return;
                }
                std::cout << "RENDER_SMOKE_PASS: real Cycles, CPU, denoise, " << renderWidth << "x"
                          << renderHeight << " image\n";
                QFile file(path);
                if (!file.open(QIODevice::ReadOnly)) {
                    std::cerr << "Unable to verify the completed render\n";
                    app.exit(1);
                    return;
                }
                successfulHash = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
                checkingFailure = true;
                std::erase_if(document.entities, [](const auto &e) { return e.type == "Camera"; });
                document.renderSettings["camera"] = "";
                job.start(document, parser.value("blender"),
                          QStringLiteral(LMX_SOURCE_DIR) + "/scripts/cycles_render.py", path, renderWidth,
                          renderHeight, renderSamples, "CPU");
            });
            QTimer::singleShot(600000, &app, [&] {
                job.cancel();
                std::cerr << "Render acceptance timeout\n";
                app.exit(1);
            });
            job.start(document, parser.value("blender"),
                      QStringLiteral(LMX_SOURCE_DIR) + "/scripts/cycles_render.py",
                      directory + "/cycles-kitchen.png", renderWidth, renderHeight, renderSamples, "CPU");
            return app.exec();
        }
        lmx::applyStudioPalette();
        QString stylesheet = QApplication::applicationDirPath() + "/../share/libremax/resources/style.qss";
        if (!QFileInfo::exists(stylesheet))
            stylesheet = QStringLiteral(LMX_SOURCE_DIR) + "/resources/style.qss";
        QFile style(stylesheet);
        if (style.open(QIODevice::ReadOnly))
            app.setStyleSheet(QString::fromUtf8(style.readAll()));
        auto recoveryDirectory = parser.isSet("recovery-fixture")  ? parser.value("recovery-fixture")
                                 : parser.isSet("recovery-verify") ? parser.value("recovery-verify")
                                                                   : QString{};
        lmx::MainWindow window(test, recoveryDirectory);
        if (!parser.positionalArguments().isEmpty())
            window.loadProject(parser.positionalArguments().first());
        window.show();
        if (parser.isSet("assembly-smoke")) {
            const auto directory = QDir(parser.value("assembly-smoke")).absolutePath();
            QDir().mkpath(directory);
            QTimer::singleShot(1500, &window, [&window, directory, &app] {
                try {
                    auto ensure = [](bool value, const char *message) {
                        if (!value)
                            throw std::runtime_error(message);
                    };
                    auto *cad = window.cad();
                    auto *assets = window.findChild<QListWidget *>("assetList");
                    ensure(assets && assets->count() == 79, "Ready model catalog missing");
                    int ready = 0;
                    for (int attempt = 0; attempt < 300; ++attempt) {
                        ready = 0;
                        for (int i = 0; i < assets->count(); ++i)
                            ready += assets->item(i)->data(Qt::UserRole + 2).toBool();
                        if (ready == assets->count())
                            break;
                        QTest::qWait(100);
                    }
                    ensure(ready == 79, "Native thumbnails missing for real models");
                    lmx::Document d;
                    lmx::addRectangularRoom(d, 4000, 3000, 2700, 120);
                    window.editor().load(d);
                    cad->setTop(true);
                    cad->frame();
                    QTest::qWait(100);
                    auto drop = [&](const QString &id, double x, double y, bool expected, double z = 0) {
                        QMimeData mime;
                        mime.setData("application/x-libremax-asset", id.toUtf8());
                        auto point = cad->project(x, y, z);
                        QDragEnterEvent enter(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
                        QApplication::sendEvent(cad, &enter);
                        QDragMoveEvent move(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
                        QApplication::sendEvent(cad, &move);
                        QTest::qWait(30);
                        if (id == "base-1")
                            ensure(window.screen()
                                       ->grabWindow(window.winId())
                                       .save(directory + "/wall-placement.png"),
                                   "Ghost screenshot failed");
                        QDropEvent event(QPointF(point), Qt::CopyAction, &mime, Qt::LeftButton,
                                         Qt::NoModifier);
                        QApplication::sendEvent(cad, &event);
                        ensure(event.isAccepted() == expected,
                               "Drop acceptance differs from available space");
                    };
                    drop("base-1", 1000, 90, true);
                    auto placed = window.editor().document().entities.back();
                    ensure(std::abs(placed.transform.y - 62) < 0.2 && std::abs(placed.transform.yaw) < 0.2,
                           "Wall alignment failed");
                    const auto beforeOutside = window.editor().document().entities.size();
                    drop("base-1", 8000, 8000, false);
                    ensure(window.editor().document().entities.size() == beforeOutside,
                           "Outside drop inserted a floating object");
                    // Move the actual detected furniture through native mouse events, one undo command.
                    const auto press =
                        cad->project(placed.transform.x + 300, placed.transform.y + 275, placed.height);
                    const auto destination = cad->project(1300, 1500);
                    const auto history = window.editor().history.count();
                    QTest::mousePress(cad, Qt::LeftButton, Qt::NoModifier, press);
                    QMouseEvent moveEvent(QEvent::MouseMove, QPointF(destination),
                                          QPointF(cad->mapToGlobal(destination)), Qt::NoButton,
                                          Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(cad, &moveEvent);
                    QTest::mouseRelease(cad, Qt::LeftButton, Qt::NoModifier, destination);
                    ensure(window.editor().history.count() == history + 1,
                           "Direct furniture drag did not make one command");
                    ensure(window.editor().document().at(placed.id).transform.y > 1000,
                           "Furniture drag did not move into room");
                    window.editor().history.undo();
                    ensure(window.editor().document().at(placed.id).transform.y == placed.transform.y,
                           "Direct drag undo failed");
                    window.editor().history.redo();
                    drop("window-ready", 3500, 90, true);
                    ensure(window.editor().document().entities.back().type == "Window",
                           "Window drop did not attach to a wall");
                    window.editor().history.undo();
                    window.editor().history.setClean();
                    cad->setTop(false);
                    cad->frame();
                    QTest::qWait(150);
                    drop("base-1", 2500, 0, true, 1400);
                    ensure(std::abs(window.editor().document().entities.back().transform.y - 62) < 0.2,
                           "3D wall ray placement failed");
                    window.editor().history.undo();
                    cad->setTop(true);
                    cad->frame();
                    ensure(QMetaObject::invokeMethod(&window, "apartmentStarter", Qt::DirectConnection),
                           "Apartment template action unavailable");
                    ensure(std::count_if(window.editor().document().entities.begin(),
                                         window.editor().document().entities.end(),
                                         [](const auto &e) { return e.type == "Room"; }) == 3,
                           "Apartment template rooms missing");
                    ensure(window.editor().document().embeddedAssets.size() >= 8,
                           "Ready meshes were not embedded in apartment");
                    ensure(std::count_if(window.editor().document().entities.begin(),
                                         window.editor().document().entities.end(),
                                         [](const auto &e) { return e.type == "Ceiling" && e.visible; }) == 3,
                           "Apartment ceilings missing from render");
                    lmx::ProjectStore::save(directory + "/apartamento.lmx", window.editor().document(),
                                            false);
                    auto reopened = lmx::ProjectStore::open(directory + "/apartamento.lmx");
                    ensure(reopened.entities.size() == window.editor().document().entities.size(),
                           "Apartment save/open differs");
                    cad->setTop(true);
                    cad->frame();
                    QTest::qWait(300);
                    ensure(
                        window.screen()->grabWindow(window.winId()).save(directory + "/apartment-plan.png"),
                        "Plan screenshot failed");
                    cad->setTop(false);
                    cad->frame();
                    QTest::qWait(300);
                    ensure(window.screen()->grabWindow(window.winId()).save(directory + "/apartment-3d.png"),
                           "3D screenshot failed");
                    cad->capture(directory + "/apartment-viewport.png");
                    window.resize(900, 700);
                    QTest::qWait(300);
                    cad->frame();
                    QTest::qWait(150);
                    auto *scroll = window.findChild<QScrollArea *>("inspectorScroll");
                    ensure(scroll && scroll->horizontalScrollBar()->maximum() == 0,
                           "Compact inspector overflows");
                    ensure(assets->horizontalScrollBar()->maximum() == 0, "Compact catalog overflows");
                    ensure(window.screen()->grabWindow(window.winId()).save(directory + "/apartment-900.png"),
                           "Compact screenshot failed");
                    std::cout << "ASSEMBLY_PASS: 79 thumbnails, wall ghost/drop, outside rejection, mouse "
                                 "move undo/redo, window wall attachment, 52 ready meshes, 3-room apartment "
                                 "save/open, 900 px panels\n";
                    app.exit(0);
                } catch (const std::exception &e) {
                    std::cerr << "ASSEMBLY_FAIL: " << e.what() << '\n';
                    app.exit(1);
                }
            });
        }
        if (parser.isSet("recovery-fixture")) {
            QTimer::singleShot(500, &window, [&] {
                auto document = lmx::kitchenExample();
                window.editor().load(document);
                window.editor().apply("Unsaved edit before forced crash", [](lmx::Document &d) {
                    auto module = std::find_if(d.entities.begin(), d.entities.end(),
                                               [](const auto &e) { return e.type == "FurnitureModule"; });
                    module->width = 812.5;
                });
                window.autosave();
                lmx::ProjectStore::save(recoveryDirectory + "/expected.snapshot", window.editor().document(),
                                        false);
                std::cout << "RECOVERY_READY\n" << std::flush;
            });
        }
        if (parser.isSet("recovery-verify")) {
            QTimer::singleShot(500, &window, [&] {
                try {
                    auto expected = lmx::ProjectStore::open(recoveryDirectory + "/expected.snapshot");
                    QTimer click;
                    QObject::connect(&click, &QTimer::timeout, &window, [] {
                        if (auto *dialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                            QTest::mouseClick(dialog->button(QMessageBox::Yes), Qt::LeftButton);
                    });
                    click.start(50);
                    if (!QMetaObject::invokeMethod(&window, "recover", Qt::DirectConnection))
                        throw std::runtime_error("Recovery dialog is unavailable");
                    expected.name += " (recuperado)";
                    if (window.editor().document().serialize() != expected.serialize() ||
                        window.editor().history.isClean())
                        throw std::runtime_error("Recovered state differs from durable autosave");
                    std::cout << "RECOVERY_RESTART_PASS\n";
                    app.exit(0);
                } catch (const std::exception &e) {
                    std::cerr << "RECOVERY_RESTART_FAIL: " << e.what() << '\n';
                    app.exit(1);
                }
            });
        }
        if (parser.isSet("ui-smoke")) {
            const auto directory = QDir(parser.value("ui-smoke")).absolutePath();
            QDir().mkpath(directory);
            QTimer::singleShot(1500, &window, [&window, directory, &app, &parser] {
                try {
                    auto ensure = [](bool value, const char *error) {
                        if (!value)
                            throw std::runtime_error(error);
                    };
                    window.editor().load(lmx::Document{});
                    window.cad()->setTop(true);
                    window.cad()->setTool("wall");
                    auto *cad = window.cad();
                    QPoint a = cad->project(0, 0), b = cad->project(2000, 0);
                    QTest::mouseClick(cad, Qt::LeftButton, Qt::NoModifier, a);
                    QTest::mouseMove(cad, b);
                    QTest::mouseClick(cad, Qt::LeftButton, Qt::NoModifier, b);
                    ensure(window.editor().document().entities.size() == 1, "Wall drawing failed");
                    ensure(window.editor().document().entities[0].width == 2000,
                           "Wall coordinates are incorrect");
                    window.editor().history.undo();
                    ensure(window.editor().document().entities.empty(), "UI undo failed");
                    window.editor().history.redo();
                    ensure(window.editor().document().entities.size() == 1, "UI redo failed");
                    cad->setTool("select");
                    auto *assets = window.findChild<QListWidget *>("assetList");
                    ensure(assets && assets->count() > 0, "Starter library is unavailable");
                    int ready = 0;
                    for (int attempt = 0; attempt < 100; ++attempt) {
                        ready = 0;
                        for (int i = 0; i < assets->count(); ++i)
                            ready += assets->item(i)->data(Qt::UserRole + 2).toBool();
                        if (ready == assets->count())
                            break;
                        QTest::qWait(100);
                    }
                    ensure(ready == 79, "Shipped asset geometry thumbnails were not generated");
                    std::cout << "THUMBNAILS_PASS: 79 actual geometry previews\n";
                    QListWidgetItem *asset = nullptr;
                    for (int i = 0; i < assets->count(); ++i)
                        if (assets->item(i)->data(Qt::UserRole).toString() == "base-1")
                            asset = assets->item(i);
                    ensure(asset, "Missing cabinet asset");
                    assets->scrollToItem(asset);
                    auto assetId = asset->data(Qt::UserRole).toString();
                    QTest::mouseClick(assets->viewport(), Qt::LeftButton, Qt::NoModifier,
                                      assets->visualItemRect(asset).center());
                    QTest::mouseDClick(assets->viewport(), Qt::LeftButton, Qt::NoModifier,
                                       assets->visualItemRect(asset).center());
                    ensure(window.editor().document().entities.size() == 1,
                           "Double-click should await a placement point");
                    QTest::mouseClick(cad, Qt::LeftButton, Qt::NoModifier, cad->project(1000, 1000));
                    ensure(window.editor().document().entities.size() == 2,
                           "Library double-click insertion failed");
                    window.editor().history.undo();
                    ensure(window.editor().document().entities.size() == 1, "Library insertion undo failed");
                    QMimeData mime;
                    mime.setData("application/x-libremax-asset", assetId.toUtf8());
                    auto dropPoint = cad->project(1000, 1000);
                    QDragEnterEvent enter(dropPoint, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(cad, &enter);
                    ensure(enter.isAccepted(), "Library drag-enter rejected");
                    QDragMoveEvent move(dropPoint, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
                    QApplication::sendEvent(cad, &move);
                    ensure(move.isAccepted(), "Library geometry ghost preview failed");
                    QDropEvent drop(QPointF(dropPoint), Qt::CopyAction, &mime, Qt::LeftButton,
                                    Qt::NoModifier);
                    QApplication::sendEvent(cad, &drop);
                    ensure(drop.isAccepted() && window.editor().document().entities.size() == 2,
                           "Library drop insertion failed");
                    const auto &dropped = window.editor().document().entities.back();
                    ensure(dropped.metadata.at("asset") == assetId.toStdString() &&
                               std::abs(dropped.transform.x - 700) < 10 &&
                               std::abs(dropped.transform.y - 725) < 10,
                           "Dropped asset identity or snapped position differs");
                    window.editor().history.undo();
                    ensure(window.editor().document().entities.size() == 1, "Drop undo failed");
                    auto kitchen = lmx::kitchenExample();
                    window.editor().load(kitchen);
                    window.cad()->setTop(false);
                    window.cad()->frame();
                    auto it = std::find_if(kitchen.entities.begin(), kitchen.entities.end(),
                                           [](const auto &e) { return e.type == "FurnitureModule"; });
                    ensure(it != kitchen.entities.end(), "Missing module");
                    window.selectIds({QString::fromStdString(it->id)});
                    auto *field = window.findChild<QLineEdit *>("widthField");
                    ensure(field, "Missing width editor");
                    field->setFocus();
                    field->clear();
                    QTest::keyClicks(field, "75,3+5,95");
                    auto *button = window.findChild<QPushButton *>("applyProperties");
                    ensure(button, "Missing apply button");
                    QTest::mouseClick(button, Qt::LeftButton);
                    ensure(window.editor().document().at(it->id).width == 812.5,
                           "Numeric expression resize failed");
                    window.editor().history.undo();
                    ensure(window.editor().document().at(it->id).width == 600, "Resize undo failed");
                    window.editor().history.redo();
                    auto *camera = window.findChild<QComboBox *>("renderCamera");
                    auto *exposure = window.findChild<QDoubleSpinBox *>("renderExposure");
                    ensure(camera && exposure && camera->count() == 2, "Render controls are unavailable");
                    auto *renderDock = window.findChild<QDockWidget *>("renderDock");
                    renderDock->raise();
                    QTest::qWait(100);
                    auto *pbrButton = window.findChild<QPushButton *>("activatePbrMaterials");
                    ensure(pbrButton, "PBR material action is unavailable");
                    window.findChild<QScrollArea *>("renderScroll")->ensureWidgetVisible(pbrButton);
                    QTest::qWait(100);
                    QTest::mouseClick(pbrButton, Qt::LeftButton);
                    for (int i = 0; i < 100 && window.editor().document().embeddedAssets.size() != 6; ++i)
                        QTest::qWait(100);
                    ensure(window.editor().document().embeddedAssets.size() == 6,
                           "PBR material activation failed");
                    auto *sky = window.findChild<QComboBox *>("renderEnvironmentMode");
                    auto *sun = window.findChild<QDoubleSpinBox *>("renderSunElevation");
                    ensure(sky && sun, "Natural sky controls unavailable");
                    sky->setCurrentIndex(1);
                    sun->setValue(42);
                    QMetaObject::invokeMethod(sun, "editingFinished", Qt::DirectConnection);
                    ensure(window.editor().document().renderSettings["sunElevation"] == 42,
                           "Sun controls did not update the project");
                    window.findChild<QDockWidget *>("propertiesDock")->raise();
                    std::cout << "PHOTOGRAPHIC_UI_PASS: six PBR maps and natural sky controls\n";
                    camera->setCurrentIndex(1);
                    exposure->setValue(-0.8);
                    QMetaObject::invokeMethod(exposure, "editingFinished", Qt::DirectConnection);
                    ensure(window.editor().document().renderSettings["camera"] ==
                                   camera->currentData().toString().toStdString() &&
                               window.editor().document().renderSettings["exposure"] == -0.8,
                           "Camera and exposure controls did not update the document");
                    lmx::ProjectStore::save(directory + "/ui-roundtrip.lmx", window.editor().document(),
                                            false);
                    auto saved = window.editor().document().serialize();
                    window.editor().load(lmx::Document{});
                    window.loadProject(directory + "/ui-roundtrip.lmx");
                    ensure(window.editor().document().serialize() == saved, "UI save/open roundtrip failed");
                    window.cad()->setTop(false);
                    window.cad()->frame();
                    window.selectIds({QString::fromStdString(it->id)});
                    QTest::qWait(700);
                    window.cad()->capture(directory + "/viewport.png");
                    ensure(window.screen()->grabWindow(window.winId()).save(directory + "/native-ui.png"),
                           "Native screenshot failed");
                    window.resize(1024, 768);
                    QTest::qWait(500);
                    ensure(
                        window.screen()->grabWindow(window.winId()).save(directory + "/native-ui-1024.png"),
                        "Compact screenshot failed");
                    window.resize(900, 650);
                    QTest::qWait(300);
                    ensure(window.screen()->grabWindow(window.winId()).save(directory + "/native-ui-900.png"),
                           "Small desktop screenshot failed");
                    auto *inspector = window.findChild<QScrollArea *>("inspectorScroll");
                    ensure(inspector && inspector->horizontalScrollBar()->maximum() == 0,
                           "Inspector requires horizontal scrolling at 900 px");
                    inspector->verticalScrollBar()->setValue(inspector->verticalScrollBar()->maximum());
                    QTest::qWait(100);
                    auto *applyButton = window.findChild<QPushButton *>("applyProperties");
                    ensure(applyButton && inspector->viewport()->rect().contains(applyButton->mapTo(
                                              inspector->viewport(), applyButton->rect().center())),
                           "Apply button is inaccessible in compact inspector");
                    std::cout
                        << "COMPACT_INSPECTOR_PASS: no horizontal overflow, Apply reachable at 900 px\n";
                    inspector->verticalScrollBar()->setValue(0);
                    if (parser.isSet("preview-image")) {
                        window.resize(1440, 900);
                        const auto renderProject =
                            QFileInfo(parser.value("preview-image")).dir().filePath("render-project.lmx");
                        if (QFileInfo::exists(renderProject))
                            window.loadProject(renderProject);
                        window.showRenderImage(parser.value("preview-image"));
                        auto *dock = window.findChild<QDockWidget *>("renderDock");
                        dock->show();
                        dock->raise();
                        QTest::qWait(400);
                        auto *canvas = window.findChild<lmx::ImageCanvas *>("renderCanvas");
                        auto *actual = window.findChild<QPushButton *>("actualRenderSize");
                        ensure(canvas && actual, "Render viewer is unavailable");
                        QTest::mouseClick(actual, Qt::LeftButton);
                        ensure(canvas->transform().m11() == 1.0, "Render viewer 1:1 scale failed");
                        QTest::mouseClick(window.findChild<QPushButton *>("fitRenderImage"), Qt::LeftButton);
                        QTest::qWait(200);
                        const auto copyPath = directory + "/exported-render.png";
                        QFile::remove(copyPath);
                        QTimer exportDeadline;
                        exportDeadline.setSingleShot(true);
                        QObject::connect(&exportDeadline, &QTimer::timeout, &window, [] {
                            if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget()))
                                dialog->reject();
                        });
                        exportDeadline.start(5000);
                        QTimer::singleShot(300, &window, [copyPath] {
                            if (auto *dialog =
                                    qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                                if (auto *filename = dialog->findChild<QLineEdit *>("fileNameEdit")) {
                                    filename->setFocus();
                                    QTest::keyClick(filename, Qt::Key_A, Qt::ControlModifier);
                                    QTest::keyClicks(filename, copyPath);
                                }
                                QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
                            }
                        });
                        auto *exportButton = window.findChild<QPushButton *>("exportRenderImage");
                        ensure(exportButton && exportButton->isVisible(), "Render export button unavailable");
                        QSignalSpy exportClick(exportButton, &QPushButton::clicked);
                        QTest::mouseClick(exportButton, Qt::LeftButton);
                        exportDeadline.stop();
                        ensure(exportClick.count() == 1, "Render export button did not receive click");
                        ensure(QImage(copyPath).convertToFormat(QImage::Format_RGBA8888) ==
                                   QImage(parser.value("preview-image"))
                                       .convertToFormat(QImage::Format_RGBA8888),
                               "Render export did not preserve image pixels");
                        ensure(window.screen()
                                   ->grabWindow(window.winId())
                                   .save(directory + "/render-workspace.png"),
                               "Render workspace screenshot failed");
                        window.resize(900, 650);
                        QTest::qWait(300);
                        auto *renderScroll = window.findChild<QScrollArea *>("renderScroll");
                        ensure(renderScroll && renderScroll->horizontalScrollBar()->maximum() == 0,
                               "Render panel requires horizontal scrolling at 900 px");
                        renderScroll->verticalScrollBar()->setValue(
                            renderScroll->verticalScrollBar()->maximum());
                        auto *startRender = window.findChild<QPushButton *>("startRender");
                        ensure(startRender && renderScroll->viewport()->rect().contains(startRender->mapTo(
                                                  renderScroll->viewport(), startRender->rect().center())),
                               "Render action is inaccessible in compact workspace");
                        renderScroll->verticalScrollBar()->setValue(0);
                        canvas->fit();
                        QTest::qWait(100);
                        ensure(window.screen()
                                   ->grabWindow(window.winId())
                                   .save(directory + "/render-workspace-900.png"),
                               "Compact render screenshot failed");
                        std::cout << "RENDER_VIEWER_PASS: real image, fit, 1:1, native PNG export preserves "
                                     "pixels\n";
                    }
                    std::cout << "UI_SMOKE_PASS: wall draw, library double-click and drop/ghost, "
                                 "undo/redo, centimeter expression edit, "
                                 "save/open, native CAD screenshots\n";
                    app.exit(0);
                } catch (const std::exception &e) {
                    std::cerr << "UI_SMOKE_FAIL: " << e.what() << '\n';
                    app.exit(1);
                }
            });
        }
        return app.exec();
    } catch (const std::exception &e) {
        spdlog::critical("{}", e.what());
        std::cerr << e.what() << '\n';
        return 1;
    }
}
