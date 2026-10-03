#pragma once
#include "document/document.h"
#include <QString>
namespace lmx {
class RenderSceneExporter {
  public:
    static void exportScene(const Document &document, const QString &filename, bool instances = false);
};
} // namespace lmx
