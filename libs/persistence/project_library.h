#pragma once
#include "document/document.h"
#include <QImage>
#include <QString>
namespace lmx {
struct RecentProject {
    QString path, name, modified, thumbnail;
    bool available = false;
};
class ProjectLibrary {
    QString directory;

  public:
    explicit ProjectLibrary(QString directory);
    std::vector<RecentProject> projects() const;
    void remember(const QString &path, const Document &document, const QImage &preview = {});
    void forget(const QString &path);
};
} // namespace lmx
