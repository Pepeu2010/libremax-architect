# Móveis prontos

A biblioteca agora inclui **70 modelos prontos**: os 52 modelos Kenney abaixo e uma coleção de 18 modelos detalhados Poly Haven. A coleção nova está em [modern-catalog.json](modern-catalog.json); a antiga mantém seus IDs e receitas.

## Apartamento atual — Poly Haven

18 modelos CC0: duas poltronas, duas estantes, duas mesas de centro, aparador, conjunto de mesa e cadeiras para varanda, cadeira de jantar, mesa redonda de mármore, mesa lateral, gaveteiro, escrivaninha, duas luminárias, dois vasos e almofadas. Fontes oficiais em [Poly Haven](https://polyhaven.com/models/furniture); licença [CC0](https://polyhaven.com/license). Autores, URLs, MD5 e SHA256 dos arquivos originais e SHA256 das malhas preparadas estão em [modern-provenance.json](modern-provenance.json). Não são modelos proprietários do VDMax.

Preparação: `python scripts/prepare-modern-models.py` com NumPy, Pillow e curl. Esse script de desenvolvimento baixa glTF e mapas de 1K da API oficial, verifica a integridade, aplica as transformações dos nós, converte Y-up para Z-up e preserva UVs, materiais e normais. O aplicativo usa os arquivos distribuídos, sem downloads. O catálogo antigo não é sobrescrito.

As medidas vêm dos limites geométricos do modelo original em metros, convertidos para milímetros. São medidas do arquivo, não certificados de produtos. As formas misturam linhas contemporâneas, madeira natural e estilo industrial. As peças não são móveis paramétricos: é possível mudar posição, rotação, escala e acabamento, mas não redesenhar seus componentes internos.

Os mapas de cor, rugosidade e normal são PNG de até 512 pixels por lado, 66 arquivos únicos no total. A metalicidade usa a média do canal original; oclusão, displacement e emissividade do material original não são importados. As miniaturas mostram a geometria e a cor média; o viewport usa a textura de cor e o Cycles usa os três mapas com o UV original. A luminária decorativa acompanha o teto, mas a fonte de luz é criada separadamente no menu Iluminação. O sofá do apartamento de exemplo ainda vem da coleção estilizada Kenney; esta coleção não adiciona um sofá fotográfico.

Plantas de alta densidade ficaram fora desta seleção para respeitar o limite de 4 MiB por malha. Não foi feita uma redução de polígonos que eliminasse detalhes apenas para aumentar a contagem.

## Kenney

52 modelos 3D reais, preparados a partir de **Furniture Kit, Kenney**, CC0. Fonte oficial: https://kenney.nl/assets/furniture-kit. Texto da licença original preservado em `LICENSE.txt`, com quebras de linha e espaços finais normalizados.

Arquivo baixado: `https://kenney.nl/media/pages/assets/furniture-kit/440e0608a4-1677580847/kenney_furniture-kit.zip`. SHA256 do ZIP: `e67652d0932cee41683f74711c03d3e192a2af9979ef8e6b237711f5482d46b0`. O pack contém 140 modelos; esta seleção contém 52 itens úteis para interiores, com nomes em português e dimensões sugeridas. As dimensões não são medidas certificadas de produtos de fabricantes.

Preparação reproduzível: `python scripts/prepare-furniture-pack.py <pasta-extraída>`. O script converte OBJ de Y-up para Z-up, normaliza os vértices, triangula as faces, remove faces repetidas/degeneradas, separa materiais e gera SHA256 por modelo. Acabamentos são adaptados aos presets do LibreMax. Arquivos do VDMax não fazem parte deste pacote.

Os modelos são **estilizados, com poucos polígonos**. Eles melhoram a variedade e o uso imediato da biblioteca; não são modelos de fotografia de produtos. Madeira, tecido, vidro e metal continuam separados. Escala por largura/altura/profundidade, posição, rotação, espelhamento e acabamento são editáveis; detalhes internos não são paramétricos como os módulos próprios de cozinha.

O catálogo vem junto com o aplicativo. O projeto incorpora os modelos usados por hash; salvar, abrir, renderizar e mover não dependem de internet, do ZIP original ou desta pasta. `MeshObject` usa triangulação no viewport OpenCASCADE e no snapshot do Cycles. Malhas decorativas não são sólidos CAD editáveis.
