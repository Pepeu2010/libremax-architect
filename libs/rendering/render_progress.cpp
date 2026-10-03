#include "render_progress.h"
#include <QDateTime>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
namespace lmx {
namespace {
qint64 remainingTime(const QString &line) {
    static const QRegularExpression remaining("Remaining:\\s*(\\d+:(?:\\d+:)?\\d+(?:\\.\\d+)?)");
    const auto match = remaining.match(line);
    if (!match.hasMatch())
        return -1;
    const auto parts = match.captured(1).split(':');
    const auto seconds = parts.back().toDouble();
    const auto minutes = parts[parts.size() - 2].toLongLong();
    const auto hours = parts.size() == 3 ? parts.front().toLongLong() : 0;
    if (seconds >= 60 || (parts.size() == 3 && minutes >= 60) || hours > 168 || minutes > 10080)
        return -1;
    const auto value = ((hours * 60 + minutes) * 60 + seconds) * 1000;
    return std::isfinite(value) && value <= 604800000 ? std::llround(value) : -1;
}
} // namespace
std::optional<RenderProgressUpdate> CyclesProgress::consume(const QString &line) {
    static const QRegularExpression samples("(?:Sample|Rendering)\\s+(\\d+)\\s*/\\s*(\\d+)");
    static const QRegularExpression tiles("Rendered\\s+(\\d+)\\s*/\\s*(\\d+)\\s+Tiles");
    const auto sample = samples.match(line);
    if (!sample.hasMatch())
        return {};
    const int current = sample.captured(1).toInt(), total = sample.captured(2).toInt();
    if (total <= 0 || total > 4096 || current < 0 || current > total)
        return {};
    auto fraction = double(current) / total;
    const auto tile = tiles.match(line);
    if (tile.hasMatch()) {
        const auto completed = tile.captured(1).toInt(), count = tile.captured(2).toInt();
        if (count <= 0 || count > 1000000 || completed > count)
            return {};
        // A changed tile counter still reports the last sample of the tile just finished.
        if (completed > completedTiles)
            fraction = 0;
        completedTiles = completed;
        fraction = (completed + fraction) / count;
    }
    percent = std::max(percent, std::clamp(int(fraction * 100), 0, 99));
    return RenderProgressUpdate{current, total, percent, remainingTime(line)};
}
QString renderDuration(qint64 milliseconds) {
    const auto seconds = std::max<qint64>(0, milliseconds) / 1000;
    if (seconds >= 3600)
        return QString("%1h %2min %3s").arg(seconds / 3600).arg(seconds / 60 % 60).arg(seconds % 60);
    if (seconds >= 60)
        return QString("%1min %2s").arg(seconds / 60).arg(seconds % 60);
    return QString("%1s").arg(seconds);
}
bool isCyclesDenoising(const QString &line) {
    static const QRegularExpression denoising("\\|\\s*Denoising(?:\\s+\\d+\\s*/\\s*\\d+)?\\s*$",
                                              QRegularExpression::CaseInsensitiveOption);
    return denoising.match(line).hasMatch();
}
QString renderTimingText(const Json &entry) {
    const auto state = entry.value("state", std::string{});
    const auto elapsed = entry.value("elapsedMs", qint64{0});
    if (state == "Completed" || state == "Failed" || state == "Cancelled" || state == "Interrupted")
        return entry.contains("elapsedMs") ? QString("Tempo em execução: %1").arg(renderDuration(elapsed))
                                           : QString("Tempo não registrado nesta imagem.");
    if (!entry.contains("started"))
        return "Aguardando o início. A estimativa aparece durante o render.";
    const auto age = elapsed - entry.value("remainingUpdatedElapsedMs", elapsed);
    const auto remaining = entry.value("remainingMs", qint64{-1}) - std::max<qint64>(0, age);
    const auto clock = QString("Decorrido: %1").arg(renderDuration(elapsed));
    if (state == "Denoising" || state == "Saving")
        return clock + " · finalizando a imagem. Tempo restante a confirmar.";
    if (state != "Rendering" || remaining <= 0 || age > 30000)
        return clock + " · calculando estimativa…";
    const auto deadline =
        QDateTime::fromString(QString::fromStdString(entry.value("estimatedFinish", std::string{})),
                              Qt::ISODateWithMs)
            .toLocalTime();
    auto prediction =
        QString("%1 · restante estimado: %2").arg(clock, renderDuration(((remaining + 999) / 1000) * 1000));
    if (deadline.isValid())
        prediction +=
            QString("\nCálculo até %1, aproximadamente; a finalização vem depois.")
                .arg(deadline.toString(deadline.date() == QDate::currentDate() ? "HH:mm:ss" : "dd/MM HH:mm"));
    return prediction;
}
} // namespace lmx
