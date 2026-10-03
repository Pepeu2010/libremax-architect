#pragma once
#include "library.h"
#include <QImage>
#include <QObject>
#include <QThreadPool>
#include <functional>
#include <map>
#include <set>

namespace lmx {
QImage renderAssetThumbnail(const Asset &asset);
class AssetThumbnails final : public QObject {
    Q_OBJECT
    QThreadPool pool;
    std::map<QString, QImage> images;
    std::set<QString> pending;
    QString cacheDirectory;

  public:
    explicit AssetThumbnails(QObject *parent = nullptr);
    ~AssetThumbnails() override;
    QImage request(const Asset &asset, std::function<Asset(const Asset &)> loader = {});
    void setWorkerLimit(int workers);
  signals:
    void ready(const QString &id, const QImage &image);
};
} // namespace lmx
