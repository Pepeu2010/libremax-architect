#pragma once
#include "document/document.h"
namespace lmx {
Json readModel(const QByteArray &bytes);
std::vector<std::string> validatedModelMaterials(const QByteArray &bytes);
} // namespace lmx
