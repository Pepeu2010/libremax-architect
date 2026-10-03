#include "environment_acceptance.h"
#include "main_window.h"
#include "persistence/project_store.h"
#include "rendering/high_dynamic_image.h"
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QImage>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <iostream>
namespace lmx {
namespace {
void ensure(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
struct Acceptance {
    int phase = 0, ticks = 0;
    std::vector<QString> jobs;
    double difference = 0;
    QElapsedTimer elapsed;
};
} // namespace
void startEnvironmentAcceptance(MainWindow &window, QApplication &app, const QString &output,
                                const QString &blender) {
    const auto directory = QDir(output).absolutePath();
    QDir().mkpath(directory);
    auto state = std::make_shared<Acceptance>();
    state->elapsed.start();
    auto *poll = new QTimer(&window);
    poll->setInterval(50);
    QObject::connect(poll, &QTimer::timeout, &window, [&window, &app, directory, blender, state, poll] {
        poll->stop();
        try {
            if (state->elapsed.elapsed() >= 240000) {
                window.screen()->grabWindow(window.winId()).save(directory + "/timeout.png");
                throw std::runtime_error("Environment acceptance timed out in phase " +
                                         std::to_string(state->phase));
            }
            auto &editor = window.editor();
            auto &queue = window.renderQueue();
            if (state->phase == 0) {
                auto document = kitchenExample();
                const auto camera = document.renderSettings.at("camera").get<std::string>();
                std::erase_if(document.entities, [&](const Entity &entity) { return entity.id != camera; });
                document.at(camera).transform = {};
                document.at(camera).parameters["target"] = {0, 1000, 0};
                editor.load(document);
                auto *dock = window.findChild<QDockWidget *>("renderDock");
                dock->show();
                dock->raise();
                auto *button = window.findChild<QPushButton *>("useDaylightHdri");
                ensure(button, "Bundled daylight button missing");
                QTest::qWait(100);
                window.findChild<QScrollArea *>("renderScroll")->ensureWidgetVisible(button);
                QTest::qWait(100);
                QSignalSpy clicked(button, &QPushButton::clicked);
                QTest::mouseClick(button, Qt::LeftButton);
                ensure(clicked.count() == 1, "Native daylight button click did not reach its target");
                state->phase = 1;
            } else if (state->phase == 1 &&
                       editor.document().renderSettings.value("environmentMode", "studio") == "hdri") {
                ensure(editor.document().version == 2, "HDRI did not version the project");
                auto document = editor.document();
                document.renderSettings["environmentStrength"] = 1.0;
                document.renderSettings["backgroundColor"] = {0.02, 0.02, 0.02};
                ProjectStore::save(directory + "/portable-hdri.lmx", document, false);
                window.loadProject(directory + "/portable-hdri.lmx");
                auto *quality = window.findChild<QComboBox *>("renderQuality");
                auto *format = window.findChild<QComboBox *>("renderFormat");
                quality->setCurrentIndex(3);
                format->setCurrentText("EXR");
                for (const auto &[key, value] :
                     std::map<QString, int>{{"width", 160}, {"height", 90}, {"samples", 8}})
                    window.findChild<QSpinBox *>("render_" + key)->setValue(value);
                window.findChild<QLineEdit *>("blenderPath")->setText(blender);
                window.findChild<QComboBox *>("renderDevice")->setCurrentIndex(1);
                auto submit = [&] {
                    window.findChild<QPushButton *>("startRender")->click();
                    const auto entries = queue.entries(editor.document().id);
                    ensure(entries.size() == state->jobs.size() + 1, "UI did not submit environment render");
                    state->jobs.push_back(QString::fromStdString(entries.back().at("id").get<std::string>()));
                    window.findChild<QPushButton *>("galleryBack")->click();
                };
                submit();
                auto *rotation = window.findChild<QDoubleSpinBox *>("hdriRotation");
                rotation->setValue(180);
                QMetaObject::invokeMethod(rotation, "editingFinished", Qt::DirectConnection);
                submit();
                format->setCurrentText("PNG");
                window.findChild<QCheckBox *>("hdriVisible")->setChecked(false);
                submit();
                window.findChild<QCheckBox *>("renderTransparent")->setChecked(true);
                submit();
                ensure(queue.busy(), "Environment queue unexpectedly idle");
                editor.apply("Editar enquanto renderiza",
                             [](Document &d) { d.name = "Edited during HDRI render"; });
                window.resize(900, 720);
                auto *dock = window.findChild<QDockWidget *>("renderDock");
                dock->show();
                dock->raise();
                auto *scroll = window.findChild<QScrollArea *>("renderScroll");
                scroll->ensureWidgetVisible(rotation);
                QTest::qWait(100);
                std::cout << "HDRI_COMPACT_LAYOUT: viewport " << scroll->viewport()->width()
                          << ", content minimum " << scroll->widget()->minimumSizeHint().width()
                          << ", horizontal " << scroll->horizontalScrollBar()->maximum() << '\n'
                          << std::flush;
                ensure(rotation->isVisible() && rotation->width() >= rotation->minimumWidth(),
                       "HDRI controls not visible");
                ensure(scroll->horizontalScrollBar()->maximum() == 0,
                       "HDRI controls overflow compact layout");
                window.screen()->grabWindow(window.winId()).save(directory + "/hdri-controls-900.png");
                state->phase = 2;
            } else if (state->phase == 2 && !queue.busy()) {
                const auto entries = queue.entries(editor.document().id);
                ensure(entries.size() == 4, "Environment history incomplete");
                for (const auto &entry : entries) {
                    std::cout << "ENVIRONMENT_JOB: " << entry.dump() << '\n';
                    ensure(entry.at("state") == "Completed", "Real environment render failed");
                    ensure(entry.at("engine").at("device") == "CPU", "CPU provenance missing");
                }
                QImage original(queue.displayPath(state->jobs[0])),
                    rotated(queue.displayPath(state->jobs[1])), hidden(queue.displayPath(state->jobs[2])),
                    transparent(queue.displayPath(state->jobs[3]));
                ensure(original.size() == QSize(160, 90) && rotated.size() == original.size(),
                       "EXR preview invalid");
                double difference = 0;
                int opaquePixels = 0;
                for (int y = 0; y < 90; ++y)
                    for (int x = 0; x < 160; ++x) {
                        difference +=
                            std::abs(original.pixelColor(x, y).red() - rotated.pixelColor(x, y).red());
                        opaquePixels += transparent.pixelColor(x, y).alpha() != 0;
                    }
                difference /= 160 * 90;
                ensure(difference > 5, "HDRI rotation did not change actual pixels");
                ensure(opaquePixels == 0, "Transparent world contains opaque pixels");
                ensure(hidden.pixelColor(5, 5) == hidden.pixelColor(150, 80),
                       "Hidden HDRI still visible to camera");
                for (int i = 0; i < 2; ++i) {
                    QFile file(queue.imagePath(state->jobs[i]));
                    ensure(file.open(QIODevice::ReadOnly), "Original EXR missing");
                    const auto image = inspectHighDynamicImage(file.readAll(), true);
                    ensure(image.format == "exr" && image.size == QSize(160, 90),
                           "Actual Cycles EXR invalid");
                    std::cout << "EXR_FLOAT_PEAK: " << image.peak << '\n';
                    const auto snapshot = ProjectStore::open(queue.snapshotPath(state->jobs[i]));
                    ensure(snapshot.name != editor.document().name, "Mutable document leaked into snapshot");
                    ensure(snapshot.renderSettings.at("hdri").at("rotation") == i * 180,
                           "Queued HDRI rotation changed");
                }
                const auto parent = QFileInfo(queue.snapshotPath(state->jobs[0])).dir();
                RenderQueue reopened(parent.absoluteFilePath("../.."));
                ensure(reopened.entries(editor.document().id).size() == 4, "EXR history did not reopen");
                ensure(QFileInfo::exists(reopened.displayPath(state->jobs[0])) &&
                           QFileInfo(reopened.displayPath(state->jobs[0])).canonicalFilePath() ==
                               QFileInfo(queue.displayPath(state->jobs[0])).canonicalFilePath(),
                       "EXR preview reference not durable");
                for (int i = 0; i < 2; ++i)
                    ensure(QFile::copy(queue.imagePath(state->jobs[i]),
                                       directory + QString("/hdri-%1.exr").arg(i * 180)),
                           "Cannot retain actual EXR evidence");
                window.resize(1440, 900);
                QMetaObject::invokeMethod(&window, "showRenderGallery", Qt::DirectConnection);
                QTest::qWait(100);
                auto *list = window.findChild<QListWidget *>("renderImages");
                list->setCurrentRow(0);
                ensure(!list->item(0)->icon().isNull(), "EXR thumbnail missing");
                QTimer::singleShot(100, &window, [&window, directory] {
                    if (auto *dialog = window.findChild<QFileDialog *>()) {
                        auto *filename = dialog->findChild<QLineEdit *>("fileNameEdit");
                        if (!filename)
                            return;
                        filename->setText(directory + "/saved-copy.exr");
                        QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
                    }
                });
                window.findChild<QPushButton *>("gallerySaveCopy")->click();
                window.findChild<QPushButton *>("galleryOpen")->click();
                QTest::qWait(100);
                window.screen()->grabWindow(window.winId()).save(directory + "/exr-native-preview.png");
                original.save(directory + "/hdri-original.png");
                rotated.save(directory + "/hdri-rotated.png");
                state->difference = difference;
                state->phase = 3;
            } else if (state->phase == 3 && QFileInfo::exists(directory + "/saved-copy.exr")) {
                QFile copy(directory + "/saved-copy.exr"), original(directory + "/hdri-0.exr");
                ensure(copy.open(QIODevice::ReadOnly) && original.open(QIODevice::ReadOnly),
                       "Saved EXR copy unavailable");
                ensure(copy.readAll() == original.readAll(),
                       "Gallery save did not preserve EXR original bytes");
                std::cout << "ENVIRONMENT_ACCEPTANCE_PASS: embedded HDRI, version 2, asynchronous import, "
                             "four real CPU renders, EXR originals and durable gallery previews, native EXR "
                             "save copy, rotation "
                             "mean difference "
                          << state->difference << ", transparent pixels, compact UI, editor ticks "
                          << state->ticks << '\n'
                          << std::flush;
                app.exit(0);
                return;
            }
            ++state->ticks;
            poll->start();
        } catch (const std::exception &error) {
            std::cerr << "ENVIRONMENT_ACCEPTANCE_FAILED: " << error.what() << '\n' << std::flush;
            app.exit(1);
        }
    });
    poll->start();
}
} // namespace lmx
