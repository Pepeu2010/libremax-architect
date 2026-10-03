# LibreMax Architect 0.9.0 — Mais modelos e modos de edição

Baixe o arquivo do seu sistema em Assets:

- **Windows 64 bits:** `LibreMax-Architect-0.9.0-Windows-x64-Setup.exe`.
- **Ubuntu 24.04, x86-64:** `LibreMax-Architect-0.9.0-Linux-Ubuntu24.04-amd64.deb`. Instale com `sudo apt install ./LibreMax-Architect-0.9.0-Linux-Ubuntu24.04-amd64.deb`. Mint 22 compartilha essa base, mas não teve teste próprio.

O catálogo passa de 115 para **175 itens**, incluindo **148 modelos 3D prontos**. São 60 novos modelos CC0: 53 KayKit estilizados e sete Poly Haven com texturas. Sofás, camas, cadeiras, mesas, prateleiras, tapetes, quadros, plantas e decoração acompanham o instalador. Não precisam de download ao inserir. As medidas preparadas são sugestões para apartamentos.

Na biblioteca, escolha **Modelos leves**, **Apartamento atual** ou **Objetos detalhados**. No menu **Vista → Desempenho durante edição**, escolha Leve, Equilibrado ou Mais detalhes. O modo Leve reduz texturas e suavização na tela e limita a geração de miniaturas a um worker. A foto final mantém as malhas e os materiais originais. A escolha fica salva neste computador.

A atualização também evita recalcular todo o catálogo a cada miniatura concluída. Os modelos e as texturas usados continuam incorporados ao arquivo único `.lmx`.

Os recursos anteriores permanecem: tutorial de 15 capítulos, logo animada, projetos recentes, montagem com encaixe, fila e galeria Cycles, iluminação HDRI e exportação EXR. **Para fotos realistas, instale Blender 5.2 LTS+ separadamente** e selecione seu executável no painel Render.

Verificação local: 35 testes e 2.191 verificações no Windows. O fluxo nativo passou com 175 miniaturas, três modos, colocação de sofá no piso e relógio na parede, salvamento e reabertura. Também passou com o processo limitado a dois núcleos lógicos, mantendo a RX 7600. Esse ensaio não representa um computador antigo completo.

Os instaladores desta revisão são publicados somente após os testes de instalação Windows e Ubuntu. As execuções e resultados estão no [relatório de testes](https://github.com/Pepeu2010/libremax-architect/blob/main/docs/TEST_REPORT.md).

Prévia em desenvolvimento. Compatibilidade com todos os computadores, GPUs integradas, pouca RAM e grandes apartamentos não está comprovada. Há pacotes Windows x86-64 e Ubuntu amd64; não há pacote macOS, ARM ou 32 bits. O viewport precisa de OpenGL e driver funcional. CPU no render permite usar GPUs sem backend Cycles compatível, desde que o Blender seja compatível com o sistema e o processador. Windows sem assinatura digital; Linux usa X11/XWayland.

Faltam recursos da master spec, incluindo desfoque HDRI, todos os canais PBR e controles de luz/câmera. A coleção leve é estilizada. Não há prova de qualidade Final 4K ou superioridade sobre concorrentes.

Projetos v2 com HDRI/EXR exigem LibreMax 0.8+. O instalador não converte seus projetos automaticamente.

`LibreMax-Architect-Source.tar.gz` contém o código exato da revisão compilada `8ae679d43c5df1bb417d6663792b1a47124ce8d6`. Confira os downloads com `SHA256SUMS.txt`. As dependências Windows têm manifesto e licenças em `share/doc/libremax-architect`. [Guias separados por sistema](https://github.com/Pepeu2010/libremax-architect/tree/main/INSTALADOR).
