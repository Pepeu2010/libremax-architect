# Biblioteca local — 0.6

`QStandardPaths::AppLocalDataLocation/library.db`; em Linux normalmente `~/.local/share/LibreMax/libremax/`. Esse caminho ainda difere do literal `~/.local/share/libremax/` da spec.

SQLite WAL, foreign_keys, busy_timeout e FTS5 `unicode61 remove_diacritics 2`. Schema v1: assets, asset_search, favorites, recent_assets. Receitas existentes são atualizadas por ID sem apagar favoritos/recentes; a busca usa argumentos SQL, termos FTS escapados e limite de 200 resultados. Biblioteca de versão futura é recusada.

O catálogo reúne 97 itens: 25 receitas originais, porta e janela associativas, 52 malhas abertas Kenney e 18 modelos detalhados Poly Haven. Escolha **Apartamento atual** no filtro para ver a nova coleção. Os modelos e mapas são distribuídos em `starter-models`, validados por SHA256 e incorporados ao projeto somente ao inserir. Não precisam de rede. Não há importação livre de OBJ/GLB pela interface nem atualização remota de packs.

As miniaturas 192×144 usam a geometria real e a cor média, em até dois workers, com cache PNG atômico e memória. O hash inclui receita, dimensões, presets e hash do modelo. A galeria adapta uma ou duas colunas e mostra nome, categoria e dimensões em cm. Receitas inválidas não recebem miniatura fictícia. O smoke nativo verifica as miniaturas e o filtro da coleção nova.

Ao inserir um modelo detalhado, seus materiais próprios e mapas são incorporados. Inserir outra cópia preserva as alterações de acabamento já feitas no projeto. UVs e normais originais seguem para a triangulação OpenCASCADE e o render Cycles. O arquivo salvo permanece independente da biblioteca instalada. [Fontes, autores e limites dos modelos](../starter-models/README.md).

O teste de busca com 10.000 registros é sintético; não comprova 10.000 modelos carregados. Cache, memória e latência com milhares de malhas detalhadas permanecem sem benchmark. [Uso e limites de montagem](ASSEMBLY.md), [procedência](ASSETS.md).
