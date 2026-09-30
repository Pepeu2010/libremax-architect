# Biblioteca local

`QStandardPaths::AppLocalDataLocation/library.db`; em Linux normalmente `~/.local/share/LibreMax/libremax/`. Esse caminho ainda difere do literal `~/.local/share/libremax/` da spec e será normalizado antes da distribuição.

SQLite WAL, foreign_keys, busy_timeout e FTS5 `unicode61 remove_diacritics 2`. Schema v1: assets (receita/dimensões/licença/autor/origem/data), asset_search, favorites, recent_assets. Categoria e tags ficam no índice. Biblioteca com versão futura é recusada; não migrar silenciosamente.

`starter-library/catalog.json` contém 25 receitas próprias que geram geometria localmente. São famílias distintas, não milhares de duplicatas para atingir meta. Atualização de receitas existentes, coleções, dependências entre assets, packs e backup completo ainda não estão implementados.

Busca usa argumentos SQL e termos FTS escapados, limita resultados a 100. A UI oferece categoria, favoritos e recentes. O teste de 10.000 registros é sintético e mede busca; não comprova 10.000 modelos carregados ou thumbnails. Cards com imagens/thumbnail assíncrono ainda faltam.
