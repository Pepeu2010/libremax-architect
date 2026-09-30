#pragma once
#include "commands/editor.h"
#include "document/examples.h"
#include "library/library.h"
#include "library/thumbnails.h"
#include "persistence/recovery_store.h"
#include "render_preview.h"
#include "rendering/render_job.h"
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
#include <QStackedWidget>
#include <QTimer>
#include <QTreeWidget>
#include <map>
#include <memory>

namespace lmx {
class MainWindow final : public QMainWindow {
    Q_OBJECT
    Editor editor_;
    std::unique_ptr<Library> library;
    AssetThumbnails thumbnails;
    RenderJob render;
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
    QComboBox *renderDevice;
    QComboBox *renderQuality;
    QLineEdit *blenderPath;
    QPlainTextEdit *renderLog;
    QComboBox *renderCamera;
    QDoubleSpinBox *renderExposure;
    QDoubleSpinBox *renderEnvironment;
    QComboBox *renderEnvironmentMode;
    QDoubleSpinBox *renderSunElevation;
    QDoubleSpinBox *renderSunRotation;
    QCheckBox *renderDenoise;
    QPushButton *renderStart;
    QPushButton *renderCancel;
    QProgressBar *renderProgress;
    QLabel *renderState;
    QLabel *projectTitle;
    QLabel *libraryCount;
    QPushButton *lightColor;
    QColor selectedLightColor;
    QStackedWidget *workspace;
    QAction *previewAction;
    RenderPreview *preview = nullptr;
    QString path;
    QStringList selectedIds;
    std::vector<Asset> visibleAssets;
    QTimer autosaveTimer;
    RecoveryStore recovery;
    bool refreshing = false;
    bool testing = false;
    void protect(const std::function<void()> &operation);
    void createShell();
    void refreshScene();
    void refreshLibrary();
    void refreshInspector();
    void applyInspector();
    bool discardOrSave();
    void newRoom();
    void opening(bool window);
    void insertAsset(const QString &id, double x, double y);
    void transform(const QString &mode);
    void automate(const std::string &kind);
    void createLight();
    void createCamera();
    void importDxf();
    void importTexture();
    Q_INVOKABLE void activatePbrMaterials();
    void renderScene();
    void applyRenderSettings();
    Q_INVOKABLE void recover();

  protected:
    void closeEvent(QCloseEvent *) override;

  public:
    explicit MainWindow(bool test = false, const QString &recoveryDirectory = {});
    Editor &editor() { return editor_; }
    CadView *cad() { return viewport; }
    void loadProject(const QString &filename);
    bool saveProject(bool saveAs = false);
    void autosave();
    void selectIds(const QStringList &ids);
    void showRenderImage(const QString &filename);
};
} // namespace lmx
