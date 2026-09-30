#pragma once
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QWidget>
namespace lmx {
class ImageCanvas final : public QGraphicsView {
    Q_OBJECT
  protected:
    void wheelEvent(QWheelEvent *) override;

  public:
    explicit ImageCanvas(QWidget *parent = nullptr);
    void fit();
    void zoom(double factor);
};
class RenderPreview final : public QWidget {
    QGraphicsScene scene;
    ImageCanvas *canvas;
    QLabel *caption;
    QString filename;

  public:
    explicit RenderPreview(QWidget *parent = nullptr);
    void open(const QString &path);
    QSize imageSize() const;
};
} // namespace lmx
