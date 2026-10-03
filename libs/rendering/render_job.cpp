#include "render_job.h"
#include "render_scene_exporter.h"
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QSaveFile>
#include <QtConcurrent>
namespace lmx {
RenderJob::RenderJob(QObject *parent) : QObject(parent) {
    connect(&bridge, &BlenderBridge::output, this, &RenderJob::consumeOutput);
    connect(&bridge, &BlenderBridge::finished, this, [this](int code, bool normalExit) {
        if (cancelled) {
            finish(false, tr("Cancelado"), code);
            return;
        }
        if (code != 0 || !normalExit) {
            finish(false,
                   tr("O mecanismo de render não produziu uma imagem válida. Veja os detalhes ou tente com "
                      "CPU."),
                   code);
            return;
        }
        emit stage("Saving");
        disconnect(&publishing, nullptr, this, nullptr);
        connect(&publishing, &QFutureWatcher<PublishedRender>::finished, this, [this, code] {
            const auto result = publishing.result();
            // A completed atomic publication wins a cancellation arriving after commit.
            if (result.error.isEmpty()) {
                cancelled = false;
                display = result.preview;
            }
            finish(result.error.isEmpty(), result.error, code);
        });
        auto directory = work;
        const auto source = renderedFile, preview = renderedPreview, destination = output;
        const auto size = resolution;
        auto cancellation = publicationCancelled;
        publishing.setFuture(QtConcurrent::run([directory, source, preview, destination, size, cancellation] {
            return publishRender(source, preview, destination, size, *cancellation);
        }));
    });
}
void RenderJob::consumeOutput(const QString &text) {
    emit log(text);
    lineBuffer += text;
    if (lineBuffer.size() > 65536)
        lineBuffer = lineBuffer.right(65536);
    int newline;
    while ((newline = lineBuffer.indexOf('\n')) >= 0) {
        const auto line = lineBuffer.left(newline);
        lineBuffer.remove(0, newline + 1);
        if (line.startsWith("LIBREMAX_GPU_FALLBACK")) {
            progressTracker = {};
            emit progress(0, 0, -1, -1);
        }
        if (const auto update = progressTracker.consume(line))
            emit progress(update->sample, update->total, update->percent, update->remainingMs);
        if (isCyclesDenoising(line))
            emit stage("Denoising");
        if (line.startsWith("LIBREMAX_ENGINE ")) {
            try {
                engineInfo = Json::parse(line.mid(16).toStdString());
            } catch (const std::exception &) { /* Keep the prior valid engine report. */
            }
        }
        if (line.startsWith("LIBREMAX_STAGE "))
            emit stage(line.mid(15).trimmed());
    }
}
void RenderJob::finish(bool success, const QString &message, int code) {
    if (!active)
        return;
    active = false;
    work.reset();
    emit state(success ? tr("Concluído") : cancelled ? tr("Cancelado") : tr("Falhou"));
    if (!message.isEmpty())
        emit log(message + '\n');
    if (success)
        emit completed(output);
    emit finished(success, cancelled, message, code);
}
RenderJob::~RenderJob() {
    disconnect(&bridge, nullptr, this, nullptr);
    disconnect(&preparing, nullptr, this, nullptr);
    disconnect(&publishing, nullptr, this, nullptr);
    cancel();
    preparing.waitForFinished();
    publishing.waitForFinished();
}
bool RenderJob::busy() const {
    return active || preparing.isRunning() || publishing.isRunning() || bridge.busy();
}
void RenderJob::start(const Document &snapshot, const QString &blender, const QString &script,
                      const QString &destination, int width, int height, int samples, const QString &device) {
    if (busy())
        throw std::runtime_error("Já existe um render em execução");
    if (!QFileInfo(blender).isExecutable() || !QFileInfo::exists(script))
        throw std::runtime_error(
            "O mecanismo de render não foi encontrado. Selecione o Blender nas configurações.");
    if (width < 16 || height < 16 || width > 8192 || height > 8192 || samples < 1 || samples > 4096)
        throw std::runtime_error("Configuração de render inválida");
    cancelled = false;
    publicationCancelled = std::make_shared<std::atomic_bool>(false);
    display.clear();
    lineBuffer.clear();
    engineInfo = Json::object();
    progressTracker = {};
    output = destination;
    work = std::make_shared<QTemporaryDir>();
    if (!work->isValid())
        throw std::runtime_error("Falha ao criar pacote de render");
    active = true;
    resolution = {width, height};
    const bool jpeg = destination.endsWith(".jpg", Qt::CaseInsensitive) ||
                      destination.endsWith(".jpeg", Qt::CaseInsensitive);
    const bool exr = destination.endsWith(".exr", Qt::CaseInsensitive);
    renderedFile = work->filePath(exr ? "render.exr" : jpeg ? "render.jpg" : "render.png");
    renderedPreview = exr ? work->filePath("preview.png") : QString{};
    emit state(tr("Preparando"));
    emit stage("Exporting");
    disconnect(&preparing, &QFutureWatcher<QString>::finished, this, nullptr);
    connect(&preparing, &QFutureWatcher<QString>::finished, this,
            [this, blender, script, width, height, samples, device] {
                const auto error = preparing.result();
                if (cancelled) {
                    finish(false, tr("Cancelado"));
                    return;
                }
                if (!error.isEmpty()) {
                    finish(false, error);
                    return;
                }
                emit state(tr("Renderizando"));
                emit stage("Rendering");
                QStringList arguments{
                    "--scene",   work->filePath("scene.json"), "--output", renderedFile,
                    "--width",   QString::number(width),       "--height", QString::number(height),
                    "--samples", QString::number(samples),     "--device", device};
                if (!renderedPreview.isEmpty())
                    arguments << "--preview" << renderedPreview;
                bridge.start(blender, script, arguments);
            });
    auto directory = work;
    preparing.setFuture(QtConcurrent::run([snapshot, directory]() -> QString {
        try {
            RenderSceneExporter::exportScene(snapshot, directory->filePath("scene.json"));
            return {};
        } catch (const std::exception &e) {
            return QString::fromUtf8(e.what());
        }
    }));
}
void RenderJob::cancel() {
    cancelled = true;
    if (publicationCancelled)
        publicationCancelled->store(true);
    bridge.cancel();
}
} // namespace lmx
