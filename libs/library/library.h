#pragma once
#include "document/document.h"
#include <QSqlDatabase>
#include <QString>
namespace lmx {
struct Asset {
    QString id, name, category;
    double width, height, depth;
    Json recipe;
    bool favorite = false;
    QByteArray model;
};
class Library {
    QString connection;
    QSqlDatabase db;
    QString modelDirectory;

  public:
    explicit Library(const QString &path, const QString &models = {});
    ~Library();
    Library(const Library &) = delete;
    Library &operator=(const Library &) = delete;
    void seed(const Json &entries);
    std::vector<Asset> search(const QString &text = {}, const QString &category = {}, bool favorites = false,
                              bool recent = false) const;
    void favorite(const QString &id, bool enabled);
    void used(const QString &id);
    static Entity instantiate(const Asset &asset, double x, double y);
    static void attachModel(Document &document, const Asset &asset);
};
} // namespace lmx
