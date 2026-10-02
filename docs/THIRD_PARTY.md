# Dependências e distribuição

| Dependência direta | Uso | Licença indicada pelo projeto |
|---|---|---|
| Qt 6 Core/Gui/Widgets/Sql/Concurrent/Test | UI/modelo/processos/testes | LGPL-3.0 / GPL-3.0 |
| OpenCASCADE | B-rep/viewport/tesselação | LGPL-2.1-or-later com OCCT exception |
| libzip | Container | BSD-3-Clause |
| SQLite (plugin Qt) | Biblioteca/FTS | Domínio público |
| nlohmann/json | Documento | MIT |
| spdlog/fmt | Logs | MIT |
| Catch2 | Testes | BSL-1.0 |
| Blender/Cycles | Processo externo | GPL |

Fontes: [Qt](https://doc.qt.io/qt-6/licensing.html), [OCCT](https://dev.opencascade.org/resources/licensing), [libzip](https://libzip.org/license/), [SQLite](https://sqlite.org/copyright.html), [JSON](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT), [spdlog](https://github.com/gabime/spdlog/blob/v1.x/LICENSE), [Catch2](https://github.com/catchorg/Catch2/blob/devel/LICENSE.txt), [Blender](https://www.blender.org/about/license/).

As dependências são vinculadas dinamicamente onde distribuídas, exceto core próprio. Binário final deve incluir avisos, licenças e fontes/ofertas aplicáveis. Dependências transitivas do MSYS2 incluem mais módulos do que o app usa; não distribuir toda a pasta de desenvolvimento. SBOM/auditoria dos pacotes finais ainda não executados. Assimp ainda não integrado.

## Texturas distribuídas

`starter-materials`: seis mapas CC0-1.0, Oak Veneer 03 (Jenelle van Heerden) e Marble 01 (Rob Tuytel), Poly Haven. Proveniência, escala e hashes em [catálogo](../starter-materials/catalog.json) e [avisos](../starter-materials/README.md). Licença dos assets: [Poly Haven](https://polyhaven.com/license). Não incluem renders promocionais, logotipos ou imagens do VDMax.

## Modelos distribuídos

`starter-models`: seleção de 52 modelos do [Furniture Kit / Kenney](https://kenney.nl/assets/furniture-kit), CC0-1.0. [Licença original](../starter-models/LICENSE.txt), [procedência e conversão](../starter-models/README.md), hashes no catálogo. Modelos estilizados, sem assets VDMax. Os fontes JSON preparados são incluídos na distribuição e os modelos utilizados ficam no projeto .lmx.

Também inclui 24 modelos detalhados e 84 mapas preparados a partir de glTF da [Poly Haven](https://polyhaven.com/models/furniture), CC0-1.0. [Autores, URLs e hashes de origem](../starter-models/modern-provenance.json), [licença oficial](https://polyhaven.com/license) e [limites da conversão](../starter-models/README.md). A distribuição inclui os modelos e mapas preparados; os arquivos glTF originais são cache de desenvolvimento e não são dependência de execução. Renders promocionais, marcas e imagens de exemplo do site não foram incorporados.

O logo LibreMax foi gerado pela ferramenta de imagem integrada; [prompt e origem](../resources/brand/README.md). A imagem de apartamento usada na abertura é um render do próprio exemplo LibreMax, produzido com Cycles.

A coleção original LibreMax acrescenta 12 designs contemporâneos CC0, criados pelo script Blender do repositório. [Dedicação dos assets](../starter-models/CURRENT_LICENSE.txt), [proveniência](../starter-models/current-provenance.json). Não deriva de modelos de fabricantes ou do VDMax.
