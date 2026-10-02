#include "first_run.h"
#include <QCheckBox>
#include <QLabel>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace lmx {
bool motionEnabled() {
    if (!QSettings().value("accessibility/animations", true).toBool())
        return false;
#ifdef Q_OS_WIN
    BOOL enabled = TRUE;
    if (SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &enabled, 0) && !enabled)
        return false;
#endif
    return true;
}
void showOpening(QWidget *parent) {
    auto *opening = new QWidget(parent, Qt::SplashScreen | Qt::FramelessWindowHint);
    opening->setAttribute(Qt::WA_DeleteOnClose);
    opening->setObjectName("openingLogo");
    opening->setFixedSize(430, 360);
    auto *layout = new QVBoxLayout(opening);
    layout->setContentsMargins(34, 24, 34, 24);
    auto *mark = new QLabel;
    mark->setAlignment(Qt::AlignCenter);
    mark->setPixmap(QPixmap(":/studio/brand/libremax-mark.png")
                        .scaled(210, 210, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(mark);
    auto *name = new QLabel("LibreMax Architect");
    name->setAlignment(Qt::AlignCenter);
    name->setProperty("role", "heading");
    layout->addWidget(name);
    auto *caption = new QLabel(QObject::tr("Seu espaço, do seu jeito."));
    caption->setAlignment(Qt::AlignCenter);
    caption->setProperty("role", "muted");
    layout->addWidget(caption);
    opening->show();
    if (motionEnabled()) {
        opening->setWindowOpacity(0);
        auto *animation = new QPropertyAnimation(opening, "windowOpacity", opening);
        animation->setDuration(700);
        animation->setKeyValueAt(0, 0.0);
        animation->setKeyValueAt(0.25, 1.0);
        animation->setKeyValueAt(0.7, 1.0);
        animation->setKeyValueAt(1, 0.0);
        QObject::connect(animation, &QPropertyAnimation::finished, opening, &QWidget::close);
        animation->start();
    } else {
        QTimer::singleShot(0, opening, &QWidget::close);
    }
}
void showTutorial(QWidget *parent) {
    if (auto *existing = parent->findChild<QWizard *>("firstRunTutorial")) {
        existing->show();
        existing->raise();
        return;
    }
    auto *wizard = new QWizard(parent);
    wizard->setAttribute(Qt::WA_DeleteOnClose);
    wizard->setObjectName("firstRunTutorial");
    wizard->setWindowTitle(QObject::tr("Aprenda a montar seu apartamento"));
    wizard->setWizardStyle(QWizard::ModernStyle);
    wizard->setButtonText(QWizard::BackButton, QObject::tr("Voltar"));
    wizard->setButtonText(QWizard::NextButton, QObject::tr("Próximo"));
    wizard->setButtonText(QWizard::FinishButton, QObject::tr("Começar a criar"));
    wizard->setButtonText(QWizard::CancelButton, QObject::tr("Explorar por conta própria"));
    wizard->resize(820, 620);
    struct Lesson {
        const char *title, *lead, *body;
    };
    const std::vector<Lesson> lessons{
        {"Bem-vindo ao LibreMax", "Você não precisa conhecer programas de arquitetura.",
         "Este guia acompanha o caminho completo: criar o espaço, colocar os móveis, escolher acabamentos e "
         "fazer uma foto.\n\nVocê pode voltar uma etapa, fechar o guia e abri-lo novamente em Ajuda → "
         "Tutorial completo. Seus projetos ficam no seu computador."},
        {"1. Seus projetos", "A tela inicial é sua biblioteca.",
         "Novo projeto começa um ambiente vazio. Abrir arquivo .lmx encontra um projeto já salvo. Dois "
         "cliques ou Enter em um cartão retomam esse projeto.\n\nExperimentar apartamento abre um exemplo "
         "com cômodos, móveis, luz e câmera para você conhecer o programa."},
        {"2. Crie um cômodo", "Comece pelas medidas do espaço real.",
         "Em Ambiente → Adicionar cômodo, escolha largura e comprimento em metros. Um cômodo de 4 por 3 "
         "metros recebe piso, paredes e teto.\n\nVocê pode adicionar outro cômodo ao lado. O programa "
         "reaproveita paredes que coincidem. Use nomes simples, como Sala, Quarto e Banheiro."},
        {"3. Veja o espaço", "Planta mostra de cima; 3D mostra o volume.",
         "Use Planta para montar os móveis e 3D para conferir o ambiente. Ver por dentro esconde o teto e "
         "parte das paredes só na tela, sem alterar a foto final.\n\nA roda aproxima e afasta. O botão do "
         "meio arrasta a vista. O botão direito gira a vista 3D. Ver tudo enquadra o projeto. A lista Cômodo "
         "aproxima a vista do ambiente escolhido."},
        {"4. Desenhe paredes", "Para formatos próprios, desenhe uma parede por vez.",
         "Escolha Parede, clique no começo e no fim. Continue clicando para formar uma sequência. Esc "
         "termina o desenho. Mureta cria uma parede mais baixa.\n\nA grade ajuda a alinhar. O tamanho da "
         "grade e Alinhar desenho ficam no rodapé. O assistente de cômodos é o caminho mais fácil para um "
         "espaço retangular."},
        {"5. Encontre um móvel", "A biblioteca fica à esquerda.",
         "Busque por um nome comum, como mesa, cama ou poltrona. Apartamento atual mostra as novas peças "
         "contemporâneas. As medidas abaixo de cada item estão em centímetros.\n\nFavoritar guarda os itens "
         "que você mais usa. Recentes mostra os itens colocados há pouco. As miniaturas representam a forma "
         "real do modelo; detalhes e texturas aparecem na vista 3D e na foto."},
        {"6. Coloque e mova", "Arraste o móvel até o lugar desejado.",
         "Perto de uma parede, o móvel se orienta e encosta nela. A prévia verde permite colocar; a vermelha "
         "explica por que não cabe. Solte para confirmar.\n\nDois cliques no catálogo ou Colocar deixam você "
         "escolher o ponto com um clique. R gira a prévia e Esc cancela. Arraste um móvel já colocado para "
         "mudar de lugar. Desfazer recupera a posição anterior."},
        {"7. Ajuste medidas e acabamento", "Clique no móvel e use Ajustar este item.",
         "Mude o nome, largura, altura e profundidade em centímetros. Você pode escrever 80,5 ou 80+10. "
         "Aplicar alterações confirma.\n\nAcabamento original mantém os materiais do modelo. Escolher um "
         "acabamento permite usar uma cor única. Ajustes avançados revela posição e câmera. Alterar a escala "
         "de um modelo pronto não redesenha seus detalhes internos."},
        {"8. Portas, janelas e teto", "Algumas peças têm um lugar próprio.",
         "Arraste portas e janelas para uma parede; o programa faz o recorte e mantém a ligação. Ajuste a "
         "largura e a distância do piso nas propriedades.\n\nLuminárias de teto acompanham a altura do "
         "cômodo. Vasos e objetos de mesa podem ficar sobre outro móvel. A luminária decorativa e a fonte de "
         "luz são itens separados."},
        {"9. Organize e finalize", "O que está no projeto lista todos os objetos.",
         "Clique na aba da lista para selecionar, ocultar ou bloquear objetos. Ctrl+clique seleciona vários. "
         "Ctrl+D duplica; Delete remove; Ctrl+Z desfaz.\n\nFinalizar móveis cria tampos, rodapés e outras "
         "peças sobre módulos compatíveis. Essas peças acompanham os móveis de origem. Salve antes de "
         "experimentar mudanças maiores; Desfazer continua disponível."},
        {"10. Materiais e luz", "A luz e o acabamento mudam o resultado da foto.",
         "Ativar materiais realistas incorpora mapas de madeira e pedra. Importar textura permite usar uma "
         "imagem sua, que fica dentro do arquivo do projeto.\n\nIluminação oferece luz de área, ponto e "
         "spot. Área produz uma luz ampla; ponto ilumina em várias direções; spot cria um feixe. Céu natural "
         "controla sol e luz do dia. Comece com a iluminação do exemplo e ajuste a exposição aos poucos."},
        {"11. Prepare uma câmera", "A câmera escolhe o que aparece na foto.",
         "Selecione o cômodo e use Preparar câmera do cômodo em 4 Foto. O programa cria uma câmera com um "
         "enquadramento inicial.\n\nEscolha a câmera no painel Criar imagem. Nos ajustes avançados, Lente "
         "altera o campo de visão e Foco escolhe a distância nítida. Se parte do ambiente desaparecer, "
         "confira a posição e o alvo da câmera antes de renderizar."},
        {"12. Gere sua imagem", "O trabalho pesado acontece em segundo plano.",
         "Escolha a qualidade, o tamanho da imagem e a câmera. Rápido ajuda a conferir; Normal equilibra "
         "tempo e qualidade; Final usa mais amostras para apresentação. Personalizado mostra controles "
         "adicionais.\n\nClique em criar a imagem e acompanhe o estado. Você pode continuar editando ou "
         "cancelar. O render usa uma cópia do projeto no momento do pedido. A imagem abre dentro do "
         "LibreMax; salve uma cópia em PNG ou JPEG. O motor Cycles precisa estar instalado, mas você não "
         "abre o Blender manualmente."},
        {"13. Salve e retome", "Um único arquivo .lmx leva o seu projeto.",
         "Ctrl+S salva. Na primeira vez, escolha o nome e a pasta. Salvar como cria outro arquivo. Ambiente, "
         "móveis e texturas ficam incorporados: não mova dezenas de arquivos separados.\n\nO projeto salvo "
         "aparece na tela inicial. O programa também mantém salvamentos de recuperação. Se ocorrer uma "
         "interrupção, use Recuperar alterações. O salvamento de recuperação ajuda, mas não substitui "
         "guardar o arquivo .lmx."},
        {"14. Você pode começar", "Faça um primeiro cômodo antes de tentar o apartamento inteiro.",
         "Uma sequência simples: cômodo → porta e janela → móveis → acabamento → luz → câmera → foto → "
         "salvar.\n\nSe algo não couber, confira o aviso da prévia e as medidas. Se quiser recomeçar uma "
         "etapa, use Desfazer. O tutorial continua disponível no menu Ajuda. Recursos avançados ainda em "
         "desenvolvimento são documentados; o programa não simula etapas que não executa."}};
    for (std::size_t i = 0; i < lessons.size(); ++i) {
        const auto &lesson = lessons[i];
        auto *page = new QWizardPage;
        page->setTitle(QString::fromUtf8(lesson.title));
        page->setSubTitle(QObject::tr("Etapa %1 de %2 · %3")
                              .arg(i + 1)
                              .arg(lessons.size())
                              .arg(QString::fromUtf8(lesson.lead)));
        auto *layout = new QVBoxLayout(page);
        auto *scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        auto *body = new QLabel(QString::fromUtf8(lesson.body));
        body->setWordWrap(true);
        body->setTextFormat(Qt::PlainText);
        body->setTextInteractionFlags(Qt::TextSelectableByMouse);
        body->setAlignment(Qt::AlignTop);
        body->setMargin(16);
        body->setStyleSheet("font-size:16px;");
        scroll->setWidget(body);
        layout->addWidget(scroll);
        if (i == 0) {
            auto *reduce = new QCheckBox(QObject::tr("Reduzir animações"));
            reduce->setChecked(!motionEnabled());
            QObject::connect(reduce, &QCheckBox::toggled, wizard, [](bool enabled) {
                QSettings().setValue("accessibility/animations", !enabled);
            });
            layout->addWidget(reduce);
        }
        wizard->addPage(page);
    }
    QObject::connect(wizard, &QWizard::accepted, parent, [] {
        QSettings().setValue("onboarding/completed", true);
        QSettings().setValue("onboarding/seen", true);
    });
    QObject::connect(wizard, &QWizard::rejected, parent,
                     [] { QSettings().setValue("onboarding/seen", true); });
    wizard->show();
}
} // namespace lmx
