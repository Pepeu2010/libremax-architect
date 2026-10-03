#pragma once
#include "library.h"
namespace lmx {
struct ModelPack {
    QString id, name;
    int version = 1;
    std::vector<Asset> assets;
    QString sha256;
};
ModelPack readModelPack(const QString &file);
void installModelPack(Library &library, const QString &directory, const ModelPack &pack);
} // namespace lmx
