# LibreMax Architect 0.14.0 — Blender já incluído

Baixe o arquivo do seu sistema em Assets:

- **Windows 64 bits:** `LibreMax-Architect-0.14.0-Windows-x64-Setup.exe`.
- **Ubuntu 24.04, x86-64:** `LibreMax-Architect-0.14.0-Linux-Ubuntu24.04-amd64.deb`.

O instalador agora inclui **Blender 4.5.9 LTS/Cycles**, Python interno, bibliotecas e licenças. O LibreMax encontra o motor automaticamente: instale, abra um projeto e escolha Criar imagem. Não precisa baixar, abrir ou selecionar Blender separadamente. Uma escolha explícita nos ajustes continua permitindo usar uma instalação externa.

O Windows inclui também o runtime Microsoft da distribuição oficial Blender e as DLLs Qt/OpenCASCADE do aplicativo. O `.deb` inclui Blender e declara as bibliotecas do sistema; o gerenciador Ubuntu instala as dependências faltantes, podendo precisar de internet. Edição e render funcionam localmente depois da instalação. [Distribuição e integridade](https://github.com/Pepeu2010/libremax-architect/blob/main/docs/BUNDLED_RUNTIME.md).

Os instaladores só são publicados depois dos gates Windows/Ubuntu. Eles executam Cycles CPU com o motor incluído sem indicar um executável externo, verificam recursos e removem o aplicativo preservando um projeto. O Windows testa com PATH limitado ao sistema; o Ubuntu instala em um runner separado sem SDK. Esta prévia conserva biblioteca de 175 itens, encaixe, geometria compartilhada, iluminação Kelvin/LED/sol, tutorial, projetos recentes, arquivo único, fila/galeria, progresso e estimativa, HDRI/EXR e compatibilidade de arquivos v1/v2/v3.

Alvo obrigatório: **i3-6006U / Intel HD Graphics 520 / 8 GB**. O teste físico de montagem, FPS, RAM e foto 1080p continua pendente. Use o Blender incluído com CPU e comece com Rápido, 640 × 360. [Aceitação do hardware](https://github.com/Pepeu2010/libremax-architect/blob/main/docs/MINIMUM_HARDWARE.md).

Prévia em desenvolvimento: não completa a master spec nem comprova qualidade fotográfica equivalente ao VDMax. Outras GPUs, distribuições e cenas grandes exigem testes próprios.

`LibreMax-Architect-Source.tar.gz` contém a fonte exata da tag. `Blender-4.5.9-Source.tar.xz` contém a fonte oficial do motor incluído. Confira `SHA256SUMS.txt`. [Guias separados por sistema](https://github.com/Pepeu2010/libremax-architect/tree/main/INSTALADOR).
