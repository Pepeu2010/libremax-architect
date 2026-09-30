# Render Cycles

Criar luz/câmera pelos menus. No painel Render escolher executável Blender, qualidade, CPU ou seleção automática. O render usa atualmente a primeira câmera visível; renomear/duplicar/excluir pode ocorrer pela árvore e inspector, mas seletor explícito ainda falta.

Snapshot validado → worker de tesselação B-rep → JSON/PNGs locais → QProcess `blender --background --factory-startup --python-exit-code 1 --python scripts/cycles_render.py -- ...` → Cycles → imagem. Sem shell, `system()` ou Python gerado com texto do projeto. O projeto pode ser editado enquanto o processo renderiza; snapshot é uma cópia estável.

AUTO tenta OptiX, CUDA, HIP, oneAPI e finalmente CPU. CPU é escolha explícita para máquinas sem GPU compatível. Denoise ligado. Rascunho 640×360/16, Preview 1280×720/64, Alta 1920×1080/256, Final 3840×2160/512. PNG/JPEG conforme extensão. Job único, cancelável; não há fila multijob, galeria embutida no `.lmx` ou retomada de sessão.

Neste host o smoke reproduz Cycles CPU/denoise/320×180/16. Não é evidência de GPU, 1080p, vidro/espelho perfeitos, exposição final ou todos os requisitos de render. Render completo da spec permanece IN_PROGRESS.

A saída é gerada no diretório temporário do job, lida/validada quanto à resolução e copiada atomicamente ao destino. Falha de Python/processo não reutiliza uma imagem antiga; o teste sem câmera confirmou a preservação do hash anterior.
