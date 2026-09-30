# Materiais PBR locais

Dois materiais, seis mapas PNG 1K originais Poly Haven, licença CC0-1.0. Não há acesso à rede em runtime. `catalog.json` registra fonte, autores, escala física, canal, URL original e SHA256 dos bytes distribuídos.

- Carvalho: [Oak Veneer 03](https://polyhaven.com/a/oak_veneer_03), Jenelle van Heerden, tile 1000 × 1000 mm. UV gira 90° na ativação para veios verticais nas frentes.
- Pedra em placas: [Marble 01](https://polyhaven.com/a/marble_01), Rob Tuytel, tile 1500 × 1500 mm; inclui juntas fotografadas e é adequada a piso/revestimento, não a uma bancada contínua.

Cada material tem albedo (`sRGB`), rugosidade e normal OpenGL (`Non-Color`). No app, **Render → Ativar texturas reais** incorpora e normaliza os mapas como PNGs com hashes do projeto, atualizando carvalho/porcelanato. O arquivo `.lmx` permanece autossuficiente. Não depende dos arquivos desta pasta para reabrir/renderizar depois da incorporação. Undo restaura o estado anterior.

Fonte e licença verificadas em 2026-09-30: [Poly Haven asset license](https://polyhaven.com/license), [CC0-1.0 legal code](https://creativecommons.org/publicdomain/zero/1.0/legalcode). A API foi usada somente na preparação destes assets; o app não integra a API ao vivo. Código do aplicativo mantém GPL-3.0-or-later; estes mapas permanecem CC0-1.0.
