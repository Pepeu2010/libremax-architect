#pragma once
#include "rendering/render_queue.h"
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QToolButton>
#include <QWidget>
namespace lmx {
class RenderGallery final : public QWidget {
    Q_OBJECT
    RenderQueue &queue;
    std::string project;
    QListWidget *images;
    QLabel *summary, *details;
    QWidget *progressPanel;
    QLabel *progressStatus, *timing;
    QProgressBar *progress;
    QPushButton *open, *saveCopy, *cancel;
    QToolButton *more;
    QAction *repeat, *cpu, *folder, *logs, *erase;
    std::map<QString, QListWidgetItem *> rows;
    void selectionChanged();
    void refreshTiming();

  public:
    explicit RenderGallery(RenderQueue &queue, QWidget *parent = nullptr);
    void setProject(const std::string &id);
    void refresh();
    QString selectedId() const;
  signals:
    void back();
    void openImage(const QString &path);
    void saveImage(const QString &path);
    void retryImage(const QString &id, bool cpu);
    void cancelImage(const QString &id);
    void removeImage(const QString &id);
};
} // namespace lmx
