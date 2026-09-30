#include "render_job.h"
#include "geometry/geometry.h"
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QSaveFile>
#include <QtConcurrent>

namespace lmx {
RenderJob::RenderJob(QObject *parent) : QObject(parent) {
    connect(&process, &QProcess::readyReadStandardOutput, this,
            [this] { emit log(QString::fromUtf8(process.readAllStandardOutput())); });
    connect(&process, &QProcess::readyReadStandardError, this,
            [this] { emit log(QString::fromUtf8(process.readAllStandardError())); });
    connect(&process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (cancelled)
            return;
        active = false;
        emit state(tr("Falhou"));
        emit log(process.errorString());
    });
    connect(&process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                active = false;
                if (cancelled) {
                    emit state(tr("Cancelado"));
                    return;
                }
                QImageReader reader(renderedFile);
                if (code != 0 || status != QProcess::NormalExit || !reader.canRead() ||
                    reader.size() != resolution) {
                    emit state(tr("Falhou"));
                    emit log(tr("Blender não produziu uma imagem válida."));
                    return;
                }
                QFile image(renderedFile);
                QSaveFile destination(output);
                destination.setDirectWriteFallback(false);
                if (!image.open(QIODevice::ReadOnly) || !destination.open(QIODevice::WriteOnly)) {
                    emit state(tr("Falhou"));
                    emit log(tr("Não foi possível salvar a imagem renderizada."));
                    return;
                }
                const auto bytes = image.readAll();
                if (destination.write(bytes) != bytes.size() || !destination.commit()) {
                    emit state(tr("Falhou"));
                    emit log(tr("Falha ao salvar a imagem; versão anterior preservada."));
                    return;
                }
                emit state(tr("Concluído"));
                emit completed(output);
            });
}
RenderJob::~RenderJob() {
    cancel();
    preparing.waitForFinished();
    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(3000);
    }
}
bool RenderJob::busy() const {
    return active || preparing.isRunning() || process.state() != QProcess::NotRunning;
}
void RenderJob::start(const Document &snapshot, const QString &blender, const QString &script,
                      const QString &destination, int width, int height, int samples, const QString &device) {
    if (busy())
        throw std::runtime_error("Já existe um render em execução");
    if (!QFileInfo(blender).isExecutable() || !QFileInfo::exists(script))
        throw std::runtime_error("Configure o executável Blender e o script de render");
    if (width < 16 || height < 16 || width > 8192 || height > 8192 || samples < 1 || samples > 4096)
        throw std::runtime_error("Configuração de render inválida");
    cancelled = false;
    output = destination;
    work = std::make_shared<QTemporaryDir>();
    if (!work->isValid())
        throw std::runtime_error("Falha ao criar pacote de render");
    active = true;
    resolution = QSize(width, height);
    const bool jpeg = destination.endsWith(".jpg", Qt::CaseInsensitive) ||
                      destination.endsWith(".jpeg", Qt::CaseInsensitive);
    renderedFile = work->filePath(jpeg ? "render.jpg" : "render.png");
    emit state(tr("Preparando"));
    disconnect(&preparing, &QFutureWatcher<QString>::finished, this, nullptr);
    connect(&preparing, &QFutureWatcher<QString>::finished, this,
            [this, blender, script, width, height, samples, device] {
                auto error = preparing.result();
                if (cancelled) {
                    active = false;
                    emit state(tr("Cancelado"));
                    return;
                }
                if (!error.isEmpty()) {
                    active = false;
                    emit state(tr("Falhou"));
                    emit log(error);
                    return;
                }
                emit state(tr("Renderizando"));
                process.setProgram(blender);
                process.setArguments({"--background", "--factory-startup", "--python-exit-code", "1",
                                      "--python", script, "--", "--scene", work->filePath("scene.json"),
                                      "--output", renderedFile, "--width", QString::number(width), "--height",
                                      QString::number(height), "--samples", QString::number(samples),
                                      "--device", device});
                process.start();
            });
    auto directory = work;
    preparing.setFuture(QtConcurrent::run([snapshot, directory]() -> QString {
        try {
            auto json = meshSnapshot(snapshot).dump();
            QFile f(directory->filePath("scene.json"));
            if (!f.open(QIODevice::WriteOnly) ||
                f.write(json.data(), static_cast<qint64>(json.size())) != static_cast<qint64>(json.size()))
                return QStringLiteral("Falha ao exportar cena");
            return {};
        } catch (const std::exception &e) {
            return QString::fromUtf8(e.what());
        }
    }));
}
void RenderJob::cancel() {
    cancelled = true;
    if (process.state() != QProcess::NotRunning)
        process.kill();
}
} // namespace lmx
