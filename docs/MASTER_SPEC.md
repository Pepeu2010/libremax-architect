# LIBREMAX ARCHITECT — MASTER SPEC

Desenvolva do início ao fim o **LibreMax Architect**, um software desktop open source para Linux destinado a:

- arquitetura de interiores;
- decoração;
- projetos residenciais e comerciais;
- móveis planejados para apresentação;
- criação visual de cozinhas;
- dormitórios;
- salas;
- banheiros;
- escritórios;
- áreas comerciais;
- ambientação;
- apresentação 3D;
- renderização fotorealista.

O objetivo é atingir **paridade funcional e de fluxo de trabalho com o VDMax 3.0 Arquitetos e Decoradores**, porém com implementação própria, gratuita, open source, local e compatível com Linux.

O alvo é especificamente:

**VDMax Arquitetos e Decoradores.**

Não utilizar como escopo:

- VDMax Marceneiro;
- VDMax Móveis Modulados;
- sistemas de fábrica;
- ERP/MRP;
- plano de corte;
- lista de peças para fabricação;
- método construtivo industrial;
- orçamento explodido de marcenaria;
- etiquetas de peças;
- ferragens para produção;
- otimização de chapas.

Essas funcionalidades NÃO pertencem ao escopo principal deste projeto.

O LibreMax Architect deve ser um software de **projeto de interiores e apresentação 3D**, não um sistema de fabricação de móveis.

---

# 1. RESULTADO FINAL

O programa deve permitir este fluxo completo:

```text
Abrir LibreMax Architect
↓
Criar novo projeto
↓
Definir medidas do ambiente
↓
Desenhar paredes
↓
Adicionar muretas
↓
Inserir portas
↓
Inserir janelas
↓
Inserir escadas
↓
Aplicar piso, parede e teto
↓
Abrir biblioteca local
↓
Escolher móveis
↓
Arrastar móveis para o ambiente
↓
Redimensionar módulos em milímetros
↓
Alterar modelos/acabamentos
↓
Adicionar eletrodomésticos
↓
Adicionar objetos decorativos
↓
Adicionar iluminação
↓
Criar tampos automaticamente
↓
Criar rodapés automaticamente
↓
Criar rodaforros
↓
Criar fechamentos
↓
Criar envelopamentos
↓
Criar sancas e geometrias
↓
Configurar câmera
↓
Configurar iluminação
↓
Visualizar em 3D
↓
Renderizar em qualidade fotorealista
↓
Salvar imagem
↓
Salvar projeto
↓
Fechar o programa
↓
Reabrir projeto sem perda
```

Se esse fluxo não funcionar de ponta a ponta, o produto não está pronto.

---

# 2. NÃO ENTREGAR UM PROTÓTIPO

Não entregar:

- mockup;
- demonstração;
- viewport com cubos;
- editor Three.js simples;
- CRUD de objetos;
- interface sem ferramentas reais;
- botões sem implementação;
- cenas fake;
- funções marcadas como TODO;
- catálogo composto por caixas genéricas;
- interface web colocada dentro de Electron;
- aplicativo que depende do Blender para editar o projeto;
- aplicativo que depende do FreeCAD como interface.

O resultado deve ser um aplicativo desktop próprio.

Blender pode ser utilizado internamente como renderer.

OpenCASCADE pode ser utilizado como kernel geométrico.

O usuário não deve precisar operar essas ferramentas manualmente.

---

# 3. CLEAN-ROOM

VDMax é referência de:

- capacidade;
- comportamento;
- workflow;
- categorias de ferramentas;
- produtividade.

Não copiar:

- código;
- executáveis;
- DLLs;
- banco de dados;
- biblioteca proprietária;
- modelos 3D proprietários;
- texturas proprietárias;
- ícones;
- logotipo;
- marca;
- arquivos internos;
- instaladores;
- identidade visual protegida.

Não descompilar o VDMax.

Não extrair dados do VDMax.

Não copiar a UI pixel por pixel.

Implemente as funcionalidades do zero.

Pode reproduzir comportamentos observáveis e conceitos gerais de interação necessários para atingir paridade funcional.

---

# 4. PESQUISA INICIAL OBRIGATÓRIA

Antes de alterar arquitetura ou implementar funcionalidades, pesquise as fontes públicas oficiais do:

```text
VDMax 3.0 Arquitetos e Decoradores
```

Pesquisar:

- página oficial do produto;
- tabela comparativa oficial;
- vídeos oficiais;
- vídeos de criação de ambientes;
- vídeos de cozinha;
- vídeos de geometria;
- vídeos de tampos;
- vídeos de sancas;
- vídeos de rodaforro;
- vídeos de DXF;
- materiais públicos de suporte.

Não pesquisar o VDMax Marceneiro para expandir o escopo.

Ele só pode ser consultado quando alguma função compartilhada entre as versões precisar ser compreendida.

Criar:

```text
docs/VDMAX_ARCHITECT_PARITY.md
```

Tabela:

```text
ID
Área
Funcionalidade VDMax
Comportamento observado
Implementação LibreMax
Teste
Status
Fonte
Observações
```

Status:

```text
NOT_STARTED
IN_PROGRESS
FUNCTIONAL
VERIFIED
BLOCKED
```

Uma função só recebe:

```text
VERIFIED
```

quando existir teste reproduzível.

---

# 5. PRINCIPAIS CAPACIDADES DE PARIDADE

A matriz deve cobrir no mínimo:

```text
Criação de ambientes
Paredes
Muretas
Portas
Janelas
Escadas
Pisos
Forros
Geometria personalizada
Importação DXF
Biblioteca de módulos
Módulos de cozinha
Módulos de dormitório
Redimensionamento milimétrico
Acabamentos
Modelos de puxadores
Portas de vidro
Biblioteca decorativa
Texturas
Materiais
Tampos automáticos
Rodatampos
Rodapés
Rodaforros
Fechamentos
Envelopamento
Iluminação
Câmeras
Render ray-tracing
Salvar projetos
Abrir projetos
Duplicação
Espelhamento
Movimentação
Rotação
Alinhamento
Visualização 3D
Exportação das imagens renderizadas
```

Durante a pesquisa podem surgir funcionalidades adicionais.

Adicione-as à matriz.

---

# 6. STACK PRINCIPAL

Construir aplicativo desktop nativo.

Preferência:

```text
C++20
Qt 6
Qt Quick / QML
OpenCASCADE
OpenGL / Qt RHI
SQLite
SQLite FTS5
CMake
Ninja
Catch2
spdlog
nlohmann/json
Assimp
Blender 5.2 LTS+ / Cycles
```

Avaliar bibliotecas adicionais quando necessário.

Toda dependência precisa possuir licença compatível com distribuição open source.

Não usar Electron.

Não fazer uma aplicação web embrulhada em desktop.

---

# 7. SISTEMAS OPERACIONAIS

Prioridade inicial:

```text
Linux Mint
Ubuntu
Debian
```

Arquitetura inicial:

```text
x86_64
```

Preparar arquitetura para:

```text
ARM64
```

futuramente.

Formatos de distribuição:

```text
AppImage
.deb
Flatpak
```

AppImage deve ser o formato portátil principal.

---

# 8. OFFLINE FIRST

O programa deve funcionar completamente sem internet.

Não exigir:

```text
conta
login
assinatura
servidor
API
cloud
telemetria
licença online
```

Devem funcionar offline:

```text
editor
biblioteca
pesquisa
materiais
texturas
módulos
decoração
renderização
importação
exportação
salvamento
```

Internet será opcional apenas para:

```text
baixar novas versões
baixar bibliotecas adicionais
acessar documentação externa
```

---

# 9. ESTRUTURA DO REPOSITÓRIO

Usar aproximadamente:

```text
LibreMax/
├── CMakeLists.txt
├── LICENSE
├── README.md
├── CHANGELOG.md
├── apps/
│   └── architect/
├── libs/
│   ├── core/
│   ├── commands/
│   ├── document/
│   ├── geometry/
│   ├── architecture/
│   ├── snapping/
│   ├── dimensions/
│   ├── scene/
│   ├── viewport/
│   ├── modules/
│   ├── library/
│   ├── assets/
│   ├── materials/
│   ├── textures/
│   ├── automation/
│   ├── lighting/
│   ├── cameras/
│   ├── rendering/
│   ├── import/
│   ├── export/
│   ├── persistence/
│   └── diagnostics/
├── qml/
│   ├── shell/
│   ├── viewport/
│   ├── library/
│   ├── inspector/
│   ├── materials/
│   ├── render/
│   └── dialogs/
├── resources/
├── starter-library/
├── schemas/
├── tests/
├── scripts/
├── examples/
├── packaging/
└── docs/
```

Interface não deve conter regras de negócio.

---

# 10. DOCUMENT MODEL

Criar um Document Model central.

Entidades principais:

```text
Project
Level
Room
Wall
HalfWall
Floor
Ceiling
Door
Window
Stair
FurnitureModule
DecorativeObject
GeometryObject
Material
Texture
Light
Camera
Dimension
Group
```

Todo objeto:

```text
UUID
type
name
parent
children
transform
visibility
locked
metadata
```

UUID deve permanecer estável durante toda a vida do projeto.

---

# 11. UNIDADES

Geometria interna:

```text
milímetros
```

Precisão:

```text
0,1 mm
```

Interface pode exibir:

```text
mm
cm
m
```

Nunca alterar a unidade interna.

---

# 12. ARQUIVO DE PROJETO

Criar:

```text
.lmx
```

Formato container ZIP versionado.

Estrutura:

```text
manifest.json
scene.json
project.json
materials.json
lighting.json
cameras.json
custom-assets/
thumbnails/
renders/
```

Não guardar informações essenciais apenas em cache.

O arquivo precisa sobreviver a atualizações futuras através de migrations.

Implementar:

```text
Novo
Abrir
Salvar
Salvar como
Projetos recentes
Autosave
Recuperação após crash
Backup
```

---

# 13. SALVAMENTO SEGURO

Nunca sobrescrever diretamente.

Fluxo:

```text
serialize
↓
temporary file
↓
flush
↓
fsync
↓
validation
↓
atomic rename
```

Se ocorrer crash durante o salvamento, o projeto anterior precisa continuar válido.

---

# 14. UNDO / REDO

Command Pattern obrigatório.

Comandos como:

```text
CreateWall
CreateDoor
CreateWindow
InsertAsset
MoveObject
RotateObject
ResizeModule
ChangeMaterial
ChangeTexture
DeleteObject
CreateCountertop
ApplyAutomation
```

Atalhos:

```text
Ctrl+Z
Ctrl+Shift+Z
```

Undo/redo deve funcionar em todas as operações editáveis relevantes.

---

# 15. EDITOR ARQUITETÔNICO

Criar ferramentas:

```text
Selecionar
Parede
Mureta
Porta
Janela
Escada
Piso
Forro
Geometria
Cota
Medir
Mover
Rotacionar
Espelhar
Duplicar
Excluir
```

O fluxo precisa ser otimizado para arquitetura de interiores.

---

# 16. PAREDES

Desenho:

```text
clique inicial
↓
preview
↓
cursor
↓
comprimento + ângulo
↓
clique final
```

Depois continuar automaticamente para permitir cadeia de paredes.

Propriedades:

```text
comprimento
altura
espessura
posição
ângulo
material interno
material externo
```

Encontros:

```text
L
T
X
```

Resolver visualmente as junções.

---

# 17. MURETAS

Mesmo motor de paredes.

Diferença:

```text
altura reduzida
```

Propriedades configuráveis.

Exemplo:

```text
altura = 1100 mm
espessura = 100 mm
```

---

# 18. AMBIENTES

Detectar regiões fechadas.

Criar entidade:

```text
Room
```

Mostrar:

```text
nome
área
perímetro
altura
```

Exemplos:

```text
Cozinha
Sala
Dormitório
Banheiro
Escritório
```

Permitir renomear.

---

# 19. GRID

Grid configurável.

Opções:

```text
5 mm
10 mm
50 mm
100 mm
500 mm
custom
```

Zoom deve adaptar visualmente o grid.

---

# 20. SNAP ENGINE

Criar engine dedicada.

Snaps:

```text
Grid
Endpoint
Midpoint
Intersection
Perpendicular
Parallel
Wall Axis
Wall Face
Object Corner
Object Center
Object Edge
Module Back
Module Side
Floor
```

Feedback visual obrigatório.

Atalhos para ativar/desativar snap.

---

# 21. COTAS

Tipos:

```text
horizontal
vertical
aligned
angular
```

Cotas devem permanecer associadas aos elementos.

Ao mover uma parede:

```text
cota atualiza
```

---

# 22. PORTAS

Porta deve criar abertura real na parede.

Não inserir apenas um modelo sobreposto.

Parâmetros:

```text
largura
altura
espessura
posição
offset
lado da dobradiça
sentido de abertura
ângulo de abertura
modelo
material
```

Visualizar folha e arco de abertura em planta.

---

# 23. JANELAS

Abertura associativa.

Parâmetros:

```text
largura
altura
peitoril
profundidade
posição
modelo
material
```

Mover parede mantém janela associada.

---

# 24. ESCADAS

Fornecer ao menos:

```text
reta
L
U
```

Parâmetros:

```text
largura
altura
degraus
espelho
piso
patamar
material
```

Gerar a geometria proceduralmente.

---

# 25. PISO

Ambientes fechados devem permitir criação de piso.

Configurações:

```text
material
textura
escala
rotação
offset
```

Permitir materiais diferentes por ambiente.

---

# 26. TETO / FORRO

Criar superfície superior associada ao ambiente.

Configurar:

```text
altura
material
visibilidade
```

Deve poder ser ocultado durante edição.

---

# 27. VIEWPORT

Modos:

```text
Planta 2D
3D
Dividido
Apresentação
```

Planta e 3D precisam representar o mesmo Document Model.

Não criar dois projetos paralelos.

---

# 28. VISTAS

Fornecer:

```text
Perspective
Orthographic
Top
Bottom
Front
Back
Left
Right
Isometric
```

Atalhos rápidos.

---

# 29. NAVEGAÇÃO 3D

Implementar:

```text
Orbit
Pan
Zoom
Walk
Look Around
Focus Selection
Frame All
```

Interação deve ser fluida.

---

# 30. SELEÇÃO

Implementar:

```text
click selection
multi-selection
box selection
hover highlight
selection outline
```

Ações:

```text
Hide
Show
Isolate
Lock
Unlock
Delete
Duplicate
Group
Ungroup
```

---

# 31. TRANSFORM GIZMO

Gizmo 3D:

```text
X
Y
Z
```

Modos:

```text
Move
Rotate
Scale
```

Para módulos paramétricos, redimensionamento principal deve ocorrer através de dimensões reais e não scale destrutivo.

---

# 32. BIBLIOTECA LOCAL

A biblioteca é componente central.

Diretório:

```text
~/.local/share/libremax/
```

Estrutura:

```text
library.db
assets/
models/
textures/
materials/
thumbnails/
packs/
user/
cache/
backups/
```

Banco:

```text
SQLite
WAL
FTS5
foreign_keys=ON
```

Nada depende de servidor.

---

# 33. BANCO DA BIBLIOTECA

Entidades:

```text
categories
assets
asset_variants
asset_parameters
asset_files
asset_dependencies
tags
asset_tags
materials
textures
favorites
recent_assets
collections
licenses
packs
pack_assets
migrations
```

Pesquisa usando FTS5.

---

# 34. CATEGORIAS DA BIBLIOTECA

Estrutura inicial:

```text
Ambiente
├── Portas
├── Janelas
└── Escadas

Cozinha
├── Balcões
├── Aéreos
├── Torres
├── Cantos
├── Nichos
├── Colunas
├── Ilhas
└── Eletrodomésticos

Dormitório
├── Roupeiros
├── Armários
├── Criados
├── Camas
├── Painéis
└── Complementos

Sala
├── Sofás
├── Poltronas
├── Mesas
├── Cadeiras
├── Racks
├── Painéis
├── Estantes
└── Aparadores

Banheiro
├── Gabinetes
├── Cubas
├── Vasos
├── Boxes
└── Acessórios

Escritório
├── Mesas
├── Cadeiras
├── Armários
└── Estantes

Decoração
├── Vasos
├── Plantas
├── Quadros
├── Esculturas
├── Tapetes
├── Cortinas
├── Almofadas
├── Livros
├── Brinquedos
└── Instrumentos

Iluminação
├── Spots
├── Pendentes
├── Lustres
├── Arandelas
├── Abajures
└── Fitas LED

Eletrodomésticos
├── Geladeiras
├── Fornos
├── Cooktops
├── Microondas
├── Coifas
├── TVs
└── Lavadoras

Materiais
├── Madeira
├── MDF visual
├── Pedra
├── Vidro
├── Metal
├── Tecido
├── Cerâmica
└── Pintura
```

---

# 35. INTERFACE DA BIBLIOTECA

Painel lateral:

```text
BIBLIOTECA

[ Pesquisar... ]

★ Favoritos
◷ Recentes
▣ Minhas Coleções

Categorias
...
```

Card:

```text
thumbnail
nome
categoria
dimensão
favorito
```

Interações:

```text
clique
duplo clique
drag-and-drop
menu contextual
```

---

# 36. BUSCA

Pesquisar por:

```text
nome
categoria
tag
ambiente
tipo
dimensão
material
coleção
```

Busca tolerante a acentos.

Exemplo:

```text
armario
```

deve encontrar:

```text
Armário
```

Meta:

```text
10.000 itens
busca < 100 ms
```

em hardware desktop comum.

---

# 37. BIBLIOTECA PADRÃO

O VDMax Arquitetos possui uma biblioteca decorativa extensa.

O LibreMax precisa alcançar a mesma classe de capacidade sem copiar nenhum asset proprietário.

Construir biblioteca própria com assets:

```text
CC0
CC-BY compatíveis
public domain
produzidos especificamente para LibreMax
procedurais
```

Registrar:

```text
autor
origem
licença
data
```

Meta do catálogo completo:

```text
3.000+ entradas úteis
```

Não gerar milhares de duplicatas artificiais para atingir número.

Os itens precisam ser visualmente úteis para interiores.

---

# 38. STARTER LIBRARY

O repositório inicial não precisa carregar vários gigabytes.

Separar:

```text
LibreMax Core
Starter Library
Extended Library Packs
```

Starter Library precisa ser funcional offline logo após instalação.

Extended packs também devem funcionar localmente após instalados.

---

# 39. LMAXPACK

Formato local:

```text
.lmaxpack
```

Estrutura:

```text
manifest.json
assets/
models/
materials/
textures/
thumbnails/
licenses/
```

Manifest:

```text
pack_id
name
version
author
license
min_version
assets
hashes
dependencies
```

Importação deve validar segurança antes de extrair.

---

# 40. IMPORTAR OBJETO DO USUÁRIO

Suportar:

```text
GLB
glTF
OBJ
STL
DAE
STEP
IGES
```

Quando tecnicamente viável:

```text
FBX
```

Assistente:

```text
Selecionar arquivo
↓
Detectar unidade
↓
Corrigir escala
↓
Escolher orientação
↓
Definir origem
↓
Definir categoria
↓
Nome
↓
Tags
↓
Gerar thumbnail
↓
Salvar
```

Depois o item passa a fazer parte da biblioteca local.

---

# 41. MÓDULOS EDITÁVEIS

Módulos de cozinha e dormitório precisam ser paramétricos.

Não usar simplesmente:

```text
scaleX
scaleY
scaleZ
```

sobre um mesh.

Parâmetros:

```text
largura
altura
profundidade
```

Exemplo:

```text
Balcão 2 portas
800 x 720 x 550
```

Alterar:

```text
800 → 970
```

deve recompor visualmente o módulo mantendo:

```text
laterais
portas
puxadores
prateleiras
proporções
```

de maneira coerente.

---

# 42. PRECISÃO DOS MÓDULOS

Permitir valores como:

```text
753 mm
812,5 mm
1197 mm
```

Não limitar a presets.

Campos:

```text
Largura
Altura
Profundidade
```

Precisão mínima:

```text
0,1 mm
```

---

# 43. FAMÍLIAS DE COZINHA

Criar módulos como:

```text
balcão 1 porta
balcão 2 portas
balcão 3 portas
gaveteiro
gaveteiro + porta
canto
aéreo 1 porta
aéreo 2 portas
aéreo basculante
torre de forno
torre de eletrodomésticos
nicho
ilha
painel
prateleira
```

Criar variações próprias.

---

# 44. FAMÍLIAS DE DORMITÓRIO

Criar:

```text
roupeiro 2 portas
roupeiro 3 portas
roupeiro 4 portas
roupeiro de canto
roupeiro com porta de correr
roupeiro com espelho
aéreo
nicho
cabeceira
painel
criado
escrivaninha
```

---

# 45. MODELOS / VARIANTES

Módulos devem suportar variantes visuais.

Exemplo:

```text
Módulo selecionado
↓
Modelos
```

Permitir trocar:

```text
acabamento
frente
puxador
vidro
cor do vidro
modelo de porta
```

Sem remover/reinserir o móvel.

---

# 46. PUXADORES

Criar biblioteca própria de puxadores.

Tipos:

```text
alça
perfil
ponto
cava visual
embutido
```

Troca instantânea.

Não é necessário modelar ferragens de fabricação.

O objetivo é visual.

---

# 47. PORTAS DE VIDRO

Suportar:

```text
transparente
fumê
bronze
canelado
fosco
espelhado
```

Como materiais visuais próprios.

---

# 48. OBJETOS DECORATIVOS

Qualquer objeto decorativo deve permitir:

```text
mover
rotacionar
escalar
duplicar
espelhar
alterar material quando aplicável
ocultar
agrupar
```

Objetos precisam possuir bounding boxes para snap e seleção.

---

# 49. MATERIAIS

Sistema PBR.

Propriedades:

```text
baseColor
baseColorTexture
roughness
metallic
normalTexture
normalStrength
opacity
transmission
IOR
emission
emissionStrength
textureScale
textureRotation
textureOffset
```

---

# 50. TEXTURAS

Importar:

```text
JPG
JPEG
PNG
WEBP
EXR
```

Fluxo:

```text
Importar textura
↓
thumbnail
↓
nome
↓
categoria
↓
escala
↓
salvar na biblioteca
```

---

# 51. APLICAÇÃO DE TEXTURA

Aplicar em:

```text
paredes
pisos
tetos
móveis
objetos
geometrias
```

Permitir:

```text
escala
rotação
offset X
offset Y
```

Não esticar automaticamente de maneira incorreta.

---

# 52. MATERIAIS VISUAIS IMPORTANTES

Fornecer presets:

```text
MDF
madeira
pedra
mármore
granito
vidro
espelho
metal
inox
tecido
cerâmica
porcelanato
pintura fosca
pintura brilhante
laca
```

---

# 53. ESPELHO

Material de espelho precisa funcionar corretamente tanto no viewport quanto no render.

---

# 54. VIDRO

Parâmetros:

```text
cor
transparência
roughness
IOR
espessura visual
```

---

# 55. AUTOMAÇÕES

Implementar rotinas equivalentes em capacidade às automações públicas do VDMax Arquitetos.

Obrigatórias:

```text
Tampo
Rodatampo
Rodapé
Rodaforro
Fechamento
Envelopamento
```

Essas ferramentas devem trabalhar diretamente sobre módulos/ambientes.

---

# 56. TAMPO AUTOMÁTICO

Fluxo:

```text
Selecionar módulos inferiores
↓
Tampo
↓
Configurar espessura
↓
Configurar avanço
↓
Configurar material
↓
Gerar
```

Detectar sequência dos móveis automaticamente.

Criar geometria contínua quando possível.

---

# 57. RODATAMPO

Criar elemento vertical associado ao tampo/parede.

Parâmetros:

```text
altura
espessura
material
```

---

# 58. RODAPÉ

Detectar módulos aplicáveis.

Parâmetros:

```text
altura
profundidade
recuo
material
```

Gerar segmentos automaticamente.

---

# 59. RODAFORRO

Automação superior equivalente.

Detectar módulos superiores.

Permitir:

```text
altura
profundidade
offset
material
```

---

# 60. FECHAMENTOS

Tipos:

```text
lateral esquerdo
lateral direito
superior
inferior
entre módulo e parede
```

Detectar automaticamente espaços apropriados.

Permitir ajuste manual.

---

# 61. ENVELOPAMENTO

Permitir criar painéis envolvendo módulo ou grupo.

Configurações:

```text
laterais
topo
base
espessura
avanço
material
```

---

# 62. ASSOCIAÇÃO DAS AUTOMAÇÕES

Objetos automáticos devem guardar relação com suas fontes.

Exemplo:

```text
tampo
└── módulos 12, 13, 14
```

Se os módulos forem redimensionados:

```text
Automação desatualizada
[Atualizar]
```

ou recalcular automaticamente quando seguro.

---

# 63. GEOMETRIA PERSONALIZADA

Criar ferramenta:

```text
Geometria
```

Para elementos que não existem na biblioteca.

Operações:

```text
retângulo
linha
polígono
arco
círculo
extrusão
```

---

# 64. GEOMETRIA PARA INTERIORES

A ferramenta deve permitir criar:

```text
tampos especiais
prateleiras
nichos
painéis
rodatetos
sancas
rebaixos
volumes decorativos
```

---

# 65. OPERAÇÕES BOOLEANAS

Quando necessário:

```text
Union
Difference
Intersection
```

Usar OpenCASCADE.

---

# 66. SANCAS

Criar fluxo de geometria apropriado para sancas.

Permitir:

```text
perfil
trajeto
extrusão
material
```

---

# 67. RODATETOS

Permitir criar rodapés superiores/rodatetos seguindo perímetro do ambiente.

---

# 68. IMPORTAÇÃO DXF

Importar DXF para servir como referência de projeto.

Suportar:

```text
LINE
LWPOLYLINE
POLYLINE
ARC
CIRCLE
```

Importador:

```text
Arquivo
↓
Unidade
↓
Layers
↓
Escala
↓
Rotação
↓
Posição
↓
Importar
```

---

# 69. DXF COMO REFERÊNCIA

Usuário deve poder:

```text
bloquear
ocultar
alterar opacidade
selecionar layer
```

Fornecer ferramenta opcional:

```text
Converter linhas selecionadas em paredes
```

---

# 70. ILUMINAÇÃO

Implementar iluminação para projeto e render.

Tipos:

```text
Point
Spot
Area
LED Strip
Sun
Sky
Emissive Material
```

---

# 71. POINT LIGHT

Parâmetros:

```text
potência
cor
temperatura
raio
```

---

# 72. SPOT

Parâmetros:

```text
potência
cor
temperatura
ângulo
blend
alcance
```

Manipulação visual da direção.

---

# 73. AREA LIGHT

Parâmetros:

```text
largura
altura
potência
cor
temperatura
```

---

# 74. LED

Criar fita LED baseada em caminho.

Parâmetros:

```text
potência/m
cor
temperatura
intensidade
difusão
```

Adequada para:

```text
nichos
armários
sancas
painéis
```

---

# 75. MATERIAL EMISSIVO

Material pode emitir luz.

Útil para:

```text
TVs
spots
letreiros
painéis
luminárias
```

---

# 76. SOL E CÉU

Configuração ambiental:

```text
posição solar
intensidade
cor
céu
background
```

Permitir ambiente HDRI futuramente.

---

# 77. CÂMERAS

Criar câmeras persistentes.

Campos:

```text
nome
posição
target
FOV
focal length
sensor
clipping
aspect ratio
```

Presets:

```text
16:9
4:3
1:1
9:16
A4 horizontal
A4 vertical
```

---

# 78. GERENCIADOR DE CÂMERAS

Painel:

```text
Câmeras

Sala principal
Cozinha
Dormitório
Detalhe bancada

[+] Nova câmera
```

Permitir:

```text
ativar
renomear
duplicar
excluir
renderizar
```

---

# 79. RENDER

Render final precisa ser ray-tracing.

Usar:

```text
Blender
Cycles
```

como backend inicial.

O usuário não deve abrir Blender manualmente.

---

# 80. PIPELINE DE RENDER

Fluxo:

```text
Cena LibreMax
↓
Snapshot consistente
↓
Exporter
↓
Scene package
↓
Script Blender
↓
Blender --background
↓
Cycles
↓
Denoise
↓
PNG/JPEG
↓
Galeria do LibreMax
```

---

# 81. GPU

Detectar:

```text
NVIDIA
AMD
Intel
CPU
```

Preferências Cycles:

```text
NVIDIA → OptiX/CUDA
AMD → HIP
Intel → oneAPI
fallback → CPU
```

Nunca falhar apenas porque uma GPU específica não existe.

---

# 82. CONFIGURAÇÃO DE RENDER

Tela:

```text
RENDER

Câmera
[ Sala Principal ]

Qualidade
[ Preview ]

Resolução
1920 x 1080

Dispositivo
[ AMD Radeon RX ... ]

Samples
[ 256 ]

Denoise
[x]

Exposure
[ 0.0 ]

[Renderizar]
```

---

# 83. PRESETS DE RENDER

Criar:

```text
Rascunho
Preview
Alta
Final
Custom
```

Os presets precisam alterar parâmetros reais.

---

# 84. FILA DE RENDER

Processo separado.

Estados:

```text
Queued
Preparing
Rendering
Denoising
Completed
Failed
Cancelled
```

Usuário deve poder continuar projetando.

---

# 85. GALERIA DE RENDERS

Manter renders do projeto.

Mostrar:

```text
thumbnail
câmera
data
resolução
qualidade
tempo
```

Ações:

```text
Abrir
Salvar como
Excluir
Renderizar novamente
```

---

# 86. INTERFACE PRINCIPAL

Layout desktop:

```text
┌───────────────────────────────────────────────────────────────┐
│ Arquivo Editar Projeto Ambiente Inserir Render Ajuda         │
├───────────────────────────────────────────────────────────────┤
│ Ferramentas principais                                       │
├───────────────┬─────────────────────────────┬─────────────────┤
│               │                             │                 │
│ BIBLIOTECA    │                             │ PROPRIEDADES    │
│               │         VIEWPORT            │                 │
│ Pesquisa      │                             │ objeto          │
│ Categorias    │                             │ posição         │
│ Objetos       │                             │ dimensão        │
│               │                             │ material        │
│               │                             │ parâmetros      │
├───────────────┴─────────────────────────────┴─────────────────┤
│ snap | unidade | coordenadas | seleção | câmera              │
└───────────────────────────────────────────────────────────────┘
```

---

# 87. IDENTIDADE VISUAL

Não copiar visualmente o VDMax.

Criar UI própria, profissional e madura.

Referências de densidade e produtividade:

```text
Blender
SketchUp
Fusion
FreeCAD
software CAD profissional
```

Evitar:

```text
dashboard SaaS
cards gigantes
glassmorphism excessivo
neon
gradientes decorativos
espaço desperdiçado
interface com aparência de site
```

É um software desktop de trabalho.

---

# 88. TOOLBARS

Organizar grupos:

```text
Projeto
Ambiente
Inserir
Transformar
Geometria
Automação
Materiais
Iluminação
Câmeras
Render
```

Ícones precisam possuir tooltip e texto quando necessário.

---

# 89. WORKSPACES

Criar:

```text
Projeto
Materiais
Iluminação
Render
Biblioteca
```

A mudança de workspace deve reorganizar painéis sem trocar de aplicativo.

---

# 90. PROPERTY INSPECTOR

Exemplo módulo:

```text
Balcão 2 Portas

Transformação
X        1200
Y        0
Z        0
Rotação  90°

Dimensões
Largura      800 mm
Altura       720 mm
Profundidade 550 mm

Modelo
Frente       Lisa
Puxador      Perfil A
Vidro        Nenhum

Materiais
Caixa        Carvalho
Frente       Branco

[Aplicar]
```

Mudanças relevantes devem aparecer imediatamente.

---

# 91. EXPRESSÕES NUMÉRICAS

Campos numéricos devem aceitar:

```text
800
800+20
1200/2
2*450
```

Validar resultado.

---

# 92. DRAG AND DROP

Fluxo fundamental:

```text
Biblioteca
↓
segura thumbnail
↓
Viewport
↓
preview fantasma
↓
snap
↓
solta
↓
objeto inserido
```

A posição deve ser previsível.

---

# 93. POSICIONAMENTO INTELIGENTE

Ao aproximar módulo de parede:

```text
snap na parede
orientação correta
encostar costas
```

Ao aproximar de outro módulo:

```text
snap lateral
```

Visualizar guia durante encaixe.

---

# 94. ALINHAMENTO

Ferramentas:

```text
Esquerda
Direita
Topo
Base
Centro horizontal
Centro vertical
Distribuir
Encostar
```

---

# 95. COLISÕES

Detectar colisões evidentes entre:

```text
módulos
paredes
portas
decoração
```

Não impedir toda colisão automaticamente.

Mostrar aviso.

---

# 96. GRUPOS

Permitir agrupar objetos.

Exemplo:

```text
Mesa
4 cadeiras
Vaso
```

→

```text
Grupo Sala de Jantar
```

Mover conjuntamente.

---

# 97. HIERARQUIA DA CENA

Painel opcional:

```text
Projeto
├── Ambiente
│   ├── Parede 01
│   ├── Parede 02
│   ├── Porta
│   └── Janela
├── Cozinha
│   ├── Balcão 01
│   └── Balcão 02
├── Decoração
├── Iluminação
└── Câmeras
```

Não forçar usuário comum a trabalhar exclusivamente pela árvore.

---

# 98. IMPORTAÇÃO DE TEXTURAS

Importar JPG precisa fazer parte da versão inicial de paridade.

Também aceitar formatos modernos.

Salvar localmente no projeto ou na biblioteca.

Nunca depender do caminho original depois da importação quando a opção de incorporar estiver ativa.

---

# 99. IMPORTAÇÃO DE ASSETS

Ao incorporar asset externo ao projeto:

```text
copy
↓
hash
↓
deduplicate
↓
register
```

Evitar projetos quebrados ao mover arquivos.

---

# 100. THUMBNAILS

Gerar thumbnails automaticamente.

Fila assíncrona.

Cachear.

Nunca bloquear UI inteira.

---

# 101. PERFORMANCE

Implementar:

```text
scene graph
dirty flags
dependency tracking
incremental recompute
mesh cache
frustum culling
LOD
instancing
asynchronous jobs
```

Objetos repetidos devem compartilhar recursos quando possível.

---

# 102. PERFORMANCE TARGETS

Projeto de teste:

```text
100 paredes
500 módulos
500 objetos decorativos
100 luzes
10 câmeras
```

deve permanecer utilizável em hardware desktop médio.

Biblioteca:

```text
10.000 assets indexados
```

deve permanecer responsiva.

---

# 103. THREADING

Nunca bloquear UI com:

```text
importação
indexação
geração de thumbnail
tessellation pesada
render
exportação
```

Usar workers.

---

# 104. SEGURANÇA

Tratar:

```text
.lmx
.lmaxpack
DXF
modelos importados
texturas
ZIP
JSON
```

como input potencialmente não confiável.

Proteger contra:

```text
path traversal
ZIP bombs
symlink escape
arquivos gigantes
manifest inválido
parsing malicioso
integer overflow
command injection
```

---

# 105. BLENDER PROCESS

Iniciar Blender com:

```text
QProcess
```

e array de argumentos.

Nunca:

```text
system()
shell concatenation
```

com dados do usuário.

---

# 106. AUTOSAVE

Padrão:

```text
5 minutos
```

Configurável.

Manter múltiplas versões temporárias.

---

# 107. CRASH RECOVERY

Após encerramento inesperado:

```text
Encontramos uma versão recuperável de "Projeto Cozinha".

Último autosave:
21:43

[Recuperar]
[Ignorar]
```

---

# 108. BACKUP DA BIBLIOTECA

Opções:

```text
Exportar Biblioteca
Restaurar Biblioteca
```

Arquivo de backup precisa conter:

```text
database
assets
models
textures
materials
thumbnails
metadata
```

---

# 109. UPDATES

Como projeto open source, atualização pode usar releases públicas.

Mas atualização automática não deve ser requisito para funcionamento.

Fornecer:

```text
Verificar atualizações
```

como recurso opcional.

Não transmitir telemetria.

---

# 110. PLUGINS

Arquitetura futura:

```text
registerCommand
registerImporter
registerExporter
registerAutomation
registerAssetGenerator
```

Plugins não devem ser necessários para funções essenciais.

---

# 111. ATALHOS

Implementar:

```text
Ctrl+N
Ctrl+O
Ctrl+S
Ctrl+Shift+S
Ctrl+Z
Ctrl+Shift+Z
Ctrl+D
Delete
G
R
F
Esc
```

Permitir customização futura.

---

# 112. ACESSIBILIDADE

Suportar:

```text
keyboard navigation
focus states
tooltips
contraste adequado
escala de interface
atalhos
labels acessíveis
```

---

# 113. IDIOMA

Principal:

```text
Português do Brasil
```

Preparar traduções:

```text
English
Spanish
```

Não hardcodar strings da interface na lógica.

---

# 114. EXEMPLOS INCLUÍDOS

Criar:

```text
examples/
├── cozinha.lmx
├── dormitorio.lmx
├── sala.lmx
├── banheiro.lmx
└── apartamento-completo.lmx
```

Cada exemplo deve usar apenas assets distribuíveis.

---

# 115. TESTES

Criar:

```text
Unit
Integration
Geometry
Persistence
Library
Assets
Materials
Automation
Import
Render Pipeline
UI Smoke
Regression
```

---

# 116. TESTES DE GEOMETRIA

Não testar CAD apenas através de screenshots.

Validar:

```text
posição
bounding box
área
volume
faces
distância
interseção
resultado booleano
```

---

# 117. TESTE DE ACEITAÇÃO — COZINHA

Criar projeto.

Ambiente:

```text
4000 x 3000 mm
altura 2700 mm
paredes 120 mm
```

Criar:

```text
porta 800 x 2100
janela 1200 x 1000
peitoril 1000
```

Inserir:

```text
geladeira
torre
balcão 600
balcão 800
gaveteiro 600
balcão 800
aéreos
```

Alterar:

```text
balcão 800 → 947 mm
```

O módulo deve permanecer visualmente correto.

Aplicar:

```text
Carvalho
Branco
Pedra
Vidro
```

Gerar:

```text
tampo
rodatampo
rodapé
fechamentos
```

Adicionar:

```text
spots
LED
area light
```

Adicionar decoração.

Criar câmera.

Render:

```text
1920x1080
Cycles
GPU
denoise
```

Salvar imagem.

Salvar projeto.

Fechar.

Reabrir.

Tudo deve ser preservado.

---

# 118. TESTE DE ACEITAÇÃO — DORMITÓRIO

Criar:

```text
roupeiro
cama
criados
cortina
tapete
quadros
iluminação
```

Redimensionar roupeiro para dimensão não padrão.

Trocar:

```text
acabamento
puxador
espelho
```

Renderizar.

---

# 119. TESTE DXF

Importar planta DXF.

Selecionar unidade.

Exibir layers.

Escalar corretamente.

Bloquear referência.

Desenhar paredes sobre ela.

Salvar.

Reabrir.

DXF deve continuar alinhado.

---

# 120. TESTE DE BIBLIOTECA

Popular banco com:

```text
10.000 registros
```

Testar:

```text
pesquisa
filtro
favoritos
recentes
categorias
drag-and-drop
```

---

# 121. TESTE OFFLINE

Desabilitar completamente a rede.

Executar:

```text
novo projeto
biblioteca
inserção
materiais
texturas
DXF
edição
render
salvar
abrir
```

Nenhuma operação principal pode falhar por falta de internet.

---

# 122. TESTE DE RECUPERAÇÃO

Criar projeto.

Alterar projeto.

Forçar encerramento inesperado.

Abrir LibreMax.

Recuperar autosave.

Validar conteúdo.

---

# 123. CI

GitHub Actions:

```text
build
unit-tests
integration-tests
clang-format
clang-tidy
dependency-audit
package
```

Não permitir merge em main com build quebrado.

---

# 124. CÓDIGO

Ativar:

```text
-Wall
-Wextra
-Wpedantic
```

Utilizar:

```text
RAII
smart pointers
const correctness
strong typing
```

Evitar:

```text
global mutable state
giant classes
UI owning business logic
```

---

# 125. LOGGING

Usar `spdlog`.

Logs:

```text
~/.local/state/libremax/logs/
```

Níveis:

```text
trace
debug
info
warning
error
critical
```

---

# 126. DIAGNÓSTICOS

Menu:

```text
Ajuda
→ Exportar Diagnóstico
```

Criar ZIP contendo:

```text
versão
Linux distro
kernel
GPU
driver
OpenGL
Qt
OpenCASCADE
Blender
logs
```

Não enviar automaticamente.

---

# 127. BUILD SCRIPTS

Criar:

```text
scripts/bootstrap.sh
scripts/build.sh
scripts/test.sh
scripts/run.sh
scripts/package.sh
```

Fluxo:

```bash
./scripts/bootstrap.sh
./scripts/build.sh
./scripts/test.sh
./scripts/run.sh
```

---

# 128. DOCUMENTAÇÃO

Criar:

```text
README.md
CHANGELOG.md

docs/
├── ARCHITECTURE.md
├── BUILDING.md
├── CONTRIBUTING.md
├── PROJECT_FORMAT.md
├── LIBRARY.md
├── ASSETS.md
├── MATERIALS.md
├── AUTOMATIONS.md
├── DXF.md
├── RENDERING.md
├── SECURITY.md
├── VDMAX_ARCHITECT_PARITY.md
└── ROADMAP.md
```

---

# 129. README

README final deve ter:

```text
nome
descrição
screenshots reais
funcionalidades
instalação
build
atalhos
bibliotecas
render
licença
contribuição
roadmap
```

Não usar mockups como screenshots do produto.

---

# 130. FASE 0 — AUDITORIA E PESQUISA

Primeiro:

1. inspecionar workspace;
2. compilar o que já existir;
3. executar testes existentes;
4. pesquisar VDMax Arquitetos;
5. criar matriz de paridade;
6. identificar funcionalidades fora de escopo;
7. remover dependências de Marceneiro;
8. criar ADRs;
9. estabelecer baseline.

---

# 131. FASE 1 — CORE

Implementar:

```text
application shell
document model
scene graph
commands
undo/redo
units
serialization
OpenCASCADE integration
viewport
selection
transform
project save/open
```

---

# 132. FASE 2 — AMBIENTE

Implementar:

```text
grid
snap
walls
half-walls
rooms
doors
windows
stairs
floor
ceiling
dimensions
measure
```

---

# 133. FASE 3 — BIBLIOTECA

Implementar:

```text
SQLite
FTS5
assets
categories
thumbnails
search
favorites
recent
collections
drag-and-drop
.lmaxpack
```

---

# 134. FASE 4 — MÓDULOS

Implementar:

```text
kitchen modules
bedroom modules
parametric sizing
models
handles
glass
materials
placement snap
```

---

# 135. FASE 5 — DECORAÇÃO

Implementar biblioteca visual de:

```text
móveis soltos
eletrodomésticos
cortinas
tapetes
quadros
plantas
vasos
estofados
mesas
cadeiras
objetos domésticos
```

---

# 136. FASE 6 — MATERIAIS

Implementar:

```text
PBR
textures
JPG import
UV controls
presets
custom materials
material browser
```

---

# 137. FASE 7 — AUTOMAÇÕES

Implementar:

```text
tampo
rodatampo
rodapé
rodaforro
fechamentos
envelopamento
```

Testar em cozinhas reais.

---

# 138. FASE 8 — GEOMETRIA

Implementar:

```text
custom geometry
extrusion
boolean operations
sancas
rodatetos
custom countertops
```

---

# 139. FASE 9 — DXF

Implementar:

```text
DXF import
layers
units
transform
locking
visibility
wall conversion
```

---

# 140. FASE 10 — ILUMINAÇÃO

Implementar:

```text
point
spot
area
LED
sun
sky
emissive
```

---

# 141. FASE 11 — RENDER

Implementar:

```text
camera
scene exporter
Blender integration
Cycles
GPU detection
presets
render queue
gallery
```

---

# 142. FASE 12 — POLIMENTO

Implementar:

```text
autosave
recovery
backup
performance
keyboard
accessibility
diagnostics
examples
packaging
```

---

# 143. FASE 13 — PARITY PASS

Abrir:

```text
docs/VDMAX_ARCHITECT_PARITY.md
```

Percorrer cada item.

Qualquer item:

```text
NOT_STARTED
IN_PROGRESS
FUNCTIONAL
```

deve ser analisado antes de considerar versão 1.0.

Objetivo:

```text
VERIFIED
```

para todos os recursos essenciais do VDMax Arquitetos publicamente documentados.

---

# 144. NÃO IMPLEMENTAR O MARCENEIRO

Não desperdiçar desenvolvimento com:

```text
plano de corte
nesting
otimização de chapas
etiquetas de produção
BOM industrial
explosão de peças
custos de MDF
ferragens de produção
lista de corte
método construtivo
ERP
MRP
produção CNC
orçamento de fabricação
```

Se alguma implementação antiga possuir esses recursos:

```text
não expandir
```

Separe-os do produto principal.

O foco é:

```text
VDMax Arquitetos e Decoradores
```

---

# 145. NÃO CONFUNDIR MÓDULO EDITÁVEL COM MÓDULO DE MARCENARIA

O LibreMax Architect precisa de:

```text
módulos visualmente editáveis
largura
altura
profundidade
acabamento
puxador
vidro
modelo
```

Não precisa inicialmente decompor armários em:

```text
laterais para fabricação
fundo produtivo
ferragens industriais
lista de corte
```

A parametrização existe para o projeto visual.

---

# 146. EXPERIÊNCIA DO USUÁRIO

O usuário típico deve conseguir:

```text
abrir o programa
criar uma cozinha
mobiliar
renderizar
```

sem conhecer:

```text
OpenCASCADE
Blender
C++
CAD mecânico
scripts
terminal
```

Todas as funções comuns devem estar acessíveis visualmente.

---

# 147. REGRAS PARA O CODEX

Não parar para pedir confirmação entre fases normais.

Quando houver pequena ambiguidade técnica:

```text
escolher a alternativa mais sustentável
registrar em ADR
continuar
```

Quando houver uma decisão que mude drasticamente o produto:

```text
registrar trade-offs
usar a opção que preserve os requisitos desta spec
```

---

# 148. NÃO GERAR FALSA IMPLEMENTAÇÃO

Nunca marcar como implementado algo que:

```text
só possui botão
só possui interface
só funciona no happy path artificial
usa mock
retorna dados fake
não persiste
não possui integração
```

---

# 149. DEFINITION OF DONE DE UMA FEATURE

Uma feature está pronta quando tiver:

```text
implementação
UI
persistência quando aplicável
undo/redo quando aplicável
erro tratado
teste
documentação
integração
```

---

# 150. REGRA DE PROGRESSO

Priorizar funcionalidades verticais completas.

Exemplo correto:

```text
Parede
→ desenhar
→ selecionar
→ editar
→ salvar
→ fechar
→ abrir
→ undo
→ redo
→ testar
```

Antes de começar mais dez funcionalidades superficiais.

---

# 151. BASE EXISTENTE

Se existir código anterior:

```text
auditar
```

Preservar apenas o que estiver alinhado com esta arquitetura.

Se o projeto anterior utilizar:

```text
Tkinter
canvas 2D fake
Three.js como kernel CAD
geometria sem topologia
arquitetura de protótipo
```

substitua a arquitetura inadequada.

Não manter tecnologia errada apenas porque já existe código.

---

# 152. GIT

Commits por domínio.

Exemplos:

```text
feat(walls): add parametric wall creation
feat(openings): add associative doors and windows
feat(library): add local FTS asset search
feat(modules): add millimetric resize
feat(automation): add countertop generation
feat(materials): add PBR texture pipeline
feat(render): add Cycles render queue
```

---

# 153. ENTREGAS ITERATIVAS

A cada ciclo relevante atualizar:

```text
README.md
CHANGELOG.md
docs/VDMAX_ARCHITECT_PARITY.md
docs/ROADMAP.md
```

Reportar:

```text
IMPLEMENTADO
TESTADO
CORRIGIDO
PENDENTE
ARQUIVOS ALTERADOS
COMANDOS DE TESTE
PARIDADE ATUAL
PRÓXIMO BLOCO
```

---

# 154. PRIMEIRA EXECUÇÃO DO CODEX

Comece imediatamente:

```text
1. inspecionar o workspace;
2. compilar estado atual;
3. rodar testes;
4. remover do roadmap funções exclusivas do VDMax Marceneiro;
5. pesquisar apenas o VDMax Arquitetos e Decoradores;
6. criar docs/VDMAX_ARCHITECT_PARITY.md;
7. listar todas as capacidades observáveis;
8. comparar com código existente;
9. criar ADRs;
10. corrigir a arquitetura quando necessário;
11. iniciar a primeira lacuna crítica de paridade;
12. implementar;
13. testar;
14. continuar.
```

Não interromper após gerar documentos.

Documentação é preparação para implementação.

---

# 155. CRITÉRIO DE PARIDADE

Não considerar suficiente:

```text
"tem editor 3D"
```

A comparação correta é:

```text
O usuário consegue executar no LibreMax Architect
a mesma tarefa prática que executaria no VDMax Arquitetos?
```

Exemplo:

```text
VDMax:
criar cozinha → inserir módulos → redimensionar → acabamento →
tampo → rodapé → decoração → iluminação → render
```

LibreMax precisa permitir fluxo equivalente.

---

# 156. CRITÉRIO DA BIBLIOTECA

Biblioteca local é requisito de produto, não complemento.

Ela precisa ser:

```text
rápida
pesquisável
categorizada
extensível
offline
portátil
segura
```

Importar novos assets precisa ser simples.

---

# 157. CRITÉRIO DO RENDER

Não considerar render completo apenas porque uma imagem foi gerada.

Validar:

```text
materiais
texturas
vidro
espelho
luz
sombra
reflexos
emissão
câmera
exposição
denoise
GPU
resolução
```

---

# 158. CRITÉRIO DE UX

Uma pessoa familiarizada com programas de interiores deve conseguir descobrir o fluxo principal sem ler documentação técnica.

O aplicativo precisa favorecer:

```text
arrastar
soltar
selecionar
editar numericamente
visualizar imediatamente
```

---

# 159. RELEASE 1.0

Somente criar:

```text
LibreMax Architect 1.0
```

quando estes blocos estiverem funcionais:

```text
Ambiente
Paredes
Muretas
Portas
Janelas
Escadas
Piso
Teto
Biblioteca local
Módulos cozinha
Módulos dormitório
Redimensionamento milimétrico
Modelos/acabamentos
Objetos decorativos
Materiais
Texturas JPG
DXF
Tampos
Rodatampos
Rodapés
Rodaforros
Fechamentos
Envelopamento
Geometria
Sancas
Iluminação
Câmeras
Ray tracing
Save/Open
Autosave
Crash recovery
```

---

# 160. ENTREGA FINAL

Gerar:

```text
LibreMax-Architect-x86_64.AppImage
libremax-architect_<version>_amd64.deb
Flatpak manifest
source.tar.gz
starter-library
sample-projects
README
documentation
test-report
benchmark-report
VDMAX_ARCHITECT_PARITY.md
SHA256SUMS
```

---

# 161. META FINAL

O objetivo final é:

**LibreMax Architect = alternativa gratuita, open source, offline e nativa para Linux ao VDMax 3.0 Arquitetos e Decoradores.**

O software deve priorizar:

```text
criação rápida de ambientes
biblioteca local extensa
módulos editáveis
decoração
materiais
automações
iluminação
apresentação 3D
renderização fotorealista
```

Não transformar o produto em:

```text
software de marcenaria
software de fábrica
CAD mecânico
ERP
plano de corte
```

A experiência final precisa ser a de um **software profissional de projeto de interiores**, com capacidade equivalente ao VDMax Arquitetos e Decoradores, construído do zero e distribuível legalmente como open source.