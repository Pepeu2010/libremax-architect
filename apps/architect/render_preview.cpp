#include "render_preview.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QHBoxLayout>
#include <QImageReader>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <stdexcept>
namespace lmx {
ImageCanvas::ImageCanvas(QWidget *parent) : QGraphicsView(parent) {
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setBackgroundBrush(QColor("#0b1219"));
    setRenderHint(QPainter::SmoothPixmapTransform);
    setAccessibleName(tr("Imagem renderizada. Use a roda para zoom e arraste para navegar."));
    setObjectName("renderCanvas");
}
void ImageCanvas::fit() {
    fitInView(sceneRect(), Qt::KeepAspectRatio);
}
void ImageCanvas::zoom(double factor) {
    auto scale = transform().m11() * factor;
    if (scale >= 0.03 && scale <= 8)
        this->scale(factor, factor);
}
void ImageCanvas::wheelEvent(QWheelEvent *event) {
    zoom(event->angleDelta().y() > 0 ? 1.15 : 1 / 1.15);
    event->accept();
}
RenderPreview::RenderPreview(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *tools = new QHBoxLayout;
    tools->setContentsMargins(12, 8, 12, 8);
    caption = new QLabel(tr("Seu render aparece aqui"));
    caption->setObjectName("renderCaption");
    caption->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    tools->addWidget(caption, 1);
    auto button = [&](const QString &text, const QString &name, auto callback) {
        auto *b = new QPushButton(text);
        b->setObjectName(name);
        b->setProperty("role", "quiet");
        tools->addWidget(b);
        connect(b, &QPushButton::clicked, this, callback);
    };
    canvas = new ImageCanvas;
    canvas->setScene(&scene);
    button(tr("Suas imagens"), "backToRenderGallery", [this] { emit backToGallery(); });
    button(tr("Ajustar"), "fitRenderImage", [this] { canvas->fit(); });
    button(tr("1:1"), "actualRenderSize", [this] { canvas->resetTransform(); });
    button(tr("Salvar cópia…"), "exportRenderImage", [this] {
        if (filename.isEmpty())
            return;
        const auto pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        const auto suggestion =
            QDir(pictures).exists() ? QDir(pictures).filePath("Imagem.png") : "Imagem.png";
        const auto destination = QFileDialog::getSaveFileName(this, tr("Salvar cópia do render"), suggestion,
                                                              tr("PNG (*.png);;JPEG (*.jpg)"));
        if (destination.isEmpty())
            return;
        QImage image(filename);
        QSaveFile file(destination);
        file.setDirectWriteFallback(false);
        const bool jpeg = destination.endsWith(".jpg", Qt::CaseInsensitive) ||
                          destination.endsWith(".jpeg", Qt::CaseInsensitive);
        if (image.isNull() || !file.open(QIODevice::WriteOnly) ||
            !image.save(&file, jpeg ? "JPEG" : "PNG", 95) || !file.commit())
            QMessageBox::warning(this, tr("Não foi possível exportar"),
                                 tr("Verifique o destino e tente novamente."));
    });
    layout->addLayout(tools);
    layout->addWidget(canvas, 1);
}
void RenderPreview::open(const QString &path, const QString &label) {
    QImageReader reader(path);
    const auto size = reader.size();
    if (!reader.canRead() || size.width() > 8192 || size.height() > 8192 || size.isEmpty())
        throw std::runtime_error("Imagem de render inválida");
    auto image = reader.read();
    if (image.isNull())
        throw std::runtime_error("Falha ao ler imagem renderizada");
    filename = path;
    scene.clear();
    scene.addPixmap(QPixmap::fromImage(image));
    scene.setSceneRect(QRectF(QPointF(0, 0), QSizeF(image.size())));
    caption->setText(tr("%1  ·  %2 × %3 px")
                         .arg(label.isEmpty() ? QFileInfo(path).fileName() : label)
                         .arg(size.width())
                         .arg(size.height()));
    caption->setToolTip(path);
    QTimer::singleShot(0, canvas, [this] { canvas->fit(); });
}
QSize RenderPreview::imageSize() const {
    return scene.sceneRect().size().toSize();
}
} // namespace lmx
