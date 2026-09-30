# Test report — ciclo 0.1.0

Execução local em 2026-09-29/30: Windows 11 Pro x86_64, Ryzen 5 5500 (6 cores/12 threads), 15,9 GiB de RAM. Toolchain isolado MSYS2 UCRT64: GCC 16.2.0, CMake 4.4.3, Ninja 1.13.2, Qt 6.11.2, OpenCASCADE 7.9.3, libzip 1.11.4, Catch2 3.16.0. Blender 5.2.1 LTS, executado como processo externo.

**Resultado: build nativo aprovado; 15 casos / 395 assertions aprovados; smoke de interface, encerramento forçado/recuperação e Cycles aprovados. Não é aceitação integral da master spec nem prova de suporte Linux.**

## Evidência executada

| Área | Exercício real | Resultado |
|---|---|---|
| Medidas | Expressões limitadas, vírgula decimal, rejeição de entradas inválidas | Passou |
| Comandos | Criação, UUIDs, undo/redo, alteração inválida preserva documento/histórico | Passou |
| Geometria | B-rep válido, volume de parede com recorte, abertura acompanha rotação da parede | Passou |
| Módulos | Largura de 947 mm, corpo de 18 mm, recomposição | Passou |
| Automações | União de tampo, recálculo por fontes, exclusão em cascata | Passou no conjunto reto coberto |
| Starter Library | Todas as 25 receitas distribuídas instanciadas; cada componente passa BRepCheck_Analyzer | Passou |
| Biblioteca | SQLite/FTS5, acentos, 10.000 fixtures, favoritos/recentes, consulta malformada | Passou |
| Persistência | Roundtrip, backup, validação antes da substituição, arquivo anterior preservado | Passou |
| Containers adversos | Caminho inesperado, ZIP truncado, UUID/órfão/ciclo inválido | Rejeitados |
| DXF | LINE, LWPOLYLINE reta, POLYLINE, ARC, CIRCLE; metros → mm; layers; save/open | Passou |
| Textura | JPG → PNG normalizado, deduplicação por SHA256, remoção do original e roundtrip | Passou |
| Integridade de assets | Bytes adulterados são rejeitados | Passou |
| Recuperação core | Cinco versões por UUID, dois projetos isolados, arquivo corrompido preservado/ignorado | Passou |
| Interface | Cliques desenham parede de 2.000 mm; biblioteca por duplo clique; eventos Qt de entrada/movimento/drop geram ghost e inserem a 1.000 × 1.000 mm; undo | Passou |
| Inspector | Clique em Aplicar com `753+59,5` → 812,5 mm; undo/redo; save/open idêntico | Passou |
| Crash recovery | Processo escritor cria autosave, recebe kill, novo processo usa diálogo nativo e restaura UUIDs/edição | Passou |
| Cycles | Snapshot de sólidos reais, processo Blender, CPU/denoise, PNG de 320 × 180 / 16 samples | Passou |
| Falha de render | Remoção da câmera causa exceção Python; aplicação informa Falhou; hash da imagem anterior permanece igual | Passou |
| Qualidade estática | Warnings habilitados, build sem warnings do C++; clang-format dry-run, Python py_compile, Bash syntax | Passou |
| Instalação em staging | `cmake --install` no Windows, recursos/licença copiados, smoke UI do executável instalado com DLLs do toolchain no PATH | Passou; não é instalador portátil nem QA Linux |

O teste de drop injeta eventos no viewport real e usa a biblioteca/engine reais; não substitui exploração manual de arraste entre janelas, múltiplos monitores ou plataformas. O smoke de crash testa encerramento **depois** de um autosave durável, sem simular perda de energia ou término durante escrita.

## Reproduzir neste host

```powershell
$env:PATH = "$env:USERPROFILE\.codex\tmp\lmx-tools\msys64\ucrt64\bin;$env:PATH"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure -V
./build/libremax-architect.exe --ui-smoke build/evidence
./build/libremax-architect.exe --recovery-smoke
./build/libremax-architect.exe --render-smoke build/render-evidence --blender 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe'
```

Logs locais: `build/evidence/core-report.txt`, `ui-report.txt`, `recovery-report.txt` e `build/render-evidence/render-report.txt`. Capturas reais versionadas: [1440 × 900](screenshots/native-ui.png), [1024 × 768](screenshots/native-ui-1024.png), [Cycles CPU 320 × 180](screenshots/cycles-kitchen.png). Os `.lmx` em `examples` são containers reais gerados pelo aplicativo.

## Correções encontradas pelos testes/revisão

- Temporário ZIP em Windows: uso de diretório temporário evitou bloqueio por handle de arquivo aberto.
- Cabeçalho `$INSUNITS` do DXF: pré-leitura resolveu unidade ignorada pelo parser de entidades.
- Transição SQLite/WAL: finalização da consulta de versão resolveu lock ativo.
- Render: arquivo temporário próprio, validação da resolução e cópia atômica impedem aceitar/reusar uma imagem antiga após erro. `--python-exit-code 1` propaga falhas do script.
- Recuperação: arquivos corrompidos deixaram de bloquear abertura; metadados da lista evitam reter todos os documentos em memória.
- Validação: famílias/dimensões decorativas, frente de vidro e metadados de material inválidos são rejeitados antes de aplicar o comando.
- Vista isométrica: opção Abrir vista permite inspecionar o interior; geometria do projeto/render permanece completa.

## Não comprovado

Linux/ARM64/Wayland nativo, AppImage/deb/Flatpak instalados, execução com rede desabilitada, crash durante ZIP write, perda de energia, timers de autosave ao longo de uma sessão extensa, escolha de múltiplas versões pela UI, importadores de malha/packs, fuzzing completo, GPU, JPG renderizado, qualidade 1080p/4K, vidro/espelho/textura em cenas de referência, todos os workflows da matriz e desempenho de cenas grandes. O Blender atual emite avisos de depreciação sobre `use_nodes` para Blender 6; não houve falha por esses avisos no 5.2.1.

CI Linux configurado, sem remoto e sem execução nesta sessão. [Matriz de paridade](VDMAX_ARCHITECT_PARITY.md) e [benchmark limitado](BENCHMARK_REPORT.md) complementam este relatório.
