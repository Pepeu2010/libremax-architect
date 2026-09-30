# Ciclo 02 — interface e apresentação

**IMPLEMENTADO:** tema Qt escuro Ateliê Graphite; ícones próprios e indicadores incorporados; duas faixas de comandos; biblioteca com 25 miniaturas geradas da geometria em workers/cache; propriedades contextuais com rolagem; câmera por UUID, exposição, ambiente e denoise persistidos; cor/spot/área no inspector; workspace central de render com Ajustar/1:1/zoom/pan/exportação. Cycles recebeu superfícies procedurais, micro relevo, bevel de shader, AgX, luzes de área e composição da cozinha original revisada.

**TESTADO:** build Release; 17 casos/408 assertions; todas as 25 miniaturas; edição via teclado `753+59,5` → 812,5 mm, parede/drop/ghost/undo; câmera/exposição pela UI e save/open; capturas reais em 1440×900, 1024×768 e 900×650. Em 900 px, inspector e Render sem rolagem horizontal; Aplicar e Renderizar alcançáveis por rolagem vertical. Cycles CPU gerou 960×540/64 samples; visualizador Ajustar/1:1 e exportação PNG conservam os pixels. Encerramento forçado/recuperação novamente aprovado. Falha Python sem câmera preserva o hash do render anterior.

**CORRIGIDO:** miniaturas mostravam o verso das peças; título do projeto ficava comprimido; controles escuros tinham setas invisíveis; inspector e Render exigiam rolagem horizontal em 900 px; composição inicial cortava o aéreo. O teste do diálogo de exportação foi corrigido para preencher o nome com eventos de teclado, pois `selectFile` não alterava o campo já focado no diálogo visível.

**REFERÊNCIAS:** 11 imagens públicas realmente inspecionadas e 36 apenas indexadas. [Auditoria e mapeamento](VISUAL_REFERENCE_AUDIT.md). Nenhuma imagem/textura/modelo de terceiros do pack foi incorporada como asset. [Paridade](VDMAX_ARCHITECT_PARITY.md) continua indicando cobertura parcial.

**PENDENTE:** Linux, GPU, 1080p/4K, JPEG pela UI, cenas específicas de spot/vidro/espelho, desempenho de milhares de miniaturas e exploração manual completa. A master spec ainda possui regiões/junções/cotas, geometria livre/sancas, importadores/packs/catálogo extenso, gizmos/grupos e galeria/fila persistente pendentes. Não é release 1.0.

**REPRODUZIR NESTE HOST:**

```powershell
$env:PATH = "$env:USERPROFILE\.codex\tmp\lmx-tools\msys64\ucrt64\bin;$env:PATH"
cmake --build build -j 4
ctest --test-dir build -V
./build/libremax-architect.exe --render-smoke build/studio-render --render-size 960x540 --render-samples 64 --blender 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe'
./build/libremax-architect.exe --ui-smoke build/studio-evidence --preview-image build/studio-render/cycles-kitchen.png
./build/libremax-architect.exe --recovery-smoke
```

Logs locais ignorados: `build/studio-test-report.txt`, `studio-ui-report.txt`, `studio-render-report.txt` e `studio-recovery-report.txt`. Capturas selecionadas em [screenshots](screenshots/native-ui.png). Toolchain/limites no [relatório de testes](TEST_REPORT.md). Fonte e exemplos são empacotados de HEAD pelo script `package-source.ps1`; os arquivos não são instaladores Linux.
