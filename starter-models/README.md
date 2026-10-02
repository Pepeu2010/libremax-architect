# Móveis prontos

A biblioteca agora inclui **88 modelos prontos**: 52 Kenney, 24 Poly Haven e 12 designs originais LibreMax. O filtro **Apartamento atual** reúne os 36 modelos Poly Haven/LibreMax. Os catálogos anteriores mantêm seus IDs e receitas.

## Designs contemporâneos LibreMax

[current-catalog.json](current-catalog.json) acrescenta sofá compacto de 2,05 m, sofá com chaise, cama queen estofada, cabeceira ripada, mesa de cabeceira suspensa, aparador compacto, rack baixo, mesa redonda com base ripada, banqueta estofada, pufe de bouclé, espelho oval e luminária de piso com arco. Bordas arredondadas, almofadas, pés, ripas e molduras têm geometria própria; acabamentos de tecido, madeira, pedra e metal permanecem separados.

São designs originais CC0, com medidas sugeridas, sem reproduzir produtos de fabricantes. [Dedicação dos assets](CURRENT_LICENSE.txt), [hashes e medidas](current-provenance.json). Fonte reproduzível: `blender --background --factory-startup --python-exit-code 1 --python scripts/create-apartment-models.py`. Blender é usado apenas na preparação; inserir esses modelos no app não precisa dele.

Cabeceira, mesa suspensa e espelho exigem parede; mesa suspensa inicia a 45 cm e espelho a 95 cm. Os demais ficam no piso. Espelho usa metalicidade 1 e rugosidade 0,015 para reflexão no Cycles. Luminárias são geometria decorativa; fontes de luz são adicionadas separadamente. Os designs originais usam acabamentos procedurais no Cycles; não contêm fotografias de produtos ou mapas PBR digitalizados. Escalar muda também almofadas e ripas: não é edição paramétrica dos componentes.

## Apartamento atual — Poly Haven

24 modelos CC0: suculenta em vaso, mesa de jantar redonda, duas banquetas metálicas, cesto de fibras naturais, mesa de apoio alta, duas poltronas, duas estantes, duas mesas de centro, aparador, conjunto de mesa e cadeiras para varanda, cadeira de jantar, mesa redonda de mármore, mesa lateral, gaveteiro, escrivaninha, duas luminárias, dois vasos e almofadas. Fontes oficiais em [Poly Haven](https://polyhaven.com/models/furniture); licença [CC0](https://polyhaven.com/license). Autores, URLs, MD5 e SHA256 dos arquivos originais e SHA256 das malhas preparadas estão em [modern-provenance.json](modern-provenance.json). Não são modelos proprietários do VDMax.

Preparação: `python scripts/prepare-modern-models.py` com NumPy, Pillow e curl. Esse script de desenvolvimento baixa glTF e mapas de 1K da API oficial, verifica a integridade, aplica as transformações dos nós, converte Y-up para Z-up e preserva UVs, materiais e normais. O aplicativo usa os arquivos distribuídos, sem downloads. O catálogo antigo não é sobrescrito.

As medidas vêm dos limites geométricos do modelo original em metros, convertidos para milímetros. A estante steel_frame_shelves_01 tinha unidades de cena incompatíveis; seu catálogo usa medidas sugeridas de 900 × 411,8 × 1.755,4 mm. A proveniência preserva também as medidas brutas de origem. Projetos antigos mantêm suas medidas salvas. São medidas do arquivo, não certificados de produtos. As formas misturam linhas contemporâneas, madeira natural e estilo industrial. As peças não são móveis paramétricos: é possível mudar posição, rotação, escala e acabamento, mas não redesenhar seus componentes internos.

Os mapas de cor, rugosidade e normal são PNG de até 512 pixels por lado, 84 arquivos únicos no total. A metalicidade usa a média do canal original; oclusão, displacement e emissividade do material original não são importados. As miniaturas mostram a geometria e a cor média; o viewport usa a textura de cor e o Cycles usa os três mapas com o UV original. A luminária decorativa acompanha o teto, mas a fonte de luz é criada separadamente no menu Iluminação. O sofá do apartamento de exemplo agora usa o design original LibreMax, com almofadas e bordas arredondadas.

Plantas de alta densidade ficaram fora desta seleção para respeitar o limite de 4 MiB por malha. Não foi feita uma redução de polígonos que eliminasse detalhes apenas para aumentar a contagem.

## Kenney

52 modelos 3D reais, preparados a partir de **Furniture Kit, Kenney**, CC0. Fonte oficial: https://kenney.nl/assets/furniture-kit. Texto da licença original preservado em `LICENSE.txt`, com quebras de linha e espaços finais normalizados.

Arquivo baixado: `https://kenney.nl/media/pages/assets/furniture-kit/440e0608a4-1677580847/kenney_furniture-kit.zip`. SHA256 do ZIP: `e67652d0932cee41683f74711c03d3e192a2af9979ef8e6b237711f5482d46b0`. O pack contém 140 modelos; esta seleção contém 52 itens úteis para interiores, com nomes em português e dimensões sugeridas. As dimensões não são medidas certificadas de produtos de fabricantes.

Preparação reproduzível: `python scripts/prepare-furniture-pack.py <pasta-extraída>`. O script converte OBJ de Y-up para Z-up, normaliza os vértices, triangula as faces, remove faces repetidas/degeneradas, separa materiais e gera SHA256 por modelo. Acabamentos são adaptados aos presets do LibreMax. Arquivos do VDMax não fazem parte deste pacote.

Os modelos são **estilizados, com poucos polígonos**. Eles melhoram a variedade e o uso imediato da biblioteca; não são modelos de fotografia de produtos. Madeira, tecido, vidro e metal continuam separados. Escala por largura/altura/profundidade, posição, rotação, espelhamento e acabamento são editáveis; detalhes internos não são paramétricos como os módulos próprios de cozinha.

O catálogo vem junto com o aplicativo. O projeto incorpora os modelos usados por hash; salvar, abrir, renderizar e mover não dependem de internet, do ZIP original ou desta pasta. `MeshObject` usa triangulação no viewport OpenCASCADE e no snapshot do Cycles. Malhas decorativas não são sólidos CAD editáveis.
