# Execução autorizada — apartamento completo

Pedido: implementar as seis frentes aceitas em 3 de outubro de 2026.

| Frente | Entrega | Aceitação necessária |
|---|---|---|
| Construção | Cômodos com contorno livre, edição, piso/forro associados, medidas | Forma em L, contorno inválido, portas após edição, salvar/reabrir/desfazer |
| Montagem | Colocação dentro do contorno, agrupamento, alinhamento e movimento conjunto | Sem atravessar parede, preservar distâncias do conjunto, desfazer |
| Catálogo | Mais peças atuais, medidas e acabamentos, procedência | Geometria distinta, licença, encaixe, arquivo portátil |
| Importação | Modelos externos e coleções pela interface | Importação real, falhas sem modificar projeto, atualização offline |
| Foto | Presets de ambiente, materiais e enquadramento | Imagem Cycles real; comparação com VDMax permanece um teste separado |
| Desempenho | Malha de edição leve e render completo, limites de memória, benchmark | Evidência no host; notebook i3/HD 520/8 GB ainda exige execução física |

O instalador deve continuar incluindo o Blender. A compilação não encerra nenhum
gate físico nem comprova superioridade ao VDMax. Evidências serão acrescentadas
após os testes, sem transformar estimativas em resultados.

## Evidência local inicial

Build Release Windows nativo aprovado; 50 casos / 3.018 assertions passaram.
`--apartment-tools-smoke` passou com desenho e edição de cômodo em L, arraste real
de conjunto, alinhamento, Desfazer/Refazer, importação OBJ via Blender 4.5.9,
incorporação no `.lmx`, coleção offline e três fotos Cycles CPU (320 × 180 / 8).
Fixtures de GLB, glTF, OBJ, FBX, STL e PLY também passaram no motor 4.5.9, com mapas
de cor nos quatro formatos que os armazenam e recusa de glTF inválido/caminho externo.
O filtro moderno passou com 64 modelos e miniaturas reais.

O benchmark usa 48 cópias da cortina: 439.872 triângulos no render e 71.952 na
edição. Dispositivo realmente identificado: RX 7600. Após cache de validação,
p95 de edição medido em 30,75 ms, p50 em 29,87 ms e carregamento frio em 68 ms,
em execução isolada no host de desenvolvimento. Não é uma medida de FPS nem
um resultado do notebook mínimo.
Logs em `build-render/tests-015-reviewed.txt`, `modern-015.txt`,
`apartment-tools-015-isolated.log` e `import-formats-45b/report.json`.

Os seis formatos também passaram no Blender 5.2.1. UI, montagem, tutorial e
recuperação após processo encerrado passaram. A inspeção das fotos identificou
forro oculto e excesso de luz; os estilos agora fecham o teto e usam luz mais suave.

O usuário confirmou disponibilidade do notebook mínimo e do VDMax.
[Teste por duplo clique e comparação](NOTEBOOK_VDMAX_TEST.md).

CI Ubuntu e instaladores 0.15 ainda precisam passar. O notebook mínimo, a imagem
1080p nele e a comparação com VDMax seguem pendentes. A matriz continua parcial.
