#include "first_run.h"
#include <QCheckBox>
#include <QComboBox>
#include <QGuiApplication>
#include <QLabel>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>
#include <QWizard>
#include <QWizardPage>
#include <vector>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
namespace lmx {
namespace {
class TutorialDiagram final : public QWidget {
    bool group_;

  public:
    TutorialDiagram(bool group, QWidget *parent) : QWidget(parent), group_(group) {
        setFixedHeight(150);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setAccessibleName(
            group ? QObject::tr("Conjunto antes e depois de girar: os móveis mantêm a distância.")
                  : QObject::tr("Cômodo em L: seis cantos delimitam o piso e o recuo fica vazio."));
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const auto scale = std::min(width() / 560.0, height() / 150.0);
        p.translate((width() - 560 * scale) / 2, (height() - 150 * scale) / 2);
        p.scale(scale, scale);
        p.setPen(QPen(QColor("#c89250"), 3));
        p.setBrush(QColor("#333946"));
        if (!group_) {
            p.drawPolygon(QPolygonF{{80, 16}, {280, 16}, {280, 66}, {160, 66}, {160, 120}, {80, 120}});
            p.setBrush(QColor("#c89250"));
            for (auto point : {QPointF{80, 16}, {280, 16}, {280, 66}, {160, 66}, {160, 120}, {80, 120}})
                p.drawEllipse(point, 4, 4);
            p.setPen(QColor("#eceff4"));
            p.drawText(QRectF(305, 25, 180, 80), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                       QObject::tr("Clique em cada canto.\nBotão direito fecha o cômodo."));
        } else {
            for (int side = 0; side < 2; ++side) {
                p.save();
                p.translate(side == 0 ? 145 : 410, 65);
                if (side)
                    p.rotate(-90);
                p.setPen(QPen(QColor("#c89250"), 2));
                p.setBrush(QColor("#333946"));
                p.drawRoundedRect(QRectF(-50, -30, 100, 60), 5, 5);
                p.drawRoundedRect(QRectF(-15, -63, 30, 22), 4, 4);
                p.drawRoundedRect(QRectF(-15, 41, 30, 22), 4, 4);
                p.restore();
            }
            p.setPen(QColor("#eceff4"));
            p.drawText(QRectF(50, 125, 190, 22), Qt::AlignCenter, QObject::tr("Antes"));
            p.drawText(QRectF(315, 125, 190, 22), Qt::AlignCenter, QObject::tr("Depois de girar"));
            p.drawText(QRectF(247, 45, 65, 40), Qt::AlignCenter, "90°");
        }
    }
};
} // namespace
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
    wizard->setButtonText(QWizard::CancelButton, QObject::tr("Explorar agora"));
    if (auto *screen = parent->screen()) {
        const auto available = screen->availableGeometry();
        wizard->resize(std::min(820, available.width() - 32), std::min(620, available.height() - 48));
    }
    struct Lesson {
        const char *title, *lead, *body;
    };
    const std::vector<Lesson> lessons{
        {"Bem-vindo ao LibreMax", "Você não precisa conhecer programas de arquitetura.",
         "Este guia acompanha o caminho completo: criar o espaço, colocar os móveis, escolher acabamentos e "
         "fazer uma foto.\n\nVocê pode voltar uma etapa, fechar o guia e abri-lo novamente em Ajuda → "
         "Tutorial completo. Seus projetos ficam no seu computador."},
        {"Seus projetos", "A tela inicial é sua biblioteca.",
         "Novo projeto começa um ambiente vazio. Abrir arquivo .lmx encontra um projeto já salvo. Dois "
         "cliques ou Enter em um cartão retomam esse projeto.\n\nExperimentar apartamento abre um exemplo "
         "com cômodos, móveis, luz e câmera para você conhecer o programa."},
        {"Crie um cômodo", "Comece pelas medidas do espaço real.",
         "Em Ambiente → Adicionar cômodo, escolha Retangular ou Em L e as medidas em metros. Um cômodo de 4 "
         "por 3 "
         "metros recebe piso, paredes e teto.\n\nVocê pode adicionar outro cômodo ao lado. O programa "
         "reaproveita paredes que coincidem. Use nomes simples, como Sala, Quarto e Banheiro."},
        {"Desenhe um cômodo em L", "O piso acompanha o formato real do seu apartamento.",
         "Em Ambiente → Desenhar contorno do cômodo, clique em cada canto na planta. O botão direito fecha "
         "o desenho; Esc cancela. Paredes, piso e teto acompanham esse contorno. Um recuo fica vazio, sem "
         "virar um piso retangular escondido.\n\nEscolha o cômodo na lista acima da planta e use Ambiente → "
         "Ajustar cantos do cômodo para corrigir as medidas em metros. Cantos que cruzam paredes são "
         "recusados. "
         "Paredes compartilhadas entre cômodos ainda não podem ser alteradas por esse "
         "ajuste.\n\nExperimente: "
         "crie uma sala em L e confira o recuo usando Planta."},
        {"Veja o espaço", "Planta mostra de cima; 3D mostra o volume.",
         "Use Planta para montar os móveis e 3D para conferir o ambiente. Ver por dentro esconde o teto e "
         "parte das paredes só na tela, sem alterar a foto final.\n\nA roda aproxima e afasta. O botão do "
         "meio arrasta a vista. O botão direito gira a vista 3D. Ver tudo enquadra o projeto. A lista Cômodo "
         "aproxima a vista do ambiente escolhido."},
        {"Deixe o programa mais leve", "A tela de edição e a foto têm qualidades separadas.",
         "Em Vista → Desempenho durante edição, escolha Leve para computadores mais lentos. Modelos que "
         "possuem uma versão simplificada usam menos detalhes enquanto você monta o apartamento. A foto "
         "continua usando o modelo completo.\n\nSe ainda ficar lento, trabalhe um cômodo por vez e oculte "
         "objetos que não está ajustando. Para conferir uma foto, comece em Rápido e com uma imagem pequena. "
         "Uma imagem Final demora mais, principalmente em computadores com vídeo integrado.\n\nExperimente: "
         "troque entre Leve e Equilibrado; compare a edição sem mudar seu projeto."},
        {"Desenhe paredes", "Para formatos próprios, desenhe uma parede por vez.",
         "Escolha Parede, clique no começo e no fim. Continue clicando para formar uma sequência. Esc "
         "termina o desenho. Mureta cria uma parede mais baixa.\n\nA grade ajuda a alinhar. O tamanho da "
         "grade e Alinhar desenho ficam no rodapé. O assistente de cômodos é o caminho mais fácil para um "
         "espaço retangular."},
        {"Encontre um móvel", "A biblioteca fica à esquerda.",
         "Busque por um nome comum, como mesa, cama ou poltrona. Apartamento atual mostra as novas peças "
         "contemporâneas. Modelos leves reúne móveis estilizados com menos detalhes geométricos. Objetos "
         "detalhados reúne peças com texturas de materiais reais. As medidas abaixo de cada item "
         "estão em centímetros.\n\nFavoritar guarda os itens "
         "que você mais usa. Recentes mostra os itens colocados há pouco. As miniaturas representam a forma "
         "real do modelo; detalhes e texturas aparecem na vista 3D e na foto."},
        {"Traga modelos de fora", "Amplie sua biblioteca pelo botão Adicionar modelos…",
         "Modelo baixado aceita GLB, glTF, OBJ, FBX, STL e PLY. Escolha o arquivo; o LibreMax prepara o "
         "modelo "
         "em segundo plano. Depois confira o nome e as medidas em centímetros antes de colocar. O modelo "
         "fica na sua biblioteca e acompanha o projeto salvo. Alguns arquivos usam texturas em uma pasta "
         "ao lado: mantenha esses arquivos juntos até importar.\n\nColeção de modelos instala um arquivo "
         ".lmaxpack com vários itens de uma vez. A coleção Apartamento contemporâneo acompanha os arquivos "
         "do LibreMax. Você pode importá-la sem internet. Respeite a licença dos modelos baixados; importar "
         "não concede direitos de uso.\n\nExperimente: importe um modelo e confira se o tamanho combina "
         "com as medidas do cômodo."},
        {"Coloque e mova", "Arraste o móvel até o lugar desejado.",
         "Perto de uma parede, o móvel se orienta e encosta nela. A prévia verde permite colocar; a vermelha "
         "explica por que não cabe. Solte para confirmar.\n\nDois cliques no catálogo ou Colocar deixam você "
         "escolher o ponto com um clique. R gira a prévia e Esc cancela. Arraste um móvel já colocado para "
         "mudar de lugar. Desfazer recupera a posição anterior."},
        {"Ajuste medidas e acabamento", "Clique no móvel e use Ajustar este item.",
         "Mude o nome, largura, altura e profundidade em centímetros. Você pode escrever 80,5 ou 80+10. "
         "Aplicar alterações confirma.\n\nAcabamento original mantém os materiais do modelo. Escolher um "
         "acabamento permite usar uma cor única. Ajustes avançados revela posição e câmera. Alterar a escala "
         "de um modelo pronto não redesenha seus detalhes internos."},
        {"Portas, janelas e teto", "Algumas peças têm um lugar próprio.",
         "Arraste portas e janelas para uma parede; o programa faz o recorte e mantém a ligação. Ajuste a "
         "largura e a distância do piso nas propriedades.\n\nLuminárias de teto acompanham a altura do "
         "cômodo. Vasos e objetos de mesa podem ficar sobre outro móvel. A luminária decorativa e a fonte de "
         "luz são itens separados."},
        {"Junte e organize móveis", "Mesa e cadeiras podem mudar de lugar juntas.",
         "Selecione os móveis com Ctrl+clique e use Editar → Juntar em conjunto ou Ctrl+G. Arraste o "
         "conjunto para mover todos juntos. Girar móvel, ou Ctrl+R, gira o conjunto 90 graus; Editar → "
         "Espelhar troca os lados mantendo as distâncias. Se não couber, o programa mantém a posição "
         "anterior.\n\nMover seleção com medidas usa centímetros. Alinhar móveis organiza pela esquerda, "
         "direita, frente ou fundo; Espaçar distribui o espaço entre os móveis. Essas opções alteram a "
         "arrumação dentro do conjunto. Ctrl+Shift+G separa o conjunto. Um móvel bloqueado impede mudanças "
         "no conjunto; desbloqueie primeiro.\n\nExperimente: junte uma mesa e duas cadeiras, gire e use "
         "Ctrl+Z para voltar."},
        {"Organize e finalize", "O que está no projeto lista todos os objetos.",
         "Clique na aba da lista para selecionar, ocultar ou bloquear objetos. Ctrl+clique seleciona vários. "
         "Ctrl+D duplica; Delete remove; Ctrl+Z desfaz.\n\nFinalizar móveis cria tampos, rodapés e outras "
         "peças sobre módulos compatíveis. Essas peças acompanham os móveis de origem. Salve antes de "
         "experimentar mudanças maiores; Desfazer continua disponível."},
        {"Materiais e luz", "A luz e o acabamento mudam o resultado da foto.",
         "Ativar materiais realistas incorpora mapas de madeira e pedra. Importar textura permite usar uma "
         "imagem sua, que fica dentro do arquivo do projeto.\n\nEm Iluminação → Nova luz, escolha spot, "
         "fita LED, painel, ponto ou sol. LED é uma faixa contínua que ilumina de verdade; painel espalha "
         "a luz; ponto ilumina em várias direções; spot cria um feixe. Escolha Quente, Neutra ou Fria, "
         "ou uma temperatura própria. Ajuste o brilho e as medidas em centímetros. O marcador no editor "
         "ajuda a posicionar; a prévia mostra a iluminação calculada. Céu natural "
         "controla sol e luz do dia. Use Usar luz do dia para adicionar uma iluminação pronta. Importar luz "
         "aceita panoramas HDR/EXR, que acompanham o projeto salvo. Girar a luz muda a direção; Mostrar na "
         "imagem controla o fundo sem apagar a iluminação. Comece com o exemplo e ajuste a exposição aos "
         "poucos."},
        {"Escolha um estilo de foto", "Comece com uma iluminação pronta para o cômodo.",
         "Escolha o cômodo na lista acima da planta. Em 4 Foto, Estilo do cômodo oferece Natural, Claro "
         "e Aconchegante. Escolher um estilo prepara luz e acabamentos; as luzes que você colocou continuam "
         "no projeto. O teto aparece na foto mesmo quando Ver por dentro o esconde na edição.\n\nVocê pode "
         "trocar o estilo e usar Desfazer para voltar. Confira primeiro uma imagem em Rápido: os estilos "
         "são pontos de partida, e a posição das janelas e da câmera também influencia o resultado.\n\n"
         "Experimente: faça uma foto Natural e outra Aconchegante do mesmo cômodo."},
        {"Prepare uma câmera", "A câmera escolhe o que aparece na foto.",
         "Selecione o cômodo e use Preparar câmera do cômodo em 4 Foto. O programa cria uma câmera com um "
         "enquadramento inicial.\n\nEscolha a câmera no painel Criar imagem. Nos ajustes avançados, Lente "
         "altera o campo de visão e Foco escolhe a distância nítida. Se parte do ambiente desaparecer, "
         "confira a posição e o alvo da câmera antes de renderizar."},
        {"Gere sua imagem", "O trabalho pesado acontece em segundo plano.",
         "Escolha a qualidade, o tamanho da imagem e a câmera. Rápido ajuda a conferir; Normal equilibra "
         "tempo e qualidade; Final usa mais amostras para apresentação. Personalizado mostra controles "
         "adicionais.\n\nClique em criar a imagem e acompanhe a barra de progresso e o tempo estimado. A "
         "estimativa se ajusta conforme o cálculo avança; preparar a cena também leva tempo. Você pode "
         "continuar editando ou "
         "cancelar. O render usa uma cópia do projeto no momento do pedido. A imagem abre dentro do "
         "LibreMax em Suas imagens. Crie imagens de todas as câmeras para enviar uma sequência à fila. "
         "Cada imagem guarda a cena usada; depois você pode repetir, tentar com o processador ou salvar uma "
         "cópia "
         "em PNG ou JPEG. A galeria permanece ao fechar o programa. O motor Cycles já acompanha o "
         "instalador do LibreMax e funciona em segundo plano."},
        {"Encontre e guarde suas imagens", "Suas imagens mantém os resultados e o histórico de cálculo.",
         "Abra a galeria Suas imagens e escolha uma imagem. Você pode abrir dentro do programa, guardar "
         "uma cópia ou abrir a pasta do arquivo. PNG preserva a qualidade; JPEG gera um arquivo menor para "
         "compartilhar. EXR preserva mais informação de luz quando esse formato foi escolhido ao criar "
         "a imagem.\n\nRenderizar novamente usa a cópia da cena que gerou aquela imagem, mesmo depois de "
         "você mudar o apartamento. Para fotografar as mudanças atuais, volte a Criar imagem. Cancelar "
         "para o cálculo sem fechar o projeto. Se houver erro, Ver detalhes do processo ajuda a descobrir "
         "o motivo; tentar com o processador pode resolver uma falha da placa de vídeo."},
        {"Salve e retome", "Um único arquivo .lmx leva o seu projeto.",
         "Ctrl+S salva. Na primeira vez, escolha o nome e a pasta. Salvar como cria outro arquivo. Ambiente, "
         "móveis e texturas ficam incorporados: não mova dezenas de arquivos separados.\n\nO projeto salvo "
         "aparece na tela inicial. O programa também mantém salvamentos de recuperação. Se ocorrer uma "
         "interrupção, use Recuperar alterações. O salvamento de recuperação ajuda, mas não substitui "
         "guardar o arquivo .lmx."},
        {"Você pode começar", "Faça um primeiro cômodo antes de tentar o apartamento inteiro.",
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
        auto *topics = new QComboBox;
        topics->setObjectName("tutorialTopics");
        topics->setAccessibleName(QObject::tr("Ir para um assunto do tutorial"));
        topics->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        for (std::size_t topic = 0; topic < lessons.size(); ++topic)
            topics->addItem(
                QObject::tr("%1 · %2").arg(topic + 1).arg(QString::fromUtf8(lessons[topic].title)));
        topics->setCurrentIndex(static_cast<int>(i));
        QObject::connect(topics, &QComboBox::activated, wizard, [wizard](int index) {
            // Finish the combo box's keyboard/popup event before changing the visible page.
            QTimer::singleShot(0, wizard, [wizard, index] {
                wizard->restart();
                for (int page = 0; page < index; ++page)
                    wizard->next();
            });
        });
        layout->addWidget(topics);
        auto *scroll = new QScrollArea;
        scroll->setObjectName("tutorialLessonScroll");
        scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto *content = new QWidget;
        auto *contentLayout = new QVBoxLayout(content);
        contentLayout->setContentsMargins(12, 8, 12, 8);
        if (i == 3 || i == 12)
            contentLayout->addWidget(new TutorialDiagram(i == 12, content));
        auto *body = new QLabel(QString::fromUtf8(lesson.body));
        body->setObjectName("tutorialLessonBody");
        body->setWordWrap(true);
        body->setTextFormat(Qt::PlainText);
        body->setTextInteractionFlags(Qt::TextSelectableByMouse);
        body->setAlignment(Qt::AlignTop);
        body->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        body->setMargin(4);
        body->setStyleSheet("font-size:16px;");
        contentLayout->addWidget(body);
        contentLayout->addStretch();
        scroll->setWidget(content);
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
    QObject::connect(wizard, &QWizard::currentIdChanged, wizard, [wizard](int id) {
        if (auto *page = wizard->currentPage())
            if (auto *topics = page->findChild<QComboBox *>("tutorialTopics"))
                topics->setCurrentIndex(id);
    });
    QObject::connect(wizard, &QWizard::accepted, parent, [] {
        QSettings().setValue("onboarding/completed", true);
        QSettings().setValue("onboarding/seen", true);
    });
    QObject::connect(wizard, &QWizard::rejected, parent,
                     [] { QSettings().setValue("onboarding/seen", true); });
    wizard->show();
}
} // namespace lmx
