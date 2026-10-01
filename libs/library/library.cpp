#include "library.h"
#include "model.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <stdexcept>

namespace lmx {
namespace {
void check(const QSqlQuery &query) {
    if (query.lastError().isValid())
        throw std::runtime_error(query.lastError().text().toStdString());
}
void exec(QSqlDatabase &db, const QString &sql) {
    QSqlQuery q(db);
    q.exec(sql);
    check(q);
}
} // namespace
Library::Library(const QString &path, const QString &models)
    : connection(QString::fromStdString(uuid())), modelDirectory(models) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    db = QSqlDatabase::addDatabase("QSQLITE", connection);
    db.setDatabaseName(path);
    if (!db.open())
        throw std::runtime_error(db.lastError().text().toStdString());
    QSqlQuery version(db);
    version.exec("PRAGMA user_version");
    check(version);
    if (!version.next() || version.value(0).toInt() > 1)
        throw std::runtime_error("Versão da biblioteca não suportada; banco preservado");
    version.finish();
    exec(db, "PRAGMA foreign_keys=ON");
    exec(db, "PRAGMA journal_mode=WAL");
    exec(db, "PRAGMA busy_timeout=3000");
    exec(db, "CREATE TABLE IF NOT EXISTS assets(id TEXT PRIMARY KEY,name TEXT NOT NULL,category TEXT NOT "
             "NULL,width REAL,height REAL,depth REAL,recipe TEXT NOT NULL,license TEXT NOT NULL,author TEXT "
             "NOT NULL,origin TEXT NOT NULL,created TEXT NOT NULL)");
    exec(db, "CREATE VIRTUAL TABLE IF NOT EXISTS asset_search USING fts5(id "
             "UNINDEXED,name,category,tags,tokenize='unicode61 remove_diacritics 2')");
    exec(db, "CREATE TABLE IF NOT EXISTS favorites(asset_id TEXT PRIMARY KEY REFERENCES assets(id) ON DELETE "
             "CASCADE)");
    exec(db, "CREATE TABLE IF NOT EXISTS recent_assets(asset_id TEXT PRIMARY KEY REFERENCES assets(id) ON "
             "DELETE CASCADE,used INTEGER NOT NULL)");
    exec(db, "PRAGMA user_version=1");
}
Library::~Library() {
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}
void Library::seed(const Json &entries) {
    if (!entries.is_array() || entries.size() > 50000)
        throw std::invalid_argument("Catálogo inválido");
    if (!db.transaction())
        throw std::runtime_error("Falha ao iniciar transação");
    try {
        for (const auto &j : entries) {
            QSqlQuery q(db);
            q.prepare(
                "INSERT INTO assets VALUES(?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(id) DO UPDATE SET "
                "name=excluded.name,category=excluded.category,width=excluded.width,height=excluded.height,"
                "depth=excluded.depth,recipe=excluded.recipe,license=excluded.license,author=excluded.author,"
                "origin=excluded.origin,created=excluded.created");
            for (const auto *key : {"id", "name", "category"})
                q.addBindValue(QString::fromStdString(j.at(key).get<std::string>()));
            q.addBindValue(j.at("width").get<double>());
            q.addBindValue(j.at("height").get<double>());
            q.addBindValue(j.at("depth").get<double>());
            q.addBindValue(QString::fromStdString(j.at("recipe").dump()));
            for (const auto *key : {"license", "author", "origin", "date"})
                q.addBindValue(QString::fromStdString(j.at(key).get<std::string>()));
            q.exec();
            check(q);
            if (q.numRowsAffected() > 0) {
                QSqlQuery remove(db);
                remove.prepare("DELETE FROM asset_search WHERE id=?");
                remove.addBindValue(QString::fromStdString(j.at("id").get<std::string>()));
                remove.exec();
                check(remove);
                QSqlQuery index(db);
                index.prepare("INSERT INTO asset_search VALUES(?,?,?,?)");
                for (const auto *key : {"id", "name", "category", "tags"})
                    index.addBindValue(QString::fromStdString(j.value(key, std::string{})));
                index.exec();
                check(index);
            }
        }
        if (!db.commit())
            throw std::runtime_error("Falha ao salvar catálogo");
    } catch (...) {
        db.rollback();
        throw;
    }
}
std::vector<Asset> Library::search(const QString &text, const QString &category, bool favorites,
                                   bool recent) const {
    QString sql = "SELECT a.id,a.name,a.category,a.width,a.height,a.depth,a.recipe,EXISTS(SELECT 1 FROM "
                  "favorites f WHERE f.asset_id=a.id) FROM assets a";
    if (!text.trimmed().isEmpty())
        sql += " JOIN asset_search s ON s.id=a.id";
    if (recent)
        sql += " JOIN recent_assets r ON r.asset_id=a.id";
    sql += " WHERE 1=1";
    if (!text.trimmed().isEmpty())
        sql += " AND asset_search MATCH ?";
    if (!category.isEmpty())
        sql += " AND a.category=?";
    if (favorites)
        sql += " AND EXISTS(SELECT 1 FROM favorites f WHERE f.asset_id=a.id)";
    sql += recent ? " ORDER BY r.used DESC LIMIT 200" : " ORDER BY a.name LIMIT 200";
    QSqlQuery q(db);
    q.prepare(sql);
    if (!text.trimmed().isEmpty()) {
        QStringList tokens = text.left(256).split(QRegularExpression("\\s+"), Qt::SkipEmptyParts), terms;
        for (auto token : tokens) {
            token.replace('"', "\"\"");
            terms << "\"" + token + "\"*";
        }
        q.addBindValue(terms.join(" AND "));
    }
    if (!category.isEmpty())
        q.addBindValue(category);
    q.exec();
    check(q);
    std::vector<Asset> result;
    while (q.next()) {
        result.push_back({q.value(0).toString(),
                          q.value(1).toString(),
                          q.value(2).toString(),
                          q.value(3).toDouble(),
                          q.value(4).toDouble(),
                          q.value(5).toDouble(),
                          Json::parse(q.value(6).toString().toStdString()),
                          q.value(7).toBool(),
                          {}});
        auto &asset = result.back();
        if (asset.recipe.contains("modelFile")) {
            const auto name = QString::fromStdString(asset.recipe.at("modelFile").get<std::string>());
            if (modelDirectory.isEmpty() ||
                !QRegularExpression("^[A-Za-z0-9_-]+\\.json$").match(name).hasMatch())
                throw std::invalid_argument("Caminho do modelo 3D inválido");
            QFile file(QDir(modelDirectory).filePath(name));
            if (!file.open(QIODevice::ReadOnly) || file.size() > 4 * 1024 * 1024)
                throw std::runtime_error("Modelo 3D indisponível");
            asset.model = file.readAll();
            const auto hash =
                QCryptographicHash::hash(asset.model, QCryptographicHash::Sha256).toHex().toStdString();
            if (hash != asset.recipe.at("parameters").at("meshAsset").get<std::string>())
                throw std::runtime_error("Integridade do modelo 3D inválida");
        }
    }
    return result;
}
void Library::favorite(const QString &id, bool enabled) {
    QSqlQuery q(db);
    q.prepare(enabled ? "INSERT OR IGNORE INTO favorites VALUES(?)"
                      : "DELETE FROM favorites WHERE asset_id=?");
    q.addBindValue(id);
    q.exec();
    check(q);
}
void Library::used(const QString &id) {
    QSqlQuery q(db);
    q.prepare("INSERT INTO recent_assets VALUES(?,unixepoch()) ON CONFLICT(asset_id) DO UPDATE SET "
              "used=unixepoch()");
    q.addBindValue(id);
    q.exec();
    check(q);
}
Entity Library::instantiate(const Asset &a, double x, double y) {
    auto e = entity(a.recipe.at("type"), a.name.toStdString());
    e.width = a.width;
    e.height = a.height;
    e.depth = a.depth;
    e.parameters = a.recipe.at("parameters");
    e.material = a.recipe.value("material", std::string("white"));
    e.transform.x = millimeters(x);
    e.transform.y = millimeters(y);
    e.transform.z = e.parameters.value("defaultElevation", 0.0);
    e.metadata = {{"asset", a.id.toStdString()}, {"license", "CC0-1.0"}, {"author", "LibreMax contributors"}};
    if (a.recipe.contains("source")) {
        e.metadata["source"] = a.recipe.at("source");
        e.metadata["author"] = "Kenney";
    }
    return e;
}
void Library::attachModel(Document &d, const Asset &a) {
    if (a.model.isEmpty())
        return;
    readModel(a.model);
    const auto hash = QCryptographicHash::hash(a.model, QCryptographicHash::Sha256).toHex().toStdString();
    if (hash != a.recipe.at("parameters").at("meshAsset").get<std::string>())
        throw std::invalid_argument("Modelo 3D adulterado");
    d.embeddedAssets[hash] = a.model;
}
} // namespace lmx
