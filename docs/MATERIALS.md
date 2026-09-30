# Materiais e texturas

Presets: pintura, branco, carvalho (cor visual), pedra, vidro, inox, espelho e tecido. Cada material guarda cor, roughness, metallic, transmission, IOR e opacity. Cor carvalho não representa uma textura de madeira distribuída.

Arquivo → Importar textura JPG/PNG normaliza imagem para PNG num worker, calcula SHA256, deduplica e incorpora ao documento. Se houver seleção desbloqueada, aplica; caso contrário cria material selecionável no inspector. O arquivo original pode ser removido. Limites: arquivo 16 MiB, dimensão máxima 8192 por eixo e 16 milhões de pixels; armazenamento convertido limitado a 16 MiB.

Viewport usa AIS_TexturedShape; Cycles usa imagem incorporada com projeção BOX e coordenadas do mundo em metros, escala padrão 1000 mm. A equivalência de UV entre viewport e render ainda precisa de validação quantitativa. Campos de rotação/offset/escala existem no modelo de material e no exporter, mas ainda não têm editor visual completo. Normal maps, emissão, biblioteca de materiais, EXR e modelos de vidro especializados ainda faltam.

Espelho é material metálico com roughness reduzida em Cycles. O viewport AIS padrão não oferece reflexos ray-traced; não declarar espelho completo nas duas vistas.
