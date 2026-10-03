# LibreMax Architect 0.13.0 — Menos geometria repetida

Baixe o arquivo do seu sistema em Assets:

- **Windows 64 bits:** `LibreMax-Architect-0.13.0-Windows-x64-Setup.exe`.
- **Ubuntu 24.04, x86-64:** `LibreMax-Architect-0.13.0-Linux-Ubuntu24.04-amd64.deb`.

Mover e girar cópias reutiliza os sólidos e suas apresentações no editor. Cada objeto conserva posição, seleção, medidas e histórico independentes. O Cycles também compartilha malhas de modelos iguais, preservando UVs, normais e materiais. Texturas CAD antigas por posição no mundo mantêm o caminho anterior para conservar a aparência.

No Windows, passaram **43 testes / 2.798 verificações**, UI e montagem. O cenário de 32 cadeiras leves reduziu o pacote de render de 582 KB para 55 KB. Uma poltrona Poly Haven com mapas reais reduziu 32,8 MB para 4,2 MB, mantendo 65 partes em sete malhas. Renders Cycles CPU com Blender 4.5.9 e 5.2.1 passaram na comparação com o formato anterior. Pedidos antigos continuam repetíveis com seu script arquivado. [Provas e limites](https://github.com/Pepeu2010/libremax-architect/blob/main/docs/SHARED_GEOMETRY.md).

Os instaladores só são publicados depois dos gates Windows/Ubuntu. Esta prévia conserva biblioteca de 175 itens, encaixe, iluminação Kelvin/LED/sol, tutorial, projetos recentes, arquivo único, fila/galeria, progresso e estimativa, HDRI/EXR e compatibilidade de arquivos v1/v2/v3.

Alvo obrigatório: **i3-6006U / Intel HD Graphics 520 / 8 GB**. O teste físico de montagem, FPS, RAM e foto 1080p continua pendente. Compartilhar geometria e começar em Leve reduz trabalho, mas não garante desempenho naquele notebook. Para esse alvo, use Blender 4.5 LTS e CPU. A instalação do Blender é separada. Comece com Rápido, 640 × 360. [Aceitação do hardware](https://github.com/Pepeu2010/libremax-architect/blob/main/docs/MINIMUM_HARDWARE.md).

Prévia em desenvolvimento: não completa a master spec nem comprova qualidade fotográfica equivalente ao VDMax. Outras GPUs, distribuições e cenas grandes exigem testes próprios.

`LibreMax-Architect-Source.tar.gz` contém a fonte exata da tag. Confira `SHA256SUMS.txt`. [Guias separados por sistema](https://github.com/Pepeu2010/libremax-architect/tree/main/INSTALADOR).
