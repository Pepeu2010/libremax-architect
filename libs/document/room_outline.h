#pragma once
#include "document.h"
#include <array>
namespace lmx {
using PlanPoint = std::array<double, 2>;
using Outline = std::vector<PlanPoint>;
Outline roomOutline(const Entity &room, bool world = false);
void validateOutline(const Outline &outline);
bool insideOutline(const Outline &outline, PlanPoint point, double tolerance = 0.1);
bool footprintInside(const Outline &outline, const Outline &footprint);
bool roomFootprintsOverlap(const Outline &first, const Outline &second);
void addPolygonRoom(Document &document, Outline outline, double height = 2700, double thickness = 120,
                    const std::string &name = "Cômodo");
void editPolygonRoom(Document &document, const std::string &id, const Outline &localOutline, double height);
PlanPoint roomInteriorPoint(const Entity &room);
} // namespace lmx
