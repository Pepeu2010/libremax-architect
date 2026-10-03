# Geometria compartilhada — ciclo 0.13

Mover e girar móveis agora reutiliza a geometria local já construída. Cópias iguais
usam o mesmo sólido OpenCASCADE, com posição e seleção independentes. Alterar
medidas, espelhamento, materiais ou parâmetros cria a variante correspondente.
Ocultar ou remover todas as cópias libera o protótipo que deixou de ser usado.
Paredes, aberturas e automações continuam usando suas dependências próprias.

O viewport atualiza a posição da apresentação e da seleção sem recriar os vértices.
O teste nativo verifica clique na posição correta depois de mover, girar, desfazer
e refazer dois armários. Isso reduz trabalho de geometria; ainda não comprova
compartilhamento de todos os buffers da GPU nem FPS no notebook mínimo.

## Exportação e Cycles

O pacote interno de render schema 2 contém definições de malhas e matrizes de
posicionamento. O Blender cria objetos independentes ligados à mesma malha.
UVs e normais dos modelos importados são preservados. Espelhamento e dimensões
diferentes geram variantes; não dependem de escala negativa no Blender.

Modelos com mapas e UVs próprias podem compartilhar malhas. Superfícies antigas
com mapas gerados por posição no mundo continuam expandidas, preservando seu
mapeamento. O pacote mantém texturas e HDRI, mas omite o JSON fonte dos modelos,
que já foi convertido em malha. O arquivo de projeto `.lmx` conserva esses modelos
e todos os assets necessários para editar e reabrir. A montagem do JSON também
evita copiar novamente os grandes arrays de vértices e triângulos.

Pedidos antigos guardam o script usado em seu render. O exporter verifica a
capacidade do leitor antes de escolher schema 2; scripts anteriores recebem
schema 1. A fixture arquivada da versão 0.12 verifica esse caminho com Cycles real.
O formato `.lmx` e os UUIDs não mudam.

## Provas reproduzíveis

`[instances]` cobre protótipos, deslocamento, rotação, variantes, ocultação, posições
equivalentes ao formato anterior, UVs/normais e retenção de texturas/HDRI.
`--ui-smoke` confere a seleção e o reaproveitamento de apresentações na interface.
`scripts/verify-instance-renders.py` cria 32 cadeiras, gira/espelha/redimensiona
cópias, salva/reabre o projeto e renderiza pelo aplicativo com o leitor atual e o
leitor 0.12 arquivado. O teste verifica objetos ligados, imagem válida, diferenças
de pixels e preservação da imagem após erro intencional.

No host Windows Ryzen/RX 7600, a fixture leve manteve **33 objetos / 4 definições /
31 objetos ligados**. O pacote caiu de **582.423 para 55.078 bytes**, redução de
90,5%. Com Blender 4.5.9 CPU, 480 × 270 e 32 amostras, a diferença média entre
formatos foi **0,01087 por canal de 0–255**; um pixel teve diferença acima de 24.
O teste de núcleo manteve erro máximo de posição de `1,78e-15` metros na fixture.

A fixture `--model authored` usa uma poltrona Poly Haven com mapas de imagem,
UVs e normais próprias. No Windows, manteve **65 partes / 7 definições / 62 partes
ligadas**, com pacote de **32.797.740 para 4.185.147 bytes**, redução de 87,2%.
Blender 4.5.9 e 5.2.1 CPU passaram com diferença média de **0,00428** e **0,00652**
por canal, respectivamente; nenhum pixel passou de 24. As execuções também
confirmaram preservação da imagem anterior após falha intencional. O núcleo passou
com **43 casos / 2.798 verificações**, além de UI e montagem nativas.
A [CI Linux](https://github.com/Pepeu2010/libremax-architect/actions/runs/37139824263) passou com ambos os cenários no Blender 4.5.9 e 5.2.1, usando motores oficiais com checksum verificado. No Linux, diferenças médias por canal: 0,01087/0,00931 para a fixture leve e 0,00431/0,00574 para a poltrona. A [CI dos instaladores](https://github.com/Pepeu2010/libremax-architect/actions/runs/37139824227) passou em Windows e Ubuntu novo. A [prévia 0.13](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.13.0-preview.1) está publicada. [Relatório](TEST_REPORT.md).

Essas imagens pequenas são testes técnicos. Não comprovam equivalência fotográfica,
qualidade Final pela interface, FPS, pico de RAM nem aceitação física no
**i3-6006U / HD 520 / 8 GB**. [Aceitação do hardware](MINIMUM_HARDWARE.md).
