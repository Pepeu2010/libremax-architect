#include "apartment_acceptance.h"
#include "camera_acceptance.h"
#include "environment_acceptance.h"
#include "first_run.h"
#include "lighting_acceptance.h"
#include "main_window.h"
#include "materials/texture.h"
#include "persistence/project_store.h"
#include "queue_acceptance.h"
#include "rendering/environment_map.h"
#include "resource_paths.h"
#include "studio_theme.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDialogButtonBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QWizard>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numbers>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    QApplication::setApplicationName("libremax");
    QApplication::setOrganizationName("LibreMax");
    QApplication::setApplicationVersion(LMX_VERSION);
    QApplication::setWindowIcon(QIcon(":/studio/brand/libremax-mark.png"));
    QCommandLineParser parser;
    parser.setApplicationDescription("LibreMax Architect — native interior design");
    parser.addHelpOption();
    parser.addOption(
        {"apartment-tools-smoke",
         "Verify room contours, grouped placement, actual model import, packs and 3 real photo styles",
         "directory"});
    parser.addVersionOption();
    parser.addOption({"camera-smoke",
                      "Verify native camera framing against actual Blender projection and render",
                      "directory"});
    parser.addOption({"queue-smoke", "Verify native batch render queue, snapshots and gallery", "directory"});
    parser.addOption({"environment-smoke", "Verify native HDRI and real Cycles EXR queue", "directory"});
    parser.addOption(
        {"lighting-smoke", "Verify native light creation and actual Cycles LED and Kelvin", "directory"});
    parser.addOption(
        {"installation-smoke", "Verify installed catalogs, plugins, assets and projects", "directory"});
    parser.addOption({"ui-smoke", "Run native UI acceptance and save real screenshots", "directory"});
    parser.addOption(
        {"modern-smoke", "Verify modern models, filter and self-contained apartment", "directory"});
    parser.addOption({"experience-smoke", "Verify first run, tutorial, new project, save and recent projects",
                      "directory"});
    parser.addOption(
        {"assembly-smoke", "Verify native apartment mounting and real furniture placement", "directory"});
    parser.addOption({"examples", "Generate valid sample .lmx projects", "directory"});
    parser.addOption({"render-smoke", "Run real Cycles through asynchronous QProcess pipeline", "directory"});
    parser.addOption({"render-size", "Acceptance render size WxH", "size", "320x180"});
    parser.addOption({"render-device", "Acceptance device CPU or AUTO", "device", "CPU"});
    parser.addOption({"expect-render-gpu", "Require the acceptance image to actually use a GPU"});
    parser.addOption({"render-samples", "Acceptance Cycles samples", "samples", "16"});
    parser.addOption({"render-project", "Use a saved .lmx for render acceptance", "project"});
    parser.addOption({"instances-fixture", "Write a portable repeated-model render fixture", "directory"});
    parser.addOption({"instances-model", "Fixture family: light or authored", "family", "light"});
    parser.addOption({"render-script", "Archived script override for render-smoke only", "script"});
    parser.addOption({"preview-image", "Exercise native image viewer with an existing real render", "image"});
    parser.addOption({"recovery-smoke", "Kill a child process and verify recovery in a fresh process"});
    parser.addOption({"recovery-fixture", "Internal crash acceptance writer", "directory"});
    parser.addOption({"recovery-verify", "Internal crash acceptance reader", "directory"});
    parser.addOption({"blender", "Blender executable for render acceptance", "executable"});
    parser.addPositionalArgument("project", ".lmx project to open");
    parser.process(app);
    bool test =
        parser.isSet("camera-smoke") || parser.isSet("apartment-tools-smoke") ||
        parser.isSet("lighting-smoke") || parser.isSet("environment-smoke") || parser.isSet("queue-smoke") ||
        parser.isSet("installation-smoke") || parser.isSet("experience-smoke") ||
        parser.isSet("modern-smoke") || parser.isSet("assembly-smoke") || parser.isSet("ui-smoke") ||
        parser.isSet("examples") || parser.isSet("render-smoke") || parser.isSet("instances-fixture") ||
        parser.isSet("recovery-smoke") || parser.isSet("recovery-fixture") || parser.isSet("recovery-verify");
    if (test)
        QStandardPaths::setTestModeEnabled(true);
    QTemporaryDir settingsDirectory;
    if (test) {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    }
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
        if (parser.isSet("render-script") && !parser.isSet("render-smoke"))
            throw std::invalid_argument("render-script is restricted to render-smoke acceptance");
        if (parser.isSet("instances-fixture")) {
            const auto directory = QDir(parser.value("instances-fixture")).absolutePath();
            QDir().mkpath(directory);
            QTemporaryDir database;
            lmx::Library library(database.filePath("models.db"), lmx::resourcePath("starter-models"));
            const auto family = parser.value("instances-model");
            if (family != "light" && family != "authored")
                throw std::invalid_argument("Fixture family must be light or authored");
            QFile catalog(lmx::resourcePath(family == "authored" ? "starter-models/modern-catalog.json"
                                                                 : "starter-models/catalog.json"));
            if (!catalog.open(QIODevice::ReadOnly))
                throw std::runtime_error("Fixture catalog unavailable");
            library.seed(lmx::Json::parse(catalog.readAll().toStdString()));
            const auto assets = library.search({}, {}, false, false, false);
            const auto asset = std::find_if(assets.begin(), assets.end(), [&](const auto &a) {
                return family == "authored" ? a.id == "modern-modern_arm_chair_01" : a.id.contains("chair");
            });
            if (asset == assets.end())
                throw std::runtime_error("Ready chair fixture unavailable");
            const auto payload = library.withPayload(*asset);
            auto document = lmx::kitchenExample();
            std::erase_if(document.entities,
                          [](const auto &e) { return e.type != "Camera" && e.type != "Light"; });
            lmx::Library::attachModel(document, payload);
            for (int i = 0; i < 32; ++i) {
                auto object = lmx::Library::instantiate(payload, (i % 8) * 750, (i / 8) * 800);
                object.transform.yaw = i * 13;
                object.transform.mirrored = i >= 30;
                if (i == 29)
                    object.width *= 1.2;
                document.entities.push_back(object);
            }
            for (auto &e : document.entities)
                if (e.type == "Camera") {
                    e.transform = {9000, -6500, 7000, 0, false};
                    e.parameters["target"] = {2750, 1250, 350};
                }
            auto floor = lmx::entity("Floor", "Piso");
            floor.width = 8000;
            floor.depth = 5500;
            floor.height = 80;
            floor.transform = {-1000, -1000, -80, 0, false};
            document.entities.push_back(floor);
            lmx::ProjectStore::save(directory + "/repeated-models.lmx", document, false);
            const auto legacy = lmx::meshSnapshot(document), compact = lmx::meshSnapshot(document, true);
            std::map<std::string, std::size_t> users;
            for (const auto &instance : compact.at("instances"))
                ++users[instance.at("mesh").get<std::string>()];
            std::size_t linked = 0;
            for (const auto &[mesh, count] : users)
                if (count > 1)
                    linked += count;
            const auto statistics = lmx::Json{
                {"objects", legacy.at("meshes").size()},
                {"definitions", compact.at("meshes").size()},
                {"linkedObjects", linked},
                {"legacyBytes", legacy.dump().size()},
                {"compactBytes",
                 compact.dump().size()}}.dump(2);
            QFile report(directory + "/fixture.json");
            if (!report.open(QIODevice::WriteOnly) ||
                report.write(statistics.data(), statistics.size()) != static_cast<qint64>(statistics.size()))
                throw std::runtime_error("Fixture report could not be written");
            std::cout << "INSTANCE_FIXTURE_PASS: " << legacy.at("meshes").size() << " objects, "
                      << compact.at("meshes").size() << " definitions; " << legacy.dump().size() << " versus "
                      << compact.dump().size() << " bytes\n";
            return 0;
        }
        if (parser.isSet("installation-smoke")) {
            const auto output = QDir(parser.value("installation-smoke")).absolutePath();
            QDir().mkpath(output);
            const auto installedRoot =
                QDir(QApplication::applicationDirPath() + "/../share/libremax").canonicalPath();
            if (installedRoot.isEmpty() || !QFileInfo(lmx::resourcePath("starter-models"))
                                                .canonicalFilePath()
                                                .startsWith(installedRoot + "/"))
                throw std::runtime_error("Installation must use packaged resources");
            QTemporaryDir database;
            lmx::Library library(database.filePath("library.db"), lmx::resourcePath("starter-models"));
            std::size_t expected = 0;
            for (const auto &catalog :
                 {"starter-library/catalog.json", "starter-models/catalog.json",
                  "starter-models/modern-catalog.json", "starter-models/current-catalog.json",
                  "starter-models/expanded-catalog.json"}) {
                QFile file(lmx::resourcePath(catalog));
                if (!file.open(QIODevice::ReadOnly))
                    throw std::runtime_error("Installed catalog missing");
                auto entries = lmx::Json::parse(file.readAll().toStdString());
                expected += entries.size();
                library.seed(entries);
            }
            const auto entries = library.search({}, {}, false, false, false);
            if (entries.size() != expected || library.search("sofa").empty())
                throw std::runtime_error("Installed SQLite/FTS catalog failed");
            auto project = lmx::ProjectStore::open(lmx::resourcePath("examples/apartamento-moderno.lmx"));
            for (const auto &entry : entries)
                if (entry.recipe.contains("modelFile")) {
                    auto payload = library.withPayload(entry);
                    lmx::Document standalone;
                    lmx::Library::attachModel(standalone, payload);
                    standalone.entities.push_back(lmx::Library::instantiate(entry, 0, 0));
                    standalone.validate();
                }
            lmx::attachEnvironment(project, lmx::importEnvironment(lmx::resourcePath(
                                                "starter-environments/kiara_1_dawn_1k.hdr")));
            lmx::ProjectStore::save(output + "/portable-project.lmx", project, false);
            if (lmx::ProjectStore::open(output + "/portable-project.lmx").serialize() != project.serialize())
                throw std::runtime_error("Installed project save/reopen failed");
            QFile script(lmx::resourcePath("scripts/cycles_render.py"));
            if (!script.open(QIODevice::ReadOnly) || script.size() < 1000 ||
                !QFileInfo::exists(lmx::resourcePath("scripts/cycles_lights.py")) ||
                !QFileInfo::exists(lmx::resourcePath("resources/style.qss")))
                throw std::runtime_error("Installed renderer resources invalid");
            const auto engine = lmx::BlenderBridge::findExecutable();
            const auto engineRoot = QDir(installedRoot + "/runtime/blender").canonicalPath();
            if (engineRoot.isEmpty() || !engine.startsWith(engineRoot + "/") ||
                !QFileInfo::exists(engineRoot + "/license/license.md") ||
                !QFileInfo::exists(engineRoot + "/libremax-runtime.json"))
                throw std::runtime_error("Installed Blender runtime missing or not automatically detected");
            QProcess engineCheck;
            engineCheck.setProcessChannelMode(QProcess::MergedChannels);
            engineCheck.start(engine, {"--version"});
            if (!engineCheck.waitForStarted(10000) || !engineCheck.waitForFinished(30000) ||
                engineCheck.exitStatus() != QProcess::NormalExit || engineCheck.exitCode() != 0 ||
                !engineCheck.readAll().contains("Blender 4.5.9"))
                throw std::runtime_error("Installed Blender could not run with its bundled dependencies");
            std::cout << "BUNDLED_BLENDER_PASS: automatically found and ran Blender 4.5.9\n";
            QImage codec(32, 32, QImage::Format_RGB32);
            codec.fill(Qt::darkBlue);
            if (!codec.save(output + "/image-codec.jpg", "JPEG") ||
                QImage(output + "/image-codec.jpg").isNull() ||
                QImage(":/studio/brand/libremax-mark.png").isNull())
                throw std::runtime_error("Installed JPEG/PNG codecs failed");
            std::cout << "INSTALLATION_PASS: " << expected
                      << " catalog entries, SQLite/FTS, all bundled models, portable HDRI project, packaged "
                         "renderer\n";
            return 0;
        }
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
            auto pack = lmx::readPbrMaterials(lmx::resourcePath("starter-materials"));
            auto kitchen = lmx::kitchenExample(), bedroom = lmx::bedroomExample();
            lmx::attachPbrMaterials(kitchen, pack);
            lmx::attachPbrMaterials(bedroom, pack);
            lmx::ProjectStore::save(dir.filePath("cozinha.lmx"), kitchen, false);
            lmx::ProjectStore::save(dir.filePath("dormitorio.lmx"), bedroom, false);
            return 0;
        }
        if (parser.isSet("render-smoke")) {
            const auto blender = lmx::BlenderBridge::findExecutable(parser.value("blender"));
            if (blender.isEmpty())
                throw std::runtime_error("Render engine not found");
            const auto script = parser.isSet("render-script")
                                    ? QFileInfo(parser.value("render-script")).absoluteFilePath()
                                    : lmx::resourcePath("scripts/cycles_render.py");
            const auto renderDevice = parser.value("render-device");
            if (renderDevice != "CPU" && renderDevice != "AUTO")
                throw std::runtime_error("Use render device CPU or AUTO");
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
            if (!parser.isSet("render-project"))
                lmx::attachPbrMaterials(document,
                                        lmx::readPbrMaterials(lmx::resourcePath("starter-materials")));
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
                const auto engine = job.engine();
                std::cout << "RENDER_ENGINE: " << engine.dump() << '\n';
                if (parser.isSet("expect-render-gpu") &&
                    (engine.value("device", std::string{}) != "GPU" || engine.value("fallback", false))) {
                    std::cerr << "Requested GPU acceptance fell back to CPU\n";
                    app.exit(1);
                    return;
                }
                std::cout << "RENDER_SMOKE_PASS: real Cycles, " << engine.value("device", std::string{})
                          << ", denoise, " << renderWidth << "x" << renderHeight << " image\n";
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
                job.start(document, blender, script, path, renderWidth, renderHeight, renderSamples,
                          renderDevice);
            });
            QTimer::singleShot(600000, &app, [&] {
                job.cancel();
                std::cerr << "Render acceptance timeout\n";
                app.exit(1);
            });
            job.start(document, blender, script, directory + "/cycles-kitchen.png", renderWidth, renderHeight,
                      renderSamples, renderDevice);
            return app.exec();
        }
        lmx::applyStudioPalette();
        QString stylesheet = QApplication::applicationDirPath() + "/../share/libremax/resources/style.qss";
        if (!QFileInfo::exists(stylesheet))
            stylesheet = lmx::resourcePath("resources/style.qss");
        QFile style(stylesheet);
        if (style.open(QIODevice::ReadOnly))
            app.setStyleSheet(QString::fromUtf8(style.readAll()));
        auto recoveryDirectory = parser.isSet("recovery-fixture")  ? parser.value("recovery-fixture")
                                 : parser.isSet("recovery-verify") ? parser.value("recovery-verify")
                                                                   : QString{};
        const auto experienceRoot =
            parser.isSet("experience-smoke")
                ? QDir(parser.value("experience-smoke"))
                      .absoluteFilePath("state-" + QString::fromStdString(lmx::uuid()))
                : QString{};
        QElapsedTimer startup;
        startup.start();
        lmx::MainWindow window(test, recoveryDirectory, parser.isSet("experience-smoke"), experienceRoot);
        std::cout << "STARTUP_SHELL_MS: " << startup.elapsed() << '\n';
        if (!parser.positionalArguments().isEmpty())
            window.loadProject(parser.positionalArguments().first());
        window.show();
        if (parser.isSet("camera-smoke"))
            lmx::startCameraAcceptance(window, app, parser.value("camera-smoke"), parser.value("blender"));
        if (parser.isSet("apartment-tools-smoke"))
            lmx::startApartmentAcceptance(window, app, parser.value("apartment-tools-smoke"),
                                          parser.value("blender"));
        if (parser.isSet("lighting-smoke"))
            lmx::startLightingAcceptance(window, app, parser.value("lighting-smoke"),
                                         parser.value("blender"));
        if (parser.isSet("environment-smoke"))
            lmx::startEnvironmentAcceptance(window, app, parser.value("environment-smoke"),
                                            parser.value("blender"));
        if (parser.isSet("queue-smoke"))
            lmx::startQueueAcceptance(window, app, parser.value("queue-smoke"), parser.value("blender"),
                                      lmx::resourcePath("scripts/cycles_render.py"));
        if (!test || parser.isSet("experience-smoke"))
            lmx::showOpening(&window);
        if (parser.isSet("experience-smoke")) {
            const auto directory = QDir(parser.value("experience-smoke")).absolutePath();
            QDir().mkpath(directory);
            QTimer::singleShot(60000, &window, [&app, directory] {
                for (auto *top : QApplication::topLevelWidgets())
                    if (top->isVisible()) {
                        std::cerr << "EXPERIENCE_TIMEOUT_WINDOW: " << top->metaObject()->className() << " "
                                  << top->windowTitle().toStdString() << '\n';
                        top->screen()
                            ->grabWindow(top->winId())
                            .save(directory + "/timeout-" + top->metaObject()->className() + ".png");
                    }
                app.exit(1);
            });
            QTimer::singleShot(300, &window, [directory] {
                for (auto *top : QApplication::topLevelWidgets())
                    if (top->objectName() == "openingLogo")
                        top->screen()->grabWindow(top->winId()).save(directory + "/opening-logo.png");
            });
            QTimer::singleShot(1500, &window, [&window, &app, directory, experienceRoot] {
                try {
                    auto ensure = [](bool condition, const char *message) {
                        if (!condition)
                            throw std::runtime_error(message);
                    };
                    auto *tutorial = window.findChild<QWizard *>("firstRunTutorial");
                    ensure(tutorial && tutorial->isVisible(), "First opening tutorial missing");
                    const auto chapterCount = tutorial->pageIds().size();
                    ensure(chapterCount == 21, "Updated tutorial chapters missing");
                    auto *assets = window.findChild<QListWidget *>("assetList");
                    ensure(assets && assets->count() == 0, "Home eagerly loaded the furniture catalog");
                    ensure(
                        tutorial->screen()->grabWindow(tutorial->winId()).save(directory + "/tutorial.png"),
                        "Tutorial capture failed");
                    tutorial->resize(680, 520);
                    for (int i = 0; i < chapterCount; ++i) {
                        auto *page = tutorial->currentPage();
                        auto *scroll = page->findChild<QScrollArea *>("tutorialLessonScroll");
                        auto *topics = page->findChild<QComboBox *>("tutorialTopics");
                        ensure(scroll && topics && topics->count() == chapterCount,
                               "Tutorial topic navigation missing");
                        QTest::qWait(20);
                        ensure(scroll->horizontalScrollBar()->maximum() == 0,
                               "Compact tutorial requires horizontal scrolling");
                        ensure(tutorial->rect().contains(tutorial->button(QWizard::CancelButton)->geometry()),
                               "Compact tutorial buttons are clipped");
                        if (i == 3 || i == 12)
                            ensure(tutorial->screen()
                                       ->grabWindow(tutorial->winId())
                                       .save(directory + QString("/tutorial-chapter-%1.png").arg(i)),
                                   "Illustrated lesson capture failed");
                        if (i == chapterCount - 1)
                            break;
                        QTest::mouseClick(tutorial->button(QWizard::NextButton), Qt::LeftButton);
                        QTest::qWait(20);
                        ensure(tutorial->currentId() == i + 1, "Tutorial keyboard/button navigation failed");
                    }
                    QTest::mouseClick(tutorial->button(QWizard::FinishButton), Qt::LeftButton);
                    QTest::qWait(50);
                    ensure(QSettings().value("onboarding/completed").toBool(),
                           "Tutorial completion was not persisted");
                    ensure(window.screen()->grabWindow(window.winId()).save(directory + "/home-empty.png"),
                           "Home capture failed");
                    QTimer acceptRoom;
                    QObject::connect(&acceptRoom, &QTimer::timeout, &window, [] {
                        if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget()))
                            if (auto *buttons = dialog->findChild<QDialogButtonBox *>())
                                QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
                    });
                    acceptRoom.start(50);
                    auto *create = window.findChild<QPushButton *>("homeNewProject");
                    ensure(create, "New project home action missing");
                    std::cout << "EXPERIENCE_STEP: new project\n" << std::flush;
                    QTest::mouseClick(create, Qt::LeftButton);
                    std::cout << "EXPERIENCE_STEP: room created\n" << std::flush;
                    acceptRoom.stop();
                    ensure(std::count_if(window.editor().document().entities.begin(),
                                         window.editor().document().entities.end(),
                                         [](const auto &e) { return e.type == "Room"; }) == 1,
                           "Home new project did not create a real room");
                    QTimer acceptFile;
                    QObject::connect(&acceptFile, &QTimer::timeout, &window, [directory] {
                        if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                            dialog->setOption(QFileDialog::DontConfirmOverwrite, true);
                            // Changing directories resets selection while the file model loads.
                            // Let that complete before entering a filename and accepting.
                            if (dialog->directory().absolutePath() != directory) {
                                dialog->setDirectory(directory);
                                return;
                            }
                            auto *filename = dialog->findChild<QLineEdit *>("fileNameEdit");
                            if (!filename)
                                return;
                            filename->setFocus();
                            filename->selectAll();
                            QTest::keyClicks(filename, "Meu apartamento.lmx");
                            QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
                        }
                    });
                    acceptFile.start(50);
                    std::cout << "EXPERIENCE_STEP: saving project\n" << std::flush;
                    ensure(window.saveProject(), "Save from first project failed");
                    acceptFile.stop();
                    auto restored = lmx::ProjectStore::open(directory + "/Meu apartamento.lmx");
                    ensure(restored.serialize() == window.editor().document().serialize(),
                           "Single file project changed after reopening");
                    ensure(restored.name == "Meu apartamento", "Saved project name missing");
                    QMetaObject::invokeMethod(&window, "showHome", Qt::DirectConnection);
                    auto *cards = window.findChild<QListWidget *>("projectCards");
                    ensure(cards && cards->count() == 1, "Saved project did not appear at home");
                    ensure(window.screen()->grabWindow(window.winId()).save(directory + "/home-projects.png"),
                           "Recent projects capture failed");
                    lmx::MainWindow restarted(true, {}, true, experienceRoot);
                    restarted.resize(900, 650);
                    restarted.show();
                    QTest::qWait(1100);
                    auto *restartedCards = restarted.findChild<QListWidget *>("projectCards");
                    ensure(restartedCards && restartedCards->count() == 1,
                           "Project library did not survive a fresh window");
                    ensure(!restarted.findChild<QWizard *>("firstRunTutorial"),
                           "Completed first-run tutorial reopened automatically");
                    ensure(
                        restarted.screen()->grabWindow(restarted.winId()).save(directory + "/home-900.png"),
                        "Compact home capture failed");
                    auto *help = restarted.findChild<QPushButton *>("homeTutorial");
                    QTest::mouseClick(help, Qt::LeftButton);
                    ensure(restarted.findChild<QWizard *>("firstRunTutorial"),
                           "Tutorial cannot be reopened from home");
                    auto *reopened = restarted.findChild<QWizard *>("firstRunTutorial");
                    QTest::qWait(50);
                    ensure(reopened->currentPage(), "Reopened tutorial has no active page");
                    auto *topic = reopened->currentPage()->findChild<QComboBox *>("tutorialTopics");
                    ensure(topic, "Reopened tutorial has no topic index");
                    topic->setFocus();
                    QTest::keyClick(topic, Qt::Key_Space);
                    QTest::keyClick(topic, Qt::Key_End);
                    QTest::keyClick(topic, Qt::Key_Return);
                    QTest::qWait(50);
                    ensure(reopened->currentId() == chapterCount - 1,
                           "Keyboard topic selection cannot jump to a chapter");
                    topic = reopened->currentPage()->findChild<QComboBox *>("tutorialTopics");
                    topic->setFocus();
                    QTest::keyClick(topic, Qt::Key_Space);
                    QTest::keyClick(topic, Qt::Key_Home);
                    QTest::keyClick(topic, Qt::Key_Return);
                    QTest::qWait(50);
                    ensure(reopened->currentId() == 0, "Topic selection cannot return to the introduction");
                    std::cout << "EXPERIENCE_PASS: logo opening, 21 tutorial chapters, keyboard topic jumps, "
                                 "compact illustrated lessons, lazy home, new room, "
                                 "single .lmx save/open, durable project cards, 900 px home\n";
                    app.exit(0);
                } catch (const std::exception &error) {
                    std::cerr << "EXPERIENCE_FAIL: " << error.what() << '\n';
                    app.exit(1);
                }
            });
        }
        if (parser.isSet("modern-smoke")) {
            const auto directory = QDir(parser.value("modern-smoke")).absolutePath();
            QDir().mkpath(directory);
            QTimer::singleShot(1500, &window, [&window, directory, &app] {
                try {
                    QSignalSpy failures(window.cad(), &lmx::CadView::failure);
                    if (!QMetaObject::invokeMethod(&window, "modernApartmentStarter", Qt::DirectConnection))
                        throw std::runtime_error("Modern apartment unavailable");
                    auto *list = window.findChild<QListWidget *>("assetList");
                    if (!list || list->count() != 64)
                        throw std::runtime_error("Modern collection filter failed");
                    for (int attempt = 0; attempt < 300; ++attempt) {
                        int ready = 0;
                        for (int i = 0; i < list->count(); ++i)
                            ready += list->item(i)->data(Qt::UserRole + 2).toBool();
                        if (ready == 64)
                            break;
                        if (attempt == 299)
                            throw std::runtime_error("Modern geometry thumbnails missing");
                        QTest::qWait(100);
                    }
                    // Exercise detailed meshes through the actual viewport drop path.
                    lmx::Document placementDocument;
                    lmx::addRectangularRoom(placementDocument, 7000, 6000, 2700, 120);
                    window.editor().load(placementDocument);
                    auto *cad = window.cad();
                    cad->setTop(true);
                    cad->frame();
                    QTest::qWait(100);
                    auto drop = [&](const QString &id, double x, double y) {
                        QMimeData mime;
                        mime.setData("application/x-libremax-asset", id.toUtf8());
                        auto point = cad->project(x, y);
                        QDragEnterEvent enter(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
                        QApplication::sendEvent(cad, &enter);
                        QDragMoveEvent move(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
                        QApplication::sendEvent(cad, &move);
                        QDropEvent event(QPointF(point), Qt::CopyAction, &mime, Qt::LeftButton,
                                         Qt::NoModifier);
                        QApplication::sendEvent(cad, &event);
                        if (!event.isAccepted())
                            throw std::runtime_error("Detailed mesh drop rejected available space");
                        return window.editor().document().entities.back();
                    };
                    const auto cabinet = drop("modern-modern_wooden_cabinet", 2000, 90);
                    if (std::abs(cabinet.transform.y - 62) > 0.2)
                        throw std::runtime_error("Modern cabinet did not attach to wall");
                    const auto table = drop("modern-side_table_01", 2000, 2200);
                    const auto vase = drop("modern-ceramic_vase_02", 2000, 2200);
                    if (std::abs(vase.transform.z - table.height - 2) > 0.2)
                        throw std::runtime_error("Modern vase did not follow table height");
                    const auto lamp = drop("modern-modern_ceiling_lamp_01", 3000, 3000);
                    if (std::abs(lamp.transform.z + lamp.height - 2680) > 0.2)
                        throw std::runtime_error("Modern pendant did not follow ceiling height");
                    const auto mounted = window.editor().document().serialize();
                    window.editor().history.undo();
                    if (window.editor().document().contains(lamp.id))
                        throw std::runtime_error("Modern insertion undo failed");
                    window.editor().history.redo();
                    if (window.editor().document().serialize() != mounted)
                        throw std::runtime_error("Modern insertion redo lost materials or textures");
                    const auto sofa = drop("current-sofa-compact", 5000, 1500);
                    const auto bed = drop("current-bed-queen", 5000, 3900);
                    const auto mirror = drop("current-oval-mirror", 5000, 90);
                    const auto nightstand = drop("current-nightstand", 6200, 90);
                    if (sofa.width != 2050 || bed.width != 1820 || mirror.transform.z != 950 ||
                        nightstand.transform.z != 450 || mirror.metadata.value("placementWall", "").empty())
                        throw std::runtime_error("Contemporary sofa, bed or wall mounting failed");
                    const auto assembled = directory + "/current-assembly.lmx";
                    lmx::ProjectStore::save(assembled, window.editor().document(), false);
                    if (lmx::ProjectStore::open(assembled).serialize() !=
                        window.editor().document().serialize())
                        throw std::runtime_error("Contemporary placed models lost during round trip");
                    QImage overview(1024, 684, QImage::Format_ARGB32_Premultiplied);
                    overview.fill(QColor("#17171f"));
                    QPainter painter(&overview);
                    painter.setPen(QColor("#eeeaf5"));
                    painter.setFont(QFont("Segoe UI", 10));
                    int index = 0;
                    for (int i = 0; i < list->count(); ++i) {
                        auto *item = list->item(i);
                        if (!item->data(Qt::UserRole).toString().startsWith("current-"))
                            continue;
                        const int x = (index % 4) * 256, y = (index / 4) * 228;
                        painter.drawPixmap(x + 32, y + 12, item->icon().pixmap(192, 144));
                        painter.drawText(QRect(x + 12, y + 164, 232, 52), Qt::AlignHCenter | Qt::TextWordWrap,
                                         item->text());
                        ++index;
                    }
                    painter.end();
                    if (index != 12 || !overview.save(directory + "/current-models.png"))
                        throw std::runtime_error("Contemporary model overview failed");
                    window.editor().history.setClean();
                    if (!QMetaObject::invokeMethod(&window, "modernApartmentStarter", Qt::DirectConnection))
                        throw std::runtime_error("Modern apartment unavailable after placement");
                    auto filename = directory + "/apartamento-moderno.lmx";
                    lmx::ProjectStore::save(filename, window.editor().document(), false);
                    auto reopened = lmx::ProjectStore::open(filename);
                    if (reopened.serialize() != window.editor().document().serialize())
                        throw std::runtime_error("Modern apartment round trip failed");
                    window.editor().load(reopened);
                    window.cad()->setTop(false);
                    window.cad()->frame();
                    QTest::qWait(400);
                    if (failures.count() != 0)
                        throw std::runtime_error("Modern viewport reported a geometry or texture failure");
                    if (!window.screen()->grabWindow(window.winId()).save(directory + "/modern-catalog.png"))
                        throw std::runtime_error("Modern screenshot failed");
                    std::cout << "MODERN_PASS: 64 models, thumbnails, wall/table/ceiling drop, undo/redo, "
                                 "embedded textures, reopen\n";
                    app.exit(0);
                } catch (const std::exception &e) {
                    std::cerr << "MODERN_FAIL: " << e.what() << '\n';
                    app.exit(1);
                }
            });
        }
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
                    ensure(assets && assets->count() == 203, "Expanded model catalog missing");
                    int ready = 0;
                    for (int attempt = 0; attempt < 300; ++attempt) {
                        ready = 0;
                        for (int i = 0; i < assets->count(); ++i)
                            ready += assets->item(i)->data(Qt::UserRole + 2).toBool();
                        if (ready == assets->count())
                            break;
                        QTest::qWait(100);
                    }
                    ensure(ready == 203, "Native thumbnails missing for real models");
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
                    std::cout
                        << "ASSEMBLY_PASS: 203 thumbnails, wall ghost/drop, outside rejection, mouse "
                           "move undo/redo, window wall attachment, ready mesh catalog, 3-room apartment "
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
                    if (QThread::idealThreadCount() <= 4 && !QSettings().contains("performance/editor")) {
                        ensure(window.cad()->performanceMode() == 0 && window.cad()->multisampling() == 0,
                               "Small CPU did not start with the lightweight viewport");
                        std::cout << "LIGHTWEIGHT_START_PASS: " << QThread::idealThreadCount()
                                  << " logical CPUs, textures and MSAA disabled\n";
                    }
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
                    for (int attempt = 0; attempt < 300; ++attempt) {
                        ready = 0;
                        for (int i = 0; i < assets->count(); ++i)
                            ready += assets->item(i)->data(Qt::UserRole + 2).toBool();
                        if (ready == assets->count())
                            break;
                        QTest::qWait(100);
                    }
                    ensure(ready == 203, "Shipped asset geometry thumbnails were not generated");
                    std::cout << "THUMBNAILS_PASS: 203 actual geometry previews\n";
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
                    if (inspector)
                        std::cout << "INSPECTOR_COMPACT_GEOMETRY: viewport " << inspector->viewport()->width()
                                  << " content " << inspector->widget()->width() << " minimum "
                                  << inspector->widget()->minimumSizeHint().width() << " horizontal "
                                  << inspector->horizontalScrollBar()->maximum() << '\n';
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
                    const auto untouched = window.editor().document().serialize().dump();
                    const auto fullMesh = lmx::meshSnapshot(window.editor().document()).dump();
                    QSignalSpy profileErrors(window.cad(), &lmx::CadView::failure);
                    for (int mode = 0; mode < 3; ++mode) {
                        auto *choice = window.findChild<QAction *>(QString("performanceMode%1").arg(mode));
                        ensure(choice, "Performance choice unavailable");
                        choice->trigger();
                        QTest::qWait(120);
                        ensure(window.cad()->performanceMode() == mode, "Editor mode did not change");
                        ensure(window.cad()->multisampling() == (mode == 0   ? 0
                                                                 : mode == 1 ? 4
                                                                             : 8),
                               "Editor multisampling did not change");
                        ensure(window.editor().document().serialize().dump() == untouched,
                               "Performance setting altered the saved project");
                        ensure(lmx::meshSnapshot(window.editor().document()).dump() == fullMesh,
                               "Performance setting reduced the render mesh");
                        window.cad()->capture(directory + QString("/performance-%1.png").arg(mode));
                    }
                    ensure(profileErrors.count() == 0, "Viewport failed during performance mode changes");
                    window.findChild<QAction *>("performanceMode1")->trigger();
                    auto *modelFilter = window.findChild<QComboBox *>("libraryCategory");
                    modelFilter->setCurrentIndex(modelFilter->findData("__light"));
                    ensure(assets->count() == 105, "Lightweight collection incomplete");
                    modelFilter->setCurrentIndex(modelFilter->findData("__detail"));
                    ensure(assets->count() == 7, "Detailed collection incomplete");
                    QTest::qWait(150);
                    ensure(
                        window.screen()->grabWindow(window.winId()).save(directory + "/expanded-details.png"),
                        "Detailed catalog screenshot failed");
                    modelFilter->setCurrentIndex(modelFilter->findData("__light"));
                    QTest::qWait(120);
                    ensure(
                        window.screen()->grabWindow(window.winId()).save(directory + "/expanded-light.png"),
                        "Lightweight catalog screenshot failed");
                    modelFilter->setCurrentIndex(0);
                    lmx::Document placementProject;
                    lmx::addRectangularRoom(placementProject, 6000, 5000, 2700, 120);
                    window.editor().load(placementProject);
                    window.cad()->setTop(true);
                    window.cad()->frame();
                    QTest::qWait(150);
                    modelFilter->setCurrentIndex(modelFilter->findData("__light"));
                    auto placeExpanded = [&](const QString &id, double x, double y) {
                        const auto count = window.editor().document().entities.size();
                        window.cad()->beginPlacement(id);
                        const auto pixel = window.cad()->project(x, y);
                        QTest::mouseMove(window.cad(), pixel);
                        QTest::mouseClick(window.cad(), Qt::LeftButton, Qt::NoModifier, pixel);
                        ensure(window.editor().document().entities.size() == count + 1,
                               "Expanded model could not be placed through the native viewport");
                    };
                    placeExpanded("kaykit-couch", 2000, 2000);
                    ensure(window.editor().document().entities.back().type == "MeshObject",
                           "KayKit placement did not create the actual mesh");
                    modelFilter->setCurrentIndex(modelFilter->findData("__detail"));
                    placeExpanded("detail-wall_clock", 3000, 0);
                    const auto &clock = window.editor().document().entities.back();
                    const auto clockWall = clock.metadata.value("placementWall", "");
                    ensure(!clockWall.empty() && window.editor().document().contains(clockWall) &&
                               std::abs(clock.transform.y - 62) < 0.2 &&
                               std::abs(clock.transform.z - 1200) < 0.2,
                           "Detailed wall clock did not attach at the expected wall and height");
                    const auto expandedPath = directory + "/expanded-placement.lmx";
                    lmx::ProjectStore::save(expandedPath, window.editor().document(), false);
                    ensure(lmx::ProjectStore::open(expandedPath).serialize() ==
                               window.editor().document().serialize(),
                           "Placed expanded models did not remain portable");
                    std::cout << "EXPANDED_PLACEMENT_PASS: native KayKit sofa and detailed wall clock, "
                                 "wall attachment, embedded meshes/textures and save/reopen\n";
                    lmx::Document copies;
                    auto cabinet = lmx::entity("FurnitureModule", "Armário");
                    cabinet.width = 800;
                    cabinet.depth = 550;
                    cabinet.height = 720;
                    copies.entities.push_back(cabinet);
                    cabinet.id = lmx::uuid();
                    cabinet.transform.x = 1500;
                    copies.entities.push_back(cabinet);
                    const auto movedId = copies.entities.front().id;
                    window.cad()->setTool("select");
                    window.editor().load(copies);
                    const auto presentations = window.cad()->presentationBuildCount();
                    window.editor().apply("Move repeated cabinet", [&](lmx::Document &d) {
                        d.at(movedId).transform = {2500, 1500, 0, 37, false};
                    });
                    ensure(window.cad()->presentationBuildCount() == presentations,
                           "Moving a repeated cabinet rebuilt its display geometry");
                    auto clickCabinet = [&] {
                        window.cad()->frame();
                        QTest::qWait(100);
                        const auto &e = window.editor().document().at(movedId);
                        const auto angle = e.transform.yaw * std::numbers::pi / 180;
                        const auto pixel = window.cad()->project(
                            e.transform.x + 400 * std::cos(angle) - 275 * std::sin(angle),
                            e.transform.y + 400 * std::sin(angle) + 275 * std::cos(angle), 720);
                        QSignalSpy selected(window.cad(), &lmx::CadView::selected);
                        QTest::mouseMove(window.cad(), pixel);
                        QTest::mouseClick(window.cad(), Qt::LeftButton, Qt::NoModifier, pixel);
                        ensure(!selected.empty() && selected.back().front().toStringList().contains(
                                                        QString::fromStdString(movedId)),
                               "Picking did not follow the moved shared geometry");
                    };
                    clickCabinet();
                    window.editor().history.undo();
                    clickCabinet();
                    window.editor().history.redo();
                    clickCabinet();
                    ensure(window.cad()->presentationBuildCount() == presentations,
                           "Undo/redo rebuilt unchanged repeated geometry");
                    std::cout << "INSTANCE_VIEWPORT_PASS: moved/rotated copies reuse presentation; "
                                 "native picking and undo/redo follow their actual locations\n";
                    std::cout << "PERFORMANCE_PASS: 3 native modes, project and render mesh preserved, "
                                 "105 lightweight models and 7 detailed additions\n";
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
