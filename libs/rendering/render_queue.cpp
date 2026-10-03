#include "render_queue.h"
#include "persistence/project_store.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>
#include <QUuid>
#include <QtConcurrent>
#include <algorithm>
namespace lmx {
namespace {
QString string(const Json &value) {
    return QString::fromStdString(value.get<std::string>());
}
bool terminal(const Json &entry) {
    const auto state = entry.at("state");
    return state == "Completed" || state == "Failed" || state == "Cancelled" || state == "Interrupted";
}
QString now() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}
} // namespace
RenderQueue::RenderQueue(QString directory, QObject *parent) : QObject(parent), root(std::move(directory)) {
    preparationPool.setMaxThreadCount(2);
    QDir().mkpath(root);
    QDir folder(root);
    for (const auto &id : folder.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        if (QUuid(id).isNull() || QFileInfo(folder.filePath(id)).isSymLink())
            continue;
        QFile file(folder.filePath(id + "/job.json"));
        if (!file.open(QIODevice::ReadOnly) || file.size() > 128 * 1024)
            continue;
        try {
            auto record =
                Json::parse(file.readAll().toStdString(), [](int depth, Json::parse_event_t, Json &) {
                    if (depth > 12)
                        throw std::invalid_argument("Histórico de render inválido");
                    return true;
                });
            if (record.at("version") != 1 || string(record.at("id")) != id)
                continue;
            validateRenderOptions(record.at("options"));
            for (const auto *key : {"elapsedMs", "remainingMs", "remainingUpdatedElapsedMs"})
                if (record.contains(key) &&
                    (!record[key].is_number_integer() || record[key].get<qint64>() < -1 ||
                     record[key].get<qint64>() > 604800000))
                    throw std::invalid_argument("Tempo de render inválido");
            if (record.contains("estimatedFinish") && !record["estimatedFinish"].is_string())
                throw std::invalid_argument("Previsão de render inválida");
            if (record.contains("preview") &&
                (!record.at("preview").is_string() || !QRegularExpression("^preview-[a-f0-9]{64}\\.png$")
                                                           .match(string(record.at("preview")))
                                                           .hasMatch()))
                throw std::invalid_argument("Prévia de render inválida");
            for (const auto *key :
                 {"project", "projectName", "camera", "cameraName", "created", "state", "blender"})
                if (!record.at(key).is_string())
                    throw std::invalid_argument("Histórico incompleto");
            const auto state = string(record.at("state"));
            if (!std::set<QString>{"Queued", "Preparing", "Exporting", "Rendering", "Denoising", "Saving",
                                   "Completed", "Failed", "Cancelled", "Interrupted"}
                     .contains(state))
                throw std::invalid_argument("Estado de render inválido");
            if (record.value("deleted", false))
                continue;
            nextOrder = std::max(nextOrder, record.value("order", std::uint64_t{0}));
            if (!terminal(record)) {
                record["state"] = "Interrupted";
                record["error"] =
                    "O aplicativo fechou antes de concluir esta imagem. Você pode renderizar novamente.";
            }
            records[id] = std::move(record);
            save(id);
        } catch (const std::exception &) { /* Corrupt records remain on disk; other images still open. */
        }
        if (records.size() >= 500)
            break;
    }
    connect(&job, &RenderJob::stage, this, [this](const QString &state) {
        if (!activeId.isEmpty()) {
            auto &entry = records.at(activeId);
            entry["elapsedMs"] = elapsed.elapsed();
            entry["remainingMs"] = -1;
            entry.erase("estimatedFinish");
            setState(activeId, state);
        }
    });
    connect(&job, &RenderJob::progress, this, [this](int current, int total, int percent, qint64 remaining) {
        if (activeId.isEmpty())
            return;
        auto &entry = records.at(activeId);
        entry["sample"] = current;
        entry["total"] = total;
        entry["progress"] = percent;
        entry["elapsedMs"] = elapsed.elapsed();
        entry["remainingMs"] = remaining;
        entry["remainingUpdatedElapsedMs"] = elapsed.elapsed();
        if (remaining >= 0)
            entry["estimatedFinish"] =
                QDateTime::currentDateTimeUtc().addMSecs(remaining).toString(Qt::ISODateWithMs).toStdString();
        else
            entry.erase("estimatedFinish");
        save(activeId);
        emit changed();
    });
    connect(&job, &RenderJob::log, this, [this](const QString &text) {
        if (activeId.isEmpty())
            return;
        QFile file(logPath(activeId));
        if (file.open(QIODevice::WriteOnly | QIODevice::Append) && file.size() < 8 * 1024 * 1024)
            file.write(text.toUtf8());
        emit log(text);
    });
    connect(&job, &RenderJob::finished, this,
            [this](bool success, bool stopped, const QString &message, int code) {
                if (activeId.isEmpty())
                    return;
                const auto id = activeId;
                auto &entry = records.at(id);
                entry["exitCode"] = code;
                entry["error"] = message.toStdString();
                entry["engine"] = job.engine();
                entry["finished"] = now().toStdString();
                entry["elapsedMs"] = elapsed.elapsed();
                entry["remainingMs"] = -1;
                entry.erase("estimatedFinish");
                if (success) {
                    entry["progress"] = 100;
                    if (entry.at("options").at("format") == "EXR")
                        entry["preview"] = QFileInfo(job.previewPath()).fileName().toStdString();
                }
                setState(id, success ? "Completed" : stopped ? "Cancelled" : "Failed");
                snapshots.erase(id);
                prepared.erase(id);
                cancelled.erase(id);
                activeId.clear();
                if (success)
                    emit completed(id, displayPath(id));
                emit changed();
                QTimer::singleShot(0, this, &RenderQueue::dispatch);
            });
    auto *heartbeat = new QTimer(this);
    heartbeat->setInterval(1000);
    connect(heartbeat, &QTimer::timeout, this, [this] {
        if (activeId.isEmpty())
            return;
        records.at(activeId)["elapsedMs"] = elapsed.elapsed();
        emit timingChanged();
    });
    heartbeat->start();
}
RenderQueue::~RenderQueue() {
    stopping = true;
    disconnect(&job, nullptr, this, nullptr);
    job.cancel();
    preparationPool.waitForDone();
}
QString RenderQueue::imagePath(const QString &id) const {
    const auto &record = records.at(id);
    const auto format = record.at("options").at("format");
    return QDir(root).filePath(id + "/output/result." +
                               (format == "EXR"    ? "exr"
                                : format == "JPEG" ? "jpg"
                                                   : "png"));
}
QString RenderQueue::displayPath(const QString &id) const {
    const auto &record = records.at(id);
    if (record.at("options").at("format") != "EXR")
        return imagePath(id);
    return record.contains("preview") ? QDir(root).filePath(id + "/output/" + string(record.at("preview")))
                                      : QString{};
}
QString RenderQueue::snapshotPath(const QString &id) const {
    return QDir(root).filePath(id + "/scene/snapshot.lmx");
}
QString RenderQueue::logPath(const QString &id) const {
    return QDir(root).filePath(id + "/logs/blender.log");
}
void RenderQueue::save(const QString &id) {
    auto &record = records.at(id);
    const auto bytes = QByteArray::fromStdString(record.dump());
    QSaveFile file(QDir(root).filePath(id + "/job.json"));
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        emit warning("Não foi possível atualizar o histórico de renders. Verifique o espaço em disco.");
}
void RenderQueue::setState(const QString &id, const QString &state) {
    records.at(id)["state"] = state.toStdString();
    save(id);
    emit changed();
}
QString RenderQueue::enqueue(const RenderSnapshot &snapshot, const QString &blender, const QString &script) {
    if (records.size() >= 500 || waiting.size() >= 20)
        throw std::runtime_error("A fila está cheia. Conclua ou remova imagens antes de adicionar mais.");
    if (!QFileInfo(blender).isExecutable() || !QFileInfo(script).isFile())
        throw std::runtime_error("O mecanismo de render não foi encontrado.");
    const auto id = QString::fromStdString(uuid());
    for (const auto &folder : {"scene", "scripts", "output", "logs"})
        if (!QDir().mkpath(QDir(root).filePath(id + '/' + folder)))
            throw std::runtime_error("Não foi possível criar a pasta desta imagem.");
    const auto &doc = snapshot.document();
    const auto cameraId = doc.renderSettings.at("camera").get<std::string>();
    records[id] = {{"version", 1},
                   {"id", id.toStdString()},
                   {"project", doc.id},
                   {"projectName", doc.name},
                   {"camera", cameraId},
                   {"cameraName", doc.at(cameraId).name},
                   {"created", now().toStdString()},
                   {"order", ++nextOrder},
                   {"state", "Preparing"},
                   {"progress", -1},
                   {"options", snapshot.options()},
                   {"blender", blender.toStdString()},
                   {"error", ""},
                   {"libremax", QCoreApplication::applicationVersion().toStdString()}};
    snapshots[id] = std::make_shared<const RenderSnapshot>(snapshot);
    waiting.push_back(id);
    save(id);
    emit changed();
    prepare(id, blender, script);
    return id;
}
void RenderQueue::prepare(const QString &id, const QString &, const QString &script) {
    auto *watcher = new QFutureWatcher<QString>(this);
    const auto destination = snapshotPath(id),
               scriptFile = QDir(root).filePath(id + "/scripts/cycles_render.py");
    const auto snapshot = snapshots.at(id);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, id] {
        const auto error = watcher->result();
        watcher->deleteLater();
        if (stopping)
            return;
        if (cancelled.contains(id)) {
            snapshots.erase(id);
            cancelled.erase(id);
            dispatch();
            emit changed();
            return;
        }
        if (!error.isEmpty()) {
            records.at(id)["error"] = error.toStdString();
            setState(id, "Failed");
            snapshots.erase(id);
        } else {
            prepared.insert(id);
            setState(id, "Queued");
        }
        dispatch();
    });
    watcher->setFuture(
        QtConcurrent::run(&preparationPool, [snapshot, destination, script, scriptFile]() -> QString {
            try {
                ProjectStore::save(destination, snapshot->document(), false);
                if (!QFile::copy(script, scriptFile))
                    throw std::runtime_error("Não foi possível preparar o mecanismo de render");
                for (const auto *module : {"cycles_lights.py", "cycles_camera.py"}) {
                    const auto translator = QFileInfo(script).dir().filePath(module);
                    if (QFileInfo::exists(translator) &&
                        !QFile::copy(translator, QFileInfo(scriptFile).dir().filePath(module)))
                        throw std::runtime_error("Não foi possível preparar os arquivos do render");
                }
                return {};
            } catch (const std::exception &e) {
                return QString::fromUtf8(e.what());
            }
        }));
}
void RenderQueue::dispatch() {
    if (stopping || !activeId.isEmpty() || job.busy())
        return;
    while (!waiting.empty()) {
        auto id = waiting.front();
        if (!records.contains(id)) {
            waiting.pop_front();
            continue;
        }
        if (terminal(records.at(id))) {
            waiting.pop_front();
            continue;
        }
        if (!prepared.contains(id))
            return;
        waiting.pop_front();
        activeId = id;
        auto &entry = records.at(id);
        entry["started"] = now().toStdString();
        elapsed.start();
        try {
            const auto &options = entry.at("options");
            job.start(snapshots.at(id)->document(), string(entry.at("blender")),
                      QDir(root).filePath(id + "/scripts/cycles_render.py"), imagePath(id),
                      options.at("width").get<int>(), options.at("height").get<int>(),
                      options.at("samples").get<int>(), string(options.at("device")));
        } catch (const std::exception &e) {
            entry["error"] = e.what();
            setState(id, "Failed");
            snapshots.erase(id);
            activeId.clear();
            continue;
        }
        return;
    }
}
QString RenderQueue::retry(const QString &id, const QString &device) {
    if (!records.contains(id) || !terminal(records.at(id)))
        throw std::runtime_error("Aguarde esta imagem terminar ou cancele antes de repetir.");
    auto doc = ProjectStore::open(snapshotPath(id));
    auto options = records.at(id).at("options");
    if (!device.isEmpty())
        options["device"] = device.toStdString();
    return enqueue(RenderSnapshot(doc, options, doc.renderSettings.at("camera").get<std::string>()),
                   string(records.at(id).at("blender")),
                   QDir(root).filePath(id + "/scripts/cycles_render.py"));
}
void RenderQueue::cancel(const QString &id) {
    if (!records.contains(id) || terminal(records.at(id)))
        return;
    cancelled.insert(id);
    if (id == activeId) {
        job.cancel();
        return;
    }
    setState(id, "Cancelled");
    if (prepared.contains(id))
        snapshots.erase(id);
    dispatch();
}
void RenderQueue::cancelActive() {
    if (!activeId.isEmpty())
        cancel(activeId);
}
void RenderQueue::cancelAll() {
    const auto pending = entries();
    for (const auto &entry : pending)
        cancel(string(entry.at("id")));
}
void RenderQueue::remove(const QString &id) {
    if (!records.contains(id) || !terminal(records.at(id)))
        throw std::runtime_error("Cancele a imagem antes de remover.");
    records.at(id)["deleted"] = true;
    save(id);
    // Only our result is removed. Keep snapshot/logs for diagnosis; never delete a project file.
    QFile::remove(imagePath(id));
    if (records.at(id).at("options").at("format") == "EXR")
        QFile::remove(displayPath(id));
    records.erase(id);
    emit changed();
}
bool RenderQueue::busy() const {
    return !activeId.isEmpty() || !waiting.empty();
}
std::vector<Json> RenderQueue::entries(const std::string &project) const {
    std::vector<Json> result;
    for (const auto &[id, record] : records)
        if (project.empty() || record.at("project") == project)
            result.push_back(record);
    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
        if (a.at("created") != b.at("created"))
            return a.at("created") < b.at("created");
        return a.value("order", std::uint64_t{0}) < b.value("order", std::uint64_t{0});
    });
    return result;
}
} // namespace lmx
