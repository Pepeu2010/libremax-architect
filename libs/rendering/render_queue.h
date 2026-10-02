#pragma once
#include "render_job.h"
#include "render_snapshot.h"
#include <QThreadPool>
#include <deque>
#include <set>
namespace lmx {
class RenderQueue final : public QObject {
    Q_OBJECT
    QString root, activeId;
    std::map<QString, Json> records;
    std::map<QString, std::shared_ptr<const RenderSnapshot>> snapshots;
    std::set<QString> prepared, cancelled;
    std::deque<QString> waiting;
    QThreadPool preparationPool;
    RenderJob job;
    bool stopping = false;
    std::uint64_t nextOrder = 0;
    void prepare(const QString &id, const QString &blender, const QString &script);
    void dispatch();
    void save(const QString &id);
    void setState(const QString &id, const QString &state);

  public:
    explicit RenderQueue(QString directory, QObject *parent = nullptr);
    ~RenderQueue() override;
    QString enqueue(const RenderSnapshot &snapshot, const QString &blender, const QString &script);
    QString retry(const QString &id, const QString &device = {});
    void cancel(const QString &id);
    void cancelActive();
    void cancelAll();
    void remove(const QString &id);
    bool busy() const;
    QString active() const { return activeId; }
    std::vector<Json> entries(const std::string &project = {}) const;
    QString imagePath(const QString &id) const;
    QString snapshotPath(const QString &id) const;
    QString logPath(const QString &id) const;
  signals:
    void changed();
    void log(const QString &text);
    void completed(const QString &id, const QString &image);
    void warning(const QString &message);
};
} // namespace lmx
