# Móveis prontos

52 modelos 3D reais, preparados a partir de **Furniture Kit, Kenney**, CC0. Fonte oficial: https://kenney.nl/assets/furniture-kit. Texto da licença original preservado em `LICENSE.txt`, com quebras de linha e espaços finais normalizados.

Arquivo baixado: `https://kenney.nl/media/pages/assets/furniture-kit/440e0608a4-1677580847/kenney_furniture-kit.zip`. SHA256 do ZIP: `e67652d0932cee41683f74711c03d3e192a2af9979ef8e6b237711f5482d46b0`. O pack contém 140 modelos; esta seleção contém 52 itens úteis para interiores, com nomes em português e dimensões sugeridas. As dimensões não são medidas certificadas de produtos de fabricantes.

Preparação reproduzível: `python scripts/prepare-furniture-pack.py <pasta-extraída>`. O script converte OBJ de Y-up para Z-up, normaliza os vértices, triangula as faces, remove faces repetidas/degeneradas, separa materiais e gera SHA256 por modelo. Acabamentos são adaptados aos presets do LibreMax. Arquivos do VDMax não fazem parte deste pacote.

Os modelos são **estilizados, com poucos polígonos**. Eles melhoram a variedade e o uso imediato da biblioteca; não são modelos de fotografia de produtos. Madeira, tecido, vidro e metal continuam separados. Escala por largura/altura/profundidade, posição, rotação, espelhamento e acabamento são editáveis; detalhes internos não são paramétricos como os módulos próprios de cozinha.

O catálogo vem junto com o aplicativo. O projeto incorpora os modelos usados por hash; salvar, abrir, renderizar e mover não dependem de internet, do ZIP original ou desta pasta. `MeshObject` usa triangulação no viewport OpenCASCADE e no snapshot do Cycles. Malhas decorativas não são sólidos CAD editáveis.
