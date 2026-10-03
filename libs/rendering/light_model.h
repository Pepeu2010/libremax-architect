#pragma once
#include "document/document.h"
namespace lmx {
Entity lightEntity(const std::string &kind);
void validateLight(const Entity &light);
bool advancedLight(const Entity &light);
} // namespace lmx
