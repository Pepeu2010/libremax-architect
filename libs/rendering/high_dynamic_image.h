#pragma once
#include <QByteArray>
#include <QSize>
#include <QString>
namespace lmx {
struct HighDynamicImage {
    QString format;
    QSize size;
    float peak = 0;
};
// Header inspection bounds allocation before the optional, full floating-point decode.
HighDynamicImage inspectHighDynamicImage(const QByteArray &bytes, bool decode = false,
                                         qint64 maximumPixels = 16 * 1024 * 1024);
} // namespace lmx
