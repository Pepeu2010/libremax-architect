# Contribuir

Preservar escopo Arquitetos e Decoradores. Não introduzir produção/marcenaria/ERP. Nenhum código/asset obtido de software proprietário.

Cada feature precisa de UI acessível, validação, persistência, histórico quando aplicável, tratamento de erro e teste reproduzível. Não adicionar botão sem ação nem marcar VERIFIED por compilação. Atualizar README, CHANGELOG, matriz e ROADMAP. Preferir fluxos verticais.

C++20, RAII, sem globals mutáveis, warnings Wall/Wextra/Wpedantic, formatar com `.clang-format`. Rodar core, smoke nativo e testes pertinentes; `git diff --check`. Não commitar builds, dumps, logs locais ou executáveis sem provenance/empacotamento. CI Linux deve passar antes de integrar; branch protection só pode ser configurada quando houver remoto.
