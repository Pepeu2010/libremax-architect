#pragma once
#include <QString>
class QApplication;
namespace lmx {
class MainWindow;
void startEnvironmentAcceptance(MainWindow &window, QApplication &app, const QString &output,
                                const QString &blender);
} // namespace lmx
