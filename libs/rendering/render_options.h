#pragma once
#include "document/document.h"
#include <QString>
namespace lmx {
Json renderPresets();
Json renderPreset(const QString &name);
void validateRenderOptions(const Json &options);
QString renderStateLabel(const QString &state);
} // namespace lmx
