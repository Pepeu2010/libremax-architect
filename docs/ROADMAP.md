# Roadmap — sem release 1.0

Este ciclo integra a fundação e subconjuntos reais dos blocos Ambiente, Biblioteca, Módulos, Automações, Materiais, DXF e Render. Nenhuma fase ampla é considerada concluída somente por existir código.

Próximo bloco prioritário:

1. Executar build/testes/viewport em Linux Mint/Ubuntu/Debian, resolver compatibilidade e produzir primeiro AppImage validado.
2. Completar ambiente: junções L/T/X, regiões fechadas, piso/forro associativos, cotas e snap, preview numérico, aberturas em planta, escadas L/U.
3. Biblioteca/UX: ampliar QA/performance de thumbnails reais já implementados, modelos fotográficos detalhados além das 52 malhas estilizadas, coleções, importadores 3D pela interface, `.lmaxpack` seguro, backup e instalação offline.
4. Completar famílias/variantes/materiais, editor completo de textura/UV e normal maps em superfícies curvas, perfis/polígonos/booleanos/sancas e geometrias interiores.
5. Validar automações em cozinhas com cantos, fechamento de lacunas, envelopamento de conjunto e seleção automática por função.
6. Ampliar presets de câmera por cômodo, LED/emissão, desfoque opcional de HDRI e ampliar QA de sol/céu já implementados, ampliar QA da exposição já implementada, política de cache e QA ampliado da fila/galeria já integradas, validar EXR em Final 4K e ampliar prova visual de PBR/vidro/espelho/GPU/1080p.
7. Ampliar QA de recuperação para múltiplos projetos/versões e faltas de energia; caches/dirty flags/instancing/LOD e benchmark completo. Autosave configurável e recuperação após encerramento forçado passaram no host Windows.
8. Aceitação cozinha, dormitório, DXF e rede realmente desabilitada; diagnóstico, audit de dependências, source/SHA256, deb e Flatpak completos.

Não expandir para corte/nesting/peças/CNC/método construtivo/custos industriais. O gate 1.0 continua sendo o workflow completo e cada requisito essencial comprovado na matriz. O host local é Windows; CI remoto pode comprovar Ubuntu/Xvfb. Isso não substitui QA de Mint, Debian, Wayland e instalação em máquina limpa.

Fontes 0.8: HDRI incorporado e EXR com prévia/galeria foram implementados e testados no Windows. [Evidência e limites](HDRI_EXR.md). Isso não encerra os gates de materiais, luzes, câmera, desempenho ou hardware.
