# LibreMax Architect 0.12.0 — Caminho para computadores com gráfico integrado

Baixe o arquivo do seu sistema em Assets:

- **Windows 64 bits:** `LibreMax-Architect-0.12.0-Windows-x64-Setup.exe`.
- **Ubuntu 24.04, x86-64:** `LibreMax-Architect-0.12.0-Linux-Ubuntu24.04-amd64.deb`.

O alvo mínimo obrigatório passa a ser **Intel i3-6006U, Intel HD Graphics 520 e 8 GB de RAM**. Sem preferência anterior, CPUs com até quatro threads começam em **Leve**, com menos efeitos no editor e um worker de miniaturas. As fotos conservam os materiais e detalhes.

O aplicativo aceita **Blender 4.5 LTS** além da linha 5.2. Para a HD 520, selecione Blender 4.5 LTS e **CPU** no painel Criar imagem. O editor usa OpenGL e exige driver funcional. A HD 520 não tem backend Cycles oneAPI compatível. A instalação do Blender continua separada. Em quatro threads disponíveis, o processo solicita duas threads de render e usa prioridade reduzida para preservar recursos de edição. Céu natural usa Nishita no Blender 4.5 e Multiple Scattering no 5+; os resultados podem diferir.

Windows: **40 testes / 2.269 verificações** passaram. Blender oficial 4.5.9 CPU produziu o apartamento com materiais PBR, quatro imagens na fila, quatro HDRI/EXR e sete de iluminação Kelvin/LED/sol. Progresso, estimativa, cancelamento, histórico, biblioteca e montagem passaram. Blender 5.2.1 conservou o render CPU do apartamento. Os instaladores passam pelos testes de instalação Windows/Ubuntu antes de publicar. [Relatório](https://github.com/Pepeu2010/libremax-architect/blob/main/docs/TEST_REPORT.md).

**O notebook i3/HD 520 ainda precisa do teste físico de montagem, fluidez, RAM e foto 1080p.** Os testes desta publicação foram executados no host Ryzen/Radeon e em CI; não comprovam desempenho naquele notebook. Comece por Rápido, 640 × 360, ajuste câmera/luzes e depois calcule a apresentação. O tempo depende da cena e da CPU. [Requisito e roteiro de aceitação](https://github.com/Pepeu2010/libremax-architect/blob/main/docs/MINIMUM_HARDWARE.md).

Mantém 175 itens / 148 modelos 3D, encaixe, iluminação, tutorial, projetos recentes, fila/galeria, barra e previsão, HDRI/EXR e arquivos v1/v2/v3. Prévia em desenvolvimento; não completa a master spec. Pacotes Windows x86-64 e Ubuntu 24.04 amd64. Outras GPUs, pouca RAM e cenas grandes continuam exigindo testes próprios.

`LibreMax-Architect-Source.tar.gz` contém o código exato da tag. Confira os arquivos com `SHA256SUMS.txt`. [Guias separados por sistema](https://github.com/Pepeu2010/libremax-architect/tree/main/INSTALADOR).
