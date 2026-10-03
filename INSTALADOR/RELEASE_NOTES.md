# LibreMax Architect 0.8.0 — Prévia HDRI e EXR

Baixe o arquivo do seu sistema em Assets:

- **Windows 64 bits:** `LibreMax-Architect-0.8.0-Windows-x64-Setup.exe`. Instalação em português, atalhos e bibliotecas incluídas.
- **Ubuntu 24.04 / base do Linux Mint 22, x86-64:** `LibreMax-Architect-0.8.0-Linux-Ubuntu24.04-amd64.deb`. Instale com `sudo apt install ./LibreMax-Architect-0.8.0-Linux-Ubuntu24.04-amd64.deb`. Mint 22 não teve teste próprio.

Esta prévia inclui os recursos anteriores: tutorial completo de primeira abertura, logo animada, biblioteca de projetos recentes, arquivo único `.lmx`, exemplos, 115 itens e 36 modelos da coleção Apartamento atual.

Novidades:

- Importar iluminação HDR/EXR ou usar o panorama de luz do dia incluído. Girar a luz e mostrar/ocultar o panorama na imagem.
- Salvar o panorama dentro do projeto portátil v2, com importação em segundo plano. Leitura dos projetos v1 preservada.
- Exportar EXR linear FLOAT32 no modo Personalizado, com prévia PNG na galeria. Salvar cópia preserva o EXR original.
- Publicação e cópia das imagens em segundo plano, mantendo a edição disponível.

**Para fotos realistas, instale o Blender 5.2 LTS+ separadamente** e selecione seu executável no painel Render. O LibreMax usa Cycles real em segundo plano; Blender não acompanha o instalador.

Verificações desta revisão:

- Núcleo Windows: 34 casos e 1.829 assertions; interface, tutorial, montagem, modelos, recuperação, fila e galeria passaram.
- CPU real no Windows e Ubuntu: quatro renders de HDRI/EXR, rotação medida nos pixels, transparência, galeria persistente e cópia do original.
- Apartamento com HDRI na RX 7600 / HIP deste Windows: 640 × 360 / 32 amostras, sem fallback CPU.
- [CI Linux](https://github.com/Pepeu2010/libremax-architect/actions/runs/37089024012): Qt 6.4 / OpenEXR 3.1, interface Mesa/Xvfb e pacote Debian.
- [CI dos instaladores](https://github.com/Pepeu2010/libremax-architect/actions/runs/37089055737): instalação Windows com PATH sem SDK, instalação Linux em runner novo e desinstalação preservando projetos.

Prévia em desenvolvimento. Faltam recursos da master spec, incluindo desfoque HDRI, todos os canais PBR e controles de luz/câmera. Não há prova de qualidade Final 4K ou superioridade sobre concorrentes. Outras GPUs físicas e outros sistemas ainda precisam de testes. Windows sem assinatura digital; Linux usa X11/XWayland.

Projetos v2 com HDRI/EXR exigem LibreMax 0.8+. Versões antigas recusam esse formato. O instalador não converte seus projetos automaticamente.

`LibreMax-Architect-Source.tar.gz` contém o código exato da revisão compilada `ed2e007fef2d51e3f16237faad73d5ef0ce75954`. Confira os downloads com `SHA256SUMS.txt`. As dependências Windows têm manifesto e licenças em `share/doc/libremax-architect`. [Guias separados por sistema](https://github.com/Pepeu2010/libremax-architect/tree/main/INSTALADOR).
