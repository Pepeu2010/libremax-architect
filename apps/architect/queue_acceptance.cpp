#include "queue_acceptance.h"
#include "main_window.h"
#include "persistence/project_store.h"
#include "rendering/render_progress.h"
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QScreen>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QToolBar>
#include <iostream>
namespace lmx {
namespace {
void ensure(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
QString id(const Json &entry) {
    return QString::fromStdString(entry.at("id").get<std::string>());
}
struct Acceptance {
    QString cancelled, failed, repeat, original;
    QString movable;
    double originalX = 0;
    std::vector<QString> cameraIds;
    std::vector<QString> started;
    QString previousActive;
    QString activeCancel;
    int phase = 0, ticks = 0;
    qint64 observedElapsed = -1;
    QElapsedTimer elapsed;
};
} // namespace
void startQueueAcceptance(MainWindow &window, QApplication &app, const QString &output,
                          const QString &blender, const QString &script) {
    const auto directory = QDir(output).absolutePath();
    QDir().mkpath(directory);
    auto state = std::make_shared<Acceptance>();
    state->elapsed.start();
    auto *poll = new QTimer(&window);
    poll->setInterval(50);
    QObject::connect(
        poll, &QTimer::timeout, &window, [&window, &app, directory, blender, script, state, poll] {
            poll->stop();
            try {
                auto &queue = window.renderQueue();
                if (state->elapsed.elapsed() > 240000)
                    throw std::runtime_error("Queue acceptance timed out");
                if (state->phase == 0) {
                    auto document = kitchenExample();
                    auto camera = *std::find_if(document.entities.begin(), document.entities.end(),
                                                [](const auto &e) { return e.type == "Camera"; });
                    std::erase_if(document.entities, [&](const auto &entry) {
                        return entry.type == "Camera" && entry.id != camera.id;
                    });
                    for (int i = 1; i < 5; ++i) {
                        auto copy = camera;
                        copy.id = uuid();
                        copy.name = "Cozinha " + std::to_string(i + 1);
                        copy.transform.x += i * 10;
                        document.entities.push_back(copy);
                    }
                    auto movable =
                        std::find_if(document.entities.begin(), document.entities.end(), [](const auto &e) {
                            return e.type != "Camera" && e.type != "Light" && e.type != "Room" &&
                                   e.type != "Wall" && e.type != "Floor" && e.type != "Ceiling";
                        });
                    ensure(movable != document.entities.end(), "Missing editable test furniture");
                    state->movable = QString::fromStdString(movable->id);
                    state->originalX = movable->transform.x;
                    window.editor().load(document);
                    auto *quality = window.findChild<QComboBox *>("renderQuality");
                    ensure(quality && quality->count() == 4, "Four quality modes missing");
                    quality->setCurrentIndex(3);
                    auto *format = window.findChild<QComboBox *>("renderFormat");
                    auto *transparent = window.findChild<QCheckBox *>("renderTransparent");
                    format->setCurrentText("JPEG");
                    transparent->setChecked(true);
                    ensure(format->currentText() == "PNG", "Transparency did not select PNG");
                    format->setCurrentText("JPEG");
                    ensure(!transparent->isChecked(), "JPEG kept unsupported transparency");
                    format->setCurrentText("PNG");
                    for (const auto &[name, value] :
                         std::map<QString, int>{{"width", 160}, {"height", 90}, {"samples", 8}})
                        window.findChild<QSpinBox *>("render_" + name)->setValue(value);
                    window.findChild<QLineEdit *>("blenderPath")->setText(blender);
                    window.findChild<QComboBox *>("renderDevice")->setCurrentIndex(1);
                    document = window.editor().document();
                    for (const auto &entry : document.entities)
                        if (entry.type == "Camera")
                            state->cameraIds.push_back(QString::fromStdString(entry.id));
                    auto *all = window.findChild<QPushButton *>("renderAllCameras");
                    ensure(all && all->isEnabled(), "Batch camera action missing");
                    auto *dock = window.findChild<QDockWidget *>("renderDock");
                    dock->show();
                    dock->raise();
                    window.findChild<QScrollArea *>("renderScroll")->ensureWidgetVisible(all);
                    QTest::qWait(100);
                    QTest::mouseClick(all, Qt::LeftButton);
                    auto entries = queue.entries(document.id);
                    std::cout << "QUEUE_BATCH_DIAGNOSTIC: " << entries.size() << " entries, button visible "
                              << all->isVisible() << ", rect " << all->width() << "x" << all->height() << '\n'
                              << std::flush;
                    window.screen()->grabWindow(window.winId()).save(directory + "/batch-diagnostic.png");
                    ensure(entries.size() == 5, "Batch did not enqueue five cameras");
                    state->original = id(entries[0]);
                    state->cancelled = id(entries[1]);
                    queue.cancel(state->cancelled);
                    QTest::mouseClick(window.findChild<QPushButton *>("galleryBack"), Qt::LeftButton);
                    window.editor().apply("Editar durante render", [&](Document &d) {
                        d.at(state->movable.toStdString()).transform.x += 80;
                    });
                    ensure(window.editor().document().at(state->movable.toStdString()).transform.x ==
                               state->originalX + 80,
                           "Editing blocked by queued images");
                    auto options = renderPreset("custom");
                    options["width"] = 160;
                    options["height"] = 90;
                    options["samples"] = 8;
                    options["device"] = "CPU";
                    QFile broken(directory + "/intentional-failure.py");
                    ensure(broken.open(QIODevice::WriteOnly), "Cannot create negative test fixture");
                    broken.write("raise RuntimeError('Intentional queue error fixture')\n");
                    broken.close();
                    state->failed = queue.enqueue(
                        RenderSnapshot(document, options, state->cameraIds.front().toStdString()), blender,
                        broken.fileName());
                    state->phase = 1;
                    std::cout
                        << "QUEUE_EDIT_PASS: five cameras, queued cancellation, furniture edit preserved\n"
                        << std::flush;
                }
                ++state->ticks;
                const auto active = queue.active();
                if (!active.isEmpty() && active != state->previousActive) {
                    state->started.push_back(active);
                    state->previousActive = active;
                }
                if (queue.busy() && state->phase != 3) {
                    poll->start();
                    return;
                }
                if (state->phase == 1) {
                    const auto entries = queue.entries(window.editor().document().id);
                    ensure(entries.size() == 6, "Queue history changed size");
                    int completed = 0, cancelled = 0, failed = 0;
                    for (const auto &entry : entries) {
                        if (entry.at("state") == "Completed") {
                            ++completed;
                            ensure(entry.value("total", 0) == 8 && entry.value("sample", -1) >= 0,
                                   "Real Cycles sample progress missing");
                            QImage image(queue.imagePath(id(entry)));
                            ensure(image.size() == QSize(160, 90), "Queue output resolution incorrect");
                            ensure(entry.contains("engine") && entry.at("engine").contains("blender"),
                                   "Engine provenance missing");
                            ensure(entry.value("elapsedMs", qint64{0}) > 0 &&
                                       entry.value("remainingMs", qint64{0}) == -1 &&
                                       entry.at("progress") == 100,
                                   "Completed timing was not frozen or estimate remained active");
                        } else if (entry.at("state") == "Cancelled")
                            ++cancelled;
                        else if (entry.at("state") == "Failed")
                            ++failed;
                        auto snapshot = ProjectStore::open(queue.snapshotPath(id(entry)));
                        if (id(entry) != state->failed)
                            ensure(snapshot.at(state->movable.toStdString()).transform.x == state->originalX,
                                   "Queued snapshot changed after furniture edit");
                    }
                    ensure(completed == 4 && cancelled == 1 && failed == 1,
                           "Incorrect completion/cancellation/error states");
                    std::vector<QString> expected;
                    for (const auto &entry : entries)
                        if (id(entry) != state->cancelled)
                            expected.push_back(id(entry));
                    ensure(expected == state->started, "Active jobs did not follow FIFO order");
                    state->repeat = queue.retry(state->original, "CPU");
                    state->phase = 2;
                    std::cout << "QUEUE_ORDER_PASS: four real renders, cancelled camera, failed process, "
                                 "immutable snapshots\n"
                              << std::flush;
                    poll->start();
                    return;
                }
                if (state->phase == 2) {
                    const auto entries = queue.entries(window.editor().document().id);
                    auto repeated = std::find_if(entries.begin(), entries.end(),
                                                 [&](const auto &e) { return id(e) == state->repeat; });
                    ensure(repeated != entries.end() && repeated->at("state") == "Completed",
                           "CPU repeat failed");
                    const auto root =
                        QFileInfo(queue.snapshotPath(state->original)).dir().absolutePath() + "/../..";
                    RenderQueue reopened(QDir(root).absolutePath());
                    ensure(reopened.entries(window.editor().document().id).size() == 7,
                           "Gallery history did not survive reopening");
                    ensure(!reopened.busy(), "Completed jobs restarted automatically");
                    ensure(reopened.entries(window.editor().document().id).front().at("elapsedMs") ==
                               entries.front().at("elapsedMs"),
                           "Completed duration did not survive reopening");
                    ProjectStore::save(directory + "/edited-project.lmx", window.editor().document(), false);
                    auto saved = ProjectStore::open(directory + "/edited-project.lmx");
                    ensure(saved.renderSettings.at("renderHistory").size() == 5,
                           "Render metadata not saved in project");
                    ensure(saved.at(state->movable.toStdString()).transform.x == state->originalX + 80,
                           "Rendered snapshot replaced current edit");
                    QMetaObject::invokeMethod(&window, "showRenderGallery", Qt::DirectConnection);
                    auto *images = window.findChild<QListWidget *>("renderImages");
                    ensure(images && images->count() == 7, "Gallery rows missing");
                    QTest::qWait(200);
                    ensure(
                        window.screen()->grabWindow(window.winId()).save(directory + "/render-gallery.png"),
                        "Gallery capture failed");
                    window.resize(900, 650);
                    QTest::qWait(150);
                    ensure(window.screen()
                               ->grabWindow(window.winId())
                               .save(directory + "/render-gallery-900.png"),
                           "Compact gallery capture failed");
                    auto options = renderPreset("custom");
                    options["width"] = 640;
                    options["height"] = 360;
                    options["samples"] = 1024;
                    options["device"] = "CPU";
                    state->activeCancel =
                        queue.enqueue(RenderSnapshot(window.editor().document(), options,
                                                     state->cameraIds.front().toStdString()),
                                      blender, script);
                    state->phase = 3;
                    poll->start();
                    return;
                }
                if (state->phase == 3) {
                    const auto records = queue.entries();
                    const auto active = std::find_if(records.begin(), records.end(), [&](const auto &entry) {
                        return id(entry) == state->activeCancel;
                    });
                    ensure(active != records.end(), "Active progress record missing");
                    if (active->at("state") != "Rendering" || active->value("remainingMs", qint64{-1}) <= 0 ||
                        active->value("progress", -1) <= 0) {
                        poll->start();
                        return;
                    }
                    if (state->observedElapsed < 0) {
                        state->observedElapsed = active->at("elapsedMs").get<qint64>();
                        poll->start();
                        return;
                    }
                    if (active->at("elapsedMs").get<qint64>() - state->observedElapsed < 1000) {
                        poll->start();
                        return;
                    }
                    auto *bar = window.findChild<QProgressBar *>("galleryRenderProgress");
                    auto *timing = window.findChild<QLabel *>("galleryRenderTiming");
                    std::cout << "TIMING_DIAGNOSTIC: " << active->at("state") << " progress "
                              << active->value("progress", -1) << " elapsed " << active->at("elapsedMs")
                              << " remaining " << active->at("remainingMs") << " label "
                              << (timing ? timing->text().toStdString() : "missing") << '\n'
                              << std::flush;
                    ensure(bar && bar->isVisible() && bar->maximum() == 100 && bar->value() < 100,
                           "Active gallery progress bar hidden or premature completion");
                    ensure(timing && timing->text().contains("Decorrido:") &&
                               timing->text().contains("restante estimado:") &&
                               timing->text().contains("aproximadamente"),
                           "Native render timing missing");
                    ensure(window.findChild<QLabel *>("renderTiming")->text() == timing->text(),
                           "Editor and gallery render times differ");
                    window.resize(1440, 900);
                    QTest::qWait(80);
                    window.screen()->grabWindow(window.winId()).save(directory + "/render-progress.png");
                    window.resize(900, 650);
                    QTest::qWait(80);
                    window.screen()->grabWindow(window.winId()).save(directory + "/render-progress-900.png");
                    std::cout << "PROGRESS_LAYOUT: bar " << bar->width() << " timing " << timing->height()
                              << " required " << timing->heightForWidth(timing->width()) << '\n'
                              << std::flush;
                    ensure(bar->width() >= 400 && timing->wordWrap() &&
                               timing->height() >= timing->heightForWidth(timing->width()),
                           "Compact render timing clipped");
                    window.screen()->grabWindow(window.winId()).save(directory + "/render-progress-900.png");
                    std::cout << "RENDER_TIMING_PASS: visible progress, engine ETA, advancing elapsed time, "
                                 "1440/900 px, completed duration persisted; "
                              << timing->text().toStdString() << '\n'
                              << std::flush;
                    auto *images = window.findChild<QListWidget *>("renderImages");
                    for (int i = 0; i < images->count(); ++i)
                        if (images->item(i)->data(Qt::UserRole).toString() == state->activeCancel)
                            images->setCurrentRow(i);
                    auto *cancel = window.findChild<QPushButton *>("galleryCancel");
                    ensure(cancel && cancel->isEnabled(), "Running render cancel action missing");
                    QTest::mouseClick(cancel, Qt::LeftButton);
                    state->phase = 4;
                    poll->start();
                    return;
                }
                if (state->phase == 4) {
                    const auto entries = queue.entries();
                    const auto cancelled =
                        std::find_if(entries.begin(), entries.end(),
                                     [&](const auto &entry) { return id(entry) == state->activeCancel; });
                    ensure(cancelled != entries.end() && cancelled->at("state") == "Cancelled",
                           "Active process cancellation failed");
                    ensure(!QFileInfo::exists(queue.imagePath(state->activeCancel)),
                           "Cancelled job left a completed image");
                    ensure(cancelled->value("elapsedMs", qint64{0}) > 0 &&
                               cancelled->value("remainingMs", qint64{0}) == -1,
                           "Cancelled render kept a live estimate");
                    QTest::mouseClick(window.findChild<QPushButton *>("galleryBack"), Qt::LeftButton);
                    ensure(window.findChild<QWidget *>("workflowSteps")->isVisible() &&
                               window.findChild<QToolBar *>("drawingToolbar")->isVisible() &&
                               window.findChild<QToolBar *>("placementToolbar")->isVisible(),
                           "Returning to the editor lost its tools");
                    std::cout << "QUEUE_PASS: five cameras, FIFO, responsive editing, immutable snapshots, "
                                 "queued and running cancellation, error, CPU retry, durable gallery, "
                                 "project metadata; ticks "
                              << state->ticks << '\n';
                    app.exit(0);
                }
            } catch (const std::exception &error) {
                poll->stop();
                window.renderQueue().cancelAll();
                std::cerr << "QUEUE_FAIL: " << error.what() << '\n';
                app.exit(1);
            }
        });
    poll->start();
}
} // namespace lmx
