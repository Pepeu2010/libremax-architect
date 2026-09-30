# Render Cycles — 0.2.0

Crie luzes/câmeras pelos menus. No painel Render selecione a câmera do projeto, qualidade, exposição (-8 a +8 EV), intensidade ambiente (0–5), denoise e CPU/seleção automática. A câmera é referenciada por UUID; câmera, exposição, ambiente e denoise entram no histórico e são persistidos em `.lmx`. Qualidade e dispositivo são escolhas do job. Configurar Blender revela o caminho do executável quando necessário.

Snapshot validado → worker de tesselação B-rep → JSON/PNGs locais → QProcess `blender --background --factory-startup --python-exit-code 1 --python scripts/cycles_render.py -- ...` → Cycles → imagem. Sem shell, `system()` ou Python gerado com texto do projeto. O snapshot permite continuar editando enquanto o processo renderiza.

AUTO tenta OptiX, CUDA, HIP, oneAPI e finalmente CPU. CPU é explícito. Presets: Rascunho 640×360/16, Prévia 1280×720/64, Alta 1920×1080/256, Final 3840×2160/512. AgX/Medium High Contrast, adaptive sampling e iluminação de área; madeira/pedra procedurais e micro relevo. Essas escolhas são configuradas no script, sem copiar texturas ou cenas de terceiros. Point/spot/area exportam os parâmetros do projeto; o inspector oferece cor, tamanho de área, ângulo e suavidade de spot.

O resultado ocupa a área central: Ajustar, 1:1, roda para zoom, arraste para pan e Salvar cópia PNG/JPEG. Alternar para Planta/3D mantém o último resultado acessível em Imagem. Preparando/Renderizando exibem progresso indeterminado real, Concluído/Falhou/Cancelado refletem o job; detalhes técnicos ficam recolhidos. Job único cancelável, sem fila multijob/galeria persistente no projeto.

Neste host foi executado Cycles CPU/denoise **960×540/64 samples**, Blender 5.2.1 LTS. A composição da cozinha foi inspecionada visualmente; todos os modelos e materiais são do projeto. O visualizador foi exercitado com essa imagem e exportou PNG com os mesmos pixels. Não comprova GPU, 1080p/4K, spot visual, todos os vidros/espelhos, Linux ou paridade completa de render.

A imagem é gerada no temporário do job, validada quanto à resolução e copiada atomicamente ao destino. Falha Python/processo preserva a imagem anterior; a exceção sem câmera foi verificada pelo smoke. Blender 5.2 informa depreciação de `use_nodes` para Blender 6; compatibilidade com essa versão futura não foi validada.

[Workspace real](screenshots/render-workspace.png), [imagem de cozinha](screenshots/cycles-kitchen.png), [referências consultadas](VISUAL_REFERENCE_AUDIT.md).
