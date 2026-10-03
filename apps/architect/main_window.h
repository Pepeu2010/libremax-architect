#pragma once
#include "commands/editor.h"
#include "document/examples.h"
#include "library/library.h"
#include "library/thumbnails.h"
#include "persistence/project_library.h"
#include "persistence/recovery_store.h"
#include "project_home.h"
#include "render_gallery.h"
#include "render_preview.h"
#include "rendering/render_queue.h"
#include "viewport/cad_view.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <map>
#include <memory>

namespace lmx {
class MainWindow final : public QMainWindow {
    Q_OBJECT
    Editor editor_;
    std::unique_ptr<QTemporaryDir> testLibraryDirectory;
    std::unique_ptr<Library> library;
    QString userModelsDirectory;
    AssetThumbnails thumbnails;
    std::unique_ptr<RenderQueue> render;
    RenderGallery *gallery;
    QByteArray editorLayout;
    bool galleryMode = false;
    CadView *viewport;
    QTreeWidget *tree;
    QListWidget *assets;
    QLineEdit *search;
    QComboBox *category;
    QCheckBox *favoriteOnly;
    QCheckBox *recentOnly;
    QLabel *selectionTitle;
    QLabel *hint;
    QWidget *inspector;
    std::map<QString, QLineEdit *> fields;
    QComboBox *material;
    QComboBox *handle;
    QCheckBox *glass;
    QCheckBox *originalModelColors;
    QComboBox *renderDevice;
    QComboBox *renderQuality;
    QComboBox *renderSize;
    QComboBox *renderFormat;
    QWidget *customRender;
    std::map<QString, QSpinBox *> renderCounts;
    std::map<QString, QDoubleSpinBox *> renderValues;
    QLineEdit *blenderPath;
    QPlainTextEdit *renderLog;
    QComboBox *renderCamera;
    QDoubleSpinBox *renderExposure;
    QDoubleSpinBox *renderEnvironment;
    QComboBox *renderEnvironmentMode;
    QWidget *hdriControls;
    QLabel *hdriStatus;
    QDoubleSpinBox *hdriRotation;
    QCheckBox *hdriVisible;
    QPushButton *backgroundColor;
    QDoubleSpinBox *renderSunElevation;
    QDoubleSpinBox *renderSunRotation;
    QCheckBox *renderDenoise;
    QPushButton *renderStart;
    QPushButton *renderCancel;
    QProgressBar *renderProgress;
    QLabel *renderState, *renderTiming;
    QLabel *projectTitle;
    QLabel *libraryCount;
    QLabel *placementBanner;
    QComboBox *roomPicker;
    QCheckBox *advancedProperties;
    QPushButton *lightColor;
    QComboBox *lightTone;
    QSpinBox *lightKelvin;
    QComboBox *lightShape;
    QColor selectedLightColor;
    QStackedWidget *workspace;
    QStackedWidget *rootPages;
    QWidget *editorPage;
    ProjectHome *home;
    std::unique_ptr<ProjectLibrary> projects;
    QAction *previewAction;
    RenderPreview *preview = nullptr;
    QString path;
    QStringList selectedIds;
    std::vector<Asset> visibleAssets;
    QTimer autosaveTimer;
    QTimer searchTimer;
    RecoveryStore recovery;
    bool refreshing = false;
    bool testing = false;
    void protect(const std::function<void()> &operation);
    void createShell();
    void refreshScene();
    void refreshLibrary();
    void refreshInspector();
    void refreshLightControls();
    void applyInspector();
    bool discardOrSave();
    void newRoom();
    Q_INVOKABLE void editRoomOutline();
    void opening(bool window);
    void insertAsset(const QString &id, const Entity &object);
    Q_INVOKABLE void apartmentStarter();
    Q_INVOKABLE void modernApartmentStarter();
    void focusRoom();
    void transform(const QString &mode);
    Q_INVOKABLE void arrangeSelection(const QString &mode);
    void moveSelection();
    void automate(const std::string &kind);
    Q_INVOKABLE void createLight();
    void createCamera();
    void simpleCamera();
    Q_INVOKABLE void roomLook(const QString &style);
    void importDxf();
    Q_INVOKABLE void importModel(const QString &file = {});
    void installCollection();
    void importTexture();
    Q_INVOKABLE void activatePbrMaterials();
    void renderScene();
    std::uint64_t environmentImportGeneration = 0;
    void importHdri(const QString &filename = {});
    Json selectedRenderOptions() const;
    void refreshRenderQueue();
    Q_INVOKABLE void showRenderGallery();
    void applyRenderSettings();
    Q_INVOKABLE void recover();
    Q_INVOKABLE void showHome();
    void enterEditor();
    void showEditorWorkspace();
    void rememberProject();

  protected:
    void closeEvent(QCloseEvent *) override;

  public:
    explicit MainWindow(bool test = false, const QString &recoveryDirectory = {}, bool welcome = false,
                        const QString &testRoot = {});
    Editor &editor() { return editor_; }
    CadView *cad() { return viewport; }
    RenderQueue &renderQueue() { return *render; }
    void loadProject(const QString &filename);
    bool saveProject(bool saveAs = false);
    void autosave();
    void selectIds(const QStringList &ids);
    void showRenderImage(const QString &filename);
};
} // namespace lmx
