# Dependências do instalador Windows

O aplicativo é GPL-3.0-or-later. O arquivo de fontes anexado à Release corresponde ao aplicativo daquela tag, incluindo scripts de compilação e empacotamento. Modelos e texturas têm proveniência em `starter-models`, `starter-materials` e `docs/THIRD_PARTY.md`.

Bibliotecas nativas são vinculadas dinamicamente. O empacotador inspeciona importações PE e inclui a cadeia de DLLs necessária do prefixo MSYS2 UCRT64 usado no build. Não copia todo o ambiente de desenvolvimento.

Na instalação:

- `share/doc/libremax-architect/runtime-manifest.json`: nomes e versões dos pacotes, licenças declaradas pelo MSYS2, endereços upstream, página do pacote e hashes dos binários.
- `share/doc/libremax-architect/dependencies/`: avisos e licenças disponíveis em `share/licenses` no prefixo de origem.
- `share/doc/libremax-architect/LICENSE`: licença do LibreMax.
- `share/libremax/runtime/blender/`: distribuição oficial Blender 4.5.9 LTS completa, incluindo Cycles, Python, `blender.crt` e `blender.shared`.
- `share/libremax/runtime/blender/libremax-runtime.json`: versão, URLs de origem e hashes do motor incluído; licenças em `license/`. Fontes oficiais em `Blender-4.5.9-Source.tar.xz`, anexado à mesma Release.

As páginas de pacotes no manifesto apontam para os projetos originais e receitas de build do [MSYS2 MINGW-packages](https://github.com/msys2/MINGW-packages). Para reproduzir uma biblioteca, use a receita e fontes correspondentes à versão registrada; o pacote atual pode ser diferente da versão da Release. O manifesto não substitui os termos das licenças, e o arquivo de fontes do LibreMax não contém as fontes de todas as bibliotecas.

O Qt é incluído como DLLs, com plugins `qwindows`, `qoffscreen`, `qsqlite`, `qjpeg` e `qico`. As DLLs podem ser substituídas por versões compatíveis; não há assinatura que bloqueie substituição. Uma auditoria jurídica completa da distribuição não foi realizada.
