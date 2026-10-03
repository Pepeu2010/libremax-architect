# Matriz de paridade — VDMax 3.0 Arquitetos e Decoradores

Pesquisa inicial: 2026-09-29. Implementação clean-room, sem binários ou assets VDMax. LibreMax: prévia 0.14.0 com instaladores Windows e Ubuntu publicados, incluindo Blender/Cycles. **Não há paridade completa nem release 1.0.**

Complemento visual em 2026-09-30: [auditoria](VISUAL_REFERENCE_AUDIT.md), 11 imagens inspecionadas de 47 referências indexadas. `visual_refs` indica somente essas imagens consultadas; não promove paridade funcional automaticamente.

Fontes oficiais consultadas:

- [P — produto Arquitetos e Decoradores](https://www.vdmax.com/vdmaxad): ambientes, aberturas, escadas, automações, módulos editáveis, decoração e ray tracing.
- [C — tabela comparativa](https://www.vdmax.com/tabela-comparativa-vdmax): módulos cozinha/dormitório, acabamentos, puxadores, vidro, DXF e JPG. Não inferir a disponibilidade de uma célula quando o conteúdo da marcação gráfica não é extraível; confirmar pelo produto/suporte.
- [V — vídeos oficiais](https://www.vdmax.com/video-tips): apresentação Arquitetos, ambientes, cozinha, DXF, geometria para tampos, rodatetos e sancas. Foram lidos títulos, descrições e identificados embeds; **não foi realizada inspeção temporal completa dos vídeos**. Nenhum gesto é declarado observado somente pelo título.
- [S — suporte oficial](https://vdmax.zendesk.com/hc/pt-br/articles/219121707-Conceitos-B%C3%A1sicos): desenho por polígono/retângulo/círculo, edição de pontos e arestas e furos internos.
- [B — página de produtos](https://www.vdmax.com/produtos): confirma biblioteca acima de 3.000 itens e distinção de públicos.
- SPEC: requisito solicitado pelo usuário, ainda sem comportamento público específico confirmado. Não é evidência de observação do VDMax.

`VERIFIED` exige teste reproduzível executado para **todo o comportamento da linha**; testes somente do kernel não comprovam uma feature inteira. `FUNCTIONAL` indica implementação integrada ainda com QA incompleto. `IN_PROGRESS` indica cobertura parcial. IDs de testes abaixo são nomes/filtros Catch2 ou smoke nativo, e não alegações de execução até o relatório registrar o resultado.

| ID | Área | Funcionalidade VDMax | Comportamento observado/documentado | Implementação LibreMax | Teste | Status | Fonte | Observações | visual_refs |
|---|---|---|---|---|---|---|---|---|---|
| A01 | Ambiente | Criação de ambientes | Desenhar ambiente com paredes | Cômodos retangulares adjacentes, piso/forro e paredes compartilhadas | Core + assembly-smoke | IN_PROGRESS | P,V | Falta detectar regiões arbitrárias e atualizar área/perímetro | — |
| A02 | Ambiente | Paredes | Desenho de paredes | Cadeia de cliques, sólidos, inspector, histórico e .lmx | `[geometry]` + UI smoke | IN_PROGRESS | P,V | Faltam junções L/T/X robustas, entrada comprimento/ângulo durante preview | — |
| A03 | Ambiente | Muretas | Altura reduzida | Mesma engine da parede, ferramenta dedicada | `[geometry]` | FUNCTIONAL | P | QA dedicado de UI pendente | — |
| A04 | Ambiente | Portas | Itens de ambientação | Recorte real, folha articulada, associação à parede | `[geometry]` | IN_PROGRESS | P | Faltam arco em planta e catálogo de modelos | — |
| A05 | Ambiente | Janelas | Itens de ambientação | Recorte real, vidro e caixilho associados | `[geometry]` | FUNCTIONAL | P | Drop pela UI testado; múltiplas plataformas pendentes | ENV-06 |
| A06 | Ambiente | Escadas | Biblioteca de escadas | Escada reta procedural | — | IN_PROGRESS | P | L e U ausentes | — |
| A07 | Ambiente | Pisos | Geometria pode criar pisos | Piso retangular com material por entidade | — | IN_PROGRESS | S,SPEC | Sem polígonos de ambientes ou UV | — |
| A08 | Ambiente | Forros | Geometria pode criar forros | Forro retangular ocultável | — | IN_PROGRESS | S,SPEC | Sem perfis/sancas | — |
| A09 | Ambiente | Grid/snap | Comportamento exigido pela spec | Grid e snap de endpoints | UI smoke | IN_PROGRESS | SPEC | Faltam outros snaps e grid adaptativo | — |
| A10 | Ambiente | Cotas/medir | Comportamento exigido pela spec | — | — | NOT_STARTED | SPEC | Cotas associativas ainda ausentes | — |
| G01 | Geometria | Geometria personalizada | Polígono, retângulo, círculo, pontos/arestas | Volume retangular | `[geometry]` | IN_PROGRESS | S,V | Polígonos, círculos, perfis e edição de arestas ausentes | — |
| G02 | Geometria | Booleanos | Furos por contorno interno | Kernel Cut/Fuse em aberturas e automações | `[geometry]` | IN_PROGRESS | S,SPEC | Não exposto como ferramenta geral | — |
| G03 | Geometria | Sancas | Tutorial oficial dedicado | — | — | NOT_STARTED | V | Falta perfil/trajeto | — |
| G04 | Geometria | Rodatetos | Tutorial oficial dedicado | — | — | NOT_STARTED | V | Falta varredura de perímetro | — |
| I01 | Importação | DXF | Tabela e tutorial dedicados | DXF ASCII, cinco tipos 2D, unidade/layers bloqueados persistentes | `[dxf]` | FUNCTIONAL | C,V | Sem malhas, bulge, conversão de linhas ou QA do diálogo | — |
| I02 | Importação | Modelos externos | Formatos exigidos pela spec | — | — | NOT_STARTED | SPEC | GLB/OBJ/STL/DAE/STEP/IGES | — |
| L01 | Biblioteca | Biblioteca de módulos | Módulos cozinha/dormitório | SQLite WAL/FTS5 + 25 receitas, duas aberturas, 52 Kenney, 53 KayKit, 31 Poly Haven e 12 LibreMax | `[library]` + UI/modern-smoke | IN_PROGRESS | P,C | 175 miniaturas reais; três modos de edição; atualização remota de packs e catálogo extenso faltam | LIB-01 |
| L02 | Biblioteca | Pesquisa local | Biblioteca categorizada | Busca sem acentos, filtros, favoritos, recentes | `[library]` | FUNCTIONAL | C,SPEC | Benchmark usa 10.000 fixtures, não 10.000 assets distribuídos | LIB-01 |
| L03 | Biblioteca | Arrastar e soltar | Workflow exigido pela spec | MIME, prévia real, parede/piso/cantos/vizinhos, colisão e posição externa recusadas | assembly-smoke: planta/3D, MIME/ghost/drop + undo | FUNCTIONAL | manual oficial | Arraste manual e múltiplas plataformas pendentes | LIB-01 |
| L04 | Biblioteca | Importar .lmaxpack | Formato LibreMax | — | — | NOT_STARTED | SPEC | Segurança antes de extração obrigatória | — |
| L05 | Biblioteca | Biblioteca decorativa extensa | Mais de 3.000 itens | 175 itens: 25 receitas próprias, duas aberturas e 148 modelos prontos | UI/modern-smoke + assembly-smoke | IN_PROGRESS | P,B | Coleções leves/detalhadas e 36 modelos em Apartamento atual; não atende meta de 3.000 | — |
| M01 | Módulos | Cozinha | Largura/altura/profundidade editáveis | Balcões, gaveteiro, aéreos, torre, nicho, ilha | `[modules]` + UI smoke | IN_PROGRESS | P,C | Cantos e famílias complexas ausentes | — |
| M02 | Módulos | Dormitório | Modulação própria da categoria | Roupeiros 2/3/4 portas, criado, cama | `[modules]` | IN_PROGRESS | P,C | Correr/canto/espelho frontal faltam | — |
| M03 | Módulos | Redimensionamento milimétrico | Editar dimensões sem trocar módulo | Recomposição de painéis/frentes/prateleiras/puxadores a 0,1 mm | `[modules]` + UI smoke | FUNCTIONAL | P | Sem scale destrutivo; limites por família precisam ampliar | — |
| M04 | Módulos | Acabamentos/modelos | Acabamentos e modelos variados | Materiais por objeto, corpo/frente separados | `[modules]` | IN_PROGRESS | C | Editor de modelos/frentes ainda limitado | — |
| M05 | Módulos | Puxadores | Diferentes modelos | Alça, perfil, ponto e nenhum | — | FUNCTIONAL | C | Cava/embutido ausentes | — |
| M06 | Módulos | Portas de vidro | Modelos com vidro | Moldura recortada e painel de vidro | `[modules]` | IN_PROGRESS | C | Fumê/canelado/bronze não implementados | — |
| T01 | Materiais | Materiais | Objetos recebem acabamentos | Presets PBR, mapas albedo/roughness/normal incorporados e materiais procedurais | `[render]` | IN_PROGRESS | P | Controle UV e propriedades emissivas ausentes; textura de cor incorporada implementada | RENDER-08,VISUAL-02 |
| T02 | Materiais | Texturas JPG | Tabela menciona importação JPG | Worker, normalização PNG, incorporação por SHA256, viewport e Cycles | `[materials][persistence]` | FUNCTIONAL | C | Roundtrip sem original testado; equivalência UV e UI QA pendentes | — |
| T03 | Materiais | Vidro e espelho | Vidro documentado; espelho da spec | Cycles PBR + transparência de preview | Smoke 1280×720/128 CPU + inspeção visual | IN_PROGRESS | C,SPEC | Viewport não oferece reflexo ray-traced | VISUAL-01,VISUAL-02 |
| U01 | Automação | Tampos | Inserção automática | União de sólidos sobre fontes com recálculo | `[automation]` | IN_PROGRESS | P | Testes retos; junções complexas/recortes de cuba faltam | — |
| U02 | Automação | Rodatampos | Inserção automática | Segmentos associados | — | FUNCTIONAL | P | QA de conjuntos diversos pendente | — |
| U03 | Automação | Rodapés | Inserção automática | Segmentos sob fontes | `[automation]` | IN_PROGRESS | P | Detectar sequências/compatibilidade por tipo falta | — |
| U04 | Automação | Rodaforros | Inserção automática | Segmentos superiores sobre fontes | — | IN_PROGRESS | P | Sem seleção automática de aéreos | — |
| U05 | Automação | Fechamentos | Fechamento de módulos | Painel lateral direito | — | IN_PROGRESS | P | Não detecta espaços/parede; outros lados faltam | — |
| U06 | Automação | Envelopamento | Envolver módulos | Laterais e topo por fonte | — | IN_PROGRESS | P | União de grupo e controles adicionais faltam | — |
| U07 | Automação | Associação | Requisito da spec | UUIDs de fontes, recálculo e exclusão em cascata | `[automation]` | FUNCTIONAL | SPEC | Snapshot/undo integrado; maior cobertura pendente | — |
| V01 | Editor | Seleção e hierarquia | Requisito da spec | Clique/Ctrl+clique, hover AIS e árvore | UI smoke | IN_PROGRESS | SPEC | Box selection/isolar/grupos ausentes | — |
| V02 | Editor | Mover/rotacionar | Workflow de edição | Inspector em cm, arrasto direto e giro centrado | assembly-smoke + UI smoke | FUNCTIONAL | SPEC | Gizmo, seleção em grupo e QA amplo faltam | — |
| V03 | Editor | Duplicação | Workflow de edição | Cópia com novos UUIDs e associação de filhos | — | FUNCTIONAL | SPEC | UI QA pendente | — |
| V04 | Editor | Espelhamento | Workflow de edição | Móveis e geometria independente | — | FUNCTIONAL | SPEC | Não é espelhamento completo de conjuntos | — |
| V05 | Editor | Alinhamento/distribuição | Requisito da spec | — | — | NOT_STARTED | SPEC | — | — |
| V06 | Editor | Visualização 3D | Apresentação de ambientes | AIS/V3d sobre B-rep, ortográfica superior/isométrica, orbit/pan/zoom | UI smoke | IN_PROGRESS | P | Dividida, perspectiva/walk e outras vistas faltam | VIEW-01,SCENE-01 |
| V07 | Editor | Undo/redo | Requisito da spec | QUndoStack + Command Pattern validado | `[commands]` | FUNCTIONAL | SPEC | UI smoke amplia cobertura | — |
| R01 | Luz | Iluminação | Render calcula iluminação | Ponto/spot/área/LED/sol persistentes; Kelvin, cor, medidas, raio, feixe e sombra; marcadores no editor | `[lighting]` + lighting-smoke Cycles CPU + LED HIP RX 7600 | IN_PROGRESS | P,SPEC | Sete imagens reais e comparação LED quente/frio/desligado; HDRI e céu integrados; QA amplo e material emissivo por mapa pendentes | LIGHT-02,LIGHT-03 |
| R02 | Câmeras | Câmeras | Requisito da spec | Posição/alvo/lente, abertura/foco e seleção por UUID; exposição/ambiente/denoise persistentes | `[render]` | IN_PROGRESS | SPEC | Seletor integrado e roundtrip testados; preset por cômodo implementado; posicionamento interativo completo falta | RENDER-06 |
| R03 | Render | Ray tracing | Ray-trace com qualidade variável | QProcess + snapshot + Cycles + denoise, exposição/ambiente, GPU/CPU | Smoke QProcess/Cycles CPU | IN_PROGRESS | P | Fila e galeria persistentes implementadas; HDRI/EXR integrados e testados no Windows; faltam mapas PBR adicionais e QA amplo de GPUs físicas | RENDER-06,RENDER-08 |
| R04 | Render | Exportar imagens | Resultado de apresentação | PNG/JPEG e EXR float32 com prévia integrada, galeria e exportação de cópia | Smoke 1280×720/128 CPU + pipeline 3840×2160/512 HIP + inspeção visual | FUNCTIONAL | P,SPEC | Cópia pela UI e saída 4K testadas; JPEG pela UI, seleção Final e QA amplo de apresentação pendentes | RENDER-08,VISUAL-03 |
| P01 | Projetos | Salvar/abrir | Requisito da spec | Container ZIP v1/v2/v3, HDRI e novas luzes, UUIDs, modelos e mapas incorporados, backup, validação e atomic replace; home com recentes | `[persistence][lighting]` + UI/modern/experience/lighting smoke | IN_PROGRESS | SPEC | Imagens de render não incorporadas; HDRI/EXR preservam v3; índice local não é backup | — |
| P02 | Projetos | Autosave | Requisito da spec | Intervalo configurável 1–60 min, 5 snapshots por UUID | `[recovery]` + encerramento forçado/reinício | FUNCTIONAL | SPEC | Timer configurável implementado; QA de configuração pela UI pendente | — |
| P03 | Projetos | Crash recovery | Requisito da spec | Seleção de versão, diálogo, descarte seguro, skip de corruptos | `[recovery]` + `--recovery-smoke` (processo morto e outro iniciado) | FUNCTIONAL | SPEC | Um projeto via UI comprovado; múltiplas versões via core; energia/interrupção durante escrita não testadas | — |
| P04 | Projetos | Backup da biblioteca | Requisito da spec | — | — | NOT_STARTED | SPEC | — | — |
| X01 | Produto | Offline completo | Exigência LibreMax | Edição, biblioteca, save/open/render locais | Rede desabilitada a executar | IN_PROGRESS | SPEC | Não declarar offline testado por ausência de código de rede | — |
| X02 | Produto | Pacotes Linux | Exigência LibreMax | CMake install/CPack e `.deb` publicado 0.14; Blender incluído, dependências do sistema declaradas | CI Linux 37144046955 + instalação Ubuntu novo 37144046796 | FUNCTIONAL | SPEC | Ubuntu 24.04 amd64, render automático, UI Mesa/Xvfb, modelos e desinstalação preservando projeto; Mint, ARM, AppImage e hardware físico adicional pendentes | — |

Fora de escopo: plano de corte, nesting, BOM industrial, ERP/MRP, CNC, custos de matéria-prima, etiquetas e produção. Nenhuma dessas funções é incluída no roadmap.

Fonte 0.13: [geometria compartilhada](SHARED_GEOMETRY.md) no editor e no pacote
Cycles, com seleção independente e compatibilidade com leitores arquivados.
Essa cobertura atende parte do requisito de instâncias da arquitetura de render;
não altera os gates abertos de paridade, mapeamento CAD, GPUs e hardware mínimo.

Pesquisa adicional de arrasto/encaixe, catálogo e comparação de limites: [VDMAX_RESEARCH_04](VDMAX_RESEARCH_04.md). Testes executados neste ciclo: [CYCLE_04](CYCLE_04.md).

Atualização 0.6: [CYCLE_06](CYCLE_06.md), modelos detalhados, tutorial e biblioteca inicial. Não altera os gates abertos de render e de paridade integral.

Atualização 0.8: [HDRI/EXR](HDRI_EXR.md), incluindo projeto v2, snapshot, Cycles CPU real, rotação, alpha, galeria e cópia EXR pela interface. Não altera os gates de paridade integral.

Atualização 0.9: [modelos e desempenho](MODEL_LIBRARIES_PERFORMANCE.md), com 60 modelos novos, 175 itens, 148 malhas e três modos de edição. Pacotes Windows/Ubuntu publicados após os gates de instalação.

Atualização 0.10: [iluminação](LIGHTING.md), com cinco tipos, Kelvin, emissor LED real, editor em centímetros e projeto v3. CI e instaladores Windows/Ubuntu passaram; uma saída 4K/512 com LED foi calculada por HIP na RX 7600 deste Windows. Não altera os gates de paridade integral.
