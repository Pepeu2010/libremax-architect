# Dependências e distribuição

A fonte 0.15 acrescenta 28 modelos originais LibreMax CC0-1.0, sem designs ou
geometria VDMax. [Gerador](../scripts/create-home-collection.py),
[procedência e hashes](../starter-models/home-provenance.json) e
[malhas menores para edição](../starter-models/editor-lod-provenance.json).
As licenças das fontes já integradas continuam preservadas abaixo. Modelos
importados pelo usuário guardam sua procedência e não recebem licença CC0
automaticamente.

| Dependência direta | Uso | Licença indicada pelo projeto |
|---|---|---|
| Qt 6 Core/Gui/Widgets/Sql/Concurrent/Test | UI/modelo/processos/testes | LGPL-3.0 / GPL-3.0 |
| OpenCASCADE | B-rep/viewport/tesselação | LGPL-2.1-or-later com OCCT exception |
| libzip | Container | BSD-3-Clause |
| SQLite (plugin Qt) | Biblioteca/FTS | Domínio público |
| nlohmann/json | Documento | MIT |
| spdlog/fmt | Logs | MIT |
| Catch2 | Testes | BSL-1.0 |
| Blender/Cycles 4.5.9 LTS | Processo externo incluído no instalador | GPL |
| FreeImage | Validação HDR em ponto flutuante | FreeImage Public License / GPL |
| OpenEXR / Imath | Validação EXR em ponto flutuante | BSD-3-Clause |

Fontes: [Qt](https://doc.qt.io/qt-6/licensing.html), [OCCT](https://dev.opencascade.org/resources/licensing), [libzip](https://libzip.org/license/), [SQLite](https://sqlite.org/copyright.html), [JSON](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT), [spdlog](https://github.com/gabime/spdlog/blob/v1.x/LICENSE), [Catch2](https://github.com/catchorg/Catch2/blob/devel/LICENSE.txt), [Blender](https://www.blender.org/about/license/).

As dependências são vinculadas dinamicamente onde distribuídas, exceto core próprio. Binário final deve incluir avisos, licenças e fontes/ofertas aplicáveis. Dependências transitivas do MSYS2 incluem mais módulos do que o app usa; não distribuir toda a pasta de desenvolvimento. SBOM/auditoria dos pacotes finais ainda não executados. Assimp ainda não integrado.

## Texturas distribuídas

Blender é distribuído no formato oficial completo, sem modificações, com Python,
bibliotecas e licenças originais. As Releases incluem o arquivo oficial
`Blender-4.5.9-Source.tar.xz` e seu checksum. [Origem, manifesto e preparação](BUNDLED_RUNTIME.md).

`starter-materials`: seis mapas CC0-1.0, Oak Veneer 03 (Jenelle van Heerden) e Marble 01 (Rob Tuytel), Poly Haven. Proveniência, escala e hashes em [catálogo](../starter-materials/catalog.json) e [avisos](../starter-materials/README.md). Licença dos assets: [Poly Haven](https://polyhaven.com/license). Não incluem renders promocionais, logotipos ou imagens do VDMax.

## Panoramas de luz distribuídos

`starter-environments` inclui Kiara 1 Dawn (Greg Zaal / Poly Haven), CC0-1.0, em HDR 1K. [Procedência, licença e hashes](../starter-environments/README.md). A validação usa FreeImage, já presente entre as dependências transitivas do viewport. O aplicativo agora a vincula diretamente para decodificar HDR sem converter os valores de luz para 8 bits. EXR usa diretamente OpenEXR/Imath, com leitura de canais FLOAT por linha. [Biblioteca OpenEXR](https://github.com/AcademySoftwareFoundation/openexr). [Licença FreeImage](https://freeimage.sourceforge.io/license.html).

## Modelos distribuídos

`starter-models`: seleção de 52 modelos do [Furniture Kit / Kenney](https://kenney.nl/assets/furniture-kit), CC0-1.0. [Licença original](../starter-models/LICENSE.txt), [procedência e conversão](../starter-models/README.md), hashes no catálogo. Modelos estilizados, sem assets VDMax. Os fontes JSON preparados são incluídos na distribuição e os modelos utilizados ficam no projeto .lmx.

Também inclui 24 modelos detalhados e 84 mapas preparados a partir de glTF da [Poly Haven](https://polyhaven.com/models/furniture), CC0-1.0. [Autores, URLs e hashes de origem](../starter-models/modern-provenance.json), [licença oficial](https://polyhaven.com/license) e [limites da conversão](../starter-models/README.md). A distribuição inclui os modelos e mapas preparados; os arquivos glTF originais são cache de desenvolvimento e não são dependência de execução. Renders promocionais, marcas e imagens de exemplo do site não foram incorporados.

O logo LibreMax foi gerado pela ferramenta de imagem integrada; [prompt e origem](../resources/brand/README.md). A imagem de apartamento usada na abertura é um render do próprio exemplo LibreMax, produzido com Cycles.

A coleção original LibreMax acrescenta 12 designs contemporâneos CC0, criados pelo script Blender do repositório. [Dedicação dos assets](../starter-models/CURRENT_LICENSE.txt), [proveniência](../starter-models/current-provenance.json). Não deriva de modelos de fabricantes ou do VDMax.

A ampliação 0.9 inclui 53 modelos do [KayKit Furniture Bits](https://github.com/KayKit-Game-Assets/KayKit-Furniture-Bits-1.0), de Kay Lousberg, CC0, na revisão `96d5930a8dbdb363409bbc2d3341718b00e17c9c`. [Licença preservada](../starter-models/KAYKIT_LICENSE.txt). Inclui também sete modelos Poly Haven adicionais, CC0. [URLs oficiais, autores, hashes de todos os arquivos de origem e preparados](../starter-models/expanded-provenance.json).
