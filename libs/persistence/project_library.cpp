#include "project_library.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <algorithm>
namespace lmx {
namespace {
QString normalizedPath(const QString &path) {
    const QFileInfo file(path);
    const auto canonical = file.canonicalFilePath();
    return canonical.isEmpty() ? file.absoluteFilePath() : canonical;
}
bool samePath(const QString &a, const QString &b) {
#ifdef Q_OS_WIN
    return a.compare(b, Qt::CaseInsensitive) == 0;
#else
    return a == b;
#endif
}
Json readIndex(const QString &directory) {
    QFile file(QDir(directory).filePath("projects.json"));
    if (!file.exists())
        return Json::array();
    if (!file.open(QIODevice::ReadOnly) || file.size() > 512 * 1024)
        throw std::runtime_error("Não foi possível ler a biblioteca de projetos");
    auto index = Json::parse(file.readAll().toStdString(), [](int depth, Json::parse_event_t, Json &) {
        if (depth > 8)
            throw std::runtime_error("Biblioteca de projetos inválida");
        return true;
    });
    if (index.at("version") != 1 || !index.at("projects").is_array() || index.at("projects").size() > 100)
        throw std::runtime_error("Biblioteca de projetos inválida; arquivos preservados");
    return index.at("projects");
}
void writeIndex(const QString &directory, const Json &entries) {
    QDir().mkpath(directory);
    QSaveFile file(QDir(directory).filePath("projects.json"));
    auto bytes = QByteArray::fromStdString(Json{{"version", 1}, {"projects", entries}}.dump());
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        throw std::runtime_error("Não foi possível atualizar a biblioteca de projetos");
}
} // namespace
ProjectLibrary::ProjectLibrary(QString root) : directory(std::move(root)) {
}
std::vector<RecentProject> ProjectLibrary::projects() const {
    std::vector<RecentProject> result;
    for (const auto &entry : readIndex(directory)) {
        auto path = QString::fromStdString(entry.at("path").get<std::string>());
        auto name = QString::fromStdString(entry.at("name").get<std::string>());
        auto thumb = QString::fromStdString(entry.value("thumbnail", std::string{}));
        if (path.size() > 8192 || name.size() > 512 || thumb.contains('/') || thumb.contains('\\'))
            throw std::runtime_error("Referência de projeto inválida");
        result.push_back({path, name, QString::fromStdString(entry.at("modified").get<std::string>()),
                          thumb.isEmpty() ? QString{} : QDir(directory).filePath(thumb),
                          QFileInfo::exists(path)});
    }
    return result;
}
void ProjectLibrary::remember(const QString &filename, const Document &document, const QImage &preview) {
    auto path = normalizedPath(filename);
    if (!QFileInfo::exists(path) || !path.endsWith(".lmx", Qt::CaseInsensitive))
        throw std::invalid_argument("Salve o projeto .lmx antes de adicioná-lo à biblioteca");
    auto entries = readIndex(directory);
    Json retained = Json::array();
    std::string previous;
    for (const auto &entry : entries) {
        if (samePath(QString::fromStdString(entry.at("path").get<std::string>()), path))
            previous = entry.value("thumbnail", std::string{});
        else
            retained.push_back(entry);
    }
    if (!preview.isNull()) {
        auto hash = QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha256).toHex();
        previous = hash.toStdString() + ".png";
        QDir().mkpath(directory);
        QSaveFile thumbnail(QDir(directory).filePath(QString::fromStdString(previous)));
        if (!thumbnail.open(QIODevice::WriteOnly) ||
            !preview.scaled(640, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                 .save(&thumbnail, "PNG") ||
            !thumbnail.commit())
            throw std::runtime_error("Não foi possível salvar a prévia do projeto");
    }
    retained.insert(retained.begin(),
                    Json{{"path", path.toStdString()},
                         {"name", document.name},
                         {"modified", QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString()},
                         {"thumbnail", previous}});
    while (retained.size() > 100)
        retained.erase(retained.end() - 1);
    writeIndex(directory, retained);
}
void ProjectLibrary::forget(const QString &filename) {
    auto path = normalizedPath(filename);
    auto entries = readIndex(directory);
    Json retained = Json::array();
    for (const auto &entry : entries)
        if (!samePath(QString::fromStdString(entry.at("path").get<std::string>()), path))
            retained.push_back(entry);
    writeIndex(directory, retained);
}
} // namespace lmx
