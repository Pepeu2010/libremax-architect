# Ciclo 0.4 — montar um apartamento

Implementado e verificado em 30/09–01/10/2026. A [pesquisa oficial](VDMAX_RESEARCH_04.md) orienta o fluxo de montagem; não há paridade integral com VDMax nem release 1.0.

## Entrega utilizável

- Arrasto do catálogo em planta e 3D sobre piso/parede; posição, orientação e geometria finais na prévia.
- Encaixe em cantos e vizinhos, colisão aproximada, recusa de área externa/posição sem espaço e indicação verde/vermelha.
- Movimento direto de objetos existentes, um comando por movimento e undo/redo; giro centrado; duplo clique espera o lugar escolhido.
- Porta/janela do catálogo com associação e recorte real na parede; pequenos objetos apoiados em móveis; tapetes sob móveis.
- 79 itens disponíveis: 25 receitas originais, duas aberturas e 52 modelos Kenney CC0. Modelos e miniaturas reais, offline, sem imagens usadas como substituição de geometria.
- Malhas incorporadas por SHA256 no .lmx, dimensões/transformação/acabamentos editáveis e mesma geometria no Cycles.
- Cômodos retangulares adjacentes, paredes coincidentes reaproveitadas, busca/foco por cômodo e câmera por cômodo.
- Galeria do catálogo, árvore em outra aba, medidas comuns em cm e criação de cômodos em metros; ajustes técnicos recolhidos.
- `examples/apartamento.lmx`: sala/cozinha, quarto e banheiro, móveis prontos, portas/janela, forros, luzes de teto, câmera e seis mapas PBR incorporados.

## Evidência executada

Host Windows, GCC 16.2, Qt 6.11.2, OpenCASCADE 7.9.3, Blender 5.2.1 LTS. Build Release sem warnings do C++ nos arquivos recompilados.

| Verificação | Resultado |
|---|---|
| Core | 22 casos / 1.054 assertions aprovados; encaixe em paredes inclinadas, lados internos, cantos/vizinhos, colisão, aberturas, apoio, tapetes, cômodos compartilhados, modelos e persistência |
| `--assembly-smoke` | 79 miniaturas; ghost/drop real em planta e parede 3D; área externa recusada; movimento por eventos mouse com undo/redo; janela vinculada; apartamento de três cômodos salvo/reaberto |
| Interface em 900 px | Catálogo em duas colunas, inspector sem rolagem horizontal, comandos e viewport alcançáveis |
| `--ui-smoke` | Regressão de desenho, colocação por clique/drop, PBR/céu, expressões em cm, undo/redo e save/open |
| `--recovery-smoke` | Escritor encerrado à força; processo novo recuperou UUID e edição de 812,5 mm pelo diálogo nativo |
| Cycles CPU | Imagem real 640×360 / 32 samples; teste posterior sem câmera falhou corretamente e preservou o hash da imagem anterior |
| Pacote de modelos | Hashes dos 52 modelos, 10.518 triângulos sem faces geométricas repetidas e hashes dos assets no apartamento conferidos |
| Qualidade estática | Todos os C++ passam clang-format 23.1.1; scripts Python compilam; sintaxe Bash/PowerShell e git diff --check aprovados |

Capturas reais: [planta](screenshots/apartment-plan.png), [3D](screenshots/apartment-3d.png), [900 px](screenshots/apartment-900.png), [encaixe](screenshots/wall-placement.png), [prévia Cycles](screenshots/apartment-render.png). Logs reproduzíveis locais em `build/assembly-*-report.txt`; não fazem parte dos pacotes de fontes.

O render revelou faces duplicadas de sentidos opostos nos OBJ originais. A preparação remove essas faces antes de distribuir os modelos; a imagem final foi inspecionada novamente. A biblioteca é estilizada e não demonstra equivalência a fotografia física.

## Reproduzir neste host

```powershell
$env:PATH = "$env:USERPROFILE\.codex\tmp\lmx-tools\msys64\ucrt64\bin;$env:PATH"
cmake --build build -j 4
ctest --test-dir build --output-on-failure
./build/libremax-architect.exe --assembly-smoke build/assembly-evidence
./build/libremax-architect.exe --ui-smoke build/assembly-regression
./build/libremax-architect.exe --recovery-smoke
./build/libremax-architect.exe --render-smoke build/assembly-render-final --render-project build/assembly-evidence/apartamento.lmx --render-size 640x360 --render-samples 32 --blender 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe'
./scripts/run-windows.ps1 ./examples/apartamento.lmx
```

O CLI de render testa deliberadamente uma câmera ausente após produzir a imagem. O último estado `Falhou` nesse teste representa a prova de preservação, não falha da imagem inicial.

## Limites e distribuição

Colisão por retângulos orientados/altura, não malha exata. Sem cálculo do giro da folha, circulação acessível ou redistribuição após editar parede. Apoio usa o topo do volume. Assistente limitado a cômodos retangulares no mesmo nível. Modelos prontos usam escala geométrica, não recomposição paramétrica. Sem importador 3D pela interface, catálogo fotográfico extenso, milhares de modelos ou atualização remota de packs. [Guia e limites](ASSEMBLY.md).

CI Ubuntu/Xvfb foi atualizado para instalar zipcmp/zipmerge/ziptool exigidos pelo CMake libzip e fixar clang-format 23.1.1. Resultado remoto deve ser consultado no repositório; evidência local não confirma execução Linux. QA Mint/Debian/Wayland, GPU, 1080p/4K e instalador Windows portátil permanecem pendentes.

`scripts/package-source.ps1` gera fontes, starter-library, starter-materials, starter-models, exemplos e SHA256 a partir do commit. Não publica binários Windows nem inclui arquivos locais de build ou referências proprietárias VDMax.
