#pragma once
#include <QString>
class QApplication;
namespace lmx {
class MainWindow;
void startCameraAcceptance(MainWindow &window, QApplication &app, const QString &directory,
                           const QString &blender);
} // namespace lmx
