# LibreMax Architect 0.6.1 — Prévia dos instaladores

Escolha um dos arquivos abaixo em Assets:

- **Windows:** `LibreMax-Architect-0.6.1-Windows-x64-Setup.exe`. Instalador em português, atalhos e desinstalação; bibliotecas incluídas.
- **Linux:** `LibreMax-Architect-0.6.1-Linux-Ubuntu24.04-amd64.deb`. Ubuntu 24.04 / base do Mint 22, x86-64; dependências instaladas pelo apt.

Inclui tutorial, logo com abertura breve, projetos recentes, exemplos, 115 itens no catálogo e coleção Apartamento atual com 36 modelos detalhados. Salve projetos em um único arquivo `.lmx`.

Para render com Cycles, instale o Blender 5.2 LTS+ e selecione seu executável no painel Render. Ele é executado em segundo plano. O Blender não está incluído neste instalador.

Os jobs desta Release verificam o instalador Windows com PATH sem ferramentas de desenvolvimento e o pacote Linux em um runner novo, incluindo SQLite/FTS, modelos, codecs de imagem, salvamento/reabertura e desinstalação. A interface Linux é verificada com Xvfb/Mesa. Esses testes não comprovam desempenho em todos os computadores ou GPUs. Mint 22 não teve execução própria.

Prévia em desenvolvimento; faltam recursos da master spec. Windows sem assinatura digital nesta versão. Wayland usa XWayland. O render atual ainda não contém a nova fila/galeria persistente solicitada.

`LibreMax-Architect-Source.tar.gz` contém o código correspondente. `SHA256SUMS.txt` permite conferir os downloads. O instalador Windows também inclui licenças disponíveis e um manifesto de dependências em `share/doc/libremax-architect/runtime-manifest.json`; veja [licenças e fontes das dependências](https://github.com/Pepeu2010/libremax-architect/blob/main/INSTALADOR/windows/DEPENDENCIES.md).
