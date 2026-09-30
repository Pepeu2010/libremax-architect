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
