#pragma once
#include "document/document.h"
#include <QString>
namespace lmx {
class ProjectStore {
  public:
    static Document open(const QString &path);
    static void save(const QString &path, const Document &document, bool backup = true);
};
} // namespace lmx
