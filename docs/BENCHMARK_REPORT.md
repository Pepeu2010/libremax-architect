# Benchmark report — limites do ciclo 0.6.0

No host Windows, reutilizar o cache da cena pequena de cozinha sem mudanças geométricas levou 0,2029 ms, contra 111,726 ms para construí-lo na primeira chamada. O teste também verifica invalidação de parede/abertura/automações. O construtor da home levou 181 ms; essa medida exclui Qt/DLLs. Modelos detalhados e seus 66 mapas somam 31,12 MiB distribuídos; o cache de payload mantém até 48 MiB. Cycles CPU 960×540/64 informou salvamento aos 55,969 s. Esses números são amostras locais, não p95, FPS, memória total ou comparação com VDMax. [CYCLE_06](CYCLE_06.md).

Busca sem payload, miniaturas em workers e atualização incremental do viewport foram implementadas; ainda falta benchmark de cenas 100/500/5.000 objetos, interação prolongada, footprint de undo e GPU/Linux.

---

# Benchmark report — limites do ciclo 0.3.0

Host e versões em [TEST_REPORT](TEST_REPORT.md). Build Release, Windows/UCRT64. Nenhuma extrapolação para Linux ou desempenho de produto completo.

| Medida | Entrada e método | Resultado observado |
|---|---|---|
| Busca FTS5 | Banco temporário com 10.000 fixtures sintéticas; consulta `armario`, uma correspondência com acento; QElapsedTimer ao redor de `Library::search` | 1,7564 ms na última execução |
| Suite core | 18 casos, 463 assertions; geometria, 25 receitas, SQLite, DXF, textures, ZIP e recuperação | CTest: 6,30 s total na última execução |
| Render smoke | Cozinha de exemplo, Cycles CPU, denoise, 1280 × 720, 128 samples | Imagem válida; Blender informou save aos 147,187 s em execução concorrente com outros testes |

A medida FTS é uma amostra local após inserção/transação do catálogo, sem limpar caches do sistema. Não há distribuição estatística/p95 nem medição de busca concorrente; o limite de teste é <100 ms. **10.000 fixtures não significam 10.000 assets de produto**: a biblioteca distribuída tem 25 receitas próprias.

O tempo informado pelo Blender não mede o workflow completo (tesselação, startup, cópia e interação) nem oferece comparação entre CPUs/GPU. A imagem intermediária foi usada para verificar integração, e não como render final de apresentação.

Pendentes: FPS/latência/memória em 100/500/5.000 objetos, catálogos com thumbnails e meshes reais, cold start, importação em massa, footprint de undo/texturas, responsividade durante operações de CAD, render 1080p/4K, GPU e Linux. Recomposição do viewport e snapshots integrais de undo ainda podem ser caros em cenas grandes. Esses gates permanecem abertos no [ROADMAP](ROADMAP.md).
