#include "main_window.h"
#include "persistence/project_store.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("libremax");
    QApplication::setOrganizationName("LibreMax");
    QApplication::setApplicationVersion("0.1.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("LibreMax Architect — native interior design");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"ui-smoke", "Run native UI acceptance and save real screenshots", "directory"});
    parser.addOption({"examples", "Generate valid sample .lmx projects", "directory"});
    parser.addOption({"render-smoke", "Run real Cycles through asynchronous QProcess pipeline", "directory"});
    parser.addOption({"recovery-smoke", "Kill a child process and verify recovery in a fresh process"});
    parser.addOption({"recovery-fixture", "Internal crash acceptance writer", "directory"});
    parser.addOption({"recovery-verify", "Internal crash acceptance reader", "directory"});
    parser.addOption({"blender", "Blender executable for render acceptance", "executable"});
    parser.addPositionalArgument("project", ".lmx project to open");
    parser.process(app);
    bool test = parser.isSet("ui-smoke") || parser.isSet("examples") || parser.isSet("render-smoke") ||
                parser.isSet("recovery-smoke") || parser.isSet("recovery-fixture") ||
                parser.isSet("recovery-verify");
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
            lmx::ProjectStore::save(dir.filePath("cozinha.lmx"), lmx::kitchenExample(), false);
            lmx::ProjectStore::save(dir.filePath("dormitorio.lmx"), lmx::bedroomExample(), false);
            return 0;
        }
        if (parser.isSet("render-smoke")) {
            const auto directory = QDir(parser.value("render-smoke")).absolutePath();
            QDir().mkpath(directory);
            auto document = lmx::kitchenExample();
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
                if (image.size() != QSize(320, 180)) {
                    std::cerr << "Invalid render dimensions\n";
                    app.exit(1);
                    return;
                }
                std::cout << "RENDER_SMOKE_PASS: real Cycles, CPU, denoise, 320x180 image\n";
                QFile file(path);
                if (!file.open(QIODevice::ReadOnly)) {
                    std::cerr << "Unable to verify the completed render\n";
                    app.exit(1);
                    return;
                }
                successfulHash = QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
                checkingFailure = true;
                std::erase_if(document.entities, [](const auto &e) { return e.type == "Camera"; });
                job.start(document, parser.value("blender"),
                          QStringLiteral(LMX_SOURCE_DIR) + "/scripts/cycles_render.py", path, 320, 180, 16,
                          "CPU");
            });
            QTimer::singleShot(180000, &app, [&] {
                job.cancel();
                std::cerr << "Render acceptance timeout\n";
                app.exit(1);
            });
            job.start(document, parser.value("blender"),
                      QStringLiteral(LMX_SOURCE_DIR) + "/scripts/cycles_render.py",
                      directory + "/cycles-kitchen.png", 320, 180, 16, "CPU");
            return app.exec();
        }
        QApplication::setStyle("Fusion");
        app.setFont(QFont("Segoe UI", 10));
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
            QTimer::singleShot(1500, &window, [&window, directory, &app] {
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
                    auto *asset = assets->item(0);
                    auto assetId = asset->data(Qt::UserRole).toString();
                    QTest::mouseClick(assets->viewport(), Qt::LeftButton, Qt::NoModifier,
                                      assets->visualItemRect(asset).center());
                    QTest::mouseDClick(assets->viewport(), Qt::LeftButton, Qt::NoModifier,
                                       assets->visualItemRect(asset).center());
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
                               dropped.transform.x == 1000 && dropped.transform.y == 1000,
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
                    field->setText("753+59,5");
                    auto *button = window.findChild<QPushButton *>("applyProperties");
                    ensure(button, "Missing apply button");
                    QTest::mouseClick(button, Qt::LeftButton);
                    ensure(window.editor().document().at(it->id).width == 812.5,
                           "Numeric expression resize failed");
                    window.editor().history.undo();
                    ensure(window.editor().document().at(it->id).width == 600, "Resize undo failed");
                    window.editor().history.redo();
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
                    std::cout << "UI_SMOKE_PASS: wall draw, library double-click and drop/ghost, "
                                 "undo/redo, millimetric expression edit, "
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
