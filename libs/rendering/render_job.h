#pragma once
#include "document/document.h"
#include <QFutureWatcher>
#include <QObject>
#include <QProcess>
#include <QSize>
#include <QTemporaryDir>
#include <memory>
namespace lmx {
class RenderJob final : public QObject {
    Q_OBJECT
    QProcess process;
    QFutureWatcher<QString> preparing;
    std::shared_ptr<QTemporaryDir> work;
    QString output;
    QString renderedFile;
    QSize resolution;
    bool active = false;
    bool cancelled = false;

  public:
    explicit RenderJob(QObject *parent = nullptr);
    ~RenderJob() override;
    bool busy() const;
    void start(const Document &snapshot, const QString &blender, const QString &script, const QString &output,
               int width = 1920, int height = 1080, int samples = 128, const QString &device = "AUTO");
    void cancel();
  signals:
    void state(const QString &state);
    void log(const QString &text);
    void completed(const QString &path);
};
} // namespace lmx
