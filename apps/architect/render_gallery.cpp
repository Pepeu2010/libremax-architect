#include "render_gallery.h"
#include "rendering/render_options.h"
#include "rendering/render_progress.h"
#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImageReader>
#include <QMenu>
#include <QUrl>
#include <QVBoxLayout>
namespace lmx {
namespace {
QString text(const Json &entry, const char *key) {
    return QString::fromStdString(entry.value(key, std::string{}));
}
bool done(const QString &state) {
    return state == "Completed" || state == "Failed" || state == "Cancelled" || state == "Interrupted";
}
QString mode(const Json &options) {
    static const std::map<std::string, QString> names{
        {"rapid", "Rápido"}, {"normal", "Normal"}, {"final", "Final"}, {"custom", "Personalizado"}};
    return names.at(options.at("preset").get<std::string>());
}
} // namespace
RenderGallery::RenderGallery(RenderQueue &renderQueue, QWidget *parent)
    : QWidget(parent), queue(renderQueue) {
    setObjectName("renderGallery");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);
    auto *header = new QHBoxLayout;
    auto *title = new QLabel(tr("Suas imagens"));
    title->setProperty("role", "heading");
    header->addWidget(title, 1);
    auto *backButton = new QPushButton(tr("Voltar ao projeto"));
    backButton->setObjectName("galleryBack");
    header->addWidget(backButton);
    connect(backButton, &QPushButton::clicked, this, &RenderGallery::back);
    layout->addLayout(header);
    summary = new QLabel;
    summary->setWordWrap(true);
    summary->setProperty("role", "muted");
    layout->addWidget(summary);
    progressPanel = new QWidget;
    progressPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    auto *progressLayout = new QVBoxLayout(progressPanel);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(8);
    progressStatus = new QLabel;
    progressStatus->setObjectName("galleryRenderStatus");
    progressStatus->setWordWrap(true);
    progressLayout->addWidget(progressStatus);
    progress = new QProgressBar;
    progress->setObjectName("galleryRenderProgress");
    progress->setAccessibleName(tr("Progresso da imagem em renderização"));
    progress->setTextVisible(true);
    progressLayout->addWidget(progress);
    timing = new QLabel;
    timing->setObjectName("galleryRenderTiming");
    timing->setWordWrap(true);
    timing->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    timing->setTextFormat(Qt::PlainText);
    timing->setTextInteractionFlags(Qt::TextSelectableByMouse);
    progressLayout->addWidget(timing);
    layout->addWidget(progressPanel);
    images = new QListWidget;
    images->setObjectName("renderImages");
    images->setAccessibleName(tr("Fila e imagens deste projeto"));
    images->setIconSize({112, 72});
    images->setSpacing(4);
    images->setWordWrap(true);
    layout->addWidget(images, 1);
    details = new QLabel;
    details->setObjectName("renderImageDetails");
    details->setWordWrap(true);
    details->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(details);
    auto *actions = new QHBoxLayout;
    open = new QPushButton(tr("Abrir imagem"));
    open->setObjectName("galleryOpen");
    open->setProperty("role", "primary");
    saveCopy = new QPushButton(tr("Salvar cópia…"));
    saveCopy->setObjectName("gallerySaveCopy");
    cancel = new QPushButton(tr("Cancelar imagem"));
    cancel->setObjectName("galleryCancel");
    more = new QToolButton;
    more->setText(tr("Mais ações"));
    more->setObjectName("galleryMore");
    more->setMinimumHeight(38);
    more->setPopupMode(QToolButton::InstantPopup);
    auto *menu = new QMenu(more);
    repeat = menu->addAction(tr("Renderizar novamente"));
    cpu = menu->addAction(tr("Renderizar com o processador"));
    folder = menu->addAction(tr("Mostrar pasta da imagem"));
    logs = menu->addAction(tr("Ver detalhes do processo"));
    menu->addSeparator();
    erase = menu->addAction(tr("Excluir da galeria…"));
    more->setMenu(menu);
    for (auto *button : {open, saveCopy, cancel})
        actions->addWidget(button);
    actions->addWidget(more);
    actions->addStretch();
    layout->addLayout(actions);
    connect(images, &QListWidget::currentRowChanged, this, [this] { selectionChanged(); });
    connect(images, &QListWidget::itemDoubleClicked, this, [this] {
        if (open->isEnabled())
            open->click();
    });
    connect(open, &QPushButton::clicked, this, [this] { emit openImage(queue.displayPath(selectedId())); });
    connect(saveCopy, &QPushButton::clicked, this, [this] { emit saveImage(queue.imagePath(selectedId())); });
    connect(cancel, &QPushButton::clicked, this, [this] { emit cancelImage(selectedId()); });
    connect(repeat, &QAction::triggered, this, [this] { emit retryImage(selectedId(), false); });
    connect(cpu, &QAction::triggered, this, [this] { emit retryImage(selectedId(), true); });
    connect(erase, &QAction::triggered, this, [this] { emit removeImage(selectedId()); });
    connect(folder, &QAction::triggered, this, [this] {
        QDesktopServices::openUrl(
            QUrl::fromLocalFile(QFileInfo(queue.imagePath(selectedId())).absolutePath()));
    });
    connect(logs, &QAction::triggered, this,
            [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(queue.logPath(selectedId()))); });
    connect(&queue, &RenderQueue::changed, this, &RenderGallery::refresh);
    connect(&queue, &RenderQueue::timingChanged, this, &RenderGallery::refreshTiming);
    refresh();
}
QString RenderGallery::selectedId() const {
    return images->currentItem() ? images->currentItem()->data(Qt::UserRole).toString() : QString{};
}
void RenderGallery::setProject(const std::string &id) {
    if (project == id)
        return;
    project = id;
    rows.clear();
    images->clear();
    refresh();
}
void RenderGallery::refresh() {
    const auto entries = queue.entries(project);
    std::set<QString> present;
    int pending = 0;
    for (const auto &entry : entries) {
        const auto id = text(entry, "id"), state = text(entry, "state");
        present.insert(id);
        if (!done(state))
            ++pending;
        auto &row = rows[id];
        if (!row) {
            row = new QListWidgetItem(images);
            row->setData(Qt::UserRole, id);
            row->setSizeHint({100, 96});
        }
        const auto &options = entry.at("options");
        auto status = renderStateLabel(state);
        if (entry.value("progress", -1) >= 0 && !done(state))
            status += QString(" · %1% do cálculo").arg(entry.at("progress").get<int>());
        const auto date = QDateTime::fromString(text(entry, "created"), Qt::ISODateWithMs).toLocalTime();
        row->setText(QString("%1\n%2 · %3 × %4 · %5\n%6")
                         .arg(text(entry, "cameraName"), mode(options))
                         .arg(options.at("width").get<int>())
                         .arg(options.at("height").get<int>())
                         .arg(date.toString("dd/MM/yyyy HH:mm"), status));
        row->setToolTip(text(entry, "error"));
        if (state == "Completed" && row->icon().isNull()) {
            QImageReader reader(queue.displayPath(id));
            reader.setScaledSize(reader.size().scaled({112, 72}, Qt::KeepAspectRatio));
            const auto image = reader.read();
            if (!image.isNull())
                row->setIcon(QPixmap::fromImage(image));
        }
    }
    for (auto it = rows.begin(); it != rows.end();) {
        if (!present.contains(it->first)) {
            delete it->second;
            it = rows.erase(it);
        } else
            ++it;
    }
    summary->setText(entries.empty() ? tr("Crie uma câmera e envie sua primeira imagem. Você pode continuar "
                                          "editando enquanto renderiza.")
                     : pending ? tr("%1 imagem(ns) em preparo ou na fila. Uma imagem renderiza por vez. Você "
                                    "pode continuar editando.")
                                     .arg(pending)
                               : tr("Imagens deste projeto. Cada imagem mantém a cena e as configurações "
                                    "usadas no momento do envio."));
    if (!images->currentItem() && images->count())
        images->setCurrentRow(images->count() - 1);
    selectionChanged();
}
void RenderGallery::selectionChanged() {
    refreshTiming();
    const auto id = selectedId();
    const auto entries = queue.entries(project);
    const auto found = std::find_if(entries.begin(), entries.end(),
                                    [&](const auto &entry) { return text(entry, "id") == id; });
    const bool available = found != entries.end();
    const auto state = available ? text(*found, "state") : QString{};
    const bool complete = available && state == "Completed" && QFileInfo::exists(queue.imagePath(id));
    open->setEnabled(complete && QFileInfo::exists(queue.displayPath(id)));
    saveCopy->setEnabled(complete);
    folder->setEnabled(complete);
    cancel->setEnabled(available && !done(state));
    repeat->setEnabled(available && done(state) && QFileInfo::exists(queue.snapshotPath(id)));
    cpu->setEnabled(repeat->isEnabled());
    erase->setEnabled(available && done(state));
    logs->setEnabled(available && QFileInfo::exists(queue.logPath(id)));
    details->setText(available ? text(*found, "error") : tr("Nenhuma imagem selecionada."));
}
void RenderGallery::refreshTiming() {
    const auto entries = queue.entries(project);
    auto found = std::find_if(entries.begin(), entries.end(),
                              [&](const auto &entry) { return text(entry, "id") == queue.active(); });
    if (found == entries.end())
        found = std::find_if(entries.begin(), entries.end(),
                             [&](const auto &entry) { return text(entry, "id") == selectedId(); });
    progressPanel->setVisible(found != entries.end());
    if (found == entries.end())
        return;
    const auto state = text(*found, "state");
    const auto value = found->value("progress", -1);
    const bool unknown = value < 0 && !done(state);
    progress->setRange(0, unknown ? 0 : 100);
    if (!unknown)
        progress->setValue(std::max(0, value));
    progress->setFormat(state == "Completed"   ? tr("Concluído · 100%")
                        : state == "Rendering" ? tr("%p% do cálculo")
                                               : renderStateLabel(state));
    progressStatus->setText(QString("%1 · %2").arg(text(*found, "cameraName"), renderStateLabel(state)));
    timing->setText(renderTimingText(*found));
}
} // namespace lmx
