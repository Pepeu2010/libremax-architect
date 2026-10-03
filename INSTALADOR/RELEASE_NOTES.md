# LibreMax Architect 0.10.0 — Iluminação mais simples

Baixe o arquivo do seu sistema em Assets:

- **Windows 64 bits:** `LibreMax-Architect-0.10.0-Windows-x64-Setup.exe`.
- **Ubuntu 24.04, x86-64:** `LibreMax-Architect-0.10.0-Linux-Ubuntu24.04-amd64.deb`. Instale com `sudo apt install ./LibreMax-Architect-0.10.0-Linux-Ubuntu24.04-amd64.deb`. Mint 22 compartilha essa base, mas não teve teste próprio.

Em **Iluminação → Nova luz**, escolha spot, fita LED, painel de luz, ponto ou sol. A fonte começa perto do teto do cômodo atual. Escolha Quente, Aconchegante, Neutra, Luz do dia, Fria ou uma temperatura Kelvin específica. Os tamanhos aparecem em centímetros; as propriedades incluem raio, feixe, formato e suavidade da sombra solar. Marcadores amarelos ajudam a localizar a fonte no editor e não aparecem na foto. O painel estreito apresenta rótulos acima dos controles, com rolagem vertical.

A fita LED é uma superfície emissiva contínua real no Cycles. Kelvin usa o cálculo nativo de cor do Blender. Brilho altera a intensidade na imagem; não equivale automaticamente ao consumo elétrico de uma lâmpada comercial.

Os recursos anteriores permanecem: **175 itens / 148 modelos 3D**, coleções leves e detalhadas, modos Leve/Equilibrado/Mais detalhes, tutorial de 15 capítulos, logo animada, projetos recentes, montagem com encaixe, fila e galeria Cycles, HDRI e EXR. Modelos e texturas acompanham o instalador. **Para renderizar, instale Blender 5.2 LTS+ separadamente** e selecione seu executável no painel Render.

**Projetos com as novas luzes usam `.lmx` v3 e exigem LibreMax 0.10+.** Projetos antigos v1/v2 continuam abrindo. Use Salvar como para conservar uma cópia anterior. HDRI, EXR e snapshots preservam v3; não há downgrade automático. Pedidos na fila arquivam também o tradutor de iluminação, permitindo repetir a cena original.

Verificação local: **37 testes / 2.233 verificações**. Cinco tipos criados/editados pela interface, undo/redo, arquivo portátil v3 e layout de 900 pixels passaram. Sete renders reais Cycles CPU compararam LED ligado/desligado e quente/frio, além de ponto, spot, área e sol/EXR. A RX 7600 deste Windows calculou o LED via HIP sem fallback, em 320 × 180 / 16 amostras. Biblioteca, montagem, tutorial, fila, HDRI e EXR passaram na regressão.

Os instaladores desta revisão são publicados somente após os testes Windows e Ubuntu. As execuções e resultados estão no [relatório de testes](https://github.com/Pepeu2010/libremax-architect/blob/main/docs/TEST_REPORT.md).

A [CI Linux](https://github.com/Pepeu2010/libremax-architect/actions/runs/37125774451) e a [CI dos instaladores Windows/Ubuntu](https://github.com/Pepeu2010/libremax-architect/actions/runs/37125785735) passaram. O pacote Linux em staging produziu as sete imagens Cycles CPU. O payload Windows também passou em criação/edição e renders CPU/HIP no host, com PATH sem SDK.

O apartamento moderno com Kelvin e LED produziu **3840 × 2160 / 512 amostras máximas / 12 reflexões**, com denoise e GPU/HIP confirmado. O motor levou 8min09s nesta RX 7600; o teste de falha preservou a imagem. Isso verifica uma saída 4K pelo pipeline, sem comprovar todos os materiais/câmeras nem o gesto de selecionar Final na galeria. [Imagem e reprodução](https://github.com/Pepeu2010/libremax-architect/blob/main/docs/LIGHTING.md).

Prévia em desenvolvimento. Não há comprovação de compatibilidade com todos os computadores, GPUs integradas, pouca RAM ou apartamentos grandes. Há pacotes Windows x86-64 e Ubuntu amd64; não há pacote macOS, ARM ou 32 bits. O viewport exige OpenGL e driver funcional. CPU permite renderizar sem backend GPU Cycles, desde que o Blender seja compatível com sistema/processador. Windows sem assinatura digital; Linux usa X11/XWayland.

Faltam requisitos da master spec, incluindo desfoque HDRI, canais PBR adicionais, enquadramento exato de câmera, instâncias e tesselação de apresentação. A coleção leve é estilizada. A saída 4K verificada não comprova equivalência a fotografia ou superioridade sobre concorrentes; outras GPUs e HIP no Linux continuam sem teste físico.

`LibreMax-Architect-Source.tar.gz` contém o código exato da revisão compilada `7c5a662e85a3b35c672f2065dbf459baf47b1fad`. Confira downloads com `SHA256SUMS.txt`. Dependências Windows têm manifesto e licenças em `share/doc/libremax-architect`. [Guias separados por sistema](https://github.com/Pepeu2010/libremax-architect/tree/main/INSTALADOR).
