#pragma once
#include <QString>
class QApplication;
namespace lmx {
class MainWindow;
void startApartmentAcceptance(MainWindow &window, QApplication &app, const QString &directory,
                              const QString &blender);
} // namespace lmx
