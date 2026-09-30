#pragma once
#include "document/document.h"
#include "library/library.h"
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
    std::map<const AIS_InteractiveObject *, std::string> owners;
    Document current;
    QTemporaryDir textureCache;
    std::optional<gp_Pnt> wallStart;
    QPoint last;
    bool top = true;
    bool cutaway = true;
    QString tool = "select";
    std::function<std::optional<Asset>(const QString &)> findAsset;
    gp_Pnt position(const QPoint &pixel) const;
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
    void scene(const Document &d);
    void setTool(const QString &mode);
    void setTop(bool enabled);
    void setCutaway(bool enabled) {
        cutaway = enabled;
        scene(current);
    }
    void setGrid(double step);
    void frame();
    void select(const std::vector<std::string> &ids);
    void assetResolver(std::function<std::optional<Asset>(const QString &)> resolver) {
        findAsset = std::move(resolver);
    }
    void capture(const QString &path);
    QPoint project(double x, double y, double z = 0) const;
  signals:
    void selected(const QStringList &ids);
    void wallCreated(double x1, double y1, double x2, double y2, bool half);
    void assetDropped(const QString &id, double x, double y);
    void coordinates(const QString &text);
    void failure(const QString &message);
};
} // namespace lmx
