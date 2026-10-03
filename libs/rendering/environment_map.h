#pragma once
#include "document/document.h"
#include <QString>
namespace lmx {
struct ImportedEnvironment {
    QByteArray bytes;
    std::string hash;
    Json settings;
};
ImportedEnvironment importEnvironment(const QString &filename);
void attachEnvironment(Document &document, const ImportedEnvironment &environment);
} // namespace lmx
