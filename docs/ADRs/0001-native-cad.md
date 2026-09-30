# ADR 0001 — Editor nativo e geometria real

Estado: aceito. Data: 2026-09-29.

O repositório estava vazio (somente `.git`, nenhum commit). Não havia baseline executável ou testes. O host é Windows, sem WSL e sem compilador no PATH. Linux continua sendo o alvo de distribuição; testes no Windows não validam pacotes Linux.

Usar C++20, Qt 6 e OpenCASCADE. Nesta primeira integração usar Qt Widgets para menus, docks, formulários e viewport AIS/V3d nativo. Qt Quick/QML é uma preferência da spec, e não uma obrigação: Widgets permite integrar diretamente o viewport CAD existente do kernel e sua seleção topológica sem criar uma segunda representação da cena. A camada de apresentação não decide geometria ou regras de documento. QML poderá ser avaliado depois, sem trocar o modelo.

Unidades internas em milímetros, coordenadas arredondadas a 0,1 mm. Z é vertical. Paredes são sólidos B-rep; portas e janelas removem volume por boolean Difference. Móveis paramétricos são conjuntos visuais de peças, sem BOM produtiva. A tesselação para Cycles deriva desses mesmos sólidos.

Não criar botões para funções inexistentes. Não declarar versão 1.0, paridade, screenshots ou pacotes sem execução reproduzível. Focar fluxos verticais completos antes de expandir cobertura.
