# Formato `.lmx` v1

ZIP com `manifest.json`, `project.json`, `scene.json`, `materials.json`, `lighting.json`, `cameras.json` e opcionais `custom-assets/<sha256>.png`.

Manifest: `format=LibreMax`, `version=1`, UUID do projeto e lista ordenada de hashes incorporados. Project: UUID/nome/unidade/versão e `renderSettings` opcional no formato v1: câmera por UUID, exposição EV, intensidade ambiente e denoise. Projetos v1 anteriores usam valores padrão quando o campo está ausente. Câmera removida limpa a referência; undo restaura. Valores fora dos limites ou câmera inexistente são recusados. Scene: entidades e filhos derivados. Lighting/cameras são índices redundantes validados contra scene. Materiais referenciam hashes; bytes essenciais de texturas estão dentro do ZIP. Não armazenar apenas o caminho original.

Geometria interna em mm, entrada numérica arredondada para 0,1 mm. Ângulos em graus. Estado completo de comandos também inclui assets incorporados. Essas cópias podem consumir memória em projetos com muitas imagens.

Até 128 entradas, 16 MiB por entrada, 64 MiB descompactados, 128 MiB no arquivo; nomes essenciais ou padrão estrito de hash PNG. Sem extração em caminhos arbitrários. PNGs incorporados precisam corresponder ao SHA256. JSON máximo 64 níveis; validação semântica exige referências, dimensões e enumerações válidas.

Save: validar documento → gerar ZIP temporário → reabrir/validar equivalência → copiar versão anterior para `.bak` atomicamente → QSaveFile → flush → fsync/_commit → atomic commit. No POSIX também fsync do diretório. Não há teste de queda física de energia; testes atuais cobrem falha de documento inválido e container truncado. Versões futuras são recusadas, preservando o arquivo. Migrations reais só serão introduzidas quando existir v2.

Galeria/renders/miniaturas e modelos 3D externos ainda não são incorporados. Autosave escreve containers completos em diretório de recuperação, mantendo cinco por UUID. A recuperação oferece seleção de projeto/versão quando há várias cópias. Save manual e fechamento normal limpam somente os snapshots do UUID atual; arquivos corrompidos são ignorados e preservados. Testes de múltiplas versões pela UI ainda precisam ser ampliados.
