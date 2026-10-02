#pragma once
#include "persistence/project_library.h"
#include <QLabel>
#include <QListWidget>
#include <QWidget>
namespace lmx {
class ProjectHome final : public QWidget {
    Q_OBJECT
    QListWidget *cards;
    QLabel *empty;
    QWidget *firstProject;

  public:
    explicit ProjectHome(QWidget *parent = nullptr);
    void setProjects(const std::vector<RecentProject> &projects);
    void showIndexWarning(const QString &message);
  signals:
    void newProject();
    void openProject();
    void openRecent(const QString &path);
    void tutorial();
    void example();
    void recovery();
};
} // namespace lmx
