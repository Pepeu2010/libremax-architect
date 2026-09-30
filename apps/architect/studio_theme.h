#pragma once
#include <QIcon>
#include <QStyledItemDelegate>
namespace lmx {
void applyStudioPalette();
QIcon studioIcon(const QString &name);
class AssetDelegate final : public QStyledItemDelegate {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const override;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override { return {240, 88}; }
};
} // namespace lmx
