#pragma once
#include <QObject>
#include <QProcess>
namespace lmx {
class BlenderBridge final : public QObject {
    Q_OBJECT
    QProcess process;
    bool reported = false;
    void *childJob = nullptr;

  public:
    explicit BlenderBridge(QObject *parent = nullptr);
    ~BlenderBridge() override;
    static QString findExecutable(const QString &preferred = {});
    void start(const QString &executable, const QString &script, const QStringList &arguments);
    void cancel();
    bool busy() const;
  signals:
    void output(const QString &text);
    void finished(int exitCode, bool normalExit);
};
} // namespace lmx
