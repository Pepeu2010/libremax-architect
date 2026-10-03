#pragma once
#include "document/document.h"
#include <QString>
#include <optional>
namespace lmx {
struct RenderProgressUpdate {
    int sample = 0, total = 0, percent = -1;
    qint64 remainingMs = -1;
};
class CyclesProgress {
    int percent = -1, completedTiles = 0;

  public:
    std::optional<RenderProgressUpdate> consume(const QString &line);
};
QString renderDuration(qint64 milliseconds);
bool isCyclesDenoising(const QString &line);
QString renderTimingText(const Json &entry);
} // namespace lmx
