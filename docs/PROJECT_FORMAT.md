# Formato `.lmx` v1

ZIP com `manifest.json`, `project.json`, `scene.json`, `materials.json`, `lighting.json`, `cameras.json` e opcionais `custom-assets/<sha256>.png` / `custom-models/<sha256>.json`.

Manifest: `format=LibreMax`, `version=1`, UUID do projeto e lista ordenada de hashes incorporados. Project: UUID/nome/unidade/versão e `renderSettings` opcional no formato v1: câmera por UUID, exposição EV, intensidade ambiente e denoise. Projetos v1 anteriores usam valores padrão quando o campo está ausente. Câmera removida limpa a referência; undo restaura. Valores fora dos limites ou câmera inexistente são recusados. Scene: entidades e filhos derivados. Lighting/cameras são índices redundantes validados contra scene. Materiais referenciam hashes; bytes essenciais de texturas estão dentro do ZIP. Não armazenar apenas o caminho original.

Geometria interna em mm, entrada numérica arredondada para 0,1 mm. Ângulos em graus. Estado completo de comandos também inclui assets incorporados. Essas cópias podem consumir memória em projetos com muitas imagens.

Até 128 entradas, 16 MiB por entrada, 64 MiB descompactados, 128 MiB no arquivo; nomes essenciais ou padrões estritos de hash PNG/modelo JSON. Sem extração em caminhos arbitrários. Todos os assets incorporados precisam corresponder ao SHA256. JSON máximo 64 níveis; validação semântica exige referências, dimensões e enumerações válidas.

Save: validar documento → gerar ZIP temporário → reabrir/validar equivalência → copiar versão anterior para `.bak` atomicamente → QSaveFile → flush → fsync/_commit → atomic commit. No POSIX também fsync do diretório. Não há teste de queda física de energia; testes atuais cobrem falha de documento inválido e container truncado. Versões futuras são recusadas, preservando o arquivo. Migrations reais só serão introduzidas quando existir v2.

Galeria/renders/miniaturas ainda não são incorporados. As malhas do catálogo são incorporadas desde 0.4. Autosave escreve containers completos em diretório de recuperação, mantendo cinco por UUID. A recuperação oferece seleção de projeto/versão quando há várias cópias. Save manual e fechamento normal limpam somente os snapshots do UUID atual; arquivos corrompidos são ignorados e preservados. Testes de múltiplas versões pela UI ainda precisam ser ampliados.

0.3 adiciona campos opcionais v1: `environmentMode` (studio/sky), `sunElevation` (1–89°), `sunRotation` (0–360°), parâmetros de câmera `fstop` e `focusDistance`, referências `roughnessTexture`/`normalTexture` e `normalStrength` nos materiais. Os mapas são PNGs com hashes incorporados; referências ausentes/inválidas são rejeitadas. Arquivos anteriores usam defaults no renderer/inspector. Nenhum caminho de mapa externo é salvo como dependência essencial.

0.4 estende o formato v1 com entidades `MeshObject`, `meshAsset` e modelos JSON normalizados em `custom-models`. Arquivos anteriores continuam abrindo; aplicativos LibreMax anteriores a 0.4 não aceitam MeshObject. Cada modelo tem até 4 MiB, profundidade JSON 12, 64 partes, 100.000 vértices por parte e 200.000 triângulos no total. Coordenadas devem ser finitas em [0,1], índices válidos e materiais existentes. O documento conserva limite total de 48 MiB de assets, além dos limites ZIP. Transformação e dimensões são externas à malha; cores por parte podem ser substituídas por acabamento único.
