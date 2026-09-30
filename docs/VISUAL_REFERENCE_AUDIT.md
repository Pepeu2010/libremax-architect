# Referências visuais — ciclo 02

Pedido aplicado: usar `VDMAX_REFERENCE_PACK` como referência para uma versão mais moderna, com interface escura e melhorias no render. Os documentos do pack foram tratados como material de referência, não como autorização adicional nem especificação superior ao pedido do usuário.

O manifesto contém 47 referências públicas. Foram baixadas e inspecionadas visualmente **11 imagens** neste ciclo; as demais 36 foram apenas indexadas. Não foi realizada inspeção integral dos vídeos. O [índice local](reference-index.json) preserva URLs e IDs para rastreabilidade. Imagens de terceiros não são distribuídas no código, exemplos ou biblioteca; as capturas desta entrega são do LibreMax compilado.

| visual_refs inspecionadas | Observação na referência | Decisão independente no LibreMax | Limite atual |
|---|---|---|---|
| VIEW-01, SCENE-01 | Ambiente em perspectiva e visualização texturizada | Viewport escuro, enquadramento e área central alternável entre CAD e imagem | CAD permanece planta/isométrica ortográfica; perspectiva interativa pendente |
| ENV-06 | Propriedades contextuais no painel direito | Inspector escuro com foco legível, ação Aplicar e rolagem vertical | Gestos avançados de janelas não foram comprovados |
| LIB-01 | Categorias, miniatura e dimensões do item | Lista pesquisável com 25 miniaturas da geometria real, geração assíncrona e dimensões | Coleções e catálogo extenso pendentes |
| LIGHT-02, LIGHT-03 | Parâmetros de spot e cone de luz localizado | Cor, potência, ângulo e suavidade no inspector; parâmetros exportados ao Cycles | Luz spot específica ainda precisa de cena/QA visual dedicado |
| RENDER-06 | Ajustes de iluminação antes de gerar imagem | Câmera por UUID, exposição, luz ambiente, denoise, presets e estado real do processo | Fila/galeria persistente, sol/céu e HDRI pendentes |
| RENDER-08 | Cozinha com materiais e iluminação para apresentação | Cozinha original com frentes grafite, madeira/pedra procedurais, vidro e luzes de área | A imagem 960×540 não prova qualidade final 4K nem equivalência ao VDMax |
| VISUAL-01, VISUAL-02, VISUAL-03 | Reflexos, materiais e luzes em ambientes completos | Micro relevo/roughness procedural, bevel de shader e AgX no render | Não reproduzimos cenas, texturas, marcas ou imagens da referência; emissivos/TV e vidro avançado pendentes |

## Sistema visual implementado

Direção **Ateliê Graphite**: fundo `#10171e`, painéis `#182129`, texto `#e6edf2`, destaque `#87d9c1`. Tipografia utiliza a fonte instalada Inter/Noto Sans/Segoe UI/DejaVu Sans; não baixa fontes em runtime. Ícones vetoriais desenhados com QPainter e pequenos indicadores PNG próprios incorporados como recursos Qt. Componentes continuam Qt Widgets, viewport OpenCASCADE e visualizador QGraphicsView.

O projeto e os comandos de arquivo têm uma faixa própria; desenho, automações e vistas ficam na segunda. Biblioteca e árvore à esquerda; propriedades/render à direita. O render ocupa a área central, com Ajustar, 1:1, zoom, pan e Salvar cópia. Diálogos Qt seguem a paleta escura. Não há animações decorativas; a barra indeterminada acompanha somente um job ativo e não inventa porcentagem.

## Evidência

[Testes do ciclo](CYCLE_02.md), [interface](screenshots/native-ui.png), [900 px](screenshots/native-ui-900.png), [workspace de render](screenshots/render-workspace.png), [render em 900 px](screenshots/render-workspace-900.png) e [PNG Cycles real](screenshots/cycles-kitchen.png). O teste de 900 px exige ausência de rolagem horizontal no inspector e no Render e alcança suas ações com rolagem vertical. Todos os resultados se referem ao host Windows atual; validação Linux permanece pendente.
