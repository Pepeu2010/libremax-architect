#pragma once
#include "blender_bridge.h"
#include "document/document.h"
#include <QFutureWatcher>
#include <QObject>
#include <QSize>
#include <QTemporaryDir>
#include <memory>
namespace lmx {
class RenderJob final : public QObject {
    Q_OBJECT
    BlenderBridge bridge;
    QFutureWatcher<QString> preparing;
    std::shared_ptr<QTemporaryDir> work;
    QString output;
    QString renderedFile;
    QSize resolution;
    bool active = false;
    bool cancelled = false;
    QString lineBuffer;
    Json engineInfo;
    void consumeOutput(const QString &text);
    void finish(bool success, const QString &message, int exitCode = 0);

  public:
    explicit RenderJob(QObject *parent = nullptr);
    ~RenderJob() override;
    bool busy() const;
    const Json &engine() const { return engineInfo; }
    void start(const Document &snapshot, const QString &blender, const QString &script, const QString &output,
               int width = 1920, int height = 1080, int samples = 128, const QString &device = "AUTO");
    void cancel();
  signals:
    void state(const QString &state);
    void log(const QString &text);
    void completed(const QString &path);
    void progress(int sample, int total);
    void stage(const QString &stage);
    void finished(bool success, bool cancelled, const QString &message, int exitCode);
};
} // namespace lmx
