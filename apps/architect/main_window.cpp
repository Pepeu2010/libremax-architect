#include "main_window.h"
#include "core/expression.h"
#include "geometry/geometry.h"
#include "import/dxf.h"
#include "materials/texture.h"
#include "persistence/project_store.h"
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
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <algorithm>
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
    auto installed = QApplication::applicationDirPath() + "/../share/libremax/" + relative;
    return QFileInfo::exists(installed) ? installed : QStringLiteral(LMX_SOURCE_DIR) + "/" + relative;
}
} // namespace
MainWindow::MainWindow(bool test, const QString &recoveryDirectory)
    : recovery(recoveryDirectory.isEmpty() ? dataRoot() + "/recovery" : recoveryDirectory), testing(test) {
    setObjectName("mainWindow");
    setWindowTitle(tr("LibreMax Architect"));
    resize(1440, 900);
    setMinimumSize(900, 600);
    library = std::make_unique<Library>(dataRoot() + "/library.db");
    QFile catalog(resourceFile("starter-library/catalog.json"));
    if (!catalog.open(QIODevice::ReadOnly))
        throw std::runtime_error("Starter Library não encontrada");
    library->seed(Json::parse(catalog.readAll().toStdString()));
    createShell();
    connect(&thumbnails, &AssetThumbnails::ready, this, [this](const QString &, const QImage &) {
        for (int i = 0; i < assets->count(); ++i) {
            auto *item = assets->item(i);
            for (const auto &asset : visibleAssets)
                if (asset.id == item->data(Qt::UserRole).toString()) {
                    auto image = thumbnails.request(asset);
                    if (!image.isNull()) {
                        item->setIcon(QIcon(QPixmap::fromImage(image)));
                        item->setData(Qt::UserRole + 2, true);
                    }
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
    connect(viewport, &CadView::assetDropped, this,
            [this](const QString &id, double x, double y) { protect([&] { insertAsset(id, x, y); }); });
    connect(viewport, &CadView::coordinates, this,
            [this](const QString &text) { statusBar()->showMessage(text); });
    connect(viewport, &CadView::failure, this, [this](const QString &text) {
        spdlog::error("Viewport: {}", text.toStdString());
        statusBar()->showMessage(text, 12000);
    });
    viewport->assetResolver([this](const QString &id) -> std::optional<Asset> {
        for (const auto &a : visibleAssets)
            if (a.id == id)
                return a;
        return std::nullopt;
    });
    connect(&render, &RenderJob::state, this, [this](const QString &state) {
        renderState->setText(state);
        const bool busy = render.busy();
        renderStart->setEnabled(!busy && renderCamera->count() > 0);
        renderCancel->setEnabled(busy);
        renderProgress->setRange(0, busy ? 0 : 100);
        renderProgress->setValue(state == tr("Concluído") ? 100 : 0);
        renderLog->appendPlainText(state);
        statusBar()->showMessage(tr("Render: %1").arg(state));
    });
    connect(&render, &RenderJob::log, renderLog, &QPlainTextEdit::appendPlainText);
    connect(&render, &RenderJob::completed, this, [this](const QString &file) {
        renderLog->appendPlainText(tr("Imagem salva: %1").arg(file));
        protect([&] { showRenderImage(file); });
    });
    connect(&autosaveTimer, &QTimer::timeout, this, [this] { protect([&] { autosave(); }); });
    auto interval = std::clamp(QSettings().value("autosaveMinutes", 5).toInt(), 1, 60);
    autosaveTimer.start(interval * 60 * 1000);
    refreshLibrary();
    refreshScene();
    if (!testing)
        QTimer::singleShot(400, this, [this] { protect([&] { recover(); }); });
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
    setCentralWidget(workspace);
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
    action(file, tr("Importar textura JPG/PNG…"), {}, [this] { importTexture(); });
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
    auto *environment = menuBar()->addMenu(tr("&Ambiente"));
    action(environment, tr("Ambiente retangular…"), {}, [this] { newRoom(); });
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
    auto *automationMenu = menuBar()->addMenu(tr("A&utomação"));
    for (const auto &[label, kind] :
         std::vector<std::pair<QString, std::string>>{{tr("Tampo"), "countertop"},
                                                      {tr("Rodatampo"), "backsplash"},
                                                      {tr("Rodapé"), "plinth"},
                                                      {tr("Rodaforro"), "cornice"},
                                                      {tr("Fechamento lateral"), "closure"},
                                                      {tr("Envelopamento"), "envelope"}})
        action(automationMenu, label + "…", {}, [this, kind] { automate(kind); });
    auto *geometryMenu = menuBar()->addMenu(tr("&Geometria"));
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
    action(viewMenu, tr("Planta superior"), QKeySequence("1"), [this] { viewport->setTop(true); });
    action(viewMenu, tr("Isométrica 3D"), QKeySequence("3"), [this] { viewport->setTop(false); });
    action(viewMenu, tr("Enquadrar projeto"), QKeySequence("F"), [this] { viewport->frame(); });
    auto *help = menuBar()->addMenu(tr("A&juda"));
    action(help, tr("Sobre LibreMax"), {}, [this] {
        QMessageBox::about(this, tr("LibreMax Architect"),
                           tr("LibreMax Architect 0.2.0 — desenvolvimento\nEditor nativo C++20 / Qt / "
                              "OpenCASCADE\nCódigo GPL-3.0-or-later · Biblioteca procedural CC0\nA paridade "
                              "completa e os pacotes Linux ainda estão em desenvolvimento."));
    });
    auto *projectBar = addToolBar(tr("Projeto"));
    projectBar->setMovable(false);
    projectBar->setIconSize({18, 18});
    projectBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto *brand = new QLabel(tr("  LIBREMAX  /  ARCHITECT  "));
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
    auto *renderShortcut = projectBar->addAction(studioIcon("render"), tr("Render"));
    connect(renderShortcut, &QAction::triggered, this, [this] {
        auto *dock = findChild<QDockWidget *>("renderDock");
        dock->show();
        dock->raise();
    });
    addToolBarBreak();
    auto *toolbar = addToolBar(tr("Projeto e desenho"));
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
    automations->setText(tr("Automação"));
    automations->setPopupMode(QToolButton::InstantPopup);
    automations->setMenu(automationMenu);
    toolbar->addWidget(automations);
    toolbar->addSeparator();
    auto *cutaway = new QCheckBox(tr("Abrir vista"));
    cutaway->setChecked(true);
    cutaway->setToolTip(tr("Oculta paredes próximas somente na vista 3D; projeto e render são preservados."));
    toolbar->addWidget(cutaway);
    connect(cutaway, &QCheckBox::toggled, viewport, &CadView::setCutaway);
    auto *top = toolbar->addAction(studioIcon("plan"), tr("Planta"));
    connect(top, &QAction::triggered, this, [this] {
        workspace->setCurrentWidget(viewport);
        viewport->setTop(true);
    });
    auto *iso = toolbar->addAction(studioIcon("cube"), tr("3D"));
    connect(iso, &QAction::triggered, this, [this] {
        workspace->setCurrentWidget(viewport);
        viewport->setTop(false);
    });
    auto *frame = toolbar->addAction(studioIcon("frame"), tr("Enquadrar"));
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
    auto *libraryDock = new QDockWidget(tr("Biblioteca"), this);
    libraryDock->setObjectName("libraryDock");
    auto *libraryPanel = new QWidget;
    auto *libraryLayout = new QVBoxLayout(libraryPanel);
    search = new QLineEdit;
    search->setPlaceholderText(tr("Pesquisar móveis e objetos…"));
    search->setAccessibleName(tr("Pesquisar biblioteca"));
    libraryLayout->addWidget(search);
    category = new QComboBox;
    category->addItem(tr("Todos os ambientes"), "");
    for (const auto &cat : {"Cozinha", "Dormitório", "Sala", "Escritório", "Decoração", "Eletrodomésticos"})
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
    assets->setMouseTracking(true);
    assets->setIconSize({76, 58});
    assets->setDragEnabled(true);
    assets->setAccessibleName(tr("Móveis disponíveis"));
    libraryLayout->addWidget(assets);
    libraryCount = new QLabel;
    libraryCount->setObjectName("libraryCount");
    libraryCount->setProperty("role", "muted");
    libraryLayout->addWidget(libraryCount);
    auto *favorite = new QPushButton(tr("Alternar favorito"));
    libraryLayout->addWidget(favorite);
    auto *instruction = new QLabel(tr("Arraste um item para a planta.\nDuplo clique insere na origem."));
    instruction->setWordWrap(true);
    libraryLayout->addWidget(instruction);
    libraryDock->setWidget(libraryPanel);
    addDockWidget(Qt::LeftDockWidgetArea, libraryDock);
    connect(search, &QLineEdit::textChanged, this, [this] { protect([&] { refreshLibrary(); }); });
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
        protect([&] { insertAsset(item->data(Qt::UserRole).toString(), 0, 0); });
    });
    auto *sceneDock = new QDockWidget(tr("Cena"), this);
    sceneDock->setObjectName("sceneDock");
    tree = new QTreeWidget;
    tree->setObjectName("sceneTree");
    tree->setHeaderLabels({tr("Objeto"), tr("Estado")});
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    sceneDock->setWidget(tree);
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock);
    splitDockWidget(libraryDock, sceneDock, Qt::Vertical);
    connect(tree, &QTreeWidget::itemSelectionChanged, this, [this] {
        if (refreshing)
            return;
        QStringList list;
        for (auto *item : tree->selectedItems())
            list << item->data(0, Qt::UserRole).toString();
        selectIds(list);
        viewport->select(ids(list));
    });
    auto *propertyDock = new QDockWidget(tr("Propriedades"), this);
    propertyDock->setObjectName("propertiesDock");
    auto *scroll = new QScrollArea;
    scroll->setObjectName("inspectorScroll");
    scroll->setWidgetResizable(true);
    inspector = new QWidget;
    auto *propertyLayout = new QFormLayout(inspector);
    propertyLayout->setContentsMargins(16, 12, 16, 16);
    propertyLayout->setVerticalSpacing(9);
    propertyLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    selectionTitle = new QLabel(tr("Nenhum objeto selecionado"));
    selectionTitle->setWordWrap(true);
    selectionTitle->setStyleSheet("font-size:16px;font-weight:600;padding:8px 0");
    propertyLayout->addRow(selectionTitle);
    for (const auto &[key, label] :
         std::vector<std::pair<QString, QString>>{{"name", tr("Nome")},
                                                  {"x", tr("X (mm)")},
                                                  {"y", tr("Y (mm)")},
                                                  {"z", tr("Z (mm)")},
                                                  {"yaw", tr("Rotação (°)")},
                                                  {"width", tr("Largura (mm)")},
                                                  {"height", tr("Altura (mm)")},
                                                  {"depth", tr("Profundidade (mm)")},
                                                  {"offset", tr("Offset da abertura (mm)")},
                                                  {"sill", tr("Peitoril (mm)")},
                                                  {"openAngle", tr("Abertura da porta (°)")},
                                                  {"power", tr("Potência (W)")},
                                                  {"size", tr("Área de luz (mm)")},
                                                  {"angle", tr("Feixe spot (°)")},
                                                  {"blend", tr("Suavidade (0–1)")},
                                                  {"targetX", tr("Alvo X (mm)")},
                                                  {"targetY", tr("Alvo Y (mm)")},
                                                  {"targetZ", tr("Alvo Z (mm)")},
                                                  {"lens", tr("Lente (mm)")}}) {
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
    lightColor = new QPushButton(tr("Escolher cor…"));
    lightColor->setObjectName("lightColor");
    propertyLayout->addRow(tr("Cor da luz"), lightColor);
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
    hint = new QLabel(
        tr("Valores aceitam expressões: 800+20, 1200/2.\nCtrl+clique permite selecionar vários objetos."));
    hint->setWordWrap(true);
    propertyLayout->addRow(hint);
    scroll->setWidget(inspector);
    propertyDock->setWidget(scroll);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock);
    auto *renderDock = new QDockWidget(tr("Render"), this);
    renderDock->setObjectName("renderDock");
    auto *renderScroll = new QScrollArea;
    renderScroll->setObjectName("renderScroll");
    renderScroll->setWidgetResizable(true);
    auto *renderPanel = new QWidget;
    auto *renderLayout = new QFormLayout(renderPanel);
    renderLayout->setContentsMargins(16, 12, 16, 16);
    renderLayout->setVerticalSpacing(12);
    renderLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto *heading = new QLabel(tr("Apresentação"));
    heading->setProperty("role", "heading");
    renderLayout->addRow(heading);
    auto *description = new QLabel(tr("Componha a câmera, equilibre a luz e gere sua imagem."));
    description->setWordWrap(true);
    description->setProperty("role", "muted");
    renderLayout->addRow(description);
    renderCamera = new QComboBox;
    renderCamera->setObjectName("renderCamera");
    renderCamera->setAccessibleName(tr("Câmera para renderizar"));
    renderLayout->addRow(tr("Câmera"), renderCamera);
    renderQuality = new QComboBox;
    renderQuality->setObjectName("renderQuality");
    renderQuality->addItem(tr("Rascunho · 640 × 360"), 16);
    renderQuality->addItem(tr("Prévia · 1280 × 720"), 64);
    renderQuality->addItem(tr("Alta · 1920 × 1080"), 256);
    renderQuality->addItem(tr("Final · 3840 × 2160"), 512);
    renderQuality->setCurrentIndex(1);
    renderLayout->addRow(tr("Qualidade"), renderQuality);
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
    renderLayout->addRow(tr("Luz ambiente"), renderEnvironment);
    renderDevice = new QComboBox;
    renderDevice->addItem(tr("GPU / CPU automático"), "AUTO");
    renderDevice->addItem(tr("CPU"), "CPU");
    renderLayout->addRow(tr("Dispositivo"), renderDevice);
    renderDenoise = new QCheckBox(tr("Reduzir ruído (denoise)"));
    renderDenoise->setObjectName("renderDenoise");
    renderDenoise->setChecked(true);
    renderLayout->addRow(renderDenoise);
    auto *engineToggle = new QPushButton(tr("Configurar Blender…"));
    engineToggle->setProperty("role", "quiet");
    engineToggle->setCheckable(true);
    renderLayout->addRow(engineToggle);
    auto *enginePanel = new QWidget;
    auto *engineLayout = new QVBoxLayout(enginePanel);
    engineLayout->setContentsMargins(0, 0, 0, 0);
    blenderPath =
        new QLineEdit(QSettings().value("blender", QStandardPaths::findExecutable("blender")).toString());
    blenderPath->setAccessibleName(tr("Executável Blender"));
#ifdef Q_OS_WIN
    if (blenderPath->text().isEmpty() &&
        QFileInfo::exists("C:/Program Files/Blender Foundation/Blender 5.2/blender.exe"))
        blenderPath->setText("C:/Program Files/Blender Foundation/Blender 5.2/blender.exe");
#endif
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
    renderProgress->setTextVisible(false);
    renderProgress->setRange(0, 100);
    renderProgress->setValue(0);
    renderProgress->setAccessibleName(tr("Estado do render"));
    renderLayout->addRow(renderProgress);
    renderStart = new QPushButton(tr("Renderizar imagem…"));
    renderStart->setObjectName("startRender");
    renderStart->setProperty("role", "primary");
    renderCancel = new QPushButton(tr("Cancelar"));
    renderCancel->setEnabled(false);
    renderLayout->addRow(renderStart);
    renderLayout->addRow(renderCancel);
    connect(renderStart, &QPushButton::clicked, this, [this] { protect([&] { renderScene(); }); });
    connect(renderCancel, &QPushButton::clicked, &render, &RenderJob::cancel);
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
    for (auto *field : {static_cast<QWidget *>(renderCamera), static_cast<QWidget *>(renderQuality),
                        static_cast<QWidget *>(renderExposure), static_cast<QWidget *>(renderEnvironment),
                        static_cast<QWidget *>(renderDevice)}) {
        field->setMinimumWidth(76);
        field->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    }
    renderScroll->setWidget(renderPanel);
    renderDock->setWidget(renderScroll);
    addDockWidget(Qt::RightDockWidgetArea, renderDock);
    tabifyDockWidget(propertyDock, renderDock);
    propertyDock->raise();
    for (auto *dock : {libraryDock, sceneDock, propertyDock, renderDock})
        viewMenu->addAction(dock->toggleViewAction());
    resizeDocks({libraryDock, propertyDock}, {300, 320}, Qt::Horizontal);
    auto *snapBox = new QCheckBox(tr("Snap"));
    snapBox->setChecked(true);
    statusBar()->addPermanentWidget(snapBox);
    connect(snapBox, &QCheckBox::toggled, this, [this](bool on) { viewport->snap = on; });
    auto *gridBox = new QComboBox;
    for (int step : {5, 10, 50, 100, 500})
        gridBox->addItem(tr("Grid %1 mm").arg(step), step);
    gridBox->setCurrentIndex(3);
    statusBar()->addPermanentWidget(gridBox);
    connect(gridBox, &QComboBox::currentIndexChanged, this,
            [this, gridBox] { viewport->setGrid(gridBox->currentData().toDouble()); });
    statusBar()->addPermanentWidget(new QLabel(tr("mm · Offline")));
}
void MainWindow::refreshScene() {
    refreshing = true;
    projectTitle->setText(q(editor_.document().name));
    projectTitle->setToolTip(q(editor_.document().name));
    {
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
        renderStart->setEnabled(!render.busy() && renderCamera->count() > 0);
        if (!render.busy())
            renderState->setText(renderCamera->count() ? tr("Pronto para renderizar")
                                                       : tr("Crie uma câmera no menu Câmeras para começar."));
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
    visibleAssets = library->search(search->text(), category->currentData().toString(),
                                    favoriteOnly->isChecked(), recentOnly->isChecked());
    assets->clear();
    for (const auto &a : visibleAssets) {
        auto *item =
            new QListWidgetItem((a.favorite ? "★ " : "") + a.name +
                                    QString("\n%1 × %2 × %3 mm").arg(a.width).arg(a.height).arg(a.depth),
                                assets);
        item->setData(Qt::UserRole, a.id);
        item->setData(Qt::UserRole + 1, a.category);
        auto thumbnail = thumbnails.request(a);
        item->setIcon(thumbnail.isNull() ? studioIcon("cube") : QIcon(QPixmap::fromImage(thumbnail)));
        item->setData(Qt::UserRole + 2, !thumbnail.isNull());
        item->setToolTip(a.category + " · CC0 · " + tr("Arraste para inserir"));
        item->setSizeHint({270, 88});
    }
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
    form->setRowVisible(material, e != nullptr);
    form->setRowVisible(handle, e && e->type == "FurnitureModule");
    form->setRowVisible(glass, e && e->type == "FurnitureModule");
    form->setRowVisible(lightColor, e && e->type == "Light");
    selectionTitle->setText(e                       ? q(e->name)
                            : selectedIds.isEmpty() ? tr("Selecione um objeto")
                                                    : tr("%1 objetos selecionados").arg(selectedIds.size()));
    for (const auto &[key, field] : fields) {
        bool visible = e != nullptr;
        if (key == "offset" || key == "sill" || key == "openAngle")
            visible = e && (e->type == "Door" || e->type == "Window");
        if (key == "power")
            visible = e && e->type == "Light";
        if (key == "size")
            visible = e && e->type == "Light" && e->parameters.value("kind", std::string("area")) == "area";
        if (key == "angle" || key == "blend")
            visible = e && e->type == "Light" && e->parameters.value("kind", std::string("area")) == "spot";
        if (key == "lens")
            visible = e && e->type == "Camera";
        if (key.startsWith("target"))
            visible = e && (e->type == "Camera" || e->type == "Light");
        form->setRowVisible(field, visible);
        field->setEnabled(e && !e->locked);
        field->clear();
        (void)key;
    }
    material->setEnabled(e && !e->locked);
    handle->setEnabled(e && !e->locked && e->type == "FurnitureModule");
    glass->setEnabled(handle->isEnabled());
    if (!e)
        return;
    auto set = [&](const QString &key, double v) { fields[key]->setText(QString::number(v, 'f', 1)); };
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
    for (const auto &key : {"size", "angle", "blend"})
        fields[key]->setEnabled(e->type == "Light" && !e->locked && fields[key]->isVisibleTo(inspector));
    set("size", e->parameters.value("size", 1000.0));
    set("angle", e->parameters.value("angle", 45.0));
    set("blend", e->parameters.value("blend", 0.3));
    auto color = e->type == "Light" ? e->parameters.value("color", Json::array({1.0, 0.89, 0.73}))
                                    : Json::array({1.0, 0.89, 0.73});
    selectedLightColor =
        QColor::fromRgbF(color[0].get<double>(), color[1].get<double>(), color[2].get<double>());
    lightColor->setText(selectedLightColor.name());
    lightColor->setEnabled(e->type == "Light" && !e->locked);
    fields["lens"]->setEnabled(e->type == "Camera" && !e->locked);
    set("lens", e->parameters.value("lens", 28.0));
    auto target = e->parameters.value("target", Json::array({2000, 1500, 1000}));
    for (int i = 0; i < 3; ++i) {
        auto key = QString("target%1").arg(QChar('X' + i));
        fields[key]->setEnabled((e->type == "Camera" || e->type == "Light") && !e->locked);
        set(key, target[i].get<double>());
    }
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
        auto value = [&](const char *key, double previous) {
            return fields[key]->isEnabled() ? millimeters(evaluate(fields[key]->text().toStdString()))
                                            : previous;
        };
        e.transform = {value("x", e.transform.x), value("y", e.transform.y), value("z", e.transform.z),
                       value("yaw", e.transform.yaw), e.transform.mirrored};
        e.width = value("width", e.width);
        e.height = value("height", e.height);
        e.depth = value("depth", e.depth);
        e.material = material->currentData().toString().toStdString();
        for (const auto *key : {"offset", "sill", "openAngle", "power", "lens", "size", "angle", "blend"})
            if (fields[key]->isEnabled())
                e.parameters[key] = value(key, 0);
        if (e.type == "Light" || e.type == "Camera")
            e.parameters["target"] = {value("targetX", 2000), value("targetY", 1500), value("targetZ", 1000)};
        if (e.type == "Light")
            e.parameters["color"] = {selectedLightColor.redF(), selectedLightColor.greenF(),
                                     selectedLightColor.blueF()};
        if (e.type == "FurnitureModule") {
            e.parameters["handle"] = handle->currentData().toString().toStdString();
            e.parameters["glass"] = glass->isChecked();
        }
    });
    viewport->select({id});
}
void MainWindow::newRoom() {
    std::vector<double> v{4000, 3000, 2700, 120};
    if (numericDialog(this, tr("Criar ambiente"),
                      {tr("Largura (mm)"), tr("Profundidade (mm)"), tr("Pé-direito (mm)"), tr("Parede (mm)")},
                      v)) {
        editor_.apply(tr("Criar ambiente"),
                      [&](Document &d) { addRectangularRoom(d, v[0], v[1], v[2], v[3]); });
        viewport->frame();
    }
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
void MainWindow::insertAsset(const QString &id, double x, double y) {
    auto all = library->search();
    auto found = std::find_if(all.begin(), all.end(), [&](const auto &a) { return a.id == id; });
    if (found == all.end())
        throw std::invalid_argument("Asset não encontrado");
    auto e = Library::instantiate(*found, x, y);
    editor_.apply(tr("Inserir %1").arg(found->name), [&](Document &d) { d.entities.push_back(e); });
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
void MainWindow::automate(const std::string &kind) {
    std::vector<double> v{30, 20};
    if (numericDialog(this, tr("Automação"), {tr("Espessura (mm)"), tr("Avanço (mm)")}, v))
        editor_.apply(tr("Criar automação"), [&](Document &d) {
            d.entities.push_back(automation(d, ids(selectedIds), kind, v[0], v[1]));
        });
}
void MainWindow::createLight() {
    bool ok = false;
    auto kind = QInputDialog::getItem(this, tr("Tipo de luz"), tr("Luz"),
                                      {tr("Área"), tr("Ponto"), tr("Spot")}, 0, false, &ok);
    if (!ok)
        return;
    std::vector<double> v{2000, 1500, 2500, 1000};
    if (numericDialog(this, tr("Nova luz"), {tr("X (mm)"), tr("Y (mm)"), tr("Z (mm)"), tr("Potência (W)")},
                      v))
        editor_.apply(tr("Criar luz"), [&](Document &d) {
            auto e = entity("Light", "Luz");
            e.transform = {v[0], v[1], v[2], 0, false};
            e.parameters = {{"kind", kind == tr("Área")    ? "area"
                                     : kind == tr("Ponto") ? "point"
                                                           : "spot"},
                            {"power", v[3]},
                            {"size", 1000},
                            {"target", {2000, 1500, 0}}};
            d.entities.push_back(e);
        });
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
void MainWindow::renderScene() {
    if (render.busy())
        throw std::runtime_error("Aguarde ou cancele o render atual");
    if (std::none_of(editor_.document().entities.begin(), editor_.document().entities.end(),
                     [](const auto &e) { return e.type == "Camera" && e.visible; }))
        throw std::runtime_error("Crie uma câmera no menu Câmeras");
    auto output = QFileDialog::getSaveFileName(this, tr("Salvar render"), {},
                                               tr("Imagem PNG (*.png);;Imagem JPEG (*.jpg)"));
    if (output.isEmpty())
        return;
    if (!testing)
        QSettings().setValue("blender", blenderPath->text());
    applyRenderSettings();
    int index = renderQuality->currentIndex();
    const int widths[] = {640, 1280, 1920, 3840}, heights[] = {360, 720, 1080, 2160};
    render.start(editor_.document(), blenderPath->text(), resourceFile("scripts/cycles_render.py"), output,
                 widths[index], heights[index], renderQuality->currentData().toInt(),
                 renderDevice->currentData().toString());
}
void MainWindow::applyRenderSettings() {
    editor_.apply(tr("Configurar render"), [&](Document &d) {
        d.renderSettings = {{"camera", renderCamera->currentData().toString().toStdString()},
                            {"exposure", renderExposure->value()},
                            {"environmentStrength", renderEnvironment->value()},
                            {"denoise", renderDenoise->isChecked()}};
    });
}
void MainWindow::showRenderImage(const QString &filename) {
    if (!preview) {
        preview = new RenderPreview;
        workspace->addWidget(preview);
    }
    preview->open(filename);
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
void MainWindow::loadProject(const QString &filename) {
    auto document = ProjectStore::open(filename);
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
    ProjectStore::save(destination, editor_.document());
    path = destination;
    editor_.history.setClean();
    // A successful manual save supersedes this project's recovery snapshots.
    recovery.clear(editor_.document().id);
    statusBar()->showMessage(tr("Projeto salvo com segurança"), 5000);
    return true;
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
        if (render.busy()) {
            auto choice =
                QMessageBox::question(this, tr("Render em andamento"), tr("Cancelar o render e fechar?"));
            if (choice != QMessageBox::Yes) {
                event->ignore();
                return;
            }
            render.cancel();
        }
        recovery.clear(editor_.document().id);
        event->accept();
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Falha ao salvar"), QString::fromUtf8(e.what()));
        event->ignore();
    }
}
} // namespace lmx
