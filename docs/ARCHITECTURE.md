# Arquitetura implementada

`apps/architect` compõe Qt Widgets e controla os fluxos da aplicação. `libs/document` guarda entidades com UUID, transformações absolutas em mm, dimensões, materiais, parâmetros e metadados. Filhos são derivados das referências de pai na serialização; aberturas usam a parede como sistema local. Não existem dois documentos para planta e 3D.

`libs/commands/Editor` valida a próxima versão antes de criar um QUndoCommand. Cada comando restaura snapshots completos; limite inicial de 200. Não há economia de memória incremental neste ciclo.

`libs/geometry` cria sólidos B-rep, recortes e uniões OpenCASCADE e recompõe módulos em partes visuais. Esses componentes não são peças para fabricação. `libs/viewport` exibe a mesma geometria com AIS/V3d, faz seleção e encaminha cliques/drop. Render usa tesselação desses sólidos.

`libs/persistence` produz ZIP com libzip, reabre o temporário para validar e grava via QSaveFile com fsync/commit. `RecoveryStore` usa o mesmo writer atômico, retém cinco versões por UUID, examina no máximo 100 arquivos, mantém somente metadados na listagem e ignora/preserva snapshots corrompidos. `libs/library` usa SQLite local, WAL, chaves estrangeiras e FTS5. `libs/import` interpreta referência DXF ASCII limitada. `libs/materials` normaliza imagens importadas para PNG e incorpora bytes por SHA256.

`libs/rendering/RenderJob` prepara snapshot em QtConcurrent e executa Blender com QProcess, programa/argumentos separados. `scripts/cycles_render.py` é código fixo da aplicação; não gera Python com strings do usuário.

Importação de imagens/DXF e preparação/render ocorrem em workers/processo separado. A recomposição de viewport ainda é síncrona e reconstrói a cena. Dirty flags, cache incremental, instancing/LOD e benchmark da cena de 100/500/5.000 objetos são trabalho pendente. O core ainda depende de Qt Gui/Widgets para QUndoStack; isolamento puro de Qt não é objetivo atual.

ADRs: [engine/interface](ADRs/0001-native-cad.md), [persistência/licenças](ADRs/0002-persistence-and-licenses.md).

O tema nativo e os ícones ficam em `studio_theme`/`resources/style.qss`. `AssetThumbnails` tessela receitas em QThreadPool limitado e publica QImages; QPixmap/delegate são usados somente na UI. `RenderPreview` reutiliza uma cena QGraphicsView para a imagem completa, sem criar uma nova dock a cada render. `Document::renderSettings` é validado, persistido e exportado no mesmo snapshot.

`PbrMaterialPack` carrega/verifica/normaliza os mapas em worker e os combina ao documento atual por comando, protegendo a identidade do projeto entre início e conclusão. O snapshot exporta mapas, parâmetros de céu/câmera e tipo de entidade para o renderer fixo. A malha de apresentação tem bevel/UV/normais; o B-rep original permanece no projeto.

## Montagem e malhas — 0.4

`libs/placement` calcula colocação em coordenadas do documento: cursor centrado, parede em sistema local, orientação, cantos/vizinhos, contenção no cômodo, sobreposição de retângulos orientados e alturas. `libs/viewport` projeta o cursor sobre a face da parede no 3D e mostra a geometria final verde/vermelha antes de confirmar. Mover um objeto existente mantém o ponto de pega e confirma um único comando. A engine não calcula colisão exata das malhas.

`libs/library/model` valida o JSON normalizado. `MeshObject` referencia bytes incorporados por SHA256. `libs/geometry` cria faces com Poly_Triangulation por acabamento e aplica dimensões/transformação; o snapshot usa diretamente essa triangulação no Cycles. São malhas de apresentação, não sólidos editáveis B-rep. OBJ é convertido somente na preparação do pacote; o runtime lê o formato limitado, sem executar arquivos do asset.
