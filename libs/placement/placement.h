#pragma once
#include "document/document.h"
#include <optional>
namespace lmx {
struct Placement {
    Entity object;
    bool allowed = false;
    std::string message;
    std::string wall;
};
bool movable(const Entity &object);
Placement placeObject(const Document &document, Entity object, double x, double y, bool assist = true,
                      const std::string &wallHint = {});
} // namespace lmx
