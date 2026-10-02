#include "project_home.h"
#include <QDateTime>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
namespace lmx {
ProjectHome::ProjectHome(QWidget *parent) : QWidget(parent) {
    setObjectName("projectHome");
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 26, 40, 26);
    layout->setSpacing(18);
    auto *header = new QHBoxLayout;
    auto *mark = new QLabel;
    mark->setPixmap(QPixmap(":/studio/brand/libremax-mark.png")
                        .scaled(66, 66, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    header->addWidget(mark);
    auto *brand = new QLabel("LIBREMAX\nARCHITECT");
    brand->setProperty("role", "brand");
    header->addWidget(brand);
    header->addStretch();
    auto *help = new QPushButton(tr("Aprender a usar"));
    help->setObjectName("homeTutorial");
    connect(help, &QPushButton::clicked, this, &ProjectHome::tutorial);
    header->addWidget(help);
    layout->addLayout(header);
    auto *title = new QLabel(tr("Seu próximo espaço começa aqui."));
    title->setWordWrap(true);
    title->setProperty("role", "hero");
    layout->addWidget(title);
    auto *description =
        new QLabel(tr("Retome uma ideia, monte um novo apartamento ou aprenda com um exemplo pronto."));
    description->setWordWrap(true);
    description->setProperty("role", "muted");
    layout->addWidget(description);
    auto *actions = new QHBoxLayout;
    auto *create = new QPushButton(tr("+  Novo projeto"));
    create->setObjectName("homeNewProject");
    create->setProperty("role", "primary");
    auto *open = new QPushButton(tr("Abrir arquivo .lmx"));
    open->setObjectName("homeOpenProject");
    auto *sample = new QPushButton(tr("Experimentar apartamento"));
    auto *recover = new QPushButton(tr("Recuperar alterações"));
    actions->addWidget(create);
    actions->addWidget(open);
    actions->addWidget(sample);
    actions->addStretch();
    connect(create, &QPushButton::clicked, this, &ProjectHome::newProject);
    connect(open, &QPushButton::clicked, this, &ProjectHome::openProject);
    connect(sample, &QPushButton::clicked, this, &ProjectHome::example);
    connect(recover, &QPushButton::clicked, this, &ProjectHome::recovery);
    layout->addLayout(actions);
    auto *recentRow = new QHBoxLayout;
    auto *heading = new QLabel(tr("Seus projetos"));
    heading->setProperty("role", "heading");
    recentRow->addWidget(heading);
    recentRow->addStretch();
    auto *filter = new QLineEdit;
    filter->setPlaceholderText(tr("Buscar projeto pelo nome…"));
    filter->setAccessibleName(tr("Buscar projetos recentes"));
    filter->setMaximumWidth(300);
    recentRow->addWidget(filter);
    layout->addLayout(recentRow);
    empty = new QLabel(tr("Você ainda não salvou um projeto.\nComece pelo botão Novo projeto ou experimente "
                          "o apartamento pronto."));
    empty->setWordWrap(true);
    empty->setProperty("role", "muted");
    layout->addWidget(empty);
    firstProject = new QWidget;
    firstProject->setObjectName("firstProjectCard");
    auto *firstLayout = new QHBoxLayout(firstProject);
    firstLayout->setContentsMargins(20, 18, 20, 18);
    firstLayout->setSpacing(24);
    auto *photo = new QLabel;
    photo->setAlignment(Qt::AlignCenter);
    photo->setPixmap(QPixmap(":/studio/brand/apartment-preview.png")
                         .scaled(500, 280, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    photo->setMinimumWidth(240);
    photo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    firstLayout->addWidget(photo, 3);
    auto *start = new QVBoxLayout;
    auto *kicker = new QLabel(tr("UM PASSO POR VEZ"));
    kicker->setProperty("role", "kicker");
    start->addWidget(kicker);
    auto *firstTitle = new QLabel(tr("Monte seu primeiro espaço."));
    firstTitle->setWordWrap(true);
    firstTitle->setProperty("role", "heading");
    start->addWidget(firstTitle);
    auto *steps = new QLabel(tr(
        "1  Crie o cômodo\n\n2  Escolha e arraste os móveis\n\n3  Ajuste cores e luz\n\n4  Faça uma foto"));
    steps->setWordWrap(true);
    start->addWidget(steps);
    auto *tryExample = new QPushButton(tr("Aprender com o apartamento"));
    tryExample->setProperty("role", "primary");
    connect(tryExample, &QPushButton::clicked, this, &ProjectHome::example);
    start->addWidget(tryExample);
    firstLayout->addLayout(start, 2);
    layout->addWidget(firstProject, 1);
    cards = new QListWidget;
    cards->setObjectName("projectCards");
    cards->setAccessibleName(tr("Biblioteca de projetos; dois cliques ou Enter para abrir"));
    cards->setViewMode(QListView::IconMode);
    cards->setResizeMode(QListView::Adjust);
    cards->setMovement(QListView::Static);
    cards->setWordWrap(true);
    cards->setIconSize({240, 140});
    cards->setGridSize({280, 235});
    cards->setSpacing(10);
    layout->addWidget(cards, 1);
    connect(cards, &QListWidget::itemActivated, this,
            [this](QListWidgetItem *item) { emit openRecent(item->data(Qt::UserRole).toString()); });
    connect(filter, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int i = 0; i < cards->count(); ++i)
            cards->item(i)->setHidden(!cards->item(i)->text().contains(text, Qt::CaseInsensitive));
    });
    auto *footer = new QHBoxLayout;
    auto *note = new QLabel(tr(
        "Seus projetos ficam no seu computador. Um arquivo .lmx leva o ambiente, os móveis e as texturas."));
    note->setWordWrap(true);
    note->setProperty("role", "muted");
    footer->addWidget(note, 1);
    footer->addWidget(recover);
    layout->addLayout(footer);
}
void ProjectHome::setProjects(const std::vector<RecentProject> &projects) {
    cards->clear();
    empty->setVisible(projects.empty());
    firstProject->setVisible(projects.empty());
    cards->setVisible(!projects.empty());
    for (const auto &project : projects) {
        auto date = QDateTime::fromString(project.modified, Qt::ISODate).toLocalTime();
        auto *item =
            new QListWidgetItem(project.name + "\n" + date.toString("dd/MM/yyyy · HH:mm") +
                                    (project.available ? "" : tr("\nArquivo movido ou indisponível")),
                                cards);
        QPixmap thumbnail(project.thumbnail);
        item->setIcon(thumbnail.isNull() ? QIcon(":/studio/brand/libremax-mark.png") : QIcon(thumbnail));
        item->setData(Qt::UserRole, project.path);
        item->setToolTip(project.path);
        item->setSizeHint({270, 230});
    }
}
void ProjectHome::showIndexWarning(const QString &message) {
    empty->setText(message);
    empty->show();
}
} // namespace lmx
