# Biblioteca local — 0.4

`QStandardPaths::AppLocalDataLocation/library.db`; em Linux normalmente `~/.local/share/LibreMax/libremax/`. Esse caminho ainda difere do literal `~/.local/share/libremax/` da spec.

SQLite WAL, foreign_keys, busy_timeout e FTS5 `unicode61 remove_diacritics 2`. Schema v1: assets, asset_search, favorites, recent_assets. Receitas existentes são atualizadas por ID sem apagar favoritos/recentes; a busca usa argumentos SQL, termos FTS escapados e limite de 200 resultados. Biblioteca de versão futura é recusada.

O catálogo reúne 79 itens: 25 receitas originais, porta e janela associativas e 52 malhas abertas Kenney. Os modelos são distribuídos em `starter-models`, validados por hash e incorporados ao projeto somente ao inserir. Não precisam de rede. Não há importação livre de OBJ/GLB pela interface nem atualização remota de packs.

As miniaturas 192×144 usam a geometria real, em até dois workers, com cache PNG atômico e memória. O hash inclui receita, dimensões, presets e hash do modelo. A galeria adapta uma ou duas colunas e mostra nome, categoria e dimensões em cm. Receitas inválidas não recebem miniatura fictícia. Os 79 itens foram verificados pelo smoke nativo.

O teste de busca com 10.000 registros é sintético; não comprova 10.000 modelos carregados. Cache, memória e latência com milhares de malhas detalhadas permanecem sem benchmark. [Uso e limites de montagem](ASSEMBLY.md), [procedência](ASSETS.md).
