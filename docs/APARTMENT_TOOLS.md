# Montagem, modelos e foto — fonte 0.16

Implementação das seis frentes autorizadas. Continua sendo uma prévia: a matriz
registra o que falta para superar o VDMax, sem declarar paridade integral.

## Cômodos

Em **Ambiente → Adicionar cômodo**, escolha retangular ou em L. Em
**Desenhar contorno do cômodo**, clique em cada canto e feche com o botão direito.
Esc cancela. O limite é 64 cantos; contornos cruzados e sobreposição são recusados.
Piso e forro acompanham o desenho. **Ajustar cantos** usa metros e preserva portas
e janelas quando ainda cabem. A edição que removeria espaço de uma abertura é
recusada inteira. Paredes em sequência têm encontros em meia-esquadria;
paredes compartilhadas não são duplicadas.

O ajuste conjunto de dois cômodos que compartilham paredes ainda está bloqueado
para preservar o apartamento. Retângulos de arquivos antigos continuam abrindo;
o ajuste de cantos é disponibilizado nos novos cômodos. Junções T/X gerais,
paredes curvas, detecção automática de regiões, escadas L/U e furos no piso não
estão completos. As etiquetas mostram comprimento de cada parede; não são cotas
de impressão nem medidas livres entre qualquer par de objetos.

## Montagem

Ctrl + clique seleciona vários móveis. **Editar → Juntar em conjunto** cria um
conjunto que pode ser arrastado por qualquer peça. **Mover seleção com medidas**
usa centímetros; os deslocamentos relativos são preservados. Os comandos de
alinhamento e distribuição recusam sobreposição e posição fora do cômodo.
**Girar móvel / Ctrl+R** gira o conjunto 90 graus sobre o centro comum;
**Espelhar** reflete os móveis na horizontal da planta. Ambas preservam distâncias,
altura e objetos sobre mesas, e recusam paredes, obstáculos ou saída do cômodo
antes de alterar qualquer peça. Bloqueios do conjunto ou de seus móveis são respeitados.
**Separar conjunto** preserva as peças. Tudo participa de Desfazer/Refazer e do
arquivo único `.lmx`.

O encaixe agora usa o contorno real do cômodo, inclusive recuos côncavos. Objetos
de mesa precisam caber inteiros sobre o apoio; itens de parede respeitam a altura.
A colisão continua usando o volume que envolve cada modelo, não cada almofada ou
vão da malha. O encaixe conjunto automático em paredes ainda exige melhoria. Giro e
espelhamento não procuram outra posição quando o destino não cabe; a operação
é recusada inteira. O movimento conjunto preserva sua orientação.

## Biblioteca e importação

São **203 itens**, incluindo **176 modelos prontos**. O filtro Apartamento atual
tem **64 modelos**. A nova coleção inclui 28 designs originais CC0: cozinha,
guarda-roupas, banheiro, cortinas, eletrodomésticos, escritório e área de serviço.
Medidas são sugestões de projeto, sem certificação de fabricantes. Procedência e
hashes estão em `starter-models/home-provenance.json`.

**Adicionar modelos** abre GLB, glTF, OBJ, FBX, STL ou PLY pelo Blender incluído.
Depois da conversão, confira nome, dimensões em centímetros e lugar de colocação.
O modelo entra em Meus modelos e é incorporado aos projetos que o usam. A licença
é registrada como fornecida pelo usuário, sem converter conteúdo externo em CC0.
Cor, acabamentos, UVs, normais e mapas diretamente ligados de cor, rugosidade e
normal são preparados. Redes complexas de shaders precisam ser convertidas em
texturas no programa de origem; animação, hierarquia, ossos e modificadores não
são importados como ferramentas de edição.

Entrada: até 128 MB / um milhão de triângulos. Acima de 16 mil triângulos, a
importação simplifica a malha e avisa. O formato portátil continua limitado a
4 MB por malha e 48 MB de modelos/texturas por projeto. Texturas são reduzidas a
até 1.024 pixels no maior lado. glTF usa arquivos relativos da mesma pasta,
sem caminhos para fora. STEP, IGES, DAE e `.blend` ainda não são importadores da UI.

**Instalar ou atualizar coleção** lê `.lmaxpack` offline, verifica caminhos,
hashes, modelos e procedência antes da transação no catálogo. Favoritos sobrevivem
a atualizações; versões antigas e versões iguais com conteúdo diferente são
recusadas. A coleção original também está em `collections/` e no instalador.
Projetos antigos preservam as malhas incorporadas. Não há atualização automática
de uma coleção remota nem sincronização com bibliotecas comerciais.

Fontes já integradas continuam sendo [Poly Haven](https://polyhaven.com/license),
Kenney e KayKit; nenhum asset VDMax foi redistribuído.

## Foto e desempenho

No painel Criar imagem, escolha o cômodo e um estilo: Natural, Claro ou
Aconchegante. Piso, paredes e uma luz suave são preparados numa operação que pode
ser desfeita. As luzes criadas pelo usuário são preservadas. O Blender/Cycles
continua produzindo a imagem real, com progresso e previsão já existentes.

Os estilos tornam o forro visível no render, para fechar o ambiente e controlar
a entrada de luz. **Ver por dentro** continua escondendo o teto apenas no editor.

**57 modelos detalhados** têm uma segunda malha para a edição em Leve. A malha
completa continua sendo usada no render. As duas acompanham o `.lmx`; dimensões,
seleção, posição e acabamentos continuam iguais. Hashes e contagens de triângulos
estão em `starter-models/editor-lod-provenance.json`. Cache de validação armazena
somente referências de materiais de malhas já verificadas, até 128 registros;
arquivos alterados precisam passar novamente pela validação.

Reprodução: `--apartment-tools-smoke PASTA --blender CAMINHO` executa montagem,
importação real, arquivo portátil, coleções, três fotos Cycles CPU e benchmark
de 48 cópias. Registra o dispositivo OpenGL realmente usado e tempos p50/p95 de
edição. No Windows instalado, `scripts/benchmark-windows.ps1` também registra
CPU, GPU, RAM e pico de memória combinado do aplicativo e seus processos filhos.

Esses testes não substituem o notebook **i3-6006U / HD 520 / 8 GB**, fotos 1080p,
FPS durante navegação ou uma comparação com o VDMax. Ainda não há evidência para
afirmar superioridade ou suporte confirmado ao hardware mínimo.

## Tutorial e linguagem

A primeira abertura oferece 21 assuntos, com índice acessível por teclado.
Desenho em L e giro de conjuntos incluem diagramas originais leves. Os textos
cobrem importação, coleções, edição Leve, estilos, progresso, galeria e arquivo
único. O tutorial usa rolagem vertical em janelas pequenas e pode ser reaberto
na tela inicial ou em Ajuda. Não marca tarefas práticas como realizadas.
A escolha de cálculo usa Automático ou Processador; os valores internos AUTO/CPU
permanecem compatíveis. Falhas na importação mostram um aviso curto; o registro
do Blender fica no botão de detalhes. Cancelar preserva o projeto sem abrir
um aviso de erro.
