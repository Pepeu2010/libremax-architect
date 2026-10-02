#pragma once
#include <QString>
class QApplication;
namespace lmx {
class MainWindow;
void startQueueAcceptance(MainWindow &window, QApplication &app, const QString &directory,
                          const QString &blender, const QString &script);
} // namespace lmx
