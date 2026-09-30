#pragma once
#include "document/document.h"
#include <QFileInfo>
#include <QStringList>
#include <vector>

namespace lmx {
struct RecoveryEntry {
    QFileInfo file;
    std::string projectId, name;
};
class RecoveryStore {
    QString directory;

  public:
    explicit RecoveryStore(QString directory);
    QString save(const Document &document) const;
    std::vector<RecoveryEntry> entries(QStringList *errors = nullptr) const;
    void clear(const std::string &projectId) const;
};
} // namespace lmx
