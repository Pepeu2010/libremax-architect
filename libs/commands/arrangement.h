#pragma once
#include "document/document.h"
namespace lmx {
std::vector<std::string> arrangementMembers(const Document &document, const std::vector<std::string> &ids);
std::string groupObjects(Document &document, const std::vector<std::string> &ids);
void ungroupObjects(Document &document, const std::vector<std::string> &ids);
void arrangeObjects(Document &document, const std::vector<std::string> &ids, const std::string &mode);
void moveObjects(Document &document, const std::vector<std::string> &ids, double dx, double dy);
std::vector<Entity> placedTogether(const Document &document, const std::vector<Entity> &objects, double dx,
                                   double dy);
} // namespace lmx
