#include "main_window.h"
#include "commands/arrangement.h"
#include "core/expression.h"
#include "document/room_outline.h"
#include "first_run.h"
#include "geometry/geometry.h"
#include "import/dxf.h"
#include "import/model_import.h"
#include "library/model_pack.h"
#include "materials/texture.h"
#include "persistence/project_store.h"
#include "rendering/environment_map.h"
#include "rendering/light_model.h"
#include "rendering/render_progress.h"
#include "rendering/room_look.h"
#include "resource_paths.h"
#include "studio_theme.h"
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QDrag>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressDialog>
#include <QPushButton>
#include <QResizeEvent>
#include <QSaveFile>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTableWidget>
#include <QThread>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <set>
#include <spdlog/spdlog.h>

namespace lmx {
namespace {
QString q(const std::string &s) {
    return QString::fromStdString(s);
}
std::vector<std::string> ids(const QStringList &list) {
    std::vector<std::string> result;
    for (const auto &id : list)
        result.push_back(id.toStdString());
    return result;
}
class AssetList final : public QListWidget {
  protected:
    void resizeEvent(QResizeEvent *event) override {
        QListWidget::resizeEvent(event);
        const int columns = viewport()->width() >= 240 ? 2 : 1;
        setGridSize(QSize(std::max(110, viewport()->width() / columns - 5), 174));
    }
    void startDrag(Qt::DropActions) override {
        if (!currentItem())
            return;
        auto *drag = new QDrag(this);
        auto *mime = new QMimeData;
        mime->setData("application/x-libremax-asset", currentItem()->data(Qt::UserRole).toString().toUtf8());
        drag->setMimeData(mime);
        drag->exec(Qt::CopyAction);
    }

  public:
    using QListWidget::QListWidget;
};
class PropertyForm final : public QWidget {
  protected:
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        if (auto *form = qobject_cast<QFormLayout *>(layout())) {
            const auto policy = width() < 280 ? QFormLayout::WrapAllRows : QFormLayout::WrapLongRows;
            if (form->rowWrapPolicy() != policy)
                form->setRowWrapPolicy(policy);
        }
    }
};
bool numericDialog(QWidget *parent, const QString &title, const QStringList &labels,
                   std::vector<double> &values) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto *layout = new QFormLayout(&dialog);
    std::vector<QLineEdit *> edits;
    for (int i = 0; i < labels.size(); ++i) {
        auto *edit = new QLineEdit(QString::number(values[static_cast<std::size_t>(i)]));
        edit->setAccessibleName(labels[i]);
        layout->addRow(labels[i], edit);
        edits.push_back(edit);
    }
    auto *error = new QLabel;
    error->setWordWrap(true);
    error->setStyleSheet("color:#ffb4a4");
    layout->addRow(error);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        try {
            for (std::size_t i = 0; i < edits.size(); ++i)
                values[i] = millimeters(evaluate(edits[i]->text().toStdString()));
            dialog.accept();
        } catch (const std::exception &e) {
            error->setText(QString::fromUtf8(e.what()));
        }
    });
    return dialog.exec() == QDialog::Accepted;
}
QString dataRoot() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}
QString resourceFile(const QString &relative) {
    return lmx::resourcePath(relative);
}
} // namespace
MainWindow::MainWindow(bool test, const QString &recoveryDirectory, bool welcome, const QString &testRoot)
    : recovery(recoveryDirectory.isEmpty() ? dataRoot() + "/recovery" : recoveryDirectory), testing(test) {
    setObjectName("mainWindow");
    setWindowTitle(tr("LibreMax Architect"));
    resize(1440, 900);
    setMinimumSize(900, 600);
    if (testing)
        testLibraryDirectory = std::make_unique<QTemporaryDir>();
    const auto localRoot =
        testing ? (testRoot.isEmpty() ? testLibraryDirectory->path() : testRoot) : dataRoot();
    userModelsDirectory = localRoot + "/user-models";
    library = std::make_unique<Library>(localRoot + "/library.db", resourceFile("starter-models"),
                                        userModelsDirectory);
    projects = std::make_unique<ProjectLibrary>(localRoot + "/projects");
    render = std::make_unique<RenderQueue>(localRoot + "/renders");
    QFile catalog(resourceFile("starter-library/catalog.json"));
    if (!catalog.open(QIODevice::ReadOnly))
        throw std::runtime_error("Starter Library não encontrada");
    library->seed(Json::parse(catalog.readAll().toStdString()));
    QFile models(resourceFile("starter-models/catalog.json"));
    if (!models.open(QIODevice::ReadOnly))
        throw std::runtime_error("Catálogo de móveis prontos não encontrado");
    library->seed(Json::parse(models.readAll().toStdString()));
    QFile modern(resourceFile("starter-models/modern-catalog.json"));
    if (!modern.open(QIODevice::ReadOnly))
        throw std::runtime_error("Coleção de apartamentos atuais não encontrada");
    library->seed(Json::parse(modern.readAll().toStdString()));
    QFile current(resourceFile("starter-models/current-catalog.json"));
    if (!current.open(QIODevice::ReadOnly))
        throw std::runtime_error("Coleção contemporânea não encontrada");
    library->seed(Json::parse(current.readAll().toStdString()));
    QFile expanded(resourceFile("starter-models/expanded-catalog.json"));
    if (!expanded.open(QIODevice::ReadOnly))
        throw std::runtime_error("Coleção ampliada não encontrada");
    library->seed(Json::parse(expanded.readAll().toStdString()));
    QFile homeCollection(resourceFile("starter-models/home-catalog.json"));
    if (!homeCollection.open(QIODevice::ReadOnly))
        throw std::runtime_error("Coleção para apartamentos não encontrada");
    library->seed(Json::parse(homeCollection.readAll().toStdString()));
    createShell();
    connect(&thumbnails, &AssetThumbnails::ready, this, [this](const QString &id, const QImage &image) {
        for (int i = 0; i < assets->count(); ++i) {
            auto *item = assets->item(i);
            if (item->data(Qt::UserRole).toString() == id) {
                item->setIcon(QIcon(QPixmap::fromImage(image)));
                item->setData(Qt::UserRole + 2, true);
                break;
            }
        }
    });
    connect(&editor_, &Editor::changed, this, &MainWindow::refreshScene);
    connect(&editor_.history, &QUndoStack::cleanChanged, this,
            [this] { setWindowModified(!editor_.history.isClean()); });
    connect(
        viewport, &CadView::wallCreated, this, [this](double x1, double y1, double x2, double y2, bool half) {
            protect([&] {
                editor_.apply(half ? tr("Criar mureta") : tr("Criar parede"), [&](Document &d) {
                    d.entities.push_back(wall(x1, y1, x2, y2, half ? 1100 : 2700, half ? 100 : 120, half));
                });
            });
        });
    connect(viewport, &CadView::selected, this, &MainWindow::selectIds);
    connect(viewport, &CadView::roomCreated, this, [this](const QPolygonF &polygon) {
        protect([&] {
            Outline outline;
            for (auto p : polygon)
                outline.push_back({p.x(), p.y()});
            editor_.apply(tr("Criar cômodo desenhado"),
                          [&](Document &d) { addPolygonRoom(d, outline, 2700, 120, "Cômodo desenhado"); });
            viewport->frame();
        });
    });
    connect(viewport, &CadView::assetDropped, this,
            [this](const QString &id, const Entity &object) { protect([&] { insertAsset(id, object); }); });
    connect(viewport, &CadView::objectMoved, this, [this](const Entity &object) {
        protect([&] {
            editor_.apply(tr("Mover %1").arg(q(object.name)), [&](Document &d) { d.at(object.id) = object; });
            selectIds({q(object.id)});
            viewport->select({object.id});
        });
    });
    connect(
        viewport, &CadView::placementStatus, this, [this](const QString &text, bool active, bool allowed) {
            placementBanner->setText(
                active ? text : tr("Arraste um móvel para o cômodo. Perto da parede, ele encaixa sozinho."));
            placementBanner->setStyleSheet(!active   ? "color:#dbc3ed;background:#2b2136;padding:10px;"
                                           : allowed ? "color:#a9e5d0;background:#172c29;padding:10px;"
                                                     : "color:#ffc1b6;background:#392420;padding:10px;");
        });
    connect(viewport, &CadView::coordinates, this,
            [this](const QString &text) { statusBar()->showMessage(text); });
    connect(viewport, &CadView::objectsMoved, this, [this](const std::vector<Entity> &objects) {
        protect([&] {
            editor_.apply(tr("Mover conjunto de móveis"), [&](Document &d) {
                for (const auto &object : objects)
                    d.at(object.id) = object;
            });
        });
    });
    connect(viewport, &CadView::failure, this, [this](const QString &text) {
        spdlog::error("Viewport: {}", text.toStdString());
        statusBar()->showMessage(text, 12000);
    });
    viewport->assetResolver([this](const QString &id) -> std::optional<Asset> {
        for (const auto &a : visibleAssets)
            if (a.id == id)
                return library->withPayload(a);
        return std::nullopt;
    });
    connect(render.get(), &RenderQueue::changed, this, &MainWindow::refreshRenderQueue);
    connect(render.get(), &RenderQueue::timingChanged, this, &MainWindow::refreshRenderQueue);
    connect(render.get(), &RenderQueue::log, renderLog, &QPlainTextEdit::appendPlainText);
    connect(render.get(), &RenderQueue::warning, this,
            [this](const QString &message) { statusBar()->showMessage(message, 10000); });
    connect(render.get(), &RenderQueue::completed, this, [this](const QString &id, const QString &) {
        statusBar()->showMessage(tr("Imagem pronta. Abra em Suas imagens."), 8000);
        const auto entries = render->entries(editor_.document().id);
        if (std::none_of(entries.begin(), entries.end(),
                         [&](const auto &entry) { return entry.at("id") == id.toStdString(); }))
            return;
        editor_.apply(tr("Registrar imagens do projeto"), [&](Document &document) {
            auto history = Json::array();
            for (const auto &entry : entries)
                if (entry.at("state") == "Completed")
                    history.push_back({{"id", entry.at("id")},
                                       {"camera", entry.at("camera")},
                                       {"cameraName", entry.at("cameraName")},
                                       {"created", entry.at("created")},
                                       {"options", entry.at("options")},
                                       {"engine", entry.value("engine", Json::object())}});
            document.renderSettings["renderHistory"] = std::move(history);
        });
    });
    refreshRenderQueue();
    connect(&autosaveTimer, &QTimer::timeout, this, [this] { protect([&] { autosave(); }); });
    auto interval = std::clamp(QSettings().value("autosaveMinutes", 5).toInt(), 1, 60);
    autosaveTimer.start(interval * 60 * 1000);
    refreshScene();
    if (testing && !welcome)
        refreshLibrary();
    else {
        showHome();
        if (!QSettings().value("onboarding/seen", false).toBool())
            QTimer::singleShot(900, this, [this] { showTutorial(this); });
    }
}
void MainWindow::protect(const std::function<void()> &operation) {
    try {
        operation();
    } catch (const std::exception &e) {
        spdlog::error("{}", e.what());
        QMessageBox::warning(this, tr("Não foi possível concluir"), QString::fromUtf8(e.what()));
    }
}
void MainWindow::createShell() {
    viewport = new CadView;
    workspace = new QStackedWidget;
    workspace->addWidget(viewport);
    auto *studio = new QWidget;
    auto *studioLayout = new QVBoxLayout(studio);
    studioLayout->setContentsMargins(0, 0, 0, 0);
    studioLayout->setSpacing(0);
    auto *steps = new QWidget;
    steps->setObjectName("workflowSteps");
    auto *stepLayout = new QHBoxLayout(steps);
    stepLayout->setContentsMargins(10, 7, 10, 7);
    auto *roomButton = new QPushButton(tr("1  Cômodo"));
    roomButton->setObjectName("createRoomButton");
    roomButton->setToolTip(tr("Criar um cômodo com suas medidas"));
    auto *furnitureButton = new QPushButton(tr("2  Móveis"));
    furnitureButton->setToolTip(tr("Escolher móveis e decoração"));
    auto *finishButton = new QPushButton(tr("3  Cor"));
    finishButton->setToolTip(tr("Mudar cor e acabamento do item selecionado"));
    auto *photoButton = new QPushButton(tr("4  Foto"));
    photoButton->setToolTip(tr("Criar uma imagem do ambiente"));
    for (auto *button : {roomButton, furnitureButton, finishButton, photoButton}) {
        button->setProperty("role", "quiet");
        button->setStyleSheet("font-size:11px;padding:7px 2px;");
        button->setMinimumWidth(0);
        button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        stepLayout->addWidget(button);
    }
    connect(roomButton, &QPushButton::clicked, this, [this] { protect([&] { newRoom(); }); });
    connect(furnitureButton, &QPushButton::clicked, this, [this] {
        auto *dock = findChild<QDockWidget *>("libraryDock");
        dock->show();
        search->setFocus();
    });
    connect(finishButton, &QPushButton::clicked, this, [this] {
        auto *dock = findChild<QDockWidget *>("propertiesDock");
        dock->show();
        dock->raise();
        material->setFocus();
    });
    connect(photoButton, &QPushButton::clicked, this, [this] {
        auto *dock = findChild<QDockWidget *>("renderDock");
        dock->show();
        dock->raise();
    });
    studioLayout->addWidget(steps);
    studioLayout->addWidget(workspace, 1);
    placementBanner = new QLabel(tr("Arraste um móvel para o cômodo. Perto da parede, ele encaixa sozinho."));
    placementBanner->setObjectName("placementBanner");
    placementBanner->setWordWrap(true);
    placementBanner->setStyleSheet("color:#dbc3ed;background:#2b2136;padding:10px;");
    studioLayout->addWidget(placementBanner);
    editorPage = studio;
    rootPages = new QStackedWidget;
    rootPages->addWidget(editorPage);
    home = new ProjectHome;
    auto *homeScroll = new QScrollArea;
    homeScroll->setWidgetResizable(true);
    homeScroll->setWidget(home);
    rootPages->addWidget(homeScroll);
    setCentralWidget(rootPages);
    auto action = [this](QMenu *menu, const QString &name, const QKeySequence &shortcut,
                         std::function<void()> callback) {
        auto *a = menu->addAction(name);
        a->setShortcut(shortcut);
        a->setToolTip(name + (shortcut.isEmpty() ? QString{} : " (" + shortcut.toString() + ")"));
        connect(a, &QAction::triggered, this, [this, callback] { protect(callback); });
        return a;
    };
    auto *file = menuBar()->addMenu(tr("&Arquivo"));
    auto *newAction = action(file, tr("&Novo projeto"), QKeySequence::New, [this] {
        if (discardOrSave()) {
            enterEditor();
            editor_.load(Document{});
            path.clear();
            selectedIds.clear();
            newRoom();
        }
    });
    auto *openAction = action(file, tr("&Abrir projeto…"), QKeySequence::Open, [this] {
        auto f = QFileDialog::getOpenFileName(this, tr("Abrir projeto"), {}, tr("LibreMax (*.lmx)"));
        if (!f.isEmpty() && discardOrSave())
            loadProject(f);
    });
    auto *saveAction = action(file, tr("&Salvar"), QKeySequence::Save, [this] { saveProject(); });
    action(file, tr("Salvar &como…"), QKeySequence::SaveAs, [this] { saveProject(true); });
    action(file, tr("Importar planta DXF…"), {}, [this] { importDxf(); });
    action(file, tr("Ativar materiais realistas"), {}, [this] { activatePbrMaterials(); });
    action(file, tr("Importar textura JPG/PNG…"), {}, [this] { importTexture(); });
    action(file, tr("Adicionar modelo 3D…"), {}, [this] { importModel(); })->setObjectName("import3DModel");
    action(file, tr("Instalar ou atualizar coleção…"), {}, [this] {
        installCollection();
    })->setObjectName("installModelCollection");
    auto *recent = file->addMenu(tr("Projetos recentes"));
    for (const auto &p : QSettings().value("recent").toStringList())
        action(recent, QFileInfo(p).fileName(), {}, [this, p] {
            if (discardOrSave())
                loadProject(p);
        });
    action(file, tr("Recuperar autosave…"), {}, [this] { recover(); });
    action(file, tr("Intervalo do autosave…"), {}, [this] {
        bool accepted = false;
        auto minutes = QInputDialog::getInt(this, tr("Proteção do projeto"),
                                            tr("Salvar uma cópia a cada quantos minutos? (1–60)"),
                                            autosaveTimer.interval() / 60000, 1, 60, 1, &accepted);
        if (accepted) {
            autosaveTimer.start(minutes * 60000);
            if (!testing)
                QSettings().setValue("autosaveMinutes", minutes);
        }
    });
    file->addSeparator();
    action(file, tr("Sair"), QKeySequence::Quit, [this] { close(); });
    auto *edit = menuBar()->addMenu(tr("&Editar"));
    auto *undo = editor_.history.createUndoAction(this, tr("Desfazer"));
    undo->setShortcut(QKeySequence::Undo);
    edit->addAction(undo);
    auto *redo = editor_.history.createRedoAction(this, tr("Refazer"));
    redo->setShortcut(QKeySequence("Ctrl+Shift+Z"));
    edit->addAction(redo);
    edit->addSeparator();
    action(edit, tr("Duplicar"), QKeySequence("Ctrl+D"), [this] { transform("duplicate"); });
    action(edit, tr("Espelhar"), {}, [this] { transform("mirror"); });
    action(edit, tr("Excluir"), QKeySequence::Delete, [this] { transform("delete"); });
    action(edit, tr("Ocultar / mostrar"), {}, [this] { transform("visibility"); });
    action(edit, tr("Bloquear / desbloquear"), {}, [this] { transform("lock"); });
    edit->addSeparator();
    action(edit, tr("Juntar em conjunto"), QKeySequence("Ctrl+G"), [this] {
        arrangeSelection("group");
    })->setObjectName("groupSelection");
    action(edit, tr("Separar conjunto"), QKeySequence("Ctrl+Shift+G"),
           [this] { arrangeSelection("ungroup"); });
    action(edit, tr("Mover seleção com medidas…"), {}, [this] { moveSelection(); });
    auto *align = edit->addMenu(tr("Alinhar móveis"));
    for (const auto &[label, id] :
         std::vector<std::pair<QString, QString>>{{tr("Pela esquerda"), "left"},
                                                  {tr("Pela direita"), "right"},
                                                  {tr("Pelo fundo"), "back"},
                                                  {tr("Pela frente"), "front"},
                                                  {tr("Centralizar na horizontal"), "centerX"},
                                                  {tr("Centralizar na vertical"), "centerY"},
                                                  {tr("Espaçar na horizontal"), "spaceX"},
                                                  {tr("Espaçar na vertical"), "spaceY"}})
        action(align, label, {}, [this, id] { arrangeSelection(id); });
    auto *environment = menuBar()->addMenu(tr("&Ambiente"));
    action(environment, tr("Adicionar cômodo…"), {}, [this] { newRoom(); });
    action(environment, tr("Desenhar contorno do cômodo"), {}, [this] {
        viewport->setTop(true);
        viewport->setTool("room");
        statusBar()->showMessage(tr("Clique em cada canto; botão direito fecha o cômodo. Esc cancela."));
    })->setObjectName("drawRoomOutline");
    action(environment, tr("Ajustar cantos do cômodo…"), {}, [this] {
        editRoomOutline();
    })->setObjectName("editRoomOutline");
    auto *apartmentAction =
        action(environment, tr("Começar com apartamento de exemplo"), {}, [this] { apartmentStarter(); });
    apartmentAction->setObjectName("apartmentStarter");
    auto *modernAction =
        action(environment, tr("Começar com apartamento moderno"), {}, [this] { modernApartmentStarter(); });
    modernAction->setObjectName("modernApartmentStarter");
    action(environment, tr("Inserir porta na parede…"), {}, [this] { opening(false); });
    action(environment, tr("Inserir janela na parede…"), {}, [this] { opening(true); });
    action(environment, tr("Escada reta…"), {}, [this] {
        std::vector<double> v{900, 2700, 4200, 15};
        if (numericDialog(this, tr("Escada reta"),
                          {tr("Largura (mm)"), tr("Altura (mm)"), tr("Comprimento (mm)"), tr("Degraus")}, v))
            editor_.apply(tr("Criar escada"), [&](Document &d) {
                auto e = entity("Stair", "Escada reta");
                e.width = v[0];
                e.height = v[1];
                e.depth = v[2];
                e.parameters = {{"steps", static_cast<int>(v[3])}};
                d.entities.push_back(e);
            });
    });
    auto *automationMenu = menuBar()->addMenu(tr("&Finalizar móveis"));
    for (const auto &[label, kind] :
         std::vector<std::pair<QString, std::string>>{{tr("Tampo"), "countertop"},
                                                      {tr("Rodatampo"), "backsplash"},
                                                      {tr("Rodapé"), "plinth"},
                                                      {tr("Rodaforro"), "cornice"},
                                                      {tr("Fechamento lateral"), "closure"},
                                                      {tr("Envelopamento"), "envelope"}})
        action(automationMenu, label + "…", {}, [this, kind] { automate(kind); });
    auto *geometryMenu = menuBar()->addMenu(tr("&Peças extras"));
    action(geometryMenu, tr("Volume retangular…"), {}, [this] {
        std::vector<double> v{1000, 30, 500};
        if (numericDialog(this, tr("Volume personalizado"),
                          {tr("Largura (mm)"), tr("Altura (mm)"), tr("Profundidade (mm)")}, v))
            editor_.apply(tr("Criar geometria"), [&](Document &d) {
                auto e = entity("GeometryObject", "Volume personalizado");
                e.width = v[0];
                e.height = v[1];
                e.depth = v[2];
                d.entities.push_back(e);
            });
    });
    auto *lightMenu = menuBar()->addMenu(tr("&Iluminação"));
    action(lightMenu, tr("Nova luz…"), {}, [this] { createLight(); });
    auto *cameraMenu = menuBar()->addMenu(tr("&Câmeras"));
    action(cameraMenu, tr("Nova câmera…"), {}, [this] { createCamera(); });
    auto *viewMenu = menuBar()->addMenu(tr("&Vista"));
    auto *measurements = action(viewMenu, tr("Mostrar medidas das paredes"), {}, [] {});
    measurements->setCheckable(true);
    measurements->setChecked(true);
    connect(measurements, &QAction::toggled, viewport, &CadView::setMeasurements);
    action(viewMenu, tr("Planta superior"), QKeySequence("1"), [this] { viewport->setTop(true); });
    action(viewMenu, tr("Ver em 3D"), QKeySequence("3"), [this] { viewport->setTop(false); });
    action(viewMenu, tr("Enquadrar projeto"), QKeySequence("F"), [this] { viewport->frame(); });
    auto *performanceMenu = viewMenu->addMenu(tr("Desempenho durante edição"));
    auto *performanceGroup = new QActionGroup(this);
    const int initialPerformance = QThread::idealThreadCount() <= 4 ? 0 : 1;
    const int savedPerformance =
        std::clamp(QSettings().value("performance/editor", initialPerformance).toInt(), 0, 2);
    viewport->setPerformanceMode(savedPerformance);
    thumbnails.setWorkerLimit(savedPerformance == 0 ? 1 : savedPerformance == 1 ? 2 : 4);
    const QStringList performanceLabels{tr("Leve — computador mais lento"), tr("Equilibrado"),
                                        tr("Mais detalhes")};
    for (int mode = 0; mode < performanceLabels.size(); ++mode) {
        auto *choice = performanceMenu->addAction(performanceLabels[mode]);
        choice->setObjectName(QString("performanceMode%1").arg(mode));
        choice->setCheckable(true);
        choice->setChecked(mode == savedPerformance);
        performanceGroup->addAction(choice);
        connect(choice, &QAction::triggered, this, [this, mode] {
            viewport->setPerformanceMode(mode);
            thumbnails.setWorkerLimit(mode == 0 ? 1 : mode == 1 ? 2 : 4);
            QSettings().setValue("performance/editor", mode);
            statusBar()->showMessage(tr("Desempenho da edição ajustado. O render final mantém os detalhes."),
                                     8000);
        });
    }
    auto *help = menuBar()->addMenu(tr("A&juda"));
    action(help, tr("Tutorial completo"), {}, [this] { showTutorial(this); });
    auto *animations = action(help, tr("Animações de abertura"), {}, [] {});
    animations->setCheckable(true);
    animations->setChecked(motionEnabled());
    connect(animations, &QAction::toggled, this,
            [](bool enabled) { QSettings().setValue("accessibility/animations", enabled); });
    action(help, tr("Sobre LibreMax"), {}, [this] {
        QMessageBox::about(this, tr("LibreMax Architect"),
                           tr("LibreMax Architect 0.6.0 — desenvolvimento\nEditor nativo C++20 / Qt / "
                              "OpenCASCADE\nCódigo GPL-3.0-or-later · Biblioteca procedural CC0\nA paridade "
                              "completa e os pacotes Linux ainda estão em desenvolvimento."));
    });
    auto *projectBar = addToolBar(tr("Projeto"));
    projectBar->setObjectName("projectToolbar");
    auto *homeAction = projectBar->addAction(tr("Meus projetos"));
    homeAction->setObjectName("projectHomeAction");
    connect(homeAction, &QAction::triggered, this, [this] { protect([&] { showHome(); }); });
    projectBar->setMovable(false);
    projectBar->setIconSize({18, 18});
    projectBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto *logo = new QLabel;
    logo->setPixmap(QPixmap(":/studio/brand/libremax-mark.png")
                        .scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    projectBar->addWidget(logo);
    auto *brand = new QLabel(tr("  LIBREMAX  "));
    brand->setObjectName("studioBrand");
    projectBar->addWidget(brand);
    projectBar->addSeparator();
    projectTitle = new QLabel;
    projectTitle->setObjectName("projectTitle");
    projectTitle->setMinimumWidth(100);
    projectTitle->setMaximumWidth(240);
    projectTitle->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    projectBar->addWidget(projectTitle);
    auto *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setStyleSheet("background:transparent");
    projectBar->addWidget(spacer);
    for (const auto &[item, icon] : std::vector<std::pair<QAction *, QString>>{{newAction, "new"},
                                                                               {openAction, "open"},
                                                                               {saveAction, "save"},
                                                                               {undo, "undo"},
                                                                               {redo, "redo"}}) {
        item->setIcon(studioIcon(icon));
        projectBar->addAction(item);
    }
    auto *renderShortcut = projectBar->addAction(studioIcon("render"), tr("Criar imagem"));
    connect(renderShortcut, &QAction::triggered, this, [this] {
        auto *dock = findChild<QDockWidget *>("renderDock");
        dock->show();
        dock->raise();
    });
    addToolBarBreak();
    auto *toolbar = addToolBar(tr("Projeto e desenho"));
    toolbar->setObjectName("drawingToolbar");
    toolbar->setMovable(false);
    toolbar->setIconSize({20, 20});
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto *modes = new QActionGroup(this);
    for (const auto &[label, mode] : std::vector<std::pair<QString, QString>>{
             {tr("Selecionar"), "select"}, {tr("Parede"), "wall"}, {tr("Mureta"), "half"}}) {
        auto *a = toolbar->addAction(label);
        a->setIcon(studioIcon(mode));
        a->setCheckable(true);
        a->setToolTip(label + tr(" · Esc encerra a cadeia"));
        modes->addAction(a);
        if (mode == "select")
            a->setChecked(true);
        connect(a, &QAction::triggered, this, [this, mode] { viewport->setTool(mode); });
    }
    toolbar->addSeparator();
    auto *insertMenu = new QToolButton;
    insertMenu->setText(tr("Ambiente"));
    insertMenu->setIcon(studioIcon("plan"));
    insertMenu->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    insertMenu->setPopupMode(QToolButton::InstantPopup);
    insertMenu->setMenu(environment);
    toolbar->addWidget(insertMenu);
    auto *automations = new QToolButton;
    automations->setText(tr("Finalizar"));
    automations->setPopupMode(QToolButton::InstantPopup);
    automations->setMenu(automationMenu);
    toolbar->addWidget(automations);
    toolbar->addSeparator();
    auto *cutaway = new QCheckBox(tr("Ver por dentro"));
    cutaway->setChecked(true);
    cutaway->setToolTip(tr("Oculta paredes próximas somente na vista 3D; projeto e render são preservados."));
    toolbar->addWidget(cutaway);
    connect(cutaway, &QCheckBox::toggled, viewport, &CadView::setCutaway);
    auto *top = toolbar->addAction(studioIcon("plan"), tr("Planta"));
    connect(top, &QAction::triggered, this, [this] {
        showEditorWorkspace();
        viewport->setTop(true);
    });
    auto *iso = toolbar->addAction(studioIcon("cube"), tr("3D"));
    connect(iso, &QAction::triggered, this, [this] {
        showEditorWorkspace();
        viewport->setTop(false);
    });
    auto *frame = toolbar->addAction(studioIcon("frame"), tr("Ver tudo"));
    connect(frame, &QAction::triggered, viewport, &CadView::frame);
    toolbar->addSeparator();
    previewAction = toolbar->addAction(studioIcon("render"), tr("Imagem"));
    previewAction->setObjectName("showRenderPreview");
    previewAction->setEnabled(false);
    viewMenu->addAction(previewAction);
    previewAction->setToolTip(tr("Exibir a última imagem renderizada, com zoom e navegação."));
    connect(previewAction, &QAction::triggered, this, [this] {
        if (preview)
            workspace->setCurrentWidget(preview);
    });
    addToolBarBreak();
    auto *placementBar = addToolBar(tr("Montagem"));
    placementBar->setObjectName("placementToolbar");
    placementBar->setMovable(false);
    auto *assistance = new QCheckBox(tr("Encaixar nas paredes"));
    assistance->setChecked(true);
    assistance->setObjectName("wallAssistance");
    assistance->setToolTip(
        tr("Ao aproximar o móvel, ajusta posição e direção. Vermelho indica que não cabe."));
    placementBar->addWidget(assistance);
    connect(assistance, &QCheckBox::toggled, this, [this](bool enabled) { viewport->assist = enabled; });
    roomPicker = new QComboBox;
    roomPicker->setObjectName("roomPicker");
    roomPicker->setMinimumWidth(110);
    roomPicker->setMaximumWidth(200);
    placementBar->addSeparator();
    placementBar->addWidget(new QLabel(tr(" Cômodo ")));
    placementBar->addWidget(roomPicker);
    connect(roomPicker, &QComboBox::activated, this, [this] { focusRoom(); });
    auto *rotate = placementBar->addAction(tr("Girar móvel"));
    rotate->setObjectName("rotateFurniture");
    rotate->setShortcut(QKeySequence("Ctrl+R"));
    connect(rotate, &QAction::triggered, this, [this] { protect([&] { transform("rotate"); }); });
    auto *libraryDock = new QDockWidget(tr("Móveis e decoração"), this);
    libraryDock->setObjectName("libraryDock");
    auto *libraryPanel = new QWidget;
    auto *libraryLayout = new QVBoxLayout(libraryPanel);
    search = new QLineEdit;
    search->setPlaceholderText(tr("Pesquisar móveis e objetos…"));
    search->setAccessibleName(tr("Pesquisar biblioteca"));
    libraryLayout->addWidget(search);
    auto *addModels = new QPushButton(tr("Adicionar modelos…"));
    addModels->setObjectName("addLibraryModels");
    auto *modelsMenu = new QMenu(addModels);
    action(modelsMenu, tr("Modelo baixado (GLB, OBJ e outros)…"), {}, [this] { importModel(); });
    action(modelsMenu, tr("Coleção LibreMax (.lmaxpack)…"), {}, [this] { installCollection(); });
    addModels->setMenu(modelsMenu);
    libraryLayout->addWidget(addModels);
    category = new QComboBox;
    category->addItem(tr("Todos os ambientes"), "");
    category->addItem(tr("Apartamento atual"), "__modern");
    category->addItem(tr("Modelos leves"), "__light");
    category->addItem(tr("Objetos detalhados"), "__detail");
    category->setObjectName("libraryCategory");
    for (const auto &cat : {"Cozinha", "Dormitório", "Sala", "Banheiro", "Escritório", "Decoração",
                            "Eletrodomésticos", "Portas e janelas", "Área de serviço", "Meus modelos"})
        category->addItem(QString::fromUtf8(cat), QString::fromUtf8(cat));
    libraryLayout->addWidget(category);
    favoriteOnly = new QCheckBox(tr("Favoritos"));
    recentOnly = new QCheckBox(tr("Recentes"));
    auto *filters = new QHBoxLayout;
    filters->addWidget(favoriteOnly);
    filters->addWidget(recentOnly);
    libraryLayout->addLayout(filters);
    assets = new AssetList;
    assets->setObjectName("assetList");
    assets->setItemDelegate(new AssetDelegate(assets));
    assets->setViewMode(QListView::IconMode);
    assets->setResizeMode(QListView::Adjust);
    assets->setMovement(QListView::Static);
    assets->setSpacing(2);
    assets->setUniformItemSizes(true);
    assets->setWordWrap(true);
    assets->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    assets->setMouseTracking(true);
    assets->setIconSize({76, 58});
    assets->setDragEnabled(true);
    assets->setAccessibleName(tr("Móveis disponíveis"));
    libraryLayout->addWidget(assets);
    libraryCount = new QLabel;
    libraryCount->setObjectName("libraryCount");
    libraryCount->setProperty("role", "muted");
    libraryLayout->addWidget(libraryCount);
    auto *place = new QPushButton(tr("Colocar"));
    place->setToolTip(tr("Clique e escolha onde colocar este item"));
    place->setObjectName("placeFurniture");
    place->setProperty("role", "primary");
    auto *libraryActions = new QHBoxLayout;
    libraryActions->addWidget(place, 1);
    libraryLayout->addLayout(libraryActions);
    connect(place, &QPushButton::clicked, this, [this] {
        if (assets->currentItem()) {
            showEditorWorkspace();
            viewport->beginPlacement(assets->currentItem()->data(Qt::UserRole).toString());
        }
    });
    auto *favorite = new QPushButton(tr("Favoritar"));
    favorite->setToolTip(tr("Guardar ou remover dos favoritos"));
    libraryActions->addWidget(favorite);
    libraryDock->setWidget(libraryPanel);
    addDockWidget(Qt::LeftDockWidgetArea, libraryDock);
    searchTimer.setSingleShot(true);
    searchTimer.setInterval(180);
    connect(&searchTimer, &QTimer::timeout, this, [this] { protect([&] { refreshLibrary(); }); });
    connect(search, &QLineEdit::textChanged, this, [this] { searchTimer.start(); });
    connect(category, &QComboBox::currentIndexChanged, this, [this] { protect([&] { refreshLibrary(); }); });
    connect(favoriteOnly, &QCheckBox::toggled, this, [this] { refreshLibrary(); });
    connect(recentOnly, &QCheckBox::toggled, this, [this] { refreshLibrary(); });
    connect(favorite, &QPushButton::clicked, this, [this] {
        protect([&] {
            if (!assets->currentItem())
                return;
            auto id = assets->currentItem()->data(Qt::UserRole).toString();
            for (const auto &a : visibleAssets)
                if (a.id == id) {
                    library->favorite(id, !a.favorite);
                    break;
                }
            refreshLibrary();
        });
    });
    connect(assets, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        showEditorWorkspace();
        viewport->beginPlacement(item->data(Qt::UserRole).toString());
    });
    auto *sceneDock = new QDockWidget(tr("O que está no projeto"), this);
    sceneDock->setObjectName("sceneDock");
    tree = new QTreeWidget;
    tree->setObjectName("sceneTree");
    tree->setHeaderLabels({tr("Objeto"), tr("Estado")});
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    sceneDock->setWidget(tree);
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock);
    tabifyDockWidget(libraryDock, sceneDock);
    libraryDock->raise();
    connect(tree, &QTreeWidget::itemSelectionChanged, this, [this] {
        if (refreshing)
            return;
        QStringList list;
        for (auto *item : tree->selectedItems())
            list << item->data(0, Qt::UserRole).toString();
        selectIds(list);
        viewport->select(ids(list));
    });
    auto *propertyDock = new QDockWidget(tr("Ajustar este item"), this);
    propertyDock->setObjectName("propertiesDock");
    auto *scroll = new QScrollArea;
    scroll->setObjectName("inspectorScroll");
    scroll->setWidgetResizable(true);
    inspector = new PropertyForm;
    auto *propertyLayout = new QFormLayout(inspector);
    propertyLayout->setContentsMargins(16, 12, 16, 16);
    propertyLayout->setVerticalSpacing(9);
    propertyLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    propertyLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    selectionTitle = new QLabel(tr("Nenhum objeto selecionado"));
    selectionTitle->setWordWrap(true);
    selectionTitle->setStyleSheet("font-size:16px;font-weight:600;padding:8px 0");
    propertyLayout->addRow(selectionTitle);
    advancedProperties = new QCheckBox(tr("Ajustes avançados"));
    advancedProperties->setObjectName("advancedProperties");
    propertyLayout->addRow(advancedProperties);
    connect(advancedProperties, &QCheckBox::toggled, this, [this] { refreshInspector(); });
    for (const auto &[key, label] :
         std::vector<std::pair<QString, QString>>{{"name", tr("Nome")},
                                                  {"x", tr("Posição lateral (mm)")},
                                                  {"y", tr("Posição no cômodo (mm)")},
                                                  {"z", tr("Distância do piso (cm)")},
                                                  {"yaw", tr("Rotação (°)")},
                                                  {"width", tr("Largura (cm)")},
                                                  {"height", tr("Altura (cm)")},
                                                  {"depth", tr("Profundidade (cm)")},
                                                  {"offset", tr("Distância do canto (cm)")},
                                                  {"sill", tr("Altura do piso (cm)")},
                                                  {"openAngle", tr("Abertura da porta (°)")},
                                                  {"power", tr("Brilho da luz")},
                                                  {"size", tr("Comprimento da luz (cm)")},
                                                  {"sizeY", tr("Largura da luz (cm)")},
                                                  {"radius", tr("Suavidade da sombra (cm)")},
                                                  {"sunAngle", tr("Suavidade do sol (°)")},
                                                  {"angle", tr("Feixe spot (°)")},
                                                  {"blend", tr("Suavidade (0–1)")},
                                                  {"targetX", tr("Alvo X (mm)")},
                                                  {"targetY", tr("Alvo Y (mm)")},
                                                  {"targetZ", tr("Alvo Z (mm)")},
                                                  {"lens", tr("Lente (mm)")},
                                                  {"fstop", tr("Abertura f/")},
                                                  {"focusDistance", tr("Foco (mm)")}}) {
        auto *field = new QLineEdit;
        field->setObjectName(key + "Field");
        field->setAccessibleName(label);
        field->setMinimumWidth(76);
        field->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        propertyLayout->addRow(label, field);
        auto *rowLabel = qobject_cast<QLabel *>(propertyLayout->labelForField(field));
        rowLabel->setWordWrap(true);
        rowLabel->setMaximumWidth(124);
        fields[key] = field;
    }
    material = new QComboBox;
    material->setMinimumContentsLength(12);
    material->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    material->setMinimumWidth(76);
    material->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    material->setAccessibleName(tr("Material"));
    for (const auto &m : editor_.document().materials)
        material->addItem(q(m["name"].get<std::string>()), q(m["id"].get<std::string>()));
    propertyLayout->addRow(tr("Acabamento"), material);
    handle = new QComboBox;
    handle->addItem(tr("Alça"), "bar");
    handle->addItem(tr("Perfil"), "profile");
    handle->addItem(tr("Ponto"), "point");
    handle->addItem(tr("Sem puxador"), "none");
    propertyLayout->addRow(tr("Puxador"), handle);
    glass = new QCheckBox(tr("Frentes com vidro"));
    propertyLayout->addRow(glass);
    originalModelColors = new QCheckBox(tr("Acabamento original"));
    propertyLayout->addRow(originalModelColors);
    connect(material, &QComboBox::activated, this, [this] { originalModelColors->setChecked(false); });
    lightColor = new QPushButton(tr("Escolher cor…"));
    lightColor->setObjectName("lightColor");
    propertyLayout->addRow(tr("Cor da luz"), lightColor);
    lightTone = new QComboBox;
    lightTone->setObjectName("lightTone");
    for (const auto &[label, kelvin] :
         std::vector<std::pair<QString, int>>{{tr("Quente · 2700 K"), 2700},
                                              {tr("Aconchegante · 3000 K"), 3000},
                                              {tr("Neutra · 4000 K"), 4000},
                                              {tr("Luz do dia · 5000 K"), 5000},
                                              {tr("Fria · 6500 K"), 6500},
                                              {tr("Outra temperatura"), 0},
                                              {tr("Cor escolhida"), -1}})
        lightTone->addItem(label, kelvin);
    lightTone->setMinimumWidth(76);
    lightTone->setMinimumContentsLength(12);
    lightTone->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    lightTone->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    propertyLayout->addRow(tr("Tom da luz"), lightTone);
    lightKelvin = new QSpinBox;
    lightKelvin->setObjectName("lightKelvin");
    lightKelvin->setRange(1000, 12000);
    lightKelvin->setSingleStep(100);
    lightKelvin->setSuffix(tr(" K"));
    propertyLayout->addRow(tr("Temperatura"), lightKelvin);
    lightShape = new QComboBox;
    lightShape->setObjectName("lightShape");
    lightShape->addItem(tr("Retângulo"), "RECTANGLE");
    lightShape->addItem(tr("Redonda"), "DISK");
    lightShape->addItem(tr("Quadrada"), "SQUARE");
    propertyLayout->addRow(tr("Formato da luz"), lightShape);
    fields["power"]->setToolTip(
        tr("Controla o brilho na foto. Não representa o consumo elétrico da lâmpada."));
    connect(lightTone, &QComboBox::currentIndexChanged, this, [this] { refreshLightControls(); });
    connect(lightShape, &QComboBox::currentIndexChanged, this, [this] { refreshLightControls(); });
    connect(lightColor, &QPushButton::clicked, this, [this] {
        auto color = QColorDialog::getColor(selectedLightColor, this, tr("Cor da luz"),
                                            QColorDialog::DontUseNativeDialog);
        if (color.isValid()) {
            selectedLightColor = color;
            lightColor->setText(color.name());
        }
    });
    auto *apply = new QPushButton(tr("Aplicar alterações"));
    apply->setObjectName("applyProperties");
    apply->setProperty("role", "primary");
    propertyLayout->addRow(apply);
    connect(apply, &QPushButton::clicked, this, [this] { protect([&] { applyInspector(); }); });
    hint = new QLabel(tr(
        "Arraste para mover. Use centímetros para ajustar o tamanho. Ctrl+clique seleciona vários itens."));
    hint->setWordWrap(true);
    propertyLayout->addRow(hint);
    scroll->setWidget(inspector);
    propertyDock->setWidget(scroll);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock);
    auto *renderDock = new QDockWidget(tr("Criar imagem"), this);
    renderDock->setObjectName("renderDock");
    auto *renderScroll = new QScrollArea;
    renderScroll->setObjectName("renderScroll");
    renderScroll->setWidgetResizable(true);
    auto *renderPanel = new QWidget;
    auto *renderLayout = new QFormLayout(renderPanel);
    renderLayout->setContentsMargins(16, 12, 16, 16);
    renderLayout->setVerticalSpacing(12);
    renderLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    renderLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    auto *heading = new QLabel(tr("Apresentação"));
    heading->setProperty("role", "heading");
    renderLayout->addRow(heading);
    auto *description = new QLabel(tr("Componha a câmera, equilibre a luz e gere sua imagem."));
    description->setWordWrap(true);
    description->setProperty("role", "muted");
    renderLayout->addRow(description);
    auto *automaticCamera = new QPushButton(tr("Preparar câmera do cômodo"));
    automaticCamera->setObjectName("simpleCamera");
    renderLayout->addRow(automaticCamera);
    connect(automaticCamera, &QPushButton::clicked, this, [this] { protect([&] { simpleCamera(); }); });
    renderCamera = new QComboBox;
    renderCamera->setObjectName("renderCamera");
    renderCamera->setAccessibleName(tr("Câmera para renderizar"));
    renderLayout->addRow(tr("Câmera"), renderCamera);
    renderQuality = new QComboBox;
    renderQuality->setObjectName("renderQuality");
    renderQuality->addItem(tr("Rápido · conferir luz e câmera"), "rapid");
    renderQuality->addItem(tr("Normal · avaliar materiais"), "normal");
    renderQuality->addItem(tr("Final · imagem de apresentação"), "final");
    renderQuality->addItem(tr("Personalizado · ajustar detalhes"), "custom");
    renderQuality->setCurrentIndex(1);
    renderLayout->addRow(tr("Qualidade"), renderQuality);
    renderSize = new QComboBox;
    renderSize->setObjectName("renderSize");
    for (const auto &size : {QSize(640, 360), QSize(1280, 720), QSize(1920, 1080), QSize(2560, 1440),
                             QSize(3840, 2160), QSize(1080, 1080), QSize(1080, 1920), QSize(1600, 1200),
                             QSize(1200, 1600), QSize(2480, 3508), QSize(3508, 2480)})
        renderSize->addItem(QString("%1 × %2").arg(size.width()).arg(size.height()), size);
    renderSize->setCurrentIndex(2);
    renderLayout->addRow(tr("Tamanho da imagem"), renderSize);
    renderFormat = new QComboBox;
    renderFormat->setObjectName("renderFormat");
    renderFormat->addItems({"PNG", "JPEG", "EXR"});
    renderFormat->setToolTip(tr("EXR preserva a luz em alta precisão. Disponível em Personalizado."));
    renderLayout->addRow(tr("Formato"), renderFormat);
    customRender = new QWidget;
    customRender->setObjectName("customRender");
    auto *customLayout = new QFormLayout(customRender);
    customLayout->setContentsMargins(0, 0, 0, 0);
    customLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    customLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    const auto defaults = renderPreset("normal");
    for (const auto &[key, label] : std::map<QString, QString>{{"width", tr("Largura (pixels)")},
                                                               {"height", tr("Altura (pixels)")},
                                                               {"samples", tr("Amostras")},
                                                               {"maxBounces", tr("Reflexões máximas")},
                                                               {"diffuseBounces", tr("Luz indireta")},
                                                               {"glossyBounces", tr("Reflexos")},
                                                               {"transmissionBounces", tr("Vidro")},
                                                               {"transparentBounces", tr("Transparência")}}) {
        auto *field = new QSpinBox;
        field->setObjectName("render_" + key);
        field->setRange(key == "samples"                    ? 1
                        : key == "width" || key == "height" ? 16
                                                            : 0,
                        key == "samples"                    ? 4096
                        : key == "width" || key == "height" ? 8192
                                                            : 64);
        field->setValue(defaults.at(key.toStdString()).get<int>());
        field->setAccessibleName(label);
        renderCounts[key] = field;
        customLayout->addRow(label, field);
    }
    for (const auto &[key, label] : std::map<QString, QString>{{"clamp", tr("Limitar pontos brilhantes")},
                                                               {"noiseThreshold", tr("Limite de ruído")}}) {
        auto *field = new QDoubleSpinBox;
        field->setObjectName("render_" + key);
        field->setRange(0, key == "clamp" ? 100 : 1);
        field->setDecimals(3);
        field->setSingleStep(key == "clamp" ? 1 : 0.001);
        field->setValue(defaults.at(key.toStdString()).get<double>());
        field->setAccessibleName(label);
        renderValues[key] = field;
        customLayout->addRow(label, field);
    }
    auto *transparent = new QCheckBox(tr("Fundo transparente"));
    transparent->setObjectName("renderTransparent");
    customLayout->addRow(transparent);
    connect(transparent, &QCheckBox::toggled, this, [this](bool checked) {
        if (!refreshing) {
            if (checked && renderFormat->currentText() == "JPEG") {
                QSignalBlocker blocker(renderFormat);
                renderFormat->setCurrentText("PNG");
            }
            protect([&] { applyRenderSettings(); });
        }
    });
    customRender->hide();
    renderLayout->addRow(customRender);
    renderExposure = new QDoubleSpinBox;
    renderExposure->setObjectName("renderExposure");
    renderExposure->setRange(-8, 8);
    renderExposure->setDecimals(1);
    renderExposure->setSingleStep(0.1);
    renderExposure->setSuffix(tr(" EV"));
    renderExposure->setAccessibleName(tr("Exposição da imagem"));
    renderLayout->addRow(tr("Exposição"), renderExposure);
    renderEnvironment = new QDoubleSpinBox;
    renderEnvironment->setObjectName("renderEnvironment");
    renderEnvironment->setRange(0, 5);
    renderEnvironment->setSingleStep(0.05);
    renderEnvironment->setDecimals(2);
    renderEnvironment->setAccessibleName(tr("Intensidade da iluminação ambiente"));
    renderEnvironment->setToolTip(tr("Complementa as luzes do projeto. Não altera sua potência."));
    renderEnvironmentMode = new QComboBox;
    renderEnvironmentMode->setObjectName("renderEnvironmentMode");
    renderEnvironmentMode->setAccessibleName(tr("Tipo de iluminação ambiente"));
    renderEnvironmentMode->addItem(tr("Luz neutra"), "studio");
    renderEnvironmentMode->addItem(tr("Céu natural"), "sky");
    renderEnvironmentMode->addItem(tr("Cor sólida"), "solid");
    renderEnvironmentMode->addItem(tr("Luz de uma imagem (HDRI)"), "hdri");
    renderLayout->addRow(tr("Ambiente"), renderEnvironmentMode);
    auto *roomStyle = new QComboBox;
    roomStyle->setObjectName("roomPhotoStyle");
    roomStyle->setMinimumWidth(76);
    roomStyle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    roomStyle->addItem(tr("Escolher um estilo pronto…"), "");
    roomStyle->addItem(tr("Natural · madeira e luz suave"), "natural");
    roomStyle->addItem(tr("Claro · pedra e luz do dia"), "bright");
    roomStyle->addItem(tr("Aconchegante · luz quente"), "evening");
    renderLayout->addRow(tr("Estilo do cômodo"), roomStyle);
    connect(roomStyle, &QComboBox::activated, this, [this, roomStyle] {
        if (!roomStyle->currentData().toString().isEmpty())
            protect([&] { roomLook(roomStyle->currentData().toString()); });
    });
    auto *importEnvironmentButton = new QPushButton(tr("Importar luz…"));
    importEnvironmentButton->setObjectName("importHdri");
    importEnvironmentButton->setToolTip(tr("Importa um panorama HDR ou EXR e o incorpora ao projeto."));
    renderLayout->addRow(importEnvironmentButton);
    connect(importEnvironmentButton, &QPushButton::clicked, this, [this] { importHdri(); });
    auto *daylight = new QPushButton(tr("Usar luz do dia"));
    daylight->setObjectName("useDaylightHdri");
    renderLayout->addRow(daylight);
    connect(daylight, &QPushButton::clicked, this,
            [this] { importHdri(resourceFile("starter-environments/kiara_1_dawn_1k.hdr")); });
    hdriControls = new QWidget;
    hdriControls->setObjectName("hdriControls");
    auto *environmentLayout = new QFormLayout(hdriControls);
    environmentLayout->setContentsMargins(0, 0, 0, 0);
    environmentLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    hdriStatus = new QLabel;
    hdriStatus->setObjectName("hdriStatus");
    hdriStatus->setTextFormat(Qt::PlainText);
    hdriStatus->setWordWrap(true);
    hdriStatus->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    environmentLayout->addRow(hdriStatus);
    hdriRotation = new QDoubleSpinBox;
    hdriRotation->setObjectName("hdriRotation");
    hdriRotation->setRange(-360, 360);
    hdriRotation->setSuffix(tr("°"));
    environmentLayout->addRow(tr("Girar a luz"), hdriRotation);
    hdriVisible = new QCheckBox(tr("Mostrar na imagem"));
    hdriVisible->setObjectName("hdriVisible");
    environmentLayout->addRow(hdriVisible);
    renderLayout->addRow(hdriControls);
    for (auto *field :
         {static_cast<QWidget *>(hdriRotation), static_cast<QWidget *>(renderEnvironmentMode)}) {
        field->setMinimumWidth(76);
        field->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    }
    connect(hdriRotation, &QDoubleSpinBox::editingFinished, this, [this] {
        if (!refreshing)
            protect([&] { applyRenderSettings(); });
    });
    connect(hdriVisible, &QCheckBox::toggled, this, [this] {
        if (!refreshing)
            protect([&] { applyRenderSettings(); });
    });
    backgroundColor = new QPushButton(tr("Escolher cor…"));
    backgroundColor->setObjectName("renderBackgroundColor");
    renderLayout->addRow(tr("Cor de fundo"), backgroundColor);
    connect(backgroundColor, &QPushButton::clicked, this, [this] {
        const auto rgb =
            editor_.document().renderSettings.value("backgroundColor", Json::array({0.7, 0.8, 1.0}));
        const auto color = QColorDialog::getColor(
            QColor::fromRgbF(rgb[0].get<double>(), rgb[1].get<double>(), rgb[2].get<double>()), this,
            tr("Cor de fundo"), QColorDialog::DontUseNativeDialog);
        if (color.isValid())
            protect([&] {
                editor_.apply(tr("Cor de fundo"), [&](Document &document) {
                    document.renderSettings["backgroundColor"] = {color.redF(), color.greenF(),
                                                                  color.blueF()};
                });
            });
    });
    renderSunElevation = new QDoubleSpinBox;
    renderSunElevation->setObjectName("renderSunElevation");
    renderSunElevation->setRange(1, 89);
    renderSunElevation->setSuffix(tr("°"));
    renderSunElevation->setValue(35);
    renderLayout->addRow(tr("Altura do sol"), renderSunElevation);
    renderSunRotation = new QDoubleSpinBox;
    renderSunRotation->setObjectName("renderSunRotation");
    renderSunRotation->setRange(0, 360);
    renderSunRotation->setSuffix(tr("°"));
    renderSunRotation->setValue(30);
    renderLayout->addRow(tr("Direção do sol"), renderSunRotation);
    renderLayout->addRow(tr("Luz ambiente"), renderEnvironment);
    renderDevice = new QComboBox;
    renderDevice->setObjectName("renderDevice");
    renderDevice->addItem(tr("GPU / CPU automático"), "AUTO");
    renderDevice->addItem(tr("CPU"), "CPU");
    renderLayout->addRow(tr("Dispositivo"), renderDevice);
    renderDenoise = new QCheckBox(tr("Reduzir ruído (denoise)"));
    renderDenoise->setObjectName("renderDenoise");
    renderDenoise->setChecked(true);
    renderDenoise->setEnabled(false);
    renderLayout->addRow(renderDenoise);
    auto *pbr = new QPushButton(tr("Ativar texturas reais"));
    pbr->setObjectName("activatePbrMaterials");
    pbr->setProperty("role", "quiet");
    pbr->setToolTip(tr("Incorpora mapas de cor, rugosidade e normal de carvalho e pedra neste projeto."));
    renderLayout->addRow(pbr);
    connect(pbr, &QPushButton::clicked, this, [this] { protect([&] { activatePbrMaterials(); }); });
    auto *engineToggle = new QPushButton(tr("Configurar Blender…"));
    engineToggle->setProperty("role", "quiet");
    engineToggle->setCheckable(true);
    renderLayout->addRow(engineToggle);
    auto *enginePanel = new QWidget;
    auto *engineLayout = new QVBoxLayout(enginePanel);
    engineLayout->setContentsMargins(0, 0, 0, 0);
    blenderPath = new QLineEdit(BlenderBridge::findExecutable(QSettings().value("blender").toString()));
    blenderPath->setObjectName("blenderPath");
    blenderPath->setAccessibleName(tr("Executável Blender"));
    engineLayout->addWidget(blenderPath);
    auto *browse = new QPushButton(tr("Escolher executável…"));
    engineLayout->addWidget(browse);
    connect(browse, &QPushButton::clicked, this, [this] {
        auto file = QFileDialog::getOpenFileName(this, tr("Executável Blender"));
        if (!file.isEmpty())
            blenderPath->setText(file);
    });
    renderLayout->addRow(enginePanel);
    enginePanel->setVisible(blenderPath->text().isEmpty());
    engineToggle->setChecked(enginePanel->isVisibleTo(renderPanel));
    connect(engineToggle, &QPushButton::toggled, enginePanel, &QWidget::setVisible);
    renderState = new QLabel(tr("Pronto para renderizar"));
    renderState->setObjectName("renderState");
    renderState->setProperty("role", "muted");
    renderState->setWordWrap(true);
    renderLayout->addRow(renderState);
    renderProgress = new QProgressBar;
    renderProgress->setObjectName("renderProgress");
    renderProgress->setTextVisible(true);
    renderProgress->setRange(0, 100);
    renderProgress->setValue(0);
    renderProgress->setAccessibleName(tr("Estado do render"));
    renderLayout->addRow(renderProgress);
    renderTiming = new QLabel;
    renderTiming->setObjectName("renderTiming");
    renderTiming->setWordWrap(true);
    renderTiming->setTextFormat(Qt::PlainText);
    renderLayout->addRow(renderTiming);
    renderStart = new QPushButton(tr("Enviar para renderizar"));
    renderStart->setObjectName("startRender");
    renderStart->setProperty("role", "primary");
    renderCancel = new QPushButton(tr("Cancelar"));
    renderCancel->setEnabled(false);
    renderLayout->addRow(renderStart);
    renderLayout->addRow(renderCancel);
    connect(renderStart, &QPushButton::clicked, this, [this] { protect([&] { renderScene(); }); });
    connect(renderCancel, &QPushButton::clicked, render.get(), &RenderQueue::cancelActive);
    auto *showImages = new QPushButton(tr("Suas imagens e fila"));
    showImages->setObjectName("showRenderGallery");
    renderLayout->addRow(showImages);
    connect(showImages, &QPushButton::clicked, this, &MainWindow::showRenderGallery);
    auto *quickPreview = new QPushButton(tr("Prévia pequena · 640 × 360"));
    quickPreview->setObjectName("renderQuickPreview");
    renderLayout->addRow(quickPreview);
    connect(quickPreview, &QPushButton::clicked, this, [this] {
        renderQuality->setCurrentIndex(0);
        protect([&] { renderScene(); });
    });
    auto *allCameras = new QPushButton(tr("Enviar todas as câmeras"));
    allCameras->setObjectName("renderAllCameras");
    renderLayout->addRow(allCameras);
    connect(allCameras, &QPushButton::clicked, this, [this] {
        protect([&] {
            applyRenderSettings();
            const auto document = editor_.document();
            for (const auto &camera : document.entities)
                if (camera.type == "Camera" && camera.visible)
                    render->enqueue(RenderSnapshot(document, selectedRenderOptions(), camera.id),
                                    blenderPath->text(), resourceFile("scripts/cycles_render.py"));
            showRenderGallery();
        });
    });
    auto *details = new QPushButton(tr("Detalhes do processo"));
    details->setProperty("role", "quiet");
    details->setCheckable(true);
    renderLayout->addRow(details);
    renderLog = new QPlainTextEdit;
    renderLog->setReadOnly(true);
    renderLog->setMaximumBlockCount(1000);
    renderLog->setMaximumHeight(130);
    renderLog->setVisible(false);
    renderLayout->addRow(renderLog);
    connect(details, &QPushButton::toggled, renderLog, &QWidget::setVisible);
    connect(renderCamera, &QComboBox::currentIndexChanged, this, [this] {
        if (!refreshing)
            protect([&] { applyRenderSettings(); });
    });
    connect(renderExposure, &QDoubleSpinBox::editingFinished, this,
            [this] { protect([&] { applyRenderSettings(); }); });
    connect(renderEnvironment, &QDoubleSpinBox::editingFinished, this,
            [this] { protect([&] { applyRenderSettings(); }); });
    connect(renderDenoise, &QCheckBox::toggled, this, [this] {
        if (!refreshing)
            protect([&] { applyRenderSettings(); });
    });
    connect(renderEnvironmentMode, &QComboBox::currentIndexChanged, this, [this] {
        if (refreshing)
            return;
        if (renderEnvironmentMode->currentData() == "hdri" &&
            editor_.document().renderSettings.value("hdri", Json::object()).empty()) {
            QSignalBlocker blocker(renderEnvironmentMode);
            renderEnvironmentMode->setCurrentIndex(renderEnvironmentMode->findData(
                q(editor_.document().renderSettings.value("environmentMode", std::string("studio")))));
            importHdri();
            return;
        }
        protect([&] { applyRenderSettings(); });
    });
    for (auto *field : {renderSunElevation, renderSunRotation})
        connect(field, &QDoubleSpinBox::editingFinished, this,
                [this] { protect([&] { applyRenderSettings(); }); });
    for (auto *field :
         {static_cast<QWidget *>(renderCamera), static_cast<QWidget *>(renderQuality),
          static_cast<QWidget *>(renderExposure), static_cast<QWidget *>(renderEnvironment),
          static_cast<QWidget *>(renderDevice), static_cast<QWidget *>(renderEnvironmentMode),
          static_cast<QWidget *>(renderSunElevation), static_cast<QWidget *>(renderSunRotation)}) {
        field->setMinimumWidth(76);
        field->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    }
    connect(renderQuality, &QComboBox::currentIndexChanged, this, [this] {
        if (refreshing)
            return;
        if (renderQuality->currentData() != "custom" && renderFormat->currentText() == "EXR") {
            QSignalBlocker blocker(renderFormat);
            renderFormat->setCurrentText("PNG");
        }
        const auto preset = renderPreset(renderQuality->currentData().toString());
        QSignalBlocker blockSize(renderSize);
        renderSize->setCurrentIndex(
            renderSize->findData(QSize(preset.at("width").get<int>(), preset.at("height").get<int>())));
        renderSize->setEnabled(renderQuality->currentData() != "custom");
        customRender->setVisible(renderQuality->currentData() == "custom");
        renderDenoise->setEnabled(renderQuality->currentData() == "custom");
        protect([&] { applyRenderSettings(); });
    });
    for (auto *field : {renderSize, renderDevice})
        connect(field, &QComboBox::currentIndexChanged, this, [this] {
            if (!refreshing)
                protect([&] { applyRenderSettings(); });
        });
    connect(renderFormat, &QComboBox::currentTextChanged, this, [this](const QString &format) {
        if (!refreshing) {
            if (format == "JPEG") {
                auto *transparent = customRender->findChild<QCheckBox *>("renderTransparent");
                QSignalBlocker blocker(transparent);
                transparent->setChecked(false);
            }
            protect([&] { applyRenderSettings(); });
        }
    });
    for (const auto &[key, field] : renderCounts)
        connect(field, &QSpinBox::editingFinished, this, [this] {
            if (!refreshing)
                protect([&] { applyRenderSettings(); });
        });
    for (const auto &[key, field] : renderValues)
        connect(field, &QDoubleSpinBox::editingFinished, this, [this] {
            if (!refreshing)
                protect([&] { applyRenderSettings(); });
        });
    gallery = new RenderGallery(*render);
    workspace->addWidget(gallery);
    connect(gallery, &RenderGallery::back, this, [this] { showEditorWorkspace(); });
    connect(gallery, &RenderGallery::openImage, this,
            [this](const QString &file) { protect([&] { showRenderImage(file); }); });
    connect(gallery, &RenderGallery::cancelImage, this, [this](const QString &id) { render->cancel(id); });
    connect(gallery, &RenderGallery::retryImage, this,
            [this](const QString &id, bool cpu) { protect([&] { render->retry(id, cpu ? "CPU" : ""); }); });
    connect(gallery, &RenderGallery::removeImage, this, [this](const QString &id) {
        if (QMessageBox::question(this, tr("Excluir imagem"),
                                  tr("Excluir esta imagem da galeria? O projeto será preservado.")) ==
            QMessageBox::Yes)
            protect([&] { render->remove(id); });
    });
    connect(gallery, &RenderGallery::saveImage, this, [this](const QString &source) {
        protect([&] {
            const auto extension = QFileInfo(source).suffix();
            const auto pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
            const auto suggestion = QDir(pictures).exists() ? QDir(pictures).filePath("Imagem." + extension)
                                                            : "Imagem." + extension;
            const auto destination = QFileDialog::getSaveFileName(
                this, tr("Salvar cópia da imagem"), suggestion, tr("Imagem (*.%1)").arg(extension));
            if (destination.isEmpty())
                return;
            auto *watcher = new QFutureWatcher<QString>(this);
            statusBar()->showMessage(tr("Salvando a cópia. Você pode continuar editando."));
            connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher] {
                const auto error = watcher->result();
                watcher->deleteLater();
                statusBar()->showMessage(error.isEmpty() ? tr("Cópia da imagem salva.") : error, 10000);
            });
            watcher->setFuture(QtConcurrent::run([source, destination]() -> QString {
                QFile input(source);
                QSaveFile output(destination);
                output.setDirectWriteFallback(false);
                if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly))
                    return tr("Não foi possível salvar a cópia.");
                while (!input.atEnd()) {
                    const auto bytes = input.read(1024 * 1024);
                    if (bytes.isEmpty() || output.write(bytes) != bytes.size())
                        return tr("Falha ao salvar a cópia; arquivo anterior preservado.");
                }
                input.close();
                return output.commit() ? QString{}
                                       : tr("Falha ao salvar a cópia; arquivo anterior preservado.");
            }));
        });
    });
    renderScroll->setWidget(renderPanel);
    renderDock->setWidget(renderScroll);
    addDockWidget(Qt::RightDockWidgetArea, renderDock);
    tabifyDockWidget(propertyDock, renderDock);
    propertyDock->raise();
    for (auto *dock : {libraryDock, sceneDock, propertyDock, renderDock})
        viewMenu->addAction(dock->toggleViewAction());
    resizeDocks({libraryDock, propertyDock}, {300, 320}, Qt::Horizontal);
    connect(home, &ProjectHome::newProject, newAction, &QAction::trigger);
    connect(home, &ProjectHome::openProject, openAction, &QAction::trigger);
    connect(home, &ProjectHome::openRecent, this, [this](const QString &file) {
        protect([&] {
            if (QFileInfo(file).absoluteFilePath() == QFileInfo(path).absoluteFilePath() && !path.isEmpty())
                enterEditor();
            else if (discardOrSave())
                loadProject(file);
        });
    });
    connect(home, &ProjectHome::tutorial, this, [this] { showTutorial(this); });
    connect(home, &ProjectHome::example, this, [this] { protect([&] { modernApartmentStarter(); }); });
    connect(home, &ProjectHome::recovery, this, [this] { protect([&] { recover(); }); });
    auto *snapBox = new QCheckBox(tr("Alinhar desenho"));
    snapBox->setChecked(true);
    statusBar()->addPermanentWidget(snapBox);
    connect(snapBox, &QCheckBox::toggled, this, [this](bool on) { viewport->snap = on; });
    auto *gridBox = new QComboBox;
    for (int step : {5, 10, 50, 100, 500})
        gridBox->addItem(tr("Grade %1 cm").arg(step / 10.0), step);
    gridBox->setCurrentIndex(3);
    statusBar()->addPermanentWidget(gridBox);
    connect(gridBox, &QComboBox::currentIndexChanged, this,
            [this, gridBox] { viewport->setGrid(gridBox->currentData().toDouble()); });
    statusBar()->addPermanentWidget(new QLabel(tr("No seu computador")));
}
void MainWindow::refreshScene() {
    {
        QSignalBlocker blocker(roomPicker);
        auto selected = roomPicker->currentData();
        roomPicker->clear();
        roomPicker->addItem(tr("Apartamento inteiro"), "");
        for (const auto &object : editor_.document().entities)
            if (object.type == "Room")
                roomPicker->addItem(q(object.name), q(object.id));
        auto index = roomPicker->findData(selected);
        if (index >= 0)
            roomPicker->setCurrentIndex(index);
    }
    refreshing = true;
    selectedIds.erase(
        std::remove_if(selectedIds.begin(), selectedIds.end(),
                       [&](const auto &id) { return !editor_.document().contains(id.toStdString()); }),
        selectedIds.end());
    projectTitle->setText(q(editor_.document().name));
    projectTitle->setToolTip(q(editor_.document().name));
    {
        QSignalBlocker blockMode(renderEnvironmentMode), blockElevation(renderSunElevation),
            blockRotation(renderSunRotation);
        QSignalBlocker blockCamera(renderCamera), blockExposure(renderExposure),
            blockEnvironment(renderEnvironment), blockDenoise(renderDenoise);
        auto cameraId = q(editor_.document().renderSettings.at("camera").get<std::string>());
        renderCamera->clear();
        for (const auto &e : editor_.document().entities)
            if (e.type == "Camera" && e.visible)
                renderCamera->addItem(q(e.name), q(e.id));
        const auto cameraIndex = renderCamera->findData(cameraId);
        if (cameraIndex >= 0)
            renderCamera->setCurrentIndex(cameraIndex);
        renderExposure->setValue(editor_.document().renderSettings.at("exposure").get<double>());
        renderEnvironment->setValue(
            editor_.document().renderSettings.at("environmentStrength").get<double>());
        renderDenoise->setChecked(editor_.document().renderSettings.at("denoise").get<bool>());
        const auto &settings = editor_.document().renderSettings;
        if (settings.contains("cycles")) {
            const auto &options = settings.at("cycles");
            QSignalBlocker quality(renderQuality), size(renderSize), format(renderFormat),
                device(renderDevice);
            renderQuality->setCurrentIndex(
                renderQuality->findData(q(options.at("preset").get<std::string>())));
            renderSize->setCurrentIndex(
                renderSize->findData(QSize(options.at("width").get<int>(), options.at("height").get<int>())));
            renderFormat->setCurrentText(q(options.at("format").get<std::string>()));
            renderDevice->setCurrentIndex(renderDevice->findData(q(options.at("device").get<std::string>())));
            for (const auto &[key, field] : renderCounts)
                field->setValue(options.at(key.toStdString()).get<int>());
            for (const auto &[key, field] : renderValues)
                field->setValue(options.at(key.toStdString()).get<double>());
            auto *transparent = customRender->findChild<QCheckBox *>("renderTransparent");
            QSignalBlocker transparency(transparent);
            transparent->setChecked(options.at("transparent").get<bool>());
        }
        const bool custom = renderQuality->currentData() == "custom";
        qobject_cast<QStandardItemModel *>(renderFormat->model())->item(2)->setEnabled(custom);
        customRender->setVisible(custom);
        renderSize->setEnabled(!custom);
        renderDenoise->setEnabled(custom);
        gallery->setProject(editor_.document().id);

        renderEnvironmentMode->setCurrentIndex(
            renderEnvironmentMode->findData(q(settings.value("environmentMode", std::string("studio")))));
        const auto environment = settings.value("hdri", Json::object());
        hdriControls->setVisible(renderEnvironmentMode->currentData() == "hdri");
        hdriStatus->setText(environment.empty() ? tr("Importe um panorama de luz.")
                                                : q(environment.at("name").get<std::string>()));
        hdriRotation->setValue(environment.value("rotation", 0.0));
        hdriVisible->setChecked(environment.value("visible", true));
        qobject_cast<QFormLayout *>(renderEnvironmentMode->parentWidget()->layout())
            ->setRowVisible(backgroundColor, renderEnvironmentMode->currentData() == "solid" ||
                                                 (renderEnvironmentMode->currentData() == "hdri" &&
                                                  !hdriVisible->isChecked()));
        renderSunElevation->setValue(settings.value("sunElevation", 35.0));
        renderSunRotation->setValue(settings.value("sunRotation", 30.0));
        auto *layout = qobject_cast<QFormLayout *>(renderEnvironmentMode->parentWidget()->layout());
        layout->setRowVisible(renderSunElevation, renderEnvironmentMode->currentIndex() == 1);
        layout->setRowVisible(renderSunRotation, renderEnvironmentMode->currentIndex() == 1);
        renderStart->setEnabled(renderCamera->count() > 0);
        if (!render->busy())
            renderState->setText(renderCamera->count() ? tr("Pronto para renderizar")
                                                       : tr("Crie uma câmera no menu Câmeras para começar."));
        auto *scroll = findChild<QScrollArea *>("renderScroll");
        scroll->setMinimumWidth(scroll->widget()->minimumSizeHint().width() +
                                scroll->verticalScrollBar()->sizeHint().width() + 2 * scroll->frameWidth());
    }
    viewport->scene(editor_.document());
    material->clear();
    for (const auto &m : editor_.document().materials)
        material->addItem(q(m.at("name").get<std::string>()), q(m.at("id").get<std::string>()));
    tree->clear();
    std::map<std::string, QTreeWidgetItem *> items;
    for (const auto &e : editor_.document().entities) {
        auto *item = new QTreeWidgetItem({q(e.name), e.locked    ? tr("Bloqueado")
                                                     : e.visible ? QString{}
                                                                 : tr("Oculto")});
        item->setData(0, Qt::UserRole, q(e.id));
        item->setIcon(0, studioIcon(e.type == "Camera"                         ? "render"
                                    : e.type == "Light"                        ? "light"
                                    : e.type == "Wall" || e.type == "HalfWall" ? "wall"
                                                                               : "cube"));
        items[e.id] = item;
    }
    for (const auto &e : editor_.document().entities) {
        auto *item = items.at(e.id);
        if (e.parent.empty())
            tree->addTopLevelItem(item);
        else
            items.at(e.parent)->addChild(item);
        if (selectedIds.contains(q(e.id)))
            item->setSelected(true);
    }
    tree->expandAll();
    tree->resizeColumnToContents(0);
    refreshing = false;
    refreshInspector();
    setWindowTitle(tr("%1[*] — LibreMax Architect").arg(q(editor_.document().name)));
    setWindowModified(!editor_.history.isClean());
}
void MainWindow::refreshLibrary() {
    const bool modern = category->currentData().toString() == "__modern";
    const bool light = category->currentData().toString() == "__light";
    const bool detail = category->currentData().toString() == "__detail";
    visibleAssets = library->search(
        search->text(), modern || light || detail ? QString{} : category->currentData().toString(),
        favoriteOnly->isChecked(), recentOnly->isChecked(), false);
    if (modern)
        std::erase_if(visibleAssets, [](const auto &asset) {
            return asset.recipe.value("collection", std::string{}) != "apartment-modern";
        });
    if (light)
        std::erase_if(visibleAssets, [](const auto &asset) {
            return !asset.id.startsWith("ready-") &&
                   asset.recipe.value("collection", std::string{}) != "lightweight-ready";
        });
    if (detail)
        std::erase_if(visibleAssets, [](const auto &asset) {
            return asset.recipe.value("collection", std::string{}) != "detail-ready";
        });
    assets->clear();
    for (const auto &a : visibleAssets) {
        auto *item = new QListWidgetItem((a.favorite ? "★ " : "") + a.name +
                                             QString("\n%1 × %2 × %3 cm")
                                                 .arg(a.width / 10, 0, 'f', 1)
                                                 .arg(a.depth / 10, 0, 'f', 1)
                                                 .arg(a.height / 10, 0, 'f', 1),
                                         assets);
        item->setData(Qt::UserRole, a.id);
        item->setData(Qt::UserRole + 1, a.category);
        auto thumbnail =
            thumbnails.request(a, [this](const Asset &asset) { return library->withPayload(asset); });
        item->setIcon(thumbnail.isNull() ? studioIcon("cube") : QIcon(QPixmap::fromImage(thumbnail)));
        item->setData(Qt::UserRole + 2, !thumbnail.isNull());
        item->setToolTip(a.category + " · " + q(a.recipe.value("license", std::string("CC0"))) + " · " +
                         tr("Arraste para inserir"));
        item->setSizeHint({114, 174});
    }
    assets->verticalScrollBar()->setValue(0);
    libraryCount->setText(visibleAssets.empty()
                              ? tr("Nenhum item encontrado. Ajuste a busca ou os filtros.")
                              : tr("%1 itens · biblioteca local").arg(visibleAssets.size()));
    libraryCount->setWordWrap(true);
}
void MainWindow::selectIds(const QStringList &list) {
    selectedIds = list;
    refreshInspector();
}
void MainWindow::refreshInspector() {
    const Entity *e = nullptr;
    if (selectedIds.size() == 1 && editor_.document().contains(selectedIds.first().toStdString()))
        e = &editor_.document().at(selectedIds.first().toStdString());
    auto *form = qobject_cast<QFormLayout *>(inspector->layout());
    advancedProperties->setVisible(e && e->type != "Group");
    findChild<QPushButton *>("applyProperties")->setEnabled(e && !e->locked);
    form->setRowVisible(material, e && e->type != "Light" && e->type != "Camera" && e->type != "Group");
    form->setRowVisible(handle, e && e->type == "FurnitureModule");
    form->setRowVisible(glass, e && e->type == "FurnitureModule");
    form->setRowVisible(originalModelColors, e && e->type == "MeshObject");
    originalModelColors->setChecked(e && e->parameters.value("originalMaterials", true));
    form->setRowVisible(lightColor, e && e->type == "Light");
    selectionTitle->setText(e                       ? q(e->name)
                            : selectedIds.isEmpty() ? tr("Selecione um objeto")
                                                    : tr("%1 objetos selecionados").arg(selectedIds.size()));
    hint->setText(
        e && e->type == "Group" ? tr("Arraste qualquer móvel do conjunto para mover todos juntos. Use Editar "
                                     "para alinhar ou separar.")
        : e && e->type == "Light"
            ? tr("O marcador mostra onde está a luz. Ajuste o brilho e crie uma prévia para ver o resultado.")
            : tr("Arraste para mover. Use centímetros para ajustar o tamanho. Ctrl+clique seleciona vários "
                 "itens."));
    for (const auto &[key, field] : fields) {
        bool visible = e != nullptr;
        if (key == "offset" || key == "sill" || key == "openAngle")
            visible = e && (e->type == "Door" || e->type == "Window");
        if (key == "power")
            visible = e && e->type == "Light";
        if (key == "size")
            visible = e && e->type == "Light" &&
                      (e->parameters.value("kind", "area") == "area" ||
                       e->parameters.value("kind", "area") == "led");
        if (key == "sizeY")
            visible = false;
        if (key == "radius")
            visible = e && e->type == "Light" &&
                      (e->parameters.value("kind", "area") == "point" ||
                       e->parameters.value("kind", "area") == "spot");
        if (key == "sunAngle")
            visible = e && e->type == "Light" && e->parameters.value("kind", "area") == "sun";
        if (key == "width" || key == "height" || key == "depth")
            visible = e && e->type != "Light" && e->type != "Camera";
        if (key == "angle" || key == "blend")
            visible = e && e->type == "Light" && e->parameters.value("kind", std::string("area")) == "spot";
        if (key == "lens" || key == "fstop" || key == "focusDistance")
            visible = e && e->type == "Camera";
        if (key.startsWith("target"))
            visible = e && (e->type == "Camera" || e->type == "Light");
        if (key == "x" || key == "y" || key == "yaw" || key.startsWith("target") || key == "lens" ||
            key == "fstop" || key == "focusDistance")
            visible = visible && advancedProperties->isChecked();
        if (e && e->type == "Group")
            visible = key == "name";
        form->setRowVisible(field, visible);
        field->setEnabled(e && !e->locked);
        field->clear();
        (void)key;
    }
    // Reserve the visible form's minimum width plus its vertical scrollbar. Qt 6.4 otherwise
    // lets the tabbed dock shrink below the form's width on small Linux desktops.
    auto *scroll = findChild<QScrollArea *>("inspectorScroll");
    scroll->setMinimumWidth(inspector->minimumSizeHint().width() +
                            scroll->verticalScrollBar()->sizeHint().width() + 2 * scroll->frameWidth());
    material->setEnabled(e && !e->locked);
    handle->setEnabled(e && !e->locked && e->type == "FurnitureModule");
    glass->setEnabled(handle->isEnabled());
    refreshLightControls();
    if (!e)
        return;
    auto set = [&](const QString &key, double v) {
        if (key == "width" || key == "height" || key == "depth" || key == "z" || key == "offset" ||
            key == "sill" || key == "size" || key == "sizeY" || key == "radius")
            v /= 10;
        fields[key]->setText(QString::number(v, 'f', key == "sunAngle" ? 3 : 2));
    };
    fields["name"]->setText(q(e->name));
    set("x", e->transform.x);
    set("y", e->transform.y);
    set("z", e->transform.z);
    set("yaw", e->transform.yaw);
    set("width", e->width);
    set("height", e->height);
    set("depth", e->depth);
    bool opening = e->type == "Door" || e->type == "Window";
    for (const auto &key : {"offset", "sill", "openAngle"}) {
        fields[key]->setEnabled(opening && !e->locked && (QString(key) != "openAngle" || e->type == "Door"));
        set(key, e->parameters.value(key, 0.0));
    }
    if (opening || e->type == "Room" || e->metadata.contains("automation"))
        for (const auto &key : {"x", "y", "z", "yaw"})
            fields[key]->setEnabled(false);
    if (e->type == "Room" || e->metadata.contains("automation"))
        for (const auto &key : {"width", "height", "depth"})
            fields[key]->setEnabled(false);
    material->setCurrentIndex(material->findData(q(e->material)));
    handle->setCurrentIndex(handle->findData(q(e->parameters.value("handle", std::string("bar")))));
    glass->setChecked(e->parameters.value("glass", false));
    fields["power"]->setEnabled(e->type == "Light" && !e->locked);
    set("power", e->parameters.value("power", 500.0));
    for (const auto &key : {"size", "sizeY", "radius", "sunAngle", "angle", "blend"})
        fields[key]->setEnabled(e->type == "Light" && !e->locked && fields[key]->isVisibleTo(inspector));
    set("size", e->parameters.value("size", 1000.0));
    set("sizeY", e->parameters.value("sizeY", 1000.0));
    set("radius", e->parameters.value("radius", 0.0));
    set("sunAngle", e->parameters.value("sunAngle", 0.526));
    set("angle", e->parameters.value("angle", 45.0));
    set("blend", e->parameters.value("blend", 0.3));
    auto color = e->type == "Light" ? e->parameters.value("color", Json::array({1.0, 0.89, 0.73}))
                                    : Json::array({1.0, 0.89, 0.73});
    selectedLightColor =
        QColor::fromRgbF(color[0].get<double>(), color[1].get<double>(), color[2].get<double>());
    lightColor->setText(selectedLightColor.name());
    lightColor->setEnabled(e->type == "Light" && !e->locked);
    {
        QSignalBlocker toneBlock(lightTone), shapeBlock(lightShape);
        const auto temperature = e->parameters.value("temperature", 3000);
        const bool kelvin = e->parameters.value("colorMode", "custom") == "kelvin";
        auto tone = lightTone->findData(temperature);
        lightTone->setCurrentIndex(kelvin ? (tone >= 0 ? tone : lightTone->findData(0))
                                          : lightTone->findData(-1));
        lightKelvin->setValue(temperature);
        lightShape->setCurrentIndex(
            lightShape->findData(q(e->parameters.value("shape", std::string("DISK")))));
    }
    refreshLightControls();
    fields["lens"]->setEnabled(e->type == "Camera" && !e->locked);
    set("lens", e->parameters.value("lens", 28.0));
    for (auto *key : {"fstop", "focusDistance"})
        fields[key]->setEnabled(e->type == "Camera" && !e->locked);
    set("fstop", e->parameters.value("fstop", 8.0));
    auto focusTarget = e->parameters.value("target", Json::array({2000, 1500, 1000}));
    const auto dx = focusTarget[0].get<double>() - e->transform.x,
               dy = focusTarget[1].get<double>() - e->transform.y,
               dz = focusTarget[2].get<double>() - e->transform.z;
    set("focusDistance", e->parameters.value("focusDistance", std::sqrt(dx * dx + dy * dy + dz * dz)));
    auto target = e->parameters.value("target", Json::array({2000, 1500, 1000}));
    for (int i = 0; i < 3; ++i) {
        auto key = QString("target%1").arg(QChar('X' + i));
        fields[key]->setEnabled((e->type == "Camera" || e->type == "Light") && !e->locked);
        set(key, target[i].get<double>());
    }
}
void MainWindow::refreshLightControls() {
    const Entity *light =
        selectedIds.size() == 1 && editor_.document().contains(selectedIds.first().toStdString())
            ? &editor_.document().at(selectedIds.first().toStdString())
            : nullptr;
    const bool active = light && light->type == "Light";
    auto *form = qobject_cast<QFormLayout *>(inspector->layout());
    form->setRowVisible(lightTone, active);
    form->setRowVisible(lightKelvin, active && lightTone->currentData().toInt() == 0);
    form->setRowVisible(lightColor, active && lightTone->currentData().toInt() == -1);
    const auto kind = active ? light->parameters.value("kind", "area") : std::string{};
    form->setRowVisible(lightShape, active && kind == "area");
    const bool widthVisible =
        active && (kind == "led" || (kind == "area" && lightShape->currentData() == "RECTANGLE"));
    form->setRowVisible(fields["sizeY"], widthVisible);
    fields["sizeY"]->setEnabled(widthVisible && !light->locked);
    for (auto *widget : {static_cast<QWidget *>(lightTone), static_cast<QWidget *>(lightKelvin),
                         static_cast<QWidget *>(lightShape)})
        widget->setEnabled(active && !light->locked);
}
void MainWindow::applyInspector() {
    if (selectedIds.size() != 1)
        return;
    auto id = selectedIds.first().toStdString();
    editor_.apply(tr("Editar propriedades"), [&](Document &d) {
        auto &e = d.at(id);
        if (e.locked)
            throw std::invalid_argument("Desbloqueie o objeto antes de editar");
        e.name = fields["name"]->text().toStdString();
        if (e.type == "Group")
            return;
        auto value = [&](const char *key, double previous) {
            const QString name = key;
            const double unit =
                (name == "width" || name == "height" || name == "depth" || name == "z" || name == "offset" ||
                 name == "sill" || name == "size" || name == "sizeY" || name == "radius")
                    ? 10
                    : 1;
            if (name == "sunAngle" || name == "blend")
                return fields[key]->isEnabled() ? evaluate(fields[key]->text().toStdString()) : previous;
            return fields[key]->isEnabled() ? millimeters(evaluate(fields[key]->text().toStdString()) * unit)
                                            : previous;
        };
        e.transform = {value("x", e.transform.x), value("y", e.transform.y), value("z", e.transform.z),
                       value("yaw", e.transform.yaw), e.transform.mirrored};
        e.width = value("width", e.width);
        e.height = value("height", e.height);
        e.depth = value("depth", e.depth);
        e.material = material->currentData().toString().toStdString();
        if (e.type == "MeshObject")
            e.parameters["originalMaterials"] = originalModelColors->isChecked();
        for (const auto *key : {"offset", "sill", "openAngle", "power", "lens", "size", "angle", "blend",
                                "fstop", "focusDistance", "sizeY", "radius", "sunAngle"})
            if (fields[key]->isEnabled())
                e.parameters[key] = value(key, 0);
        if (e.type == "Light" || e.type == "Camera")
            e.parameters["target"] = {value("targetX", 2000), value("targetY", 1500), value("targetZ", 1000)};
        if (e.type == "Light") {
            e.parameters["color"] = {selectedLightColor.redF(), selectedLightColor.greenF(),
                                     selectedLightColor.blueF()};
            const auto tone = lightTone->currentData().toInt();
            e.parameters["colorMode"] = tone < 0 ? "custom" : "kelvin";
            e.parameters["temperature"] = tone > 0 ? tone : lightKelvin->value();
            if (e.parameters.value("kind", "area") == "area")
                e.parameters["shape"] = lightShape->currentData().toString().toStdString();
            d.version = 3;
        }
        if (e.type == "FurnitureModule") {
            e.parameters["handle"] = handle->currentData().toString().toStdString();
            e.parameters["glass"] = glass->isChecked();
        }
    });
    viewport->select({id});
}
void MainWindow::newRoom() {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Adicionar um cômodo"));
    auto *layout = new QFormLayout(&dialog);
    auto *name = new QComboBox;
    name->setEditable(true);
    name->addItems({tr("Sala e cozinha"), tr("Quarto"), tr("Banheiro"), tr("Escritório"), tr("Varanda")});
    layout->addRow(tr("Nome"), name);
    auto *width = new QDoubleSpinBox, *depth = new QDoubleSpinBox;
    for (auto *input : {width, depth}) {
        input->setRange(1, 50);
        input->setDecimals(2);
        input->setSuffix(tr(" m"));
        input->setSingleStep(0.1);
    }
    width->setValue(4);
    depth->setValue(3);
    layout->addRow(tr("Largura"), width);
    layout->addRow(tr("Comprimento"), depth);
    auto *shape = new QComboBox;
    shape->addItems({tr("Retangular"), tr("Em L")});
    shape->setObjectName("roomShape");
    auto *cutWidth = new QDoubleSpinBox, *cutDepth = new QDoubleSpinBox;
    for (auto *input : {cutWidth, cutDepth}) {
        input->setRange(.5, 49);
        input->setDecimals(2);
        input->setSuffix(tr(" m"));
        input->setValue(1.5);
        input->setEnabled(false);
    }
    layout->addRow(tr("Formato"), shape);
    layout->addRow(tr("Largura do recuo"), cutWidth);
    layout->addRow(tr("Comprimento do recuo"), cutDepth);
    connect(shape, &QComboBox::currentIndexChanged, &dialog, [=](int index) {
        cutWidth->setEnabled(index == 1);
        cutDepth->setEnabled(index == 1);
    });
    auto *neighbor = new QComboBox;
    neighbor->addItem(tr("Primeiro cômodo"), "");
    for (const auto &e : editor_.document().entities)
        if (e.type == "Room")
            neighbor->addItem(q(e.name), q(e.id));
    if (neighbor->count() > 1) {
        neighbor->removeItem(0);
    }
    auto *side = new QComboBox;
    side->addItems({tr("À direita"), tr("Acima"), tr("À esquerda"), tr("Abaixo")});
    layout->addRow(tr("Ao lado de"), neighbor);
    layout->addRow(tr("Posição"), side);
    side->setEnabled(neighbor->count() > 0 && !neighbor->currentData().toString().isEmpty());
    auto *help = new QLabel(tr("As medidas são em metros. Os móveis usam centímetros no catálogo. Você pode "
                               "ajustar detalhes depois."));
    help->setWordWrap(true);
    help->setMaximumWidth(360);
    layout->addRow(help);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Criar cômodo"));
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return;
    double x = 0, y = 0;
    const auto roomId = neighbor->currentData().toString().toStdString();
    if (!roomId.empty()) {
        const auto &r = editor_.document().at(roomId);
        x = r.transform.x;
        y = r.transform.y;
        if (side->currentIndex() == 0)
            x += r.width;
        else if (side->currentIndex() == 1)
            y += r.depth;
        else if (side->currentIndex() == 2)
            x -= width->value() * 1000;
        else
            y -= depth->value() * 1000;
    }
    editor_.apply(tr("Adicionar %1").arg(name->currentText()), [&](Document &d) {
        if (shape->currentIndex() == 0)
            addPolygonRoom(d,
                           {{x, y},
                            {x + width->value() * 1000, y},
                            {x + width->value() * 1000, y + depth->value() * 1000},
                            {x, y + depth->value() * 1000}},
                           2700, 120, name->currentText().toStdString());
        else {
            const double w = width->value() * 1000, h = depth->value() * 1000, cw = cutWidth->value() * 1000,
                         ch = cutDepth->value() * 1000;
            if (cw >= w - 100 || ch >= h - 100)
                throw std::invalid_argument("O recuo precisa ser menor que o cômodo.");
            addPolygonRoom(d,
                           {{x, y},
                            {x + w, y},
                            {x + w, y + h - ch},
                            {x + w - cw, y + h - ch},
                            {x + w - cw, y + h},
                            {x, y + h}},
                           2700, 120, name->currentText().toStdString());
        }
    });
    viewport->setTool("select");
    viewport->frame();
}
void MainWindow::editRoomOutline() {
    auto id = roomPicker->currentData().toString().toStdString();
    if (selectedIds.size() == 1 && editor_.document().at(selectedIds.front().toStdString()).type == "Room")
        id = selectedIds.front().toStdString();
    if (id.empty())
        throw std::invalid_argument("Escolha o cômodo na lista acima da planta.");
    const auto room = editor_.document().at(id);
    if (!room.parameters.contains("outline"))
        throw std::invalid_argument("Este ajuste é para cômodos em L ou desenhados pelo contorno.");
    const auto outline = roomOutline(room);
    QDialog dialog(this);
    dialog.setObjectName("roomOutlineDialog");
    dialog.setWindowTitle(tr("Ajustar cantos de %1").arg(q(room.name)));
    dialog.resize(470, 440);
    auto *layout = new QVBoxLayout(&dialog);
    auto *help = new QLabel(tr("Medidas em metros, a partir do canto inicial do cômodo. Piso, forro e "
                               "paredes acompanham o contorno."));
    help->setWordWrap(true);
    layout->addWidget(help);
    auto *table = new QTableWidget(static_cast<int>(outline.size()), 2);
    table->setObjectName("roomOutlinePoints");
    table->setHorizontalHeaderLabels({tr("Distância horizontal (m)"), tr("Distância vertical (m)")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for (size_t i = 0; i < outline.size(); ++i)
        for (int axis = 0; axis < 2; ++axis)
            table->setItem(static_cast<int>(i), axis,
                           new QTableWidgetItem(QString::number(outline[i][axis] / 1000, 'f', 3)));
    layout->addWidget(table);
    auto *height = new QDoubleSpinBox;
    height->setObjectName("roomOutlineHeight");
    height->setRange(.5, 10);
    height->setValue(room.height / 1000);
    height->setSuffix(tr(" m de altura"));
    layout->addWidget(height);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return;
    Outline updated;
    for (int row = 0; row < table->rowCount(); ++row)
        updated.push_back({evaluate(table->item(row, 0)->text().replace(',', '.').toStdString()) * 1000,
                           evaluate(table->item(row, 1)->text().replace(',', '.').toStdString()) * 1000});
    editor_.apply(tr("Ajustar contorno do cômodo"),
                  [&](Document &d) { editPolygonRoom(d, id, updated, height->value() * 1000); });
}
void MainWindow::focusRoom() {
    showEditorWorkspace();
    const auto id = roomPicker->currentData().toString().toStdString();
    if (id.empty())
        viewport->frame();
    else
        viewport->frameRoom(id);
}
void MainWindow::modernApartmentStarter() {
    // Reuse the complete room, opening, camera and lighting setup.
    if (!discardOrSave())
        return;
    editor_.history.setClean();
    apartmentStarter();
    auto all = library->search({}, {}, false, false, false);
    editor_.apply(tr("Mobiliar apartamento moderno"), [&](Document &d) {
        d.name = "Apartamento moderno";
        std::erase_if(d.entities, [](const auto &e) {
            const auto asset = e.metadata.value("asset", std::string{});
            return asset == "ready-loungeSofa" || asset == "ready-tableCoffee" ||
                   asset == "ready-tableRound" || asset == "ready-bedDouble" ||
                   asset == "ready-cabinetBedDrawer";
        });
        auto add = [&](const char *id, double x, double y, double yaw = 0, double z = 0) {
            auto asset = std::find_if(all.begin(), all.end(), [&](const auto &a) { return a.id == id; });
            if (asset == all.end())
                throw std::runtime_error("Móvel moderno indisponível");
            Library::attachModel(d, library->withPayload(*asset));
            auto object = Library::instantiate(*asset, x, y);
            object.transform.yaw = yaw;
            object.transform.z = z;
            d.entities.push_back(object);
        };
        add("current-sofa-compact", 180, 1000);
        add("current-bed-queen", 7800, 3350, 180);
        add("current-nightstand", 5780, 3438, 180, 450);
        add("modern-modern_coffee_table_01", 3000, 2500, 90);
        add("modern-modern_arm_chair_01", 4250, 1750, 180);
        add("modern-coffee_table_round_01", 900, 4000);
        add("modern-side_table_01", 300, 2400);
        add("modern-ceramic_vase_02", 450, 2520, 0, 552.7);
        add("modern-modern_ceiling_lamp_01", 2500, 2800, 0, 1728.4);
    });
    category->setCurrentIndex(category->findData("__modern"));
    viewport->setTop(false);
    viewport->frame();
}
void MainWindow::apartmentStarter() {
    if (!discardOrSave())
        return;
    enterEditor();
    Document apartment;
    apartment.name = "Meu apartamento";
    addRectangularRoom(apartment, 5000, 6000, 2700, 120, 0, 0, "Sala e cozinha");
    std::erase_if(apartment.entities, [](const auto &e) {
        return e.type == "Wall" && std::abs(e.transform.x - 5000) < 1 && std::abs(e.width - 6000) < 1;
    });
    addRectangularRoom(apartment, 3000, 3500, 2700, 120, 5000, 0, "Quarto");
    addRectangularRoom(apartment, 3000, 2500, 2700, 120, 5000, 3500, "Banheiro");
    for (auto &object : apartment.entities)
        if (object.type == "Floor")
            object.material = "porcelain";
        else if (object.type == "Ceiling")
            object.visible = true;
    auto all = library->search({}, {}, false, false, false);
    auto add = [&](const char *id, double x, double y, double yaw = 0) {
        auto asset = std::find_if(all.begin(), all.end(), [&](const auto &a) { return a.id == id; });
        if (asset == all.end())
            throw std::runtime_error("Móvel pronto indisponível");
        auto object = Library::instantiate(*asset, x, y);
        object.transform.yaw = yaw;
        Library::attachModel(apartment, library->withPayload(*asset));
        apartment.entities.push_back(object);
    };
    add("ready-loungeSofa", 180, 1000, 0);
    add("ready-tableCoffee", 2050, 1600);
    add("ready-tableRound", 1000, 3700);
    add("ready-chair", 1850, 5400, 180);
    add("ready-kitchenFridge", 4300, 5938, 180);
    add("base-2", 1000, 5938, 180);
    add("drawer", 1600, 5938, 180);
    add("ready-bedDouble", 7800, 3350, 180);
    add("ready-cabinetBedDrawer", 5550, 2900);
    add("ready-toilet", 6750, 5350, 180);
    add("ready-bathroomSink", 7938, 3800, 90);
    add("ready-shower", 5200, 5000);
    std::vector<Entity> doors;
    for (const auto &wall : apartment.entities)
        if (wall.type == "Wall" && std::abs(wall.transform.x - 5000) < 1 &&
            std::abs(std::fmod(std::abs(wall.transform.yaw), 180) - 90) < 1 && wall.width >= 2500) {
            auto door = entity("Door", "Porta interna");
            door.parent = wall.id;
            door.width = 800;
            door.height = 2100;
            door.depth = wall.depth;
            door.parameters = {{"offset", 500}, {"sill", 0}, {"openAngle", 0}, {"hinge", "left"}};
            doors.push_back(door);
        }
    apartment.entities.insert(apartment.entities.end(), doors.begin(), doors.end());
    auto livingWindow = entity("Window", "Janela da sala");
    auto southWall = std::find_if(apartment.entities.begin(), apartment.entities.end(), [](const auto &e) {
        return e.type == "Wall" && e.transform.y == 0 && e.transform.x == 0;
    });
    livingWindow.parent = southWall->id;
    livingWindow.width = 1400;
    livingWindow.height = 1100;
    livingWindow.depth = 120;
    livingWindow.parameters = {{"offset", 2200}, {"sill", 900}};
    apartment.entities.push_back(livingWindow);
    std::vector<Entity> lights;
    for (const auto &room : apartment.entities)
        if (room.type == "Room") {
            auto light = entity("Light", "Luz de " + room.name);
            const auto x = room.transform.x + room.width / 2, y = room.transform.y + room.depth / 2;
            light.transform = {x, y, 2600, 0, false};
            light.parameters = {{"kind", "area"},
                                {"power", room.width * room.depth / 200000},
                                {"size", 1800},
                                {"target", {x, y, 0}},
                                {"color", {1.0, 0.94, 0.85}}};
            lights.push_back(light);
        }
    apartment.entities.insert(apartment.entities.end(), lights.begin(), lights.end());
    auto camera = entity("Camera", "Foto da sala");
    camera.transform = {2400, 5500, 1500, 0, false};
    camera.parameters = {{"target", {2000, 1800, 1100}}, {"lens", 20}, {"fstop", 8}};
    apartment.entities.push_back(camera);
    apartment.renderSettings["camera"] = camera.id;
    apartment.renderSettings["environmentMode"] = "sky";
    apartment.renderSettings["environmentStrength"] = 0.15;
    apartment.renderSettings["exposure"] = 0.6;
    apartment.validate();
    path.clear();
    selectedIds.clear();
    editor_.load(apartment);
    viewport->setTop(true);
    viewport->frame();
}
void MainWindow::opening(bool window) {
    if (selectedIds.size() != 1)
        throw std::invalid_argument("Selecione uma parede");
    auto id = selectedIds.first().toStdString();
    const auto &w = editor_.document().at(id);
    if ((w.type != "Wall" && w.type != "HalfWall") || w.locked)
        throw std::invalid_argument("Selecione uma parede desbloqueada");
    std::vector<double> v =
        window ? std::vector<double>{1200, 1000, 1000, 1000} : std::vector<double>{800, 2100, 200, 0};
    if (numericDialog(
            this, window ? tr("Janela associativa") : tr("Porta associativa"),
            {tr("Largura (mm)"), tr("Altura (mm)"), tr("Offset na parede (mm)"), tr("Peitoril (mm)")}, v))
        editor_.apply(tr("Inserir abertura"), [&](Document &d) {
            auto e = entity(window ? "Window" : "Door", window ? "Janela" : "Porta");
            e.parent = id;
            e.width = v[0];
            e.height = v[1];
            e.depth = w.depth;
            e.parameters = {{"offset", v[2]}, {"sill", v[3]}, {"hinge", "left"}, {"openAngle", 30}};
            d.entities.push_back(e);
        });
}
void MainWindow::insertAsset(const QString &id, const Entity &e) {
    auto all = library->search({}, {}, false, false, false);
    auto found = std::find_if(all.begin(), all.end(), [&](const auto &a) { return a.id == id; });
    if (found == all.end())
        throw std::invalid_argument("Asset não encontrado");
    editor_.apply(tr("Inserir %1").arg(found->name), [&](Document &d) {
        Library::attachModel(d, library->withPayload(*found));
        d.entities.push_back(e);
    });
    library->used(id);
    selectIds({q(e.id)});
    viewport->select({e.id});
}
void MainWindow::transform(const QString &mode) {
    if (selectedIds.isEmpty())
        return;
    editor_.apply(tr("Editar seleção"), [&](Document &d) {
        auto selected = ids(selectedIds);
        for (const auto &id : selected)
            if (d.at(id).locked && mode != "lock")
                throw std::invalid_argument("A seleção contém objetos bloqueados");
        if (mode == "delete") {
            eraseCascade(d, selected);
            return;
        }
        if (mode == "rotate") {
            for (const auto &id : selected) {
                auto &e = d.at(id);
                if (!movable(e))
                    continue;
                const double angle = e.transform.yaw * std::numbers::pi / 180;
                const double x = e.transform.x + (e.width * std::cos(angle) - e.depth * std::sin(angle)) / 2;
                const double y = e.transform.y + (e.width * std::sin(angle) + e.depth * std::cos(angle)) / 2;
                e.transform.yaw = std::fmod(e.transform.yaw + 90, 360);
                auto placement = placeObject(d, e, x, y, false);
                if (!placement.allowed)
                    throw std::invalid_argument(placement.message);
                e = placement.object;
            }
            return;
        }
        if (mode == "duplicate") {
            std::set<std::string> sources(selected.begin(), selected.end());
            bool more = true;
            while (more) {
                more = false;
                for (const auto &e : d.entities)
                    if (sources.contains(e.parent) && !sources.contains(e.id)) {
                        sources.insert(e.id);
                        more = true;
                    }
            }
            std::map<std::string, std::string> replacement;
            for (const auto &id : sources)
                replacement[id] = uuid();
            auto copy = d.entities;
            for (auto e : copy)
                if (sources.contains(e.id)) {
                    auto oldId = e.id;
                    e.id = replacement.at(oldId);
                    if (replacement.contains(e.parent))
                        e.parent = replacement.at(e.parent);
                    else if (e.type == "Door" || e.type == "Window") {
                        throw std::invalid_argument("Duplique a parede para duplicar suas aberturas");
                    } else
                        e.parent.clear();
                    e.name += " (cópia)";
                    e.transform.x += 200;
                    e.transform.y += 200;
                    if (e.metadata.contains("sources"))
                        for (auto &source : e.metadata["sources"]) {
                            auto old = source.get<std::string>();
                            if (replacement.contains(old))
                                source = replacement.at(old);
                        }
                    d.entities.push_back(e);
                }
            return;
        }
        for (const auto &id : selected) {
            auto &e = d.at(id);
            if (mode == "mirror") {
                if (e.type == "Wall" || e.type == "HalfWall" || e.type == "Door" || e.type == "Window" ||
                    e.type == "Room" || e.metadata.contains("automation"))
                    throw std::invalid_argument(
                        "Espelhamento disponível para móveis e geometrias independentes");
                e.transform.mirrored = !e.transform.mirrored;
            }
            if (mode == "visibility")
                e.visible = !e.visible;
            if (mode == "lock")
                e.locked = !e.locked;
        }
    });
}
void MainWindow::arrangeSelection(const QString &mode) {
    const auto selected = ids(selectedIds);
    std::string group;
    editor_.apply(tr("Organizar móveis"), [&](Document &d) {
        if (mode == "group")
            group = groupObjects(d, selected);
        else if (mode == "ungroup")
            ungroupObjects(d, selected);
        else
            arrangeObjects(d, selected, mode.toStdString());
    });
    if (!group.empty()) {
        selectIds({q(group)});
        viewport->select({group});
    }
}
void MainWindow::moveSelection() {
    std::vector<double> v{0, 0};
    if (numericDialog(
            this, tr("Mover os móveis juntos"),
            {tr("Para a direita (cm; negativo = esquerda)"), tr("Para cima (cm; negativo = baixo)")}, v))
        editor_.apply(tr("Mover seleção"),
                      [&](Document &d) { moveObjects(d, ids(selectedIds), v[0] * 10, v[1] * 10); });
}
void MainWindow::automate(const std::string &kind) {
    std::vector<double> v{30, 20};
    if (numericDialog(this, tr("Automação"), {tr("Espessura (mm)"), tr("Avanço (mm)")}, v))
        editor_.apply(tr("Criar automação"), [&](Document &d) {
            d.entities.push_back(automation(d, ids(selectedIds), kind, v[0], v[1]));
        });
}
void MainWindow::createLight() {
    QDialog dialog(this);
    dialog.setObjectName("newLightDialog");
    dialog.setWindowTitle(tr("Adicionar iluminação"));
    dialog.resize(480, 450);
    auto *form = new QFormLayout(&dialog);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    auto *description = new QLabel(
        tr("Escolha o tipo e o tom. A luz será colocada no cômodo atual; você pode ajustar depois."));
    description->setWordWrap(true);
    form->addRow(description);
    auto *kind = new QComboBox;
    kind->setObjectName("newLightKind");
    for (const auto &[label, id] :
         std::vector<std::pair<QString, QString>>{{tr("Spot no teto"), "spot"},
                                                  {tr("Fita LED"), "led"},
                                                  {tr("Painel de luz"), "area"},
                                                  {tr("Luz em todas as direções"), "point"},
                                                  {tr("Luz do sol"), "sun"}})
        kind->addItem(label, id);
    form->addRow(tr("Tipo"), kind);
    auto *name = new QLineEdit(kind->currentText());
    name->setObjectName("newLightName");
    form->addRow(tr("Nome"), name);
    auto *tone = new QComboBox;
    tone->setObjectName("newLightTone");
    for (int i = 0; i < lightTone->count(); ++i)
        tone->addItem(lightTone->itemText(i), lightTone->itemData(i));
    tone->setCurrentIndex(tone->findData(3000));
    form->addRow(tr("Tom da luz"), tone);
    auto *temperature = new QSpinBox;
    temperature->setObjectName("newLightKelvin");
    temperature->setRange(1000, 12000);
    temperature->setSingleStep(100);
    temperature->setValue(3000);
    temperature->setSuffix(tr(" K"));
    form->addRow(tr("Temperatura"), temperature);
    QColor color(Qt::white);
    auto *colorButton = new QPushButton(tr("Escolher cor…"));
    form->addRow(tr("Cor"), colorButton);
    connect(colorButton, &QPushButton::clicked, &dialog, [&] {
        const auto chosen =
            QColorDialog::getColor(color, &dialog, tr("Cor da luz"), QColorDialog::DontUseNativeDialog);
        if (chosen.isValid()) {
            color = chosen;
            colorButton->setText(color.name());
        }
    });
    auto *power = new QDoubleSpinBox;
    power->setObjectName("newLightPower");
    power->setRange(0, 100000);
    power->setValue(100);
    power->setToolTip(fields["power"]->toolTip());
    form->addRow(tr("Brilho"), power);
    auto *length = new QDoubleSpinBox, *width = new QDoubleSpinBox;
    length->setObjectName("newLightLength");
    width->setObjectName("newLightWidth");
    for (auto *dimension : {length, width}) {
        dimension->setRange(0.1, 10000);
        dimension->setSuffix(tr(" cm"));
        dimension->setDecimals(2);
        dimension->setValue(100);
    }
    form->addRow(tr("Comprimento"), length);
    form->addRow(tr("Largura"), width);
    auto *positionToggle = new QCheckBox(tr("Ajustar posição ao criar"));
    form->addRow(positionToggle);
    auto *position = new QWidget;
    auto *positionForm = new QFormLayout(position);
    positionForm->setContentsMargins(0, 0, 0, 0);
    double x = 2000, y = 1500, z = 2500;
    const auto roomId = roomPicker->currentData().toString().toStdString();
    const Entity *room = nullptr;
    for (const auto &object : editor_.document().entities)
        if (object.type == "Room" && (!room || object.id == roomId))
            room = &object;
    if (room) {
        x = room->transform.x + room->width / 2;
        y = room->transform.y + room->depth / 2;
        z = room->height - 200;
    }
    std::array<QDoubleSpinBox *, 3> coordinates{};
    const std::array<double, 3> initial{x, y, z};
    const QStringList labels{tr("Posição lateral"), tr("Posição no cômodo"), tr("Altura do piso")};
    for (int i = 0; i < 3; ++i) {
        coordinates[i] = new QDoubleSpinBox;
        coordinates[i]->setRange(i == 2 ? 1 : -1e6, 1e6);
        coordinates[i]->setDecimals(1);
        coordinates[i]->setSuffix(tr(" cm"));
        coordinates[i]->setValue(initial[i] / 10);
        coordinates[i]->setObjectName(QString("newLightPosition%1").arg(i));
        positionForm->addRow(labels[i], coordinates[i]);
    }
    form->addRow(position);
    position->hide();
    connect(positionToggle, &QCheckBox::toggled, position, &QWidget::setVisible);
    auto refresh = [&] {
        const bool extended = kind->currentData() == "led" || kind->currentData() == "area";
        form->setRowVisible(length, extended);
        form->setRowVisible(width, extended);
        form->setRowVisible(temperature, tone->currentData().toInt() == 0);
        form->setRowVisible(colorButton, tone->currentData().toInt() == -1);
    };
    QString previousName = kind->currentText();
    connect(kind, &QComboBox::currentIndexChanged, &dialog, [&] {
        const auto preset = lightEntity(kind->currentData().toString().toStdString());
        power->setValue(preset.parameters.at("power").get<double>());
        length->setValue(preset.parameters.at("size").get<double>() / 10);
        width->setValue(preset.parameters.at("sizeY").get<double>() / 10);
        const auto kelvin = preset.parameters.at("temperature").get<int>();
        temperature->setValue(kelvin);
        const auto toneIndex = tone->findData(kelvin);
        tone->setCurrentIndex(toneIndex >= 0 ? toneIndex : tone->findData(0));
        if (name->text() == previousName)
            name->setText(kind->currentText());
        previousName = kind->currentText();
        refresh();
    });
    connect(tone, &QComboBox::currentIndexChanged, &dialog, refresh);
    refresh();
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Adicionar luz"));
    buttons->setObjectName("newLightButtons");
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return;
    auto light = lightEntity(kind->currentData().toString().toStdString());
    light.name = name->text().trimmed().toStdString();
    light.transform = {coordinates[0]->value() * 10, coordinates[1]->value() * 10,
                       coordinates[2]->value() * 10, 0, false};
    light.parameters["target"] = {light.transform.x, light.transform.y, 0};
    light.parameters["power"] = power->value();
    light.parameters["size"] = length->value() * 10;
    light.parameters["sizeY"] = width->value() * 10;
    light.parameters["colorMode"] = tone->currentData().toInt() < 0 ? "custom" : "kelvin";
    light.parameters["temperature"] =
        tone->currentData().toInt() > 0 ? tone->currentData().toInt() : temperature->value();
    light.parameters["color"] = {color.redF(), color.greenF(), color.blueF()};
    editor_.apply(tr("Criar luz"), [&](Document &d) {
        d.version = 3;
        d.entities.push_back(light);
    });
    selectIds({q(light.id)});
    viewport->select({light.id});
    findChild<QDockWidget *>("propertiesDock")->raise();
}
void MainWindow::createCamera() {
    std::vector<double> v{2000, 4500, 2200, 2000, 1000, 1100, 28};
    if (numericDialog(this, tr("Nova câmera"),
                      {tr("X (mm)"), tr("Y (mm)"), tr("Z (mm)"), tr("Alvo X (mm)"), tr("Alvo Y (mm)"),
                       tr("Alvo Z (mm)"), tr("Lente (mm)")},
                      v))
        editor_.apply(tr("Criar câmera"), [&](Document &d) {
            auto e = entity("Camera", "Câmera");
            e.transform = {v[0], v[1], v[2], 0, false};
            e.parameters = {{"target", {v[3], v[4], v[5]}}, {"lens", v[6]}};
            d.entities.push_back(e);
        });
}
void MainWindow::roomLook(const QString &style) {
    const auto id = roomPicker->currentData().toString().toStdString();
    if (id.empty())
        throw std::invalid_argument("Escolha o cômodo na lista acima da planta para aplicar o estilo.");
    editor_.apply(tr("Preparar estilo de foto"),
                  [&](Document &d) { applyRoomLook(d, id, style.toStdString()); });
    statusBar()->showMessage(tr("Paredes, piso e luz preparados. Desfazer volta ao estilo anterior."), 10000);
}
void MainWindow::simpleCamera() {
    const auto &document = editor_.document();
    auto id = roomPicker->currentData().toString().toStdString();
    const Entity *room = nullptr;
    if (!id.empty() && document.contains(id))
        room = &document.at(id);
    if (!room)
        for (const auto &object : document.entities)
            if (object.type == "Room") {
                room = &object;
                break;
            }
    if (!room)
        throw std::invalid_argument("Crie um cômodo primeiro. Depois prepare a câmera.");
    auto camera = entity("Camera", "Foto de " + room->name);
    camera.transform = {room->transform.x + room->width * 0.5, room->transform.y + room->depth * 0.88, 1500,
                        0, false};
    camera.parameters = {
        {"target", {room->transform.x + room->width * 0.5, room->transform.y + room->depth * 0.24, 1100}},
        {"lens", 20},
        {"fstop", 8}};
    if (room->parameters.contains("outline")) {
        const auto center = roomInteriorPoint(*room);
        auto outline = roomOutline(*room, true);
        double cameraY = center[1];
        for (double offset = 100; offset < room->depth; offset += 100) {
            if (!insideOutline(outline, {center[0], center[1] + offset}, -120))
                break;
            cameraY = center[1] + offset;
        }
        camera.transform = {center[0], cameraY, 1500, 0, false};
        camera.parameters["target"] = {center[0], center[1], 1100};
    }
    editor_.apply(tr("Preparar câmera do cômodo"), [&](Document &d) {
        d.entities.push_back(camera);
        d.renderSettings["camera"] = camera.id;
    });
    statusBar()->showMessage(tr("Câmera preparada. Escolha a qualidade e clique em Criar imagem."), 8000);
}
Json MainWindow::selectedRenderOptions() const {
    auto options = renderPreset(renderQuality->currentData().toString());
    const bool custom = renderQuality->currentData() == "custom";
    const auto size = renderSize->currentData().toSize();
    options["width"] = size.width();
    options["height"] = size.height();
    options["device"] = renderDevice->currentData().toString().toStdString();
    options["format"] = renderFormat->currentText().toStdString();
    if (custom) {
        for (const auto &[key, field] : renderCounts)
            options[key.toStdString()] = field->value();
        for (const auto &[key, field] : renderValues)
            options[key.toStdString()] = field->value();
        options["denoise"] = renderDenoise->isChecked();
        options["transparent"] = customRender->findChild<QCheckBox *>("renderTransparent")->isChecked();
    }
    validateRenderOptions(options);
    return options;
}
void MainWindow::importHdri(const QString &provided) {
    const auto filename = provided.isEmpty()
                              ? QFileDialog::getOpenFileName(this, tr("Importar panorama de luz"), {},
                                                             tr("Panorama HDR/EXR (*.hdr *.exr)"))
                              : provided;
    if (filename.isEmpty())
        return;
    struct Result {
        ImportedEnvironment environment;
        QString error;
    };
    const auto project = editor_.document().id;
    const auto generation = ++environmentImportGeneration;
    auto *watcher = new QFutureWatcher<Result>(this);
    statusBar()->showMessage(tr("Lendo a imagem de luz. Você pode continuar editando."));
    connect(watcher, &QFutureWatcher<Result>::finished, this, [this, watcher, project, generation] {
        const auto result = watcher->result();
        watcher->deleteLater();
        if (generation != environmentImportGeneration)
            return;
        if (editor_.document().id != project) {
            statusBar()->showMessage(tr("O projeto mudou. Importe a imagem de luz no projeto desejado."),
                                     7000);
            return;
        }
        if (!result.error.isEmpty()) {
            statusBar()->showMessage(result.error, 10000);
            return;
        }
        protect([&] {
            editor_.apply(tr("Importar imagem de luz"),
                          [&](Document &document) { attachEnvironment(document, result.environment); });
        });
        statusBar()->showMessage(tr("Imagem de luz incorporada ao projeto."), 7000);
    });
    watcher->setFuture(QtConcurrent::run([filename]() -> Result {
        try {
            return {importEnvironment(filename), {}};
        } catch (const std::exception &error) {
            return {{}, QString::fromUtf8(error.what())};
        }
    }));
}
void MainWindow::renderScene() {
    if (renderCamera->currentData().toString().isEmpty())
        throw std::runtime_error("Prepare uma câmera do cômodo antes de criar a imagem.");
    const auto executable = BlenderBridge::findExecutable(blenderPath->text());
    if (executable.isEmpty())
        throw std::runtime_error("O mecanismo de render não foi encontrado. Reinstale o LibreMax para "
                                 "restaurá-lo ou escolha uma instalação Blender 4.5 LTS ou superior em "
                                 "Configurar Blender.");
    blenderPath->setText(executable);
    if (!testing)
        QSettings().setValue("blender", executable);
    applyRenderSettings();
    render->enqueue(RenderSnapshot(editor_.document(), selectedRenderOptions(),
                                   renderCamera->currentData().toString().toStdString()),
                    executable, resourceFile("scripts/cycles_render.py"));
    showRenderGallery();
}
void MainWindow::showEditorWorkspace() {
    if (galleryMode)
        restoreState(editorLayout);
    galleryMode = false;
    findChild<QWidget *>("workflowSteps")->show();
    placementBanner->show();
    workspace->setCurrentWidget(viewport);
}
void MainWindow::showRenderGallery() {
    if (!galleryMode) {
        editorLayout = saveState();
        galleryMode = true;
        for (auto *dock : findChildren<QDockWidget *>())
            dock->hide();
        for (auto *toolbar : findChildren<QToolBar *>())
            if (toolbar->objectName() != "projectToolbar")
                toolbar->hide();
        findChild<QWidget *>("workflowSteps")->hide();
        placementBanner->hide();
    }
    gallery->setProject(editor_.document().id);
    workspace->setCurrentWidget(gallery);
}
void MainWindow::refreshRenderQueue() {
    gallery->setProject(editor_.document().id);
    renderCancel->setEnabled(!render->active().isEmpty());
    renderStart->setEnabled(renderCamera->count() > 0);
    const auto entries = render->entries();
    const auto active = render->active().toStdString();
    auto entry =
        std::find_if(entries.begin(), entries.end(), [&](const auto &e) { return e.at("id") == active; });
    if (entry == entries.end()) {
        renderProgress->setRange(0, 100);
        renderProgress->setValue(0);
        renderProgress->setFormat(render->busy() ? tr("Na fila") : tr("Pronto"));
        renderState->setText(render->busy() ? tr("Preparando imagens. Você pode continuar editando.")
                                            : tr("Pronto. Abra Suas imagens para ver os resultados."));
        renderTiming->setText(render->busy() ? tr("A estimativa aparece quando o cálculo começar.")
                                             : QString{});
        return;
    }
    const auto progress = entry->value("progress", -1);
    renderProgress->setRange(0, progress < 0 ? 0 : 100);
    if (progress >= 0)
        renderProgress->setValue(progress);
    renderProgress->setFormat(entry->at("state") == "Rendering"
                                  ? tr("%p% do cálculo")
                                  : renderStateLabel(q(entry->at("state").get<std::string>())));
    renderTiming->setText(renderTimingText(*entry));
    renderState->setText(QString("%1 · %2").arg(q(entry->at("cameraName").get<std::string>()),
                                                renderStateLabel(q(entry->at("state").get<std::string>()))));
}
void MainWindow::applyRenderSettings() {
    const auto options = selectedRenderOptions();
    editor_.apply(tr("Configurar imagem"), [&](Document &d) {
        d.renderSettings["camera"] = renderCamera->currentData().toString().toStdString();
        d.renderSettings["exposure"] = renderExposure->value();
        d.renderSettings["environmentStrength"] = renderEnvironment->value();
        d.renderSettings["environmentMode"] = renderEnvironmentMode->currentData().toString().toStdString();
        d.renderSettings["sunElevation"] = renderSunElevation->value();
        d.renderSettings["sunRotation"] = renderSunRotation->value();
        if (d.renderSettings.contains("hdri")) {
            d.renderSettings["hdri"]["rotation"] = hdriRotation->value();
            d.renderSettings["hdri"]["visible"] = hdriVisible->isChecked();
        }
        d.renderSettings["denoise"] = options.at("denoise");
        d.renderSettings["cycles"] = options;
        if (options.at("format") == "EXR" || d.renderSettings.at("environmentMode") == "solid")
            d.version = std::max(2, d.version);
    });
}
void MainWindow::showRenderImage(const QString &filename) {
    if (!preview) {
        preview = new RenderPreview;
        connect(preview, &RenderPreview::backToGallery, this, &MainWindow::showRenderGallery);
        workspace->addWidget(preview);
    }
    QString label;
    for (const auto &entry : render->entries()) {
        const auto id = q(entry.at("id").get<std::string>());
        if (render->displayPath(id) == filename) {
            label = q(entry.at("cameraName").get<std::string>());
            if (entry.at("options").at("format") == "EXR")
                label += tr(" · EXR (prévia)");
            break;
        }
    }
    preview->open(filename, label);
    previewAction->setEnabled(true);
    workspace->setCurrentWidget(preview);
}
bool MainWindow::discardOrSave() {
    if (editor_.history.isClean())
        return true;
    auto choice = QMessageBox::question(
        this, tr("Salvar alterações?"), tr("O projeto tem alterações. Deseja salvar antes de continuar?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (choice == QMessageBox::Cancel)
        return false;
    if (choice == QMessageBox::Save)
        return saveProject();
    return true;
}
void MainWindow::importModel(const QString &providedFile) {
    const auto file =
        providedFile.isEmpty()
            ? QFileDialog::getOpenFileName(this, tr("Adicionar modelo 3D"), {},
                                           tr("Modelos 3D (*.glb *.gltf *.obj *.fbx *.stl *.ply)"))
            : providedFile;
    if (file.isEmpty())
        return;
    const auto executable = BlenderBridge::findExecutable(blenderPath->text());
    auto *job = new ModelImporter(this);
    auto *progress =
        new QProgressDialog(tr("Preparando o modelo e os acabamentos…"), tr("Cancelar"), 0, 0, this);
    progress->setObjectName("modelImportProgress");
    progress->setWindowTitle(tr("Adicionar modelo 3D"));
    progress->setMinimumDuration(0);
    progress->setAutoClose(false);
    connect(progress, &QProgressDialog::canceled, job, &ModelImporter::cancel);
    connect(job, &ModelImporter::failed, this, [this, job, progress](const QString &message) {
        progress->close();
        progress->deleteLater();
        statusBar()->showMessage(message, 20000);
        job->deleteLater();
    });
    connect(job, &ModelImporter::ready, this, [this, job, progress](Asset asset, const Json &details) {
        progress->close();
        progress->deleteLater();
        job->deleteLater();
        protect([&] {
            QDialog dialog(this);
            dialog.setObjectName("importModelDetails");
            dialog.setWindowTitle(tr("Confira o tamanho do modelo"));
            dialog.resize(450, 360);
            auto *form = new QFormLayout(&dialog);
            auto *name = new QLineEdit(asset.name);
            name->setObjectName("importModelName");
            form->addRow(tr("Nome no catálogo"), name);
            auto *width = new QDoubleSpinBox, *depth = new QDoubleSpinBox, *height = new QDoubleSpinBox;
            width->setObjectName("importModelWidth");
            depth->setObjectName("importModelDepth");
            height->setObjectName("importModelHeight");
            for (auto *field : {width, depth, height}) {
                field->setRange(.1, 10000);
                field->setDecimals(2);
                field->setSuffix(tr(" cm"));
            }
            width->setValue(asset.width / 10);
            depth->setValue(asset.depth / 10);
            height->setValue(asset.height / 10);
            form->addRow(tr("Largura"), width);
            form->addRow(tr("Comprimento"), depth);
            form->addRow(tr("Altura"), height);
            auto *placement = new QComboBox;
            placement->setObjectName("importModelPlacement");
            for (const auto &[label, id] :
                 std::vector<std::pair<QString, QString>>{{tr("No piso"), "floor"},
                                                          {tr("Na parede"), "wall"},
                                                          {tr("Sobre uma mesa ou móvel"), "surface"},
                                                          {tr("No teto"), "ceiling"}})
                placement->addItem(label, id);
            form->addRow(tr("Onde colocar"), placement);
            auto *note =
                new QLabel(details.value("simplified", false)
                               ? tr("Este modelo foi simplificado para caber no arquivo e usar menos "
                                    "memória. Confira as medidas antes de adicionar.")
                               : tr("Confira as medidas: alguns arquivos usam unidades diferentes. A origem "
                                    "e os acabamentos serão guardados junto com o modelo."));
            note->setWordWrap(true);
            form->addRow(note);
            auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            buttons->button(QDialogButtonBox::Ok)->setText(tr("Adicionar à biblioteca"));
            form->addRow(buttons);
            connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
            if (dialog.exec() != QDialog::Accepted)
                return;
            if (name->text().trimmed().isEmpty())
                throw std::invalid_argument("Dê um nome para o modelo.");
            asset.name = name->text().trimmed();
            asset.width = width->value() * 10;
            asset.depth = depth->value() * 10;
            asset.height = height->value() * 10;
            asset.recipe["parameters"]["placement"] = placement->currentData().toString().toStdString();
            asset.recipe["importDetails"] = details;
            storeUserAsset(*library, userModelsDirectory, asset);
            category->setCurrentIndex(0);
            search->setText(asset.name);
            refreshLibrary();
            viewport->beginPlacement(asset.id);
            statusBar()->showMessage(tr("Modelo adicionado. Clique no cômodo para colocar."), 10000);
        });
    });
    job->start(file, executable, resourceFile("scripts/import-model.py"));
}
void MainWindow::installCollection() {
    const auto file = QFileDialog::getOpenFileName(this, tr("Instalar ou atualizar coleção"), {},
                                                   tr("Coleções LibreMax (*.lmaxpack)"));
    if (file.isEmpty())
        return;
    auto *progress = new QProgressDialog(tr("Verificando modelos e acabamentos…"), {}, 0, 0, this);
    progress->setMinimumDuration(0);
    progress->setAutoClose(false);
    auto *watcher = new QFutureWatcher<ModelPack>(this);
    connect(watcher, &QFutureWatcher<ModelPack>::finished, this, [this, watcher, progress] {
        progress->close();
        progress->deleteLater();
        watcher->deleteLater();
        protect([&] {
            const auto pack = watcher->result();
            installModelPack(*library, userModelsDirectory, pack);
            category->setCurrentIndex(0);
            search->clear();
            refreshLibrary();
            statusBar()->showMessage(tr("%1: %2 modelos instalados, versão %3.")
                                         .arg(pack.name)
                                         .arg(pack.assets.size())
                                         .arg(pack.version),
                                     15000);
        });
    });
    watcher->setFuture(QtConcurrent::run([file] { return readModelPack(file); }));
}
void MainWindow::importDxf() {
    auto filename =
        QFileDialog::getOpenFileName(this, tr("Importar referência DXF"), {}, tr("DXF ASCII (*.dxf)"));
    if (filename.isEmpty())
        return;
    auto *worker = new QFutureWatcher<std::pair<DxfDrawing, QString>>(this);
    connect(worker, &QFutureWatcher<std::pair<DxfDrawing, QString>>::finished, this, [this, worker] {
        auto result = worker->result();
        worker->deleteLater();
        protect([&] {
            if (!result.second.isEmpty())
                throw std::runtime_error(result.second.toStdString());
            auto &drawing = result.first;
            QDialog dialog(this);
            dialog.setWindowTitle(tr("Unidade e layers DXF"));
            auto *layout = new QVBoxLayout(&dialog);
            auto *scale = new QLineEdit(QString::number(drawing.millimeterScale));
            layout->addWidget(new QLabel(tr("Milímetros por unidade do arquivo (mm=1, cm=10, m=1000):")));
            layout->addWidget(scale);
            std::vector<std::pair<std::string, QCheckBox *>> layers;
            for (const auto &[name, primitives] : drawing.layers) {
                auto *checkbox = new QCheckBox(q(name) + QString(" (%1 segmentos)").arg(primitives.size()));
                checkbox->setChecked(true);
                layout->addWidget(checkbox);
                layers.emplace_back(name, checkbox);
            }
            if (!drawing.unsupported.empty()) {
                QStringList names;
                for (const auto &type : drawing.unsupported)
                    names << q(type);
                auto *warning = new QLabel(tr("Entidades ignoradas: %1").arg(names.join(", ")));
                warning->setWordWrap(true);
                layout->addWidget(warning);
            }
            auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            layout->addWidget(buttons);
            connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
            if (dialog.exec() != QDialog::Accepted)
                return;
            std::vector<std::string> selected;
            for (const auto &[name, checkbox] : layers)
                if (checkbox->isChecked())
                    selected.push_back(name);
            if (selected.empty())
                throw std::runtime_error("Selecione ao menos um layer");
            auto reference = dxfEntities(drawing, evaluate(scale->text().toStdString()), selected);
            editor_.apply(tr("Importar DXF"), [&](Document &d) {
                d.entities.insert(d.entities.end(), reference.begin(), reference.end());
            });
            viewport->setTop(true);
            viewport->frame();
        });
    });
    worker->setFuture(QtConcurrent::run([filename]() -> std::pair<DxfDrawing, QString> {
        try {
            return {readDxf(filename), {}};
        } catch (const std::exception &e) {
            return {{}, QString::fromUtf8(e.what())};
        }
    }));
}
void MainWindow::importTexture() {
    auto filename = QFileDialog::getOpenFileName(this, tr("Importar textura"), {},
                                                 tr("Imagens (*.jpg *.jpeg *.png *.webp)"));
    if (filename.isEmpty())
        return;
    auto selected = selectedIds;
    auto projectId = editor_.document().id;
    auto *worker = new QFutureWatcher<std::pair<ImportedTexture, QString>>(this);
    connect(worker, &QFutureWatcher<std::pair<ImportedTexture, QString>>::finished, this,
            [this, worker, selected, projectId] {
                auto result = worker->result();
                worker->deleteLater();
                protect([&] {
                    if (!result.second.isEmpty())
                        throw std::runtime_error(result.second.toStdString());
                    if (editor_.document().id != projectId)
                        throw std::runtime_error("O projeto mudou durante a importação; importe novamente");
                    editor_.apply(tr("Incorporar textura"), [&](Document &d) {
                        auto materialId = attachTexture(d, result.first);
                        for (const auto &id : selected)
                            if (d.contains(id.toStdString())) {
                                auto &e = d.at(id.toStdString());
                                if (e.locked)
                                    throw std::runtime_error(
                                        "Desbloqueie a seleção antes de aplicar textura");
                                e.material = materialId;
                            }
                    });
                });
            });
    worker->setFuture(QtConcurrent::run([filename]() -> std::pair<ImportedTexture, QString> {
        try {
            return {readTexture(filename), {}};
        } catch (const std::exception &e) {
            return {{}, QString::fromUtf8(e.what())};
        }
    }));
}
void MainWindow::activatePbrMaterials() {
    const auto projectId = editor_.document().id;
    auto *worker = new QFutureWatcher<std::pair<PbrMaterialPack, QString>>(this);
    connect(worker, &QFutureWatcher<std::pair<PbrMaterialPack, QString>>::finished, this,
            [this, worker, projectId] {
                auto result = worker->result();
                worker->deleteLater();
                protect([&] {
                    if (!result.second.isEmpty())
                        throw std::runtime_error(result.second.toStdString());
                    if (editor_.document().id != projectId)
                        throw std::runtime_error("O projeto mudou; ative os materiais novamente");
                    editor_.apply(tr("Incorporar materiais PBR"),
                                  [&](Document &d) { attachPbrMaterials(d, result.first); });
                    statusBar()->showMessage(tr("Materiais realistas incorporados ao projeto"), 5000);
                });
            });
    auto directory = resourceFile("starter-materials");
    statusBar()->showMessage(tr("Preparando materiais realistas…"));
    worker->setFuture(QtConcurrent::run([directory]() -> std::pair<PbrMaterialPack, QString> {
        try {
            return {readPbrMaterials(directory), {}};
        } catch (const std::exception &e) {
            return {{}, QString::fromUtf8(e.what())};
        }
    }));
}
void MainWindow::loadProject(const QString &filename) {
    auto document = ProjectStore::open(filename);
    enterEditor();
    editor_.load(document);
    path = filename;
    selectedIds.clear();
    refreshInspector();
    viewport->frame();
    if (testing)
        return;
    auto recent = QSettings().value("recent").toStringList();
    recent.removeAll(filename);
    recent.prepend(filename);
    while (recent.size() > 10)
        recent.removeLast();
    QSettings().setValue("recent", recent);
    rememberProject();
}
bool MainWindow::saveProject(bool saveAs) {
    QString destination = path;
    if (saveAs || destination.isEmpty())
        destination =
            QFileDialog::getSaveFileName(this, tr("Salvar projeto"), destination, tr("LibreMax (*.lmx)"));
    if (destination.isEmpty())
        return false;
    if (!destination.endsWith(".lmx", Qt::CaseInsensitive))
        destination += ".lmx";
    if (editor_.document().name == "Projeto sem título")
        editor_.apply(tr("Nomear projeto"),
                      [&](Document &d) { d.name = QFileInfo(destination).completeBaseName().toStdString(); });
    ProjectStore::save(destination, editor_.document());
    path = destination;
    editor_.history.setClean();
    // A successful manual save supersedes this project's recovery snapshots.
    recovery.clear(editor_.document().id);
    statusBar()->showMessage(tr("Projeto salvo com segurança"), 5000);
    rememberProject();
    return true;
}
void MainWindow::rememberProject() {
    if (path.isEmpty())
        return;
    QImage cover;
    try {
        const auto filename = dataRoot() + "/project-cover.png";
        viewport->capture(filename);
        cover.load(filename);
        projects->remember(path, editor_.document(), cover);
    } catch (const std::exception &error) {
        spdlog::warn("Project library: {}", error.what());
        try {
            projects->remember(path, editor_.document());
        } catch (...) {
        }
    }
}
void MainWindow::enterEditor() {
    galleryMode = false;
    findChild<QWidget *>("workflowSteps")->show();
    placementBanner->show();
    statusBar()->show();
    for (auto *action : menuBar()->actions())
        action->setVisible(true);
    rootPages->setCurrentWidget(editorPage);
    for (auto *dock : findChildren<QDockWidget *>())
        dock->show();
    for (auto *toolbar : findChildren<QToolBar *>())
        toolbar->show();
    if (visibleAssets.empty())
        refreshLibrary();
    if (auto *dock = findChild<QDockWidget *>("libraryDock"))
        dock->raise();
    if (auto *dock = findChild<QDockWidget *>("propertiesDock"))
        dock->raise();
}
void MainWindow::showHome() {
    if (!path.isEmpty())
        rememberProject();
    std::vector<RecentProject> records;
    QString warning;
    try {
        records = projects->projects();
    } catch (const std::exception &error) {
        spdlog::warn("Project index: {}", error.what());
        warning = tr("A lista de projetos não pôde ser lida. Seus arquivos estão preservados; use Abrir "
                     "arquivo .lmx.");
    }
    if (!testing)
        for (const auto &filename : QSettings().value("recent").toStringList()) {
            if (std::any_of(records.begin(), records.end(),
                            [&](const auto &entry) { return entry.path == filename; }))
                continue;
            const QFileInfo file(filename);
            records.push_back({filename,
                               file.completeBaseName(),
                               file.lastModified().toUTC().toString(Qt::ISODate),
                               {},
                               file.exists()});
        }
    home->setProjects(records);
    if (!warning.isEmpty())
        home->showIndexWarning(warning);
    statusBar()->hide();
    for (auto *action : menuBar()->actions())
        action->setVisible(action->text() == tr("&Arquivo") || action->text() == tr("A&juda"));
    rootPages->setCurrentIndex(1);
    for (auto *dock : findChildren<QDockWidget *>())
        dock->hide();
    for (auto *toolbar : findChildren<QToolBar *>())
        toolbar->hide();
}
void MainWindow::autosave() {
    if (editor_.history.isClean())
        return;
    recovery.save(editor_.document());
}
void MainWindow::recover() {
    QStringList errors;
    auto entries = recovery.entries(&errors);
    for (const auto &error : errors)
        spdlog::warn("Recovery: {}", error.toStdString());
    if (!errors.isEmpty())
        statusBar()->showMessage(
            tr("%1 autosave(s) não puderam ser lidos; arquivos preservados").arg(errors.size()), 15000);
    if (entries.empty())
        return;
    std::size_t index = 0;
    if (entries.size() > 1) {
        QStringList labels;
        for (const auto &entry : entries)
            labels.append(tr("%1 — %2 — %3")
                              .arg(q(entry.name), entry.file.lastModified().toString("dd/MM/yyyy HH:mm:ss"),
                                   entry.file.fileName()));
        bool accepted = false;
        auto selected = QInputDialog::getItem(this, tr("Versões recuperáveis"),
                                              tr("Escolha o projeto e a versão a recuperar:"), labels, 0,
                                              false, &accepted);
        if (!accepted)
            return;
        index = static_cast<std::size_t>(labels.indexOf(selected));
    }
    const auto &latest = entries.at(index);
    auto recovered = ProjectStore::open(latest.file.absoluteFilePath());
    auto choice = QMessageBox::question(
        this, tr("Recuperar projeto"),
        tr("Encontramos uma versão recuperável de “%1”.\nAutosave: %2\nRecuperar esta versão?")
            .arg(q(recovered.name), latest.file.lastModified().toString("dd/MM/yyyy HH:mm")),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (choice == QMessageBox::Yes && discardOrSave()) {
        enterEditor();
        editor_.load(recovered);
        path.clear();
        editor_.apply(tr("Recuperar autosave"), [](Document &d) { d.name += " (recuperado)"; });
        viewport->frame();
    }
}
void MainWindow::closeEvent(QCloseEvent *event) {
    if (testing) {
        event->accept();
        return;
    }
    try {
        if (!discardOrSave()) {
            event->ignore();
            return;
        }
        if (render->busy()) {
            auto choice = QMessageBox::question(this, tr("Render em andamento"),
                                                tr("Cancelar as imagens na fila e fechar?"));
            if (choice != QMessageBox::Yes) {
                event->ignore();
                return;
            }
            render->cancelAll();
        }
        recovery.clear(editor_.document().id);
        event->accept();
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Falha ao salvar"), QString::fromUtf8(e.what()));
        event->ignore();
    }
}
} // namespace lmx
