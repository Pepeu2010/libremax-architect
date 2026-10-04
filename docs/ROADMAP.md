# Roadmap — sem release 1.0

Este ciclo integra a fundação e subconjuntos reais dos blocos Ambiente, Biblioteca, Módulos, Automações, Materiais, DXF e Render. Nenhuma fase ampla é considerada concluída somente por existir código.

Próximo bloco prioritário: concluir a montagem de sala, quarto e cozinha reais,
com medidas, cantos, acabamento e apresentação que possam ser corrigidos pela interface.
Mais modelos ajudam, mas não substituem esses fluxos completos.

**Gate de hardware mínimo:** i3-6006U, Intel HD Graphics 520 e 8 GB de RAM. Montagem, arquivo portátil, edição durante render e foto CPU devem passar nesse notebook antes de afirmar suporte confirmado. [Caminho implementado e aceitação](MINIMUM_HARDWARE.md).

1. Completar os fluxos de montagem descritos nos itens seguintes. Ubuntu 24.04 já tem CI de build, núcleo, viewport Mesa, render CPU e instalação em runner novo; QA próprio de Mint/Debian e AppImage continuam pendentes.
2. Completar ambiente: junções T/X gerais, regiões fechadas, edição conjunta de paredes compartilhadas, cotas livres e snap, preview numérico, aberturas em planta, escadas L/U. Contornos, piso/forro associados e encontros de duas paredes foram implementados na fonte 0.15.
3. Biblioteca/UX: ampliar QA/performance e catálogo fotográfico, atualização remota de coleções, backup e formatos CAD. A fonte 0.15 inclui 203 itens, importação de seis formatos pela interface e `.lmaxpack` local validado.
4. Completar famílias/variantes/materiais, editor completo de textura/UV e normal maps em superfícies curvas, perfis/polígonos/booleanos/sancas e geometrias interiores.
5. Completar e validar cozinha e dormitório planejados: módulos de canto, famílias/variantes, acabamentos associados, fechamento de lacunas, tampos em L, envelopamento de conjunto e seleção automática por função.
6. Ampliar presets de câmera por cômodo, mapas de metalicidade/opacidade/emissão, desfoque opcional de HDRI e QA de sol/céu/exposição, cache e fila/galeria; validar EXR em Final 4K e ampliar prova visual de PBR/vidro/espelho/GPU/1080p. A fonte 0.17 já integra moldura e composição da câmera no editor, comparadas com a projeção real do Blender 4.5/5.2. [Provas e limites](EXECUTION_017.md).
7. Ampliar QA de recuperação para múltiplos projetos/versões e faltas de energia; caches/dirty flags/instancing/LOD e benchmark completo. Autosave configurável e recuperação após encerramento forçado passaram no host Windows.
8. Aceitação cozinha, dormitório, DXF e rede realmente desabilitada; diagnóstico, audit de dependências, source/SHA256, deb e Flatpak completos.

Fonte 0.13: protótipos de geometria e instâncias Cycles passaram nos testes
Windows, com comparação contra o leitor anterior. Seleção, posição e histórico
continuam independentes. [Provas e limites](SHARED_GEOMETRY.md). Próximos pontos
de desempenho implementados na fonte 0.15: validação por hash em cache limitado e
57 malhas de edição separadas das malhas de render. Mover/rotacionar cópias já
reutiliza apresentações; faltam ampliar compartilhamento e medir apartamentos maiores. O gate físico i3/HD 520
permanece obrigatório. O usuário confirmou disponibilidade das máquinas; siga o
[roteiro de teste](NOTEBOOK_VDMAX_TEST.md).

Não expandir para corte/nesting/peças/CNC/método construtivo/custos industriais. O gate 1.0 continua sendo o workflow completo e cada requisito essencial comprovado na matriz. O host local é Windows; CI remoto pode comprovar Ubuntu/Xvfb. Isso não substitui QA de Mint, Debian, Wayland e instalação em máquina limpa.

Fontes 0.8: HDRI incorporado e EXR com prévia/galeria foram implementados e testados no Windows. [Evidência e limites](HDRI_EXR.md). Isso não encerra os gates de materiais, luzes, câmera, desempenho ou hardware.
