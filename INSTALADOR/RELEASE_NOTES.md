# LibreMax Architect 0.11.0 — Progresso e tempo de render

Baixe o arquivo do seu sistema em Assets:

- **Windows 64 bits:** `LibreMax-Architect-0.11.0-Windows-x64-Setup.exe`.
- **Ubuntu 24.04, x86-64:** `LibreMax-Architect-0.11.0-Linux-Ubuntu24.04-amd64.deb`. Instale com `sudo apt install ./LibreMax-Architect-0.11.0-Linux-Ubuntu24.04-amd64.deb`.

Ao gerar uma imagem, **Suas imagens** mostra uma barra visível, o tempo decorrido, o restante estimado e o horário aproximado de terminar o cálculo. Esses tempos também aparecem no painel **Criar imagem** enquanto você edita o apartamento. O histórico guarda a duração da execução após terminar, cancelar ou falhar.

A previsão vem do Cycles e pode mudar durante o render. Preparação sem estimativa mostra atividade; denoise e salvamento mostram finalização. O horário previsto se refere ao cálculo, com finalização depois. Em imagens divididas em partes, acabar a primeira parte não completa a barra; 100% exige uma saída válida. Uma tentativa reiniciada com CPU reinicia progresso e previsão. A galeria em janela de 900 pixels preserva a barra e as duas linhas de tempo; voltar ao projeto restaura as ferramentas de montagem.

Verificação local Windows: **40 testes / 2.269 verificações** passaram. O teste nativo usou Cycles real, confirmou a evolução do relógio, previsão positiva, duração persistida, layouts de 1440 e 900 pixels, cancelamento, falha, repetição com CPU e preservação dos snapshots. [Capturas e detalhes](https://github.com/Pepeu2010/libremax-architect/blob/main/docs/RENDER_PROGRESS.md). Os instaladores são publicados após os testes Windows e Ubuntu da CI; execuções e limites ficam no [relatório](https://github.com/Pepeu2010/libremax-architect/blob/main/docs/TEST_REPORT.md).

Inclui os recursos anteriores: **175 itens / 148 modelos 3D**, coleção Apartamento atual, modos Leve/Equilibrado/Mais detalhes, montagem com encaixe, Kelvin/LED/sol, tutorial, projetos recentes, fila/galeria, HDRI e EXR. Modelos, texturas e exemplos acompanham o instalador. **Para renderizar, instale Blender 5.2 LTS+ separadamente** e selecione seu executável no painel Render. Projetos `.lmx` v1/v2/v3 continuam suportados; luzes v3 exigem LibreMax 0.10+.

Prévia em desenvolvimento. Pacotes Windows x86-64 e Ubuntu 24.04 amd64; Mint 22 compartilha a base, sem teste próprio. O viewport exige OpenGL e driver funcional. Não há comprovação para todos os computadores, GPUs, pouca RAM ou apartamentos grandes. A revisão anterior produziu uma saída 4K na RX 7600/HIP; esta alteração não repete esse teste nem comprova equivalência fotográfica ou paridade integral com a master spec.

`LibreMax-Architect-Source.tar.gz` contém o código exato da tag desta publicação. Confira downloads com `SHA256SUMS.txt`. Dependências Windows têm manifesto e licenças em `share/doc/libremax-architect`. [Guias separados por sistema](https://github.com/Pepeu2010/libremax-architect/tree/main/INSTALADOR).
