# Ciclo 03 — render com aparência fotográfica

**IMPLEMENTADO:** seis mapas PBR CC0 de carvalho/pedra, com proveniência e SHA256; ativação assíncrona e incorporação em `.lmx`; UV por face em escala física; mapas normal OpenGL e roughness Non-Color; céu natural Multiple Scattering, altura/direção do sol; câmera com f/ e foco em mm; bevel geométrico de apresentação e weighted normals; denoise mais preciso; puxadores cilíndricos com suportes; quartzo e cerâmica; cozinha original com revestimento de pedra, madeira e luz da janela.

**TESTADO:** build Release no host Windows existente; **18 casos / 463 assertions**. Pack real de dois materiais/seis mapas, ZIP autossuficiente, snapshot com seis assets, undo/redo, caminho de escape e SHA adulterado recusados, normal ausente e céu inválido rejeitados. Core e catálogo de 25 receitas continuam passando BRepCheck. UI ativou seis mapas pelo botão real, alterou o sol e persistiu configurações; edição em mm/drop/ghost/save/open e viewport em 1440/1024/900 px. Visualizador real Ajustar/1:1/exportação PNG preserva pixels; painéis sem rolagem horizontal em 900 px e ações alcançáveis por rolagem vertical. Recuperação após encerramento forçado aprovada.

**RENDER REAL:** Cycles CPU, Blender 5.2.1 LTS, 1280×720 / 128 samples / denoise. Log reconheceu seis mapas, UV em escala física, céu natural e f/8. Blender informou save aos **2 min 27,187 s**, em execução concorrente com build/testes e geração dos exemplos; não é tempo garantido nem benchmark comparativo. Falha proposital sem câmera preservou o hash da imagem anterior. [Imagem gerada](screenshots/photoreal-kitchen.png), [workspace nativo](screenshots/render-workspace.png).

**CORRIGIDO POR INSPEÇÃO:** superfícies uniformes receberam mapas; puxadores sólidos simplificados ganharam haste/suportes; o vaso passou a respeitar o material da entidade; vidro refletia uma luz de preenchimento artificial excessiva, e o exemplo final usa só iluminação do céu/sol; a câmera foi nivelada e recomposta; bevel de paredes/forro abria frestas de luz, agora esses encontros preservam a geometria fechada. Exposição final do exemplo: +1,6 EV.

**LIMITES:** os modelos procedurais ainda têm detalhes limitados; aparência fotográfica depende da cena. Bevels são de apresentação e não alteram o B-rep. Janelas usam transmissão de shadow rays aproximada, sem caustics completas. Sem displacement geométrico, HDRI, validação GPU/Linux, presets Foto 1080p/4K, comparação com fotografia física ou garantia de imagem indistinguível de fotografia. Não é conclusão da master spec nem release 1.0.

**REPRODUZIR:**

```powershell
$env:PATH = "$env:USERPROFILE\.codex\tmp\lmx-tools\msys64\ucrt64\bin;$env:PATH"
cmake --build build -j 4
ctest --test-dir build -V
./build/libremax-architect.exe --render-smoke build/photoreal-final --render-size 1280x720 --render-samples 128 --blender 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe'
./build/libremax-architect.exe --ui-smoke build/photoreal-evidence --preview-image build/photoreal-final/cycles-kitchen.png
./build/libremax-architect.exe --recovery-smoke
```

Logs locais: `build/photoreal-{core,render,ui,recovery}-report.txt`. Assets/avisos em [starter-materials](../starter-materials/README.md), [guia](RENDERING.md), [paridade](VDMAX_ARCHITECT_PARITY.md). Pacotes de fonte, biblioteca, materiais e exemplos usam HEAD commitado e SHA256; não são instaladores Linux.
