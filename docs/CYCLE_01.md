# Ciclo 01 — fundação nativa integrada

**IMPLEMENTADO:** C++20/Qt Widgets/OpenCASCADE; documento único em mm; paredes/muretas e ambiente retangular; aberturas booleanas; módulos paramétricos; biblioteca SQLite/FTS5 e 25 receitas próprias; automações por fontes; DXF ASCII; textura incorporada; ZIP `.lmx` atômico/backup; histórico; autosave configurável/recuperação; iluminação/câmeras e processo Blender/Cycles.

**TESTADO:** build Release; 15 casos/395 assertions; interface nativa em 1440×900 e 1024×768; edição de expressão e biblioteca com drop/ghost; save/open; processo morto e recuperação em novo processo; imagem Cycles e falha que preserva arquivo anterior. [Relatório](TEST_REPORT.md).

**CORRIGIDO:** bloqueio do temporário ZIP, leitura de unidade DXF, consulta SQLite antes de WAL, render antigo aceito após falha, abertura interrompida por autosave corrompido, dimensões decorativas/materiais inválidos e visualização do interior.

**PENDENTE:** regiões/junções/cotas/snap completo, escadas L/U, perfis/sancas/geometria livre, importadores 3D, packs/thumbnails/catálogo extenso, grupos/gizmos/alinhamento, automações de cantos e lacunas, UV/emissão/LED/sol/exposição, gerenciador de câmeras/fila/galeria, performance, segurança/fuzzing completo, offline com rede desabilitada e distribuição Linux. [Roadmap](ROADMAP.md).

**ARQUIVOS ALTERADOS:** `apps/architect`, `libs/{core,document,commands,geometry,persistence,library,import,materials,viewport,rendering}`, `tests`, `resources`, `scripts`, `starter-library`, `examples`, `docs`, CMake, CI e manifest Flatpak preliminar. Não existia implementação anterior; não há código Marceneiro/fábrica/CNC a remover.

**COMANDOS DE TESTE:** [BUILDING](BUILDING.md) e [TEST_REPORT](TEST_REPORT.md). Logs/smokes em `build`, ignorado pelo Git; capturas reais selecionadas versionadas em `docs/screenshots`.

**PARIDADE ATUAL:** 54 linhas: 17 FUNCTIONAL, 29 IN_PROGRESS, 7 NOT_STARTED, 1 BLOCKED, 0 VERIFIED. São estados de linhas amplas, não percentual de conclusão; os testes não cobrem tudo que cada linha exige. **0.1.0 de desenvolvimento, sem release 1.0.**

**PRÓXIMO BLOCO:** validar viewport/build/core/smokes em Linux e gerar pacote instalável; em paralelo ao gate de plataforma, completar ambiente (regiões fechadas, junções e cotas) antes de ampliar catálogo/render. Não encerrar o objetivo geral com este ciclo.
