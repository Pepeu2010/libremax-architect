#include "lighting_acceptance.h"
#include "main_window.h"
#include "persistence/project_store.h"
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <array>
#include <cmath>
#include <iostream>
namespace lmx {
namespace {
void ensure(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
struct State {
    int phase = 0, ticks = 0;
    std::vector<QString> jobs;
    std::vector<Entity> lights;
    QElapsedTimer elapsed;
};
Json descriptor(const QString &filename) {
    QFile log(filename);
    ensure(log.open(QIODevice::ReadOnly), "Light engine log missing");
    for (const auto &line : log.readAll().split('\n')) {
        const auto marker = line.indexOf("LIBREMAX_LIGHT ");
        if (marker >= 0)
            return Json::parse(line.mid(marker + 15).toStdString());
    }
    throw std::runtime_error("Actual Cycles light descriptor missing");
}
std::array<double, 3> meanColor(const QImage &image) {
    ensure(!image.isNull(), "Light image missing");
    std::array<double, 3> color{};
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x) {
            const auto pixel = image.pixelColor(x, y);
            color[0] += pixel.red();
            color[1] += pixel.green();
            color[2] += pixel.blue();
        }
    for (auto &channel : color)
        channel /= image.width() * image.height();
    return color;
}
} // namespace
void startLightingAcceptance(MainWindow &window, QApplication &app, const QString &output,
                             const QString &blender) {
    const auto directory = QDir(output).absolutePath();
    QDir().mkpath(directory);
    auto state = std::make_shared<State>();
    state->elapsed.start();
    auto *poll = new QTimer(&window);
    poll->setInterval(50);
    QObject::connect(poll, &QTimer::timeout, &window, [&window, &app, directory, blender, state, poll] {
        poll->stop();
        try {
            ensure(state->elapsed.elapsed() < 240000, "Lighting acceptance timed out");
            auto &editor = window.editor();
            auto &queue = window.renderQueue();
            if (state->phase == 0) {
                Document document;
                addRectangularRoom(document, 4000, 4000, 2700, 120);
                editor.load(document);
                window.resize(900, 700);
                window.cad()->setTop(false);
                window.cad()->frame();
                for (const auto &kind : {"led", "point", "spot", "area", "sun"}) {
                    std::string dialogError;
                    QTimer::singleShot(50, &window, [&window, directory, kind, &dialogError] {
                        auto *dialog = window.findChild<QDialog *>("newLightDialog");
                        try {
                            ensure(dialog && dialog->isVisible(), "Native light dialog missing");
                            auto *type = dialog->findChild<QComboBox *>("newLightKind");
                            type->setCurrentIndex(type->findData(QString(kind)));
                            if (QString(kind) == "led") {
                                ensure(dialog->findChild<QDoubleSpinBox *>("newLightLength")->value() == 120,
                                       "LED dimensions not presented in centimeters");
                                QTest::qWait(100);
                                window.screen()
                                    ->grabWindow(dialog->winId())
                                    .save(directory + "/new-led-dialog.png");
                            }
                            QTest::mouseClick(dialog->findChild<QDialogButtonBox *>("newLightButtons")
                                                  ->button(QDialogButtonBox::Ok),
                                              Qt::LeftButton);
                        } catch (const std::exception &error) {
                            dialogError = error.what();
                            if (dialog)
                                dialog->reject();
                        }
                    });
                    ensure(QMetaObject::invokeMethod(&window, "createLight", Qt::DirectConnection),
                           "Create light action unavailable");
                    ensure(dialogError.empty(), dialogError.c_str());
                    const auto light = editor.document().entities.back();
                    ensure(light.type == "Light" && light.parameters.at("kind") == kind &&
                               editor.document().version == 3,
                           "Native light creation lost type or format version");
                    auto *advanced = window.findChild<QCheckBox *>("advancedProperties");
                    advanced->setChecked(true);
                    if (QString(kind) == "led") {
                        auto *tone = window.findChild<QComboBox *>("lightTone");
                        tone->setCurrentIndex(tone->findData(0));
                        window.findChild<QSpinBox *>("lightKelvin")->setValue(3400);
                        window.findChild<QLineEdit *>("sizeField")->setText("160");
                        window.findChild<QLineEdit *>("sizeYField")->setText("1,5");
                    }
                    if (QString(kind) == "area") {
                        auto *shape = window.findChild<QComboBox *>("lightShape");
                        shape->setCurrentIndex(shape->findData("RECTANGLE"));
                        window.findChild<QLineEdit *>("sizeField")->setText("120");
                        window.findChild<QLineEdit *>("sizeYField")->setText("35");
                    }
                    if (QString(kind) == "point" || QString(kind) == "spot")
                        window.findChild<QLineEdit *>("radiusField")->setText("7");
                    if (QString(kind) == "spot") {
                        window.findChild<QLineEdit *>("angleField")->setText("60");
                        window.findChild<QLineEdit *>("blendField")->setText("0,34");
                    }
                    if (QString(kind) == "sun")
                        window.findChild<QLineEdit *>("sunAngleField")->setText("0,75");
                    window.findChild<QLineEdit *>("yawField")->setText("35");
                    auto *apply = window.findChild<QPushButton *>("applyProperties");
                    auto *scroll = window.findChild<QScrollArea *>("inspectorScroll");
                    scroll->ensureWidgetVisible(apply);
                    QTest::qWait(80);
                    ensure(scroll->horizontalScrollBar()->maximum() == 0,
                           "Lighting inspector has horizontal overflow");
                    QTest::mouseClick(apply, Qt::LeftButton);
                    const auto edited = editor.document().serialize();
                    ensure(editor.document().at(light.id).transform.yaw == 35,
                           "Light orientation edit failed");
                    editor.history.undo();
                    editor.history.redo();
                    ensure(editor.document().serialize() == edited, "Light editing undo/redo lost data");
                    state->lights.push_back(editor.document().at(light.id));
                    if (QString(kind) == "led") {
                        ensure(state->lights.back().parameters.at("temperature") == 3400 &&
                                   state->lights.back().parameters.at("size") == 1600 &&
                                   state->lights.back().parameters.at("sizeY") == 15,
                               "LED temperature or dimensions changed during editing");
                        advanced->setChecked(false);
                        scroll->verticalScrollBar()->setValue(0);
                        QTest::qWait(80);
                        window.screen()
                            ->grabWindow(window.winId())
                            .save(directory + "/led-inspector-900.png");
                    }
                }
                ProjectStore::save(directory + "/five-lights.lmx", editor.document(), false);
                ensure(ProjectStore::open(directory + "/five-lights.lmx").serialize() ==
                           editor.document().serialize(),
                       "Five light project not portable");
                std::cout << "LIGHT_UI_PASS: five native creation dialogs, Kelvin, dimensions in cm, "
                             "rectangular area, radius, sun angle, rotation, undo/redo and v3 save/reopen\n"
                          << std::flush;
                Document fixture;
                fixture.version = 3;
                fixture.name = "Verificação de iluminação";
                auto floor = entity("GeometryObject", "Superfície branca");
                floor.width = floor.depth = 4000;
                floor.height = 20;
                floor.transform.z = -20;
                fixture.entities.push_back(floor);
                auto camera = entity("Camera", "Câmera de teste");
                camera.transform = {2000, -4500, 2600, 0, false};
                camera.parameters = {{"target", {2000, 2000, 0}}, {"lens", 35}, {"fstop", 64}};
                fixture.entities.push_back(camera);
                fixture.renderSettings["camera"] = camera.id;
                fixture.renderSettings["environmentStrength"] = 0.0;
                fixture.renderSettings["backgroundColor"] = {0, 0, 0};
                for (int i = 0; i < 7; ++i) {
                    auto variant = fixture;
                    auto light = state->lights[i < 3 ? 0 : i - 2];
                    light.transform = {2000, 2000, 2200, 35, false};
                    light.parameters["target"] = {2000, 2000, 0};
                    if (i < 3) {
                        light.parameters["power"] = i == 2 ? 0 : 20;
                        light.parameters["temperature"] = i == 1 ? 6500 : 3000;
                    }
                    variant.entities.push_back(light);
                    if (i == 0)
                        ProjectStore::save(directory + "/led-render.lmx", variant, false);
                    editor.load(variant);
                    window.findChild<QComboBox *>("renderQuality")->setCurrentIndex(3);
                    for (const auto &[key, value] :
                         std::map<QString, int>{{"width", 160}, {"height", 90}, {"samples", 16}})
                        window.findChild<QSpinBox *>("render_" + key)->setValue(value);
                    window.findChild<QComboBox *>("renderFormat")->setCurrentText(i == 6 ? "EXR" : "PNG");
                    window.findChild<QLineEdit *>("blenderPath")->setText(blender);
                    window.findChild<QComboBox *>("renderDevice")->setCurrentIndex(1);
                    window.findChild<QPushButton *>("startRender")->click();
                    const auto entries = queue.entries(variant.id);
                    ensure(entries.size() == state->jobs.size() + 1, "UI did not submit lighting render");
                    state->jobs.push_back(QString::fromStdString(entries.back().at("id").get<std::string>()));
                    window.findChild<QPushButton *>("galleryBack")->click();
                }
                editor.apply("Edit during light rendering",
                             [](Document &d) { d.entities.back().parameters["temperature"] = 7999; });
                state->phase = 1;
            } else if (state->phase == 1 && !queue.busy()) {
                std::vector<std::array<double, 3>> colors;
                for (int i = 0; i < 7; ++i) {
                    const auto record = queue.entries(editor.document().id)[i];
                    QFile::copy(queue.logPath(state->jobs[i]), directory + QString("/light-%1.log").arg(i));
                    if (record.at("state") != "Completed")
                        std::cerr << "LIGHT_FAILED_RECORD " << record.dump() << '\n';
                    ensure(record.at("state") == "Completed" && record.at("engine").at("device") == "CPU",
                           "Actual lighting render failed");
                    const auto snapshot = ProjectStore::open(queue.snapshotPath(state->jobs[i]));
                    ensure(QFileInfo::exists(QFileInfo(queue.snapshotPath(state->jobs[i]))
                                                 .dir()
                                                 .filePath("../scripts/cycles_lights.py")),
                           "Queued light translator missing");
                    ensure(snapshot.version == 3 &&
                               snapshot.entities.back().parameters.at("temperature") != 7999,
                           "Lighting snapshot mutated or v3 downgraded");
                    const auto translated = descriptor(queue.logPath(state->jobs[i]));
                    ensure(translated.at("kind") == snapshot.entities.back().parameters.at("kind"),
                           "Cycles light type changed");
                    ensure(translated.at("temperature") ==
                               snapshot.entities.back().parameters.at("temperature"),
                           "Kelvin not passed to Cycles");
                    ensure(translated.at("rotationZ") == 35, "Cycles light rotation changed");
                    if (i < 3) {
                        ensure(translated.at("type") == "EMISSIVE_MESH" &&
                                   std::abs(translated.at("size").get<double>() - 1.6) < .001 &&
                                   std::abs(translated.at("sizeY").get<double>() - .015) < .001,
                               "LED did not become continuous correctly-sized emission");
                    } else if (i == 3 || i == 4)
                        ensure(std::abs(translated.at("radius").get<double>() - .07) < .001,
                               "Light radius not converted to meters");
                    else if (i == 5)
                        ensure(translated.at("shape") == "RECTANGLE" &&
                                   std::abs(translated.at("sizeY").get<double>() - .35) < .001,
                               "Area light lost rectangular dimensions");
                    else
                        ensure(translated.at("type") == "SUN" &&
                                   std::abs(translated.at("angle").get<double>() -
                                            .75 * 3.141592653589793 / 180) < .001,
                               "Sun angle not converted");
                    QImage image(queue.displayPath(state->jobs[i]));
                    ensure(image.size() == QSize(160, 90), "Lighting preview dimensions changed");
                    colors.push_back(meanColor(image));
                    image.save(directory + QString("/light-%1.png").arg(i));
                    std::cout << "LIGHT_PIXELS " << i << " " << colors.back()[0] << " " << colors.back()[1]
                              << " " << colors.back()[2] << " " << translated.dump() << '\n';
                }
                ensure(colors[0][0] > colors[2][0] + 3, "LED emission did not illuminate the scene");
                ensure(colors[0][0] / std::max(1.0, colors[0][2]) >
                           1.15 * colors[1][0] / std::max(1.0, colors[1][2]),
                       "Kelvin did not change actual reflected light color");
                for (int i = 3; i < 7; ++i)
                    ensure(colors[i][0] > 3, "Light type did not illuminate the floor");
                std::cout << "LIGHTING_ACCEPTANCE_PASS: seven real Cycles CPU renders, LED off/warm/cold "
                             "pixel comparison, point/spot/area/sun conversion, original immutable v3 "
                             "snapshots, EXR and editor ticks "
                          << state->ticks << '\n'
                          << std::flush;
                app.exit(0);
                return;
            }
            ++state->ticks;
            poll->start();
        } catch (const std::exception &error) {
            std::cerr << "LIGHTING_ACCEPTANCE_FAILED: " << error.what() << '\n' << std::flush;
            app.exit(1);
        }
    });
    poll->start(1500);
}
} // namespace lmx
