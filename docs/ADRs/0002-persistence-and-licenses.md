# ADR 0002 — Documento, comandos, container e licenças

Estado: aceito. Data: 2026-09-29.

Documento é um grafo com UUID persistente e entidades tipadas. Comandos guardam snapshots imutáveis anteriores/posteriores; somente mutações validadas entram no histórico. Isso favorece undo determinístico nesta escala inicial, com custo de memória a medir antes de cenas grandes.

`.lmx` é ZIP versionado produzido por libzip, com JSON validado, limites de entrada e tamanho, nomes permitidos, sem extração arbitrária. QSaveFile escreve temporário no mesmo diretório, flush/fsync e commit atômico. Linux sincroniza também o diretório. Falha de parsing não substitui o documento aberto.

Código LibreMax sob GPL-3.0-or-later. Qt usado dinamicamente sob LGPL-3.0/GPL, OpenCASCADE LGPL-2.1-or-later com exceção OCCT, libzip BSD-3-Clause, SQLite domínio público, Catch2 BSL-1.0, spdlog MIT. Blender é executável separado, GPL, invocado com argumentos sem shell. Assets procedurais próprios são CC0; não usar bibliotecas VDMax. Auditoria dos binários distribuídos permanece requisito de release.
