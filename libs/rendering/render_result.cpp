#include "render_result.h"
#include "high_dynamic_image.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QSaveFile>
#include <stdexcept>
namespace lmx {
namespace {
void checkCancellation(const std::atomic_bool &cancelled) {
    if (cancelled.load())
        throw std::runtime_error("Cancelado");
}
void copyAtomic(const QString &source, const QString &target, const std::atomic_bool &cancelled) {
    QFile input(source);
    QSaveFile output(target);
    output.setDirectWriteFallback(false);
    if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly))
        throw std::runtime_error("Não foi possível salvar a imagem renderizada");
    while (!input.atEnd()) {
        checkCancellation(cancelled);
        const auto chunk = input.read(1024 * 1024);
        if (chunk.isEmpty() || output.write(chunk) != chunk.size())
            throw std::runtime_error("Falha ao salvar a imagem; versão anterior preservada");
    }
    checkCancellation(cancelled);
    if (!output.commit())
        throw std::runtime_error("Falha ao salvar a imagem; versão anterior preservada");
}
void validatePreview(const QString &path, QSize resolution) {
    QImageReader image(path);
    if (!image.canRead() || image.size() != resolution || image.read().isNull())
        throw std::runtime_error("O mecanismo de render não produziu uma imagem válida");
}
} // namespace
PublishedRender publishRender(const QString &source, const QString &preview, const QString &destination,
                              QSize resolution, const std::atomic_bool &cancelled) {
    QString createdPreview;
    try {
        checkCancellation(cancelled);
        const bool exr = destination.endsWith(".exr", Qt::CaseInsensitive);
        auto display = destination;
        if (exr) {
            QFile original(source);
            if (!original.open(QIODevice::ReadOnly) || original.size() > 512 * 1024 * 1024)
                throw std::runtime_error("Imagem EXR ausente ou grande demais");
            const auto hdr = inspectHighDynamicImage(original.readAll(), true, 64 * 1024 * 1024);
            if (hdr.format != "exr" || hdr.size != resolution)
                throw std::runtime_error("A imagem EXR está incompleta ou tem tamanho incorreto");
            validatePreview(preview, resolution);
            QFile png(preview);
            QCryptographicHash hash(QCryptographicHash::Sha256);
            if (!png.open(QIODevice::ReadOnly) || !hash.addData(&png))
                throw std::runtime_error("Não foi possível ler a prévia da imagem");
            display = QDir(QFileInfo(destination).absolutePath())
                          .filePath("preview-" + QString::fromLatin1(hash.result().toHex()) + ".png");
            if (!QFileInfo::exists(display)) {
                copyAtomic(preview, display, cancelled);
                createdPreview = display;
            } else {
                QFile existing(display);
                QCryptographicHash existingHash(QCryptographicHash::Sha256);
                if (!existing.open(QIODevice::ReadOnly) || !existingHash.addData(&existing) ||
                    existingHash.result() != hash.result())
                    throw std::runtime_error("A prévia salva está inválida; arquivos anteriores preservados");
            }
        } else
            validatePreview(source, resolution);
        copyAtomic(source, destination, cancelled);
        return {display, {}};
    } catch (const std::exception &error) {
        if (!createdPreview.isEmpty())
            QFile::remove(createdPreview);
        return {{}, QString::fromUtf8(error.what())};
    }
}
} // namespace lmx
