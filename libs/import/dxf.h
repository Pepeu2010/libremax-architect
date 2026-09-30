#pragma once
#include "document/document.h"
#include <QString>
#include <map>
namespace lmx {
struct DxfDrawing {
    std::map<std::string, Json> layers;
    double millimeterScale = 1;
    std::vector<std::string> unsupported;
};
DxfDrawing readDxf(const QString &path);
std::vector<Entity> dxfEntities(const DxfDrawing &drawing, double scale,
                                const std::vector<std::string> &layers);
} // namespace lmx
