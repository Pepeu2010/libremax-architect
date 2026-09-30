#pragma once
#include "document/document.h"
#include <QString>
namespace lmx {
struct ImportedTexture {
    std::string hash;
    std::string name;
    QByteArray png;
};
ImportedTexture readTexture(const QString &filename);
std::string attachTexture(Document &document, const ImportedTexture &texture);
} // namespace lmx
