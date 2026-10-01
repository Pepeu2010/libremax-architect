# Changelog

## 0.4.0 — 2026-10-01 — montagem de apartamento, em desenvolvimento

- Pesquisa do manual e suporte oficiais VDMax para montagem contextual.
- Prévia na parede/piso, cantos e vizinhos, recusa de colisão/espaço externo, portas/janelas vinculadas e apoio de pequenos objetos.
- Arrasto de móveis existentes com undo/redo; duplo clique espera escolher o lugar; giro da prévia e do móvel.
- Catálogo offline com 52 malhas Kenney CC0, 25 receitas próprias e duas aberturas; 79 miniaturas reais.
- Malhas com cores por parte incorporadas no .lmx, validadas por SHA256, compartilhadas entre viewport e Cycles.
- Cômodos adjacentes, paredes compartilhadas, apartamento pronto e câmera por cômodo.
- Catálogo em galeria, árvore em outra aba, medidas comuns em cm, ajustes técnicos recolhidos e comandos curtos.
- Testes nativos de montagem em planta/3D e telas de 900 px; correção de dependências libzip e versão do formatador no CI.

## 0.3.0 — 2026-09-30 — desenvolvimento, sem release

- Materiais PBR locais CC0 com albedo/roughness/normal, hashes, escala e incorporação em .lmx.
- Céu natural/sol editáveis e câmera com abertura/foco fotográficos.
- UV por face, bevel de apresentação, weighted normals e denoise mais preciso.
- Puxadores cilíndricos com suportes e cozinha original revisada com pedra/cerâmica.
- Presets de apresentação com mais samples; testes de pack, roundtrip e controles nativos.

## 0.2.0 — 2026-09-30 — desenvolvimento, sem release

- Workspace Qt escuro, faixas de comandos, ícones próprios, foco e indicadores legíveis.
- Miniaturas reais assíncronas com cache; inspector com rolagem vertical em 900 px.
- Câmera por UUID, exposição/ambiente/denoise persistidos, luz spot/cor e área.
- Render central com Ajustar/1:1/zoom/pan/exportar cópia, estado real do job e logs recolhidos.
- Shaders procedurais de madeira/pedra/tecido/pintura, AgX e exemplo de cozinha revisado.
- Auditoria de 11 imagens das 47 referências e testes de render real/roundtrip/miniaturas.

## 0.1.0 — 2026-09-29 — desenvolvimento, sem release

- Pesquisa clean-room, baseline e matriz de paridade de Arquitetos e Decoradores.
- Core C++20: documentos/UUIDs, milímetros, expressões numéricas e comandos undo/redo.
- Editor nativo Qt/OpenCASCADE, paredes/muretas, aberturas booleanas e ambiente retangular.
- Biblioteca SQLite/FTS5, 25 receitas próprias, favoritos/recentes e inserção.
- Módulos recompostos parametricamente, frentes, puxadores, vidro e presets PBR.
- Automações sobre UUIDs de fontes, recálculo e exclusão em cascata.
- `.lmx` ZIP, validação, gravação atômica, backups, autosave configurável e recuperação comprovada após encerramento forçado.
- Importação DXF ASCII por layer/unidade; texturas incorporadas por hash.
- Snapshot B-rep → tesselação → Blender/Cycles/QProcess; testes de render real.
- Scripts, exemplos de cozinha/dormitório, CI Linux preparado e documentação.

Não completados: paridade integral, QA Linux/ARM64, AppImage/.deb/Flatpak prontos para distribuição, catálogo de 3.000+, performance de cenas grandes e todos os testes de aceitação da spec.
