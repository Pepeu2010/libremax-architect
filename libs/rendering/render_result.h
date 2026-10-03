#pragma once
#include <QSize>
#include <QString>
#include <atomic>
namespace lmx {
struct PublishedRender {
    QString preview;
    QString error;
};
PublishedRender publishRender(const QString &source, const QString &preview, const QString &destination,
                              QSize resolution, const std::atomic_bool &cancelled);
} // namespace lmx
