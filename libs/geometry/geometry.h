#pragma once
#include "document/document.h"
#include <TopoDS_Shape.hxx>
#include <vector>
namespace lmx {
struct Part {
    TopoDS_Shape shape;
    std::string material;
    std::string owner;
};
std::vector<Part> buildEntity(const Document &document, const Entity &entity);
std::vector<Part> buildScene(const Document &document);
TopoDS_Shape compound(const std::vector<Part> &parts);
double volume(const TopoDS_Shape &shape);
Json meshSnapshot(const Document &document, bool instances = false);
Entity automation(const Document &document, const std::vector<std::string> &ids, const std::string &kind,
                  double thickness = 30, double overhang = 20);
} // namespace lmx
