# Assets e procedência

As 25 receitas próprias, porta e janela são originais, LibreMax contributors, CC0-1.0. `scripts/create_starter.py` reproduz esse catálogo; a geometria é gerada em C++ durante inserção.

Os 52 modelos prontos vêm do [Furniture Kit de Kenney](https://kenney.nl/assets/furniture-kit), CC0. São malhas efetivas de mesas, cadeiras, camas, sofás, aparelhos, louças sanitárias e decoração. A licença original, URL, hash do download, hash de cada modelo e conversão estão em [starter-models](../starter-models/README.md). Nenhum asset proprietário VDMax foi incorporado.

`scripts/prepare-furniture-pack.py` converte OBJ do pacote original para o formato normalizado LibreMax, transforma Y-up para Z-up, elimina faces repetidas/degeneradas, separa materiais e sugere medidas em milímetros. As cores usam presets LibreMax, não reprodução exata de MTL. Medidas sugeridas não são certificação de fabricante. O redimensionamento de malhas estica a geometria; módulos procedurais preservam recomposição paramétrica.

O catálogo agora soma 97 itens, incluindo 18 modelos detalhados [Poly Haven](https://polyhaven.com/models/furniture), CC0. Duas poltronas, estantes, mesas e decoração mantêm UVs, normais e mapas de cor/rugosidade/normal. Autores, hashes e URLs estão em [modern-provenance.json](../starter-models/modern-provenance.json). A coleção Kenney permanece estilizada. O sofá do exemplo moderno ainda é Kenney.

O catálogo continua abaixo da meta de 3.000 itens e da cobertura fotográfica de todos os móveis de um apartamento. Importadores de GLB/glTF/OBJ/STL/DAE/STEP/IGES/FBX pela interface e atualização remota de packs permanecem ausentes. As ferramentas de preparação de OBJ e glTF são ferramentas de desenvolvimento, não importadores integrados para usuários.
