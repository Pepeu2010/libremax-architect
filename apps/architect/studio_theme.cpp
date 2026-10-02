#include "studio_theme.h"
#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QProxyStyle>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
namespace lmx {
namespace {
class StudioStyle final : public QProxyStyle {
  public:
    StudioStyle() : QProxyStyle(QStyleFactory::create("Fusion")) {}
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                       const QWidget *widget = nullptr) const override {
        if (element == PE_IndicatorArrowDown || element == PE_IndicatorArrowUp ||
            element == PE_IndicatorArrowLeft || element == PE_IndicatorArrowRight) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->translate(option->rect.center());
            if (element == PE_IndicatorArrowUp)
                painter->rotate(180);
            if (element == PE_IndicatorArrowLeft)
                painter->rotate(90);
            if (element == PE_IndicatorArrowRight)
                painter->rotate(-90);
            painter->setPen(QPen(QColor(option->state & State_Enabled ? "#ded4ec" : "#95869f"), 1.4,
                                 Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->drawPolyline(QPolygonF{{-3, -1}, {0, 2}, {3, -1}});
            painter->restore();
            return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};
} // namespace
void applyStudioPalette() {
    QApplication::setStyle(new StudioStyle);
    QPalette p;
    p.setColor(QPalette::Window, QColor("#1b1b25"));
    p.setColor(QPalette::WindowText, QColor("#f1eff8"));
    p.setColor(QPalette::Base, QColor("#14141c"));
    p.setColor(QPalette::AlternateBase, QColor("#252534"));
    p.setColor(QPalette::Text, QColor("#f1eff8"));
    p.setColor(QPalette::Button, QColor("#292938"));
    p.setColor(QPalette::ButtonText, QColor("#f1eff8"));
    p.setColor(QPalette::Highlight, QColor("#534b81"));
    p.setColor(QPalette::HighlightedText, QColor("#faf8ff"));
    p.setColor(QPalette::ToolTipBase, QColor("#2c283b"));
    p.setColor(QPalette::ToolTipText, QColor("#f1eff8"));
    p.setColor(QPalette::Disabled, QPalette::Text, QColor("#9b90ad"));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#9b90ad"));
    p.setColor(QPalette::Light, QColor("#635773"));
    p.setColor(QPalette::Dark, QColor("#101017"));
    QApplication::setPalette(p);
    const auto families = QFontDatabase::families();
    for (const auto &family :
         {QString("Inter"), QString("Noto Sans"), QString("Segoe UI"), QString("DejaVu Sans")})
        if (families.contains(family)) {
            QApplication::setFont(QFont(family, 10));
            break;
        }
}
QIcon studioIcon(const QString &name) {
    QPixmap pixmap(48, 48);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(2, 2);
    p.setPen(QPen(QColor("#d7c5e6"), 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (name == "new") {
        p.drawRect(QRectF(5, 3, 14, 18));
        p.drawLine(12, 8, 12, 16);
        p.drawLine(8, 12, 16, 12);
    } else if (name == "open") {
        p.drawPolyline(QPolygonF{{3, 19}, {3, 5}, {10, 5}, {12, 8}, {21, 8}});
        p.drawPolygon(QPolygonF{{3, 19}, {6, 10}, {22, 10}, {19, 19}});
    } else if (name == "save") {
        p.drawRect(QRectF(4, 3, 16, 18));
        p.drawRect(QRectF(8, 3, 8, 6));
        p.drawRect(QRectF(8, 14, 8, 7));
    } else if (name == "undo" || name == "redo") {
        if (name == "redo") {
            p.translate(24, 0);
            p.scale(-1, 1);
        }
        p.drawPolyline(QPolygonF{{9, 5}, {4, 10}, {9, 15}});
        QPainterPath path;
        path.moveTo(4, 10);
        path.lineTo(14, 10);
        path.cubicTo(22, 10, 22, 19, 14, 19);
        p.drawPath(path);
    } else if (name == "select")
        p.drawPolygon(QPolygonF{{5, 3}, {5, 20}, {10, 15}, {14, 21}, {17, 19}, {13, 13}, {20, 12}});
    else if (name == "wall" || name == "half") {
        auto h = name == "half" ? 7 : 12;
        p.drawPolygon(QPolygonF{{3, 20},
                                {3, static_cast<qreal>(20 - h)},
                                {17, 3},
                                {21, 6},
                                {21, static_cast<qreal>(6 + h)},
                                {7, 23}});
        p.drawLine(7, 23, 7, 23 - h);
        p.drawLine(7, 23 - h, 21, 6);
    } else if (name == "plan") {
        p.drawRect(QRectF(4, 4, 16, 16));
        p.drawLine(4, 12, 10, 12);
        p.drawLine(14, 12, 20, 12);
        p.drawLine(12, 4, 12, 10);
    } else if (name == "frame") {
        for (int x : {4, 20})
            for (int y : {4, 20}) {
                p.drawLine(x, y, x + (x == 4 ? 5 : -5), y);
                p.drawLine(x, y, x, y + (y == 4 ? 5 : -5));
            }
    } else if (name == "render") {
        p.drawRect(QRectF(3, 6, 18, 14));
        p.drawEllipse(QPointF(12, 13), 4, 4);
        p.drawPolyline(QPolygonF{{6, 6}, {8, 3}, {14, 3}, {16, 6}});
    } else if (name == "light") {
        p.drawEllipse(QPointF(12, 9), 5, 5);
        p.drawLine(10, 16, 14, 16);
        p.drawLine(10, 19, 14, 19);
    } else {
        p.drawPolygon(QPolygonF{{12, 3}, {21, 8}, {21, 17}, {12, 22}, {3, 17}, {3, 8}});
        p.drawPolyline(QPolygonF{{3, 8}, {12, 13}, {21, 8}});
        p.drawLine(12, 13, 12, 22);
    }
    return QIcon(pixmap);
}
void AssetDelegate::paint(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    p->save();
    const bool selected = option.state & QStyle::State_Selected;
    const bool hover = option.state & QStyle::State_MouseOver;
    auto rect = option.rect.adjusted(2, 2, -2, -2);
    p->fillRect(rect, QColor(selected ? "#223e43" : hover ? "#23313c" : "#1b1a25"));
    if (selected)
        p->fillRect(QRect(rect.left(), rect.top(), 3, rect.height()), QColor("#7dd9c2"));
    auto icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    icon.paint(p, QRect(rect.left() + 7, rect.top() + 8, rect.width() - 14, 96));
    auto font = option.font;
    font.setPixelSize(12);
    font.setWeight(QFont::DemiBold);
    p->setFont(font);
    p->setPen(QColor("#e8eff4"));
    const int x = rect.left() + 8, width = std::max(0, rect.width() - 16);
    p->drawText(QRect(x, rect.top() + 105, width, 32), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                index.data().toString().section('\n', 0, 0));
    font.setPixelSize(11);
    font.setWeight(QFont::Normal);
    p->setFont(font);
    p->setPen(QColor("#a7b9c5"));
    p->drawText(
        QRect(x, rect.top() + 137, width, 16), Qt::AlignVCenter,
        p->fontMetrics().elidedText(index.data().toString().section('\n', 1, 1), Qt::ElideRight, width));
    p->setPen(QColor("#7dd9c2"));
    p->drawText(QRect(x, rect.top() + 153, width, 16), Qt::AlignVCenter,
                index.data(Qt::UserRole + 1).toString());
    if (option.state & QStyle::State_HasFocus) {
        p->setPen(QColor("#7dd9c2"));
        p->setBrush(Qt::NoBrush);
        p->drawRect(rect.adjusted(1, 1, -2, -2));
    }
    p->restore();
}
} // namespace lmx
