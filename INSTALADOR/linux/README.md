# Linux

Pacote inicial para **Ubuntu 24.04 e Linux Mint 22, x86-64**. Outras distribuições ainda não foram validadas.

1. Abra a [Release dos instaladores](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.13.0-preview.1).
2. Baixe **LibreMax-Architect-0.13.0-Linux-Ubuntu24.04-amd64.deb**.
3. Abra o pacote no instalador de aplicativos da sua distribuição. Se ele não oferecer essa opção, use no Terminal, na pasta do download:

```bash
sudo apt install ./LibreMax-Architect-0.13.0-Linux-Ubuntu24.04-amd64.deb
```

O gerenciador instala as bibliotecas necessárias. Depois, procure **LibreMax Architect** no menu de aplicativos. Modelos, texturas e exemplos já acompanham o pacote.

O atalho usa X11/XWayland. Wayland nativo ainda não foi validado. Para abrir pelo Terminal:

```bash
QT_QPA_PLATFORM=xcb libremax-architect
```

Para desinstalar:

```bash
sudo apt remove libremax-architect
```

Seus arquivos `.lmx` e preferências são preservados. O pacote foi projetado para teste de instalação em Ubuntu 24.04; Mint 22 compartilha essa base, mas não possui teste próprio nesta entrega.

Para fotos realistas, instale o Blender 4.5 LTS ou 5.2 LTS separadamente e selecione seu executável no painel Render.

Para o alvo mínimo **i3-6006U / HD 520 / 8 GB**, use [Blender 4.5 LTS](https://www.blender.org/download/lts/4-5/) com dispositivo **CPU** e edição em **Leve**. O render e o editor foram testados em CI; desempenho na HD 520 depende da validação física e do driver Mesa/Intel. [Requisito de aceitação](../../docs/MINIMUM_HARDWARE.md).
