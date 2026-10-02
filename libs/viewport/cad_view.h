#pragma once
#include "document/document.h"
#include "geometry/scene_cache.h"
#include "library/library.h"
#include "placement/placement.h"
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <QTemporaryDir>
#include <QWidget>
#include <V3d_View.hxx>
#include <map>
#include <optional>

namespace lmx {
class CadView final : public QWidget {
    Q_OBJECT
    Handle(AIS_InteractiveContext) context;
    Handle(V3d_View) view;
    Handle(AIS_Shape) preview;
    std::string previewKey;
    std::optional<bool> previewAllowed;
    SceneGeometryCache geometryCache;
    struct DisplayEntry {
        std::string key;
        Handle(AIS_Shape) shape;
    };
    std::map<std::string, DisplayEntry> displayed;
    std::map<const AIS_InteractiveObject *, std::string> owners;
    Document current;
    QTemporaryDir textureCache;
    std::optional<gp_Pnt> wallStart;
    QPoint last;
    bool top = true;
    bool cutaway = true;
    QString tool = "select";
    std::function<std::optional<Asset>(const QString &)> findAsset;
    std::optional<Asset> placingAsset;
    std::optional<Entity> movingObject;
    std::optional<Placement> pendingPlacement;
    QPoint pressed;
    QPointF grabOffset;
    bool moved = false;
    double placementYaw = 0;
    gp_Pnt position(const QPoint &pixel, bool applySnap = true) const;
    std::pair<gp_Pnt, std::string> surfacePosition(const QPoint &pixel);
    void showPlacement(Placement placement, const Asset *asset = nullptr);
    void previewAsset(const Asset &asset, const QPoint &pixel);
    void initialize();
    void clearPreview();
    void selection();

  protected:
    QPaintEngine *paintEngine() const override { return nullptr; }
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragMoveEvent(QDragMoveEvent *) override;
    void dragLeaveEvent(QDragLeaveEvent *) override;
    void dropEvent(QDropEvent *) override;

  public:
    explicit CadView(QWidget *parent = nullptr);
    double grid = 100;
    bool snap = true;
    bool assist = true;
    void beginPlacement(const QString &assetId);
    void scene(const Document &d);
    void setTool(const QString &mode);
    void setTop(bool enabled);
    void setCutaway(bool enabled) {
        cutaway = enabled;
        scene(current);
    }
    void setGrid(double step);
    void frame();
    void frameRoom(const std::string &id);
    void select(const std::vector<std::string> &ids);
    void assetResolver(std::function<std::optional<Asset>(const QString &)> resolver) {
        findAsset = std::move(resolver);
    }
    void capture(const QString &path);
    QPoint project(double x, double y, double z = 0) const;
  signals:
    void selected(const QStringList &ids);
    void wallCreated(double x1, double y1, double x2, double y2, bool half);
    void assetDropped(const QString &id, const lmx::Entity &object);
    void objectMoved(const lmx::Entity &object);
    void placementStatus(const QString &text, bool active, bool allowed);
    void coordinates(const QString &text);
    void failure(const QString &message);
};
} // namespace lmx
