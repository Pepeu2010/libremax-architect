# Benchmark report — limites do ciclo 0.1.0

Host e versões em [TEST_REPORT](TEST_REPORT.md). Build Release, Windows/UCRT64. Nenhuma extrapolação para Linux ou desempenho de produto completo.

| Medida | Entrada e método | Resultado observado |
|---|---|---|
| Busca FTS5 | Banco temporário com 10.000 fixtures sintéticas; consulta `armario`, uma correspondência com acento; QElapsedTimer ao redor de `Library::search` | 0,4138 ms na última execução |
| Suite core | 15 casos, 395 assertions; geometria, 25 receitas, SQLite, DXF, textures, ZIP e recuperação | CTest: 1,10 s total na última execução |
| Render smoke | Cozinha de exemplo, Cycles CPU, denoise, 320 × 180, 16 samples | Imagem válida; Blender informou save aos 7,578 s em execução concorrente com outros testes |

A medida FTS é uma amostra local após inserção/transação do catálogo, sem limpar caches do sistema. Não há distribuição estatística/p95 nem medição de busca concorrente; o limite de teste é <100 ms. **10.000 fixtures não significam 10.000 assets de produto**: a biblioteca distribuída tem 25 receitas próprias.

O tempo informado pelo Blender não mede o workflow completo (tesselação, startup, cópia e interação) nem oferece comparação entre CPUs/GPU. A imagem pequena foi usada para verificar integração, e não como render final de apresentação.

Pendentes: FPS/latência/memória em 100/500/5.000 objetos, catálogos com thumbnails e meshes reais, cold start, importação em massa, footprint de undo/texturas, responsividade durante operações de CAD, render 1080p/4K, GPU e Linux. Recomposição do viewport e snapshots integrais de undo ainda podem ser caros em cenas grandes. Esses gates permanecem abertos no [ROADMAP](ROADMAP.md).
