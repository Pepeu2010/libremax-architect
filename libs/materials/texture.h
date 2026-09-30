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
struct PbrMaterialPack {
    Json materials = Json::array();
    std::map<std::string, QByteArray> assets;
};
PbrMaterialPack readPbrMaterials(const QString &directory);
void attachPbrMaterials(Document &document, const PbrMaterialPack &pack);
} // namespace lmx
