# Test report — fonte 0.16

Windows Release: **52 testes / 3.065 verificações**, build e formato C++ passaram.
Tutorial com 21 assuntos, navegação por teclado e janela compacta; giro e
espelhamento de conjuntos pela interface; importação real, erro de OBJ com
diagnóstico separado e três fotos Cycles CPU passaram no Windows.
[Evidência, reprodução e limites](EXECUTION_016.md).
A [CI Linux](https://github.com/Pepeu2010/libremax-architect/actions/runs/37160630378)
e a [CI dos instaladores Windows/Ubuntu](https://github.com/Pepeu2010/libremax-architect/actions/runs/37160630397)
passaram; a [prévia 0.16](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.16.0-preview.1)
está publicada. Os oito digests GitHub correspondem aos checksums, e o Setup
Windows baixado integralmente tem o SHA256 publicado. O relatório abaixo
mantém as provas das versões anteriores.

# Test report — montagem e modelos 0.15.0

Windows Release: **50 testes / 3.018 verificações**, build e formato C++ passaram.
UI, montagem com 203 miniaturas, tutorial, recuperação em outro processo e
`apartment-tools-smoke` passaram: contorno em L, cantos editáveis, piso/forro,
arraste de conjunto, alinhamento, histórico, importação real e arquivo portátil.
Os seis formatos GLB/glTF/OBJ/FBX/STL/PLY passaram em Blender 4.5.9 e 5.2.1,
incluindo mapas de cor e recusa de caminhos glTF inválidos.

Três estilos distintos produziram imagens Cycles CPU de 320 × 180 / 8 amostras.
A inspeção visual levou à correção do forro oculto e da potência excessiva de luz.
As fotos pequenas são evidência do fluxo. O estilo Natural também concluiu
1920 × 1080 / 64 amostras em Blender 4.5.9 CPU: 3min53s no host Ryzen/RX 7600,
com pico de memória interno do Cycles de 326,83 MB. A imagem foi inspecionada.
Esse resultado não mede o notebook mínimo nem comprova equivalência ao VDMax.
No benchmark isolado de 48 cortinas, a RX 7600 carregou a cena em 69 ms e editou
com p50 de 29,27 ms e p95 de 30,48 ms. Malha completa: 439.872 triângulos;
edição: 71.952. Isso não é uma medida de FPS nem comprova desempenho da HD 520.

Logs: `build-render/Testing/Temporary/LastTest.log`, `ui-015.txt`,
`assembly-015.txt`, `experience-015.txt`, `recovery-015.txt`,
`apartment-tools-015-enclosed.txt`, `import-formats-45b/report.json` e
`import-formats-52/report.json`.
O pacote de distribuição Windows passou no benchmark com PATH restrito ao
sistema e Blender incluído; pico conjunto medido de 589,7 MiB na cena controlada.
A instalação/desinstalação Windows passou na CI da primeira tentativa 0.15.
O Linux parou no teste de clique do botão HDRI antes de produzir renders.
O seletor de estilo agora respeita a largura do painel; o teste verifica posição,
ausência de rolagem horizontal e chegada do clique, usando a janela nativa.
HDRI/EXR e UI local passaram novamente. A
[CI Linux completa](https://github.com/Pepeu2010/libremax-architect/actions/runs/37157341255)
passou com Blender 4.5.9 e 5.2.1: interface, montagem, importação dos seis formatos,
fila, HDRI/EXR, luzes e comparação de instâncias. A
[CI dos instaladores](https://github.com/Pepeu2010/libremax-architect/actions/runs/37157341232)
passou: instalação Windows com PATH restrito ao sistema; Ubuntu em runner novo,
montagem e importação nativas, renders com motor incluído, desinstalação
preservando um projeto. A
[prévia 0.15.0-preview.3](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.15.0-preview.3)
publica a fonte exata `3c6b8d32f6cbec3271f90bee45c7626826a58c18`.

Os oito digests publicados correspondem aos três manifestos de checksum baixados.
O instalador Windows também foi baixado integralmente e conferido: 487.451.300
bytes, SHA256 `8820a32d43e32155ae36c021a490943187b5f24cd50087851c3f89249b3a2044`.
O `.deb` tem 596.162.134 bytes e digest
`22fc93f6a01fdb163b0076db6e6fa4845dfcb3ec200761c6249ccf8b9ac9f651`;
seu binário completo não foi baixado de novo no host Windows. A instalação real
do `.deb` foi comprovada no runner Ubuntu novo. A coleção publicada corresponde
ao arquivo local original; fontes e demais payloads foram conferidos por
manifestos/digests da API. Relatório local: `build-bundled/release-015-verification.json`.
O usuário confirmou disponibilidade de notebook e VDMax;
[teste físico e comparação](NOTEBOOK_VDMAX_TEST.md) continuam pendentes.

---

# Test report — dependências incluídas 0.14.0

Instaladores Windows/Ubuntu da tag `v0.14.0-preview.3`, fonte
`5257f576bdc99b4fabc1666ae62ac77382038220`, incluem Blender 4.5.9 LTS,
Cycles, Python e as bibliotecas da distribuição oficial.
[CI dos instaladores](https://github.com/Pepeu2010/libremax-architect/actions/runs/37144046796)
passou: instalação real em Windows com PATH limpo e Ubuntu separado, descoberta
do motor dentro do pacote, render CPU do apartamento sem `--blender`,
320 × 180/oito amostras, erro de câmera preservando a imagem anterior e
desinstalação removendo o motor sem remover um projeto. Ubuntu também passou
na UI e na montagem dos modelos modernos.

Windows local: 43 testes/2.798 verificações, compilação e formato C++ passaram.
Arquivo com checksum errado, destino de motor já preenchido e build de
distribuição sem motor foram rejeitados. O filtro de dependências Debian passou
em 12 verificações que preservam CPU/Python e limitam a exclusão a quatro plugins
de drivers opcionais de GPU. O Setup local foi gerado; o payload final também
executou descoberta e render com PATH limitado ao sistema.

A [CI Linux completa](https://github.com/Pepeu2010/libremax-architect/actions/runs/37144046955)
passou em núcleo, formatação, interface, montagem, tutorial, recuperação,
fila/galeria, HDRI/EXR, luzes e instâncias nos motores 4.5/5.2.
Os sete assets da Release e os três manifestos de checksum baixados tiveram
digests conferidos. Não houve novo download dos instaladores completos no host;
a validação usa os digests GitHub/manifestos e a instalação nos runners.
[Distribuição, tamanhos e limites](BUNDLED_RUNTIME.md).
O teste físico i3-6006U/HD 520/8 GB continua pendente.

---

# Test report — geometria compartilhada 0.13.0

Windows Release: **43 testes / 2.798 verificações** passaram. Build, formatação
C++, compilação dos scripts Python e diff passaram. Protótipos, movimento/rotação,
dimensões, espelhamento, ocultação, UVs/normais, texturas e HDRI foram verificados.
A interface manteve apresentações ao mover/girar/desfazer/refazer armários e
selecionou cada cópia em sua posição correta. O smoke de montagem passou com
catálogo, encaixe, rejeição fora do ambiente, janela, arquivo portátil e 900 pixels.

Blender 4.5.9 CPU: fixture leve com 32 cadeiras e piso, **33 objetos / 4 definições /
31 objetos ligados**; pacote **582.423 → 55.078 bytes**. A poltrona Poly Haven com
mapas/UVs/normais próprias manteve **65 partes / 7 definições / 62 partes ligadas**,
com pacote **32.797.740 → 4.185.147 bytes**. O mesmo cenário passou no Blender
5.2.1 CPU. Comparação com o leitor 0.12 arquivado, 480 × 270 / 32 amostras:
diferenças médias por canal de 0–255 de **0,01087** (leve/4.5), **0,00428**
(Poly Haven/4.5) e **0,00652** (Poly Haven/5.2). Os dois leitores renderizaram e
preservaram a imagem após erro intencional. O apartamento salvo também passou
no Blender 5.2 CPU, 320 × 180 / oito amostras, mantendo seus 21 mapas existentes.
O teste de projeto salvo agora preserva seus materiais, sem aplicar o pack padrão.

Relatórios locais: `build-render/instances-ui.log`, `instances-assembly.log`,
`instances-45-verified/comparison.json`, `instances-45-polyhaven/comparison.json`
e `instances-52-authored/comparison.json`. A [CI Linux 0.13](https://github.com/Pepeu2010/libremax-architect/actions/runs/37139824263) passou: núcleo, formato, UI, montagem, tutorial, recuperação, fila, HDRI/EXR, sete luzes e ambos os cenários de instâncias nos motores 4.5.9/5.2.1. A [CI dos instaladores](https://github.com/Pepeu2010/libremax-architect/actions/runs/37139824227) passou: Windows com instalação real e PATH sem SDK; Ubuntu instalado em runner novo, biblioteca/modos, seleção de cópias e remoção preservando projeto. O pacote Linux em staging também calculou os renders de fila/HDRI/EXR/luzes.

A [prévia 0.13.0-preview.1](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.13.0-preview.1) distribui a fonte exata `8554c71422b9fb0af03ef1c519f8e7f1b8126654`. Os digests dos seis assets correspondem aos manifestos combinado e de plataforma; os três arquivos de checksum baixados também correspondem aos seus digests GitHub. Windows SHA256: `d3c30790f73178a9d8d87663bfbda36cfc90e27a7f680357fbdef01f1e787ab1`; Linux: `910a8c34d128522d3077f870b1f8bc8978beba43f004a4230357f742a2c48503`; fonte: `5e8c8f2a46dd72efa6420c075a5b21c6291837ed9d4c264d8fded9e320aae21d`. Os binários completos não foram baixados novamente no host neste ciclo; a verificação usa os digests GitHub e os manifestos pequenos baixados.
[Implementação, reprodução e limites](SHARED_GEOMETRY.md). **Não comprova FPS,
pico de RAM, qualidade Final pela UI nem aceitação física no i3/HD 520/8 GB.**

---

# Test report — caminho para hardware mínimo 0.12.0

Alvo obrigatório confirmado pelo usuário: **i3-6006U, Intel HD Graphics 520 e 8 GB de RAM**. O hardware físico ainda não foi testado. [Caminho implementado, fontes oficiais e aceitação](MINIMUM_HARDWARE.md).

Windows Release: **40 testes / 2.269 verificações** passaram, com build, formatação e diff. Blender oficial **4.5.9 LTS**, SHA256 conferido, calculou o apartamento moderno com céu Nishita, 27 mapas PBR, CPU/denoise em 320 × 180 / oito amostras; falha posterior preservou a imagem. A fila nativa calculou quatro imagens e confirmou cancelamento, repetição, snapshots, persistência e tempos em 1440/900 pixels. Quatro imagens HDRI/EXR e sete de iluminação passaram, incluindo Kelvin e LED ligado/desligado/quente/frio. A interface conferiu 175 miniaturas, três modos, encaixe e arquivo portátil. Blender 5.2.1 manteve o céu Multiple Scattering e a imagem CPU do apartamento.

A primeira execução do teste de fila 4.5 perdeu o marcador de início ao ler apenas os últimos 32 KiB do log mais verboso desse motor. O teste passou a consultar o estado estruturado do pedido; a nova execução completa passou com barra a 7%, 27s decorridos e estimativa positiva. A falha era da espera do teste; os registros do render já continham progresso e previsão válidos.

CPUs com até quatro threads disponíveis começam em Leve sem preferência anterior. A CI acrescenta execução real com Blender 4.5.9 e confirma a inicialização do viewport leve no runner com poucos threads. A [CI Linux 0.12](https://github.com/Pepeu2010/libremax-architect/actions/runs/37130831318) passou com os dois motores, núcleo/formato, UI/montagem/tutorial/recuperação e pacote. `LIGHTWEIGHT_START_PASS` confirmou duas CPUs lógicas, viewport em Leve e MSAA desligado. Ambos os motores passaram em fila/tempos, HDRI/EXR e sete luzes; 4.5 também renderizou o apartamento com céu Nishita. A [CI dos instaladores](https://github.com/Pepeu2010/libremax-architect/actions/runs/37130831328) passou: Windows instalado sem SDK, Ubuntu em runner novo, recursos, biblioteca/modos, montagem e remoção preservando projetos.

A [prévia 0.12.0-preview.1](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.12.0-preview.1) publica binários e fonte exata da revisão `f9cffec8020cd882c2d12c6987f622829ac8c9ef`. Os digests dos seis assets correspondem ao manifesto combinado e aos arquivos de checksum baixados; os dois manifestos de plataforma também correspondem. Windows SHA256: `41cafb6d3742ee0258f065a51c7c681af497c26c10ffb55e01349be2ca5caab4`; Linux: `6a3cbc4d9792e920f1b8ed8c8ed17edf1ce4da4b8be26215aa5eb36b4b33301b`; fonte: `b0a896dc93b55e64f8e76ed7f29f74a667bc691f20a078625f3f9f2fed6e0898`. Não extrapolar os tempos nem a fluidez do Ryzen/RX 7600 para o i3/HD 520. O caminho CPU não garante baixo uso de RAM em projetos grandes.

---

# Test report — progresso de render 0.11.0

Windows Release: **40 testes / 2.269 verificações** passaram. Build, formatação C++ e `git diff --check` passaram. Os testes de núcleo verificaram amostras e transições de partes de imagem baseadas no log 4K anterior, limites de tempo, previsões inválidas/ausentes/vencidas e duração congelada. O estado de carregar kernels de denoise já não oculta a estimativa durante o cálculo.

O teste nativo `--queue-smoke` calculou quatro imagens reais Cycles CPU e confirmou FIFO, edição sem alterar snapshots, falha, repetição com CPU, cancelamento na fila e durante execução. Durante o pedido longo, a barra mostrou **6%**, **22s decorridos** e **4min30s restantes estimados**, com horário aproximado do cálculo. O contador avançou a cada segundo, a duração concluída persistiu após reabrir e as ferramentas voltaram ao sair da galeria. Layout e tempos passaram em **1440 × 900 e 900 × 650**. [Capturas e semântica dos tempos](RENDER_PROGRESS.md).

A [CI Linux 0.11](https://github.com/Pepeu2010/libremax-architect/actions/runs/37129048387) passou: build, núcleo, formato, biblioteca/montagem/tutorial/recuperação e Cycles CPU com fila/HDRI/EXR/luzes. O teste da barra registrou 3%, 37s decorridos e 17min23s restantes estimados nesse runner; layout de 900 pixels sem corte. A [CI dos instaladores](https://github.com/Pepeu2010/libremax-architect/actions/runs/37129048284) passou: Windows com instalação real e PATH sem SDK; Ubuntu instalado em runner novo, biblioteca/modos/montagem e remoção preservando projetos. O pacote Linux em staging também executou os testes Cycles.

A [prévia 0.11.0-preview.1](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.11.0-preview.1) foi publicada com instaladores e fonte exata da revisão `0860341982ef637da304b10b88fbcee715d7ac76`. Os digests dos seis assets GitHub correspondem ao manifesto combinado e aos arquivos de checksum baixados; os manifestos individuais Windows/Linux também correspondem ao combinado. Windows SHA256: `f46bd8202a35e77ce230fa2a39b9c93a0e71db155b08808c03c9ab4fe066469f`; Linux: `b5e7cd0c99d674723b4c2e0bec7ec69754c79e02ff0187865ec84a86aefdb58e`; fonte: `cc9b3c444521c20eeebdef646de199f6b4a939f0073f94126f36083a9c2f5ace`. Esta revisão não repete o render 4K nem verifica outras GPUs; a estimativa do motor pode variar e não prevê o tempo adicional de finalização.

---

# Test report — iluminação 0.10.0

Windows Release: **37 testes / 2.233 verificações** passaram. O teste nativo criou cinco tipos de luz pela interface, editou Kelvin, dimensões em centímetros, raio, feixe, formato, sombra do sol e giro; verificou undo/redo e projeto v3 portátil. O layout de 900 pixels preservou controles e rótulos sem rolagem horizontal. Sete renders reais Cycles CPU passaram: LED quente/frio/desligado, ponto, spot, painel retangular e sol; o último em EXR. A comparação de pixels confirmou emissão LED e alteração da cor refletida. Edição durante a fila preservou os snapshots v3. Biblioteca completa, modos de edição, coleção moderna, colocação e tutorial passaram na regressão nativa. [Capturas e detalhes](LIGHTING.md).

A RX 7600 deste Windows calculou o LED com GPU/HIP confirmado, sem fallback, em 320 × 180 / 16 amostras. Também calculou o apartamento moderno com Kelvin/LED em **3840 × 2160 / 512 amostras máximas / 12 reflexões**, denoise e HIP confirmado: **08:08,98** no motor. A falha intencional de câmera preservou a saída anterior. Hash da imagem 4K: `a4bc54b4911ef7576a2395a0eb72de606bba65b79c8ef412dc8fa0db1d610165`. Formato C++, compilação e sintaxe Python passaram. O teste 4K usa parâmetros de apresentação pelo pipeline; não verifica o gesto de selecionar Final na galeria, equivalência fotográfica ou outros drivers/GPUs.

A [CI Linux 0.10](https://github.com/Pepeu2010/libremax-architect/actions/runs/37125774451) passou: build, núcleo, formato, biblioteca/modos, montagem, tutorial/recuperação e Cycles CPU com fila/HDRI/EXR/sete luzes. A [CI dos instaladores 0.10](https://github.com/Pepeu2010/libremax-architect/actions/runs/37125785735) passou: Windows com PATH sem SDK e todos os 148 modelos; Ubuntu instalado em runner novo, com UI, biblioteca, três modos, encaixe, salvamento e desinstalação preservando projetos. O `.deb` em staging também produziu os sete renders. No host Windows, o payload empacotado passou com PATH sem SDK, cinco diálogos nativos, sete renders CPU e LED HIP. Revisão compilada/testada: `7c5a662e85a3b35c672f2065dbf459baf47b1fad`.

A [Release 0.10.0-rc.1](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.10.0-rc.1) está publicada com os binários dessa CI e o código exato da revisão acima. Os seis assets tiveram tamanho/digest GitHub conferidos contra os arquivos locais; os instaladores baixados também correspondem aos checksums da CI. Windows SHA256: `78dcce2d1c56eeaee32422de7b87993b0daabaf95269332fb0c850c8f6bc3219`; Linux: `29e8490a325f0af5477b0d24f9c441adb30f85b6e0cf99c803690d5ef0fd40f4`; fonte: `5e35fa31710ad459fe67b7fec96a1a818a00b22c9849919df4f5aa5025e50e8f`.

---

# Test report — biblioteca e desempenho 0.9.0

Windows Release: **35 testes / 2.191 verificações** passaram. Todos os 60 novos assets reais foram validados e salvos/reabertos com malhas e texturas incorporadas. O smoke nativo verificou 175 miniaturas, filtros com 105 modelos leves e sete objetos detalhados, três modos de edição e preservação do documento/malha do render. Sofá KayKit no piso e relógio detalhado na parede passaram pela colocação nativa e reabertura do projeto. Montagem, coleção moderna e tutorial passaram. O mesmo smoke passou com afinidade limitada a dois núcleos lógicos, mantendo a RX 7600: 19,14 segundos e janela em 415 ms. Esse limite não representa um computador antigo completo. [Fontes, capturas e limites](MODEL_LIBRARIES_PERFORMANCE.md). A [CI Linux 0.9](https://github.com/Pepeu2010/libremax-architect/actions/runs/37121303969) passou com build, núcleo, formato, colocação nativa dos modelos novos, três modos, montagem/tutorial/recuperação, pacote Debian e Cycles CPU com fila/HDRI/EXR. A [CI dos instaladores 0.9](https://github.com/Pepeu2010/libremax-architect/actions/runs/37121304063) passou: Windows com PATH sem SDK e todos os 148 modelos empacotados; Ubuntu em runner novo com biblioteca completa, interface, três modos, encaixe e desinstalação preservando projetos. No host Windows, os recursos empacotados também passaram em UI/modelos/tutorial com PATH sem SDK. A [Release 0.9.0-rc.1](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.9.0-rc.1) distribui os binários dessa CI e o código exato `8ae679d43c5df1bb417d6663792b1a47124ce8d6`. Os checksums dos arquivos baixados coincidem com os da CI. Windows SHA256: `176d5f3462e2f8717a1d70463360dcc8ffe84b98bcd14ffab4b3b23de745aafe`; Linux: `571073e591f32d5c608d87e0e1745242653c78211133f47002f899c80c202e06`; fonte: `e1e094baec82f25abde6d8859886e99ae048e39b90fce171cc3cc7e56a3d0692`.

---

# Test report — HDRI e EXR 0.8.0

Build Release no Windows: **34 casos / 1.829 assertions** aprovados. Importação HDR/EXR em ponto flutuante, projeto v2 portátil, snapshots e publicação segura testados. O fluxo nativo completou quatro renders Cycles CPU, verificou rotação nos pixels, fundo oculto, alpha, originais EXR FLOAT32, histórico, miniaturas e cópia pela galeria byte a byte. UI, montagem, modelos modernos, tutorial, recuperação e fila de cinco câmeras passaram. A prévia do apartamento com HDRI também passou na RX 7600 / HIP, 640 × 360 / 32 amostras, sem fallback e com preservação da imagem após falha. [Capturas, números e limites](HDRI_EXR.md).

A [CI Linux 0.8](https://github.com/Pepeu2010/libremax-architect/actions/runs/37089024012) passou: build com Qt 6.4/OpenEXR 3.1, núcleo, formato, smokes nativos, pacote Debian e Cycles CPU com fila/galeria e HDRI/EXR. A [CI dos instaladores 0.8](https://github.com/Pepeu2010/libremax-architect/actions/runs/37089055737) passou: Setup Windows com PATH sem SDK, biblioteca/modelos/codecs/HDRI, salvamento e desinstalação; pacote Debian com fila Cycles/HDRI/EXR e instalação Ubuntu em runner novo, incluindo interface e desinstalação preservando projeto. A prévia usa os binários dessa execução e o código exato `ed2e007`. A [Release 0.8.0-rc.1](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.8.0-rc.1) contém esses arquivos; os digests publicados pelo GitHub coincidem com os checksums da CI e dos arquivos locais. Windows SHA256: `5b6bfa208740749ee0d159565263c1038c530df48aa47b6895c9768d546d467f`; Linux: `cf7bee691cff1128e362f191a6f7820707096a645dbda70d57d8bdae30f2e7c3`; fonte: `fc87a680bdb1020b4722e279614f36e921d62a1dcd33f48a9104378e40199568`. Resultados anteriores abaixo referem-se às versões publicadas indicadas.

---

# Test report — fila e galeria 0.7.0

Verificado no host Windows em 02/10/2026: **31 casos / 1.737 assertions** aprovados. A fila nativa com Blender 5.2.1 LTS/Cycles CPU passou com cinco câmeras, snapshots imutáveis, edição durante o render, progresso de amostras, cancelamento na fila e durante o processo, falha intencional, repetição CPU, reabertura do histórico e metadados no `.lmx`. Imagens positivas: 160 × 90, oito amostras; o caso de cancelamento inicia 640 × 360/1024 e interrompe o processo.

Também passaram a regressão de UI/montagem, tutorial de 15 capítulos e projeto único, render real 320 × 180 e preservação da imagem anterior após erro. PowerShell, script Python, formatador e diff foram conferidos. [Arquitetura, capturas e limites](RENDER_PIPELINE.md). Relatórios locais: `build-render/queue-report.txt`, `build-render/render-report.txt` e `build-render/Testing/Temporary/LastTest.log`.

A [CI Linux](https://github.com/Pepeu2010/libremax-architect/actions/runs/37019798505) passou com build, núcleo, interface compacta, tutorial/montagem/recuperação, pacote Debian e fila com Blender/Cycles CPU real. No Windows, a RX 7600 gerou o apartamento em 640 × 360/32 com HIP confirmado, sem fallback, e preservou a imagem após falha. Os novos instaladores têm verificações próprias. O resultado de uma build não substitui esses gates nem QA em GPUs físicas, 4K ou apartamentos grandes. A [Release 0.7.0-preview.1](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.7.0-preview.1) foi publicada a partir de `8f59f37`. A [CI dos instaladores](https://github.com/Pepeu2010/libremax-architect/actions/runs/37019803477) passou com Windows, Cycles CPU no Linux, instalação Ubuntu em runner novo e publicação. O `.deb` publicado foi baixado e seu SHA256 conferido: `d6684f019bb6e0778c443e64d3f4c77ee1fa1f8af986147b16983a3214420cad`.

No host Windows, o pacote final também passou com PATH limpo, interface/tutorial/recuperação, render CPU, desinstalação preservando projeto e render HIP usando os recursos e DLLs empacotados, sem SDK no PATH. Imagem HIP: 640 × 360/32, sem fallback; falha posterior preservou a imagem. Logs locais em `build-render-package/installer-release-report.txt` e `build-render-package/gpu-release-report.txt`.

Os resultados abaixo são históricos.

---

# Test report — instaladores 0.6.1

A [primeira CI de instalação](https://github.com/Pepeu2010/libremax-architect/actions/runs/37010283076) passou nos dois sistemas. Os testes verificaram os pacotes realmente instalados, sem usar o checkout para recursos:

| Verificação | Resultado |
|---|---|
| Windows Server 2022: NSIS, atalhos, registro e execução com PATH só do Windows | Passou |
| Ubuntu 24.04: `.deb` em runner novo, dependências de runtime pelo apt, sem SDK nem checkout | Passou |
| Recursos instalados: 115 itens, SQLite/FTS, 36 modelos detalhados, JPEG/PNG e projeto portátil | Passou nos dois sistemas |
| Linux: montagem real com Xvfb/Mesa, encaixe, undo/redo e reabertura | Passou |
| Desinstalação e preservação de projeto | Passou nos dois sistemas |

Neste host Windows, o instalador reconstruído com a correção da automação de salvar também passou em `--modern-smoke`, `--experience-smoke` e `--recovery-smoke`: tutorial completo, novo cômodo, teclado no diálogo de salvar, `.lmx`, biblioteca inicial e recuperação após encerramento forçado. O teste de render usou **Blender 5.2.1 LTS, Cycles CPU, 320×180/8**, com o script e modelos instalados. A imagem foi gerada e preservada após uma falha de câmera provocada. Logs locais em `build-install/verify-fixed-report.txt` e `build-install/installation-evidence`; os testes não alteram projetos/preferências do usuário.

A [CI da tag de Release](https://github.com/Pepeu2010/libremax-architect/actions/runs/37011097009) gera os arquivos publicáveis. Os guias e limites estão em [INSTALADOR](../INSTALADOR/README.md).

Não comprova: Mint 22 em execução própria, todas as versões do Windows, drivers/GPU variados, render final fotorealista de apresentação, execução Cycles no Linux, AMD HIP/NVIDIA OptiX, cenas grandes ou paridade integral com VDMax. Windows não possui assinatura digital nesta prévia; Linux usa X11/XWayland.

Os relatórios abaixo são históricos.

---

# Test report — ampliação contemporânea 0.6.0

Windows: **28 casos / 1.706 assertions** aprovados. Catálogo de 115 itens, filtro com 36 modelos, 18 acréscimos, ocultação correta das miniaturas, arraste de sofá/cama/peças suspensas, arquivo único, UI a 900 px, tutorial/home e recuperação passaram. Render real Cycles CPU com o novo sofá também passou. [Evidência e limites](CATALOG_EXPANSION.md). Linux: implementação `ae0fe5b` aprovada em [CI Ubuntu 24.04](https://github.com/Pepeu2010/libremax-architect/actions/runs/36951914174), incluindo build/core/formatador, smokes nativos Xvfb/Mesa e pacote Debian. Hardware gráfico físico, instalação limpa e Cycles no Linux continuam sem validação. O resultado abaixo refere-se ao ciclo anterior.

---

# Test report — ciclo 0.6.0

Executado em 01/10/2026 no host Windows: **26 casos / 1.461 assertions** aprovados. Build Release, coleção de 18 modelos detalhados, hashes/UVs/normais/mapas, catálogo de 97 itens, montagem, undo/redo, arquivo único `.lmx`, home/tutorial de 15 capítulos e telas a 900 px passaram. Render real Cycles CPU 960×540/64 e preservação da imagem anterior após falha também passaram. Evidência, reprodução e limites: [CYCLE_06](CYCLE_06.md). GPU, cenas grandes e arquitetura completa de fila/galeria continuam pendentes. Os relatórios abaixo são históricos.

CI Linux da implementação `34c3f80` também aprovada: build/core/formatador, smokes nativos com Xvfb/Mesa e criação de `.deb` em Ubuntu 24.04. [Execução verificada](https://github.com/Pepeu2010/libremax-architect/actions/runs/36947008639). Instalação limpa, hardware gráfico físico e render Cycles no Linux não foram validados por esse job.

---

# Test report — ciclo 0.4.0

Executado em 30/09–01/10/2026 no host Windows: **22 casos / 1.054 assertions** aprovados. Build Release, 79 miniaturas reais, encaixe na parede em planta e 3D, bloqueio de área externa, arrasto de móvel existente com undo/redo, janela associada, apartamento com três cômodos e modelos incorporados, save/open e painéis em 900 px passaram no executável nativo. Regressão de UI/PBR/expressões em cm e recuperação após encerramento forçado também passaram.

Render real do apartamento no Cycles CPU: 640×360 / 32 samples; falha posterior por remoção deliberada da câmera preserva o hash da imagem anterior. Modelos preparados não contêm faces geométricas duplicadas. Relatório, reprodução e limites: [CYCLE_04](CYCLE_04.md). Screenshots [3D](screenshots/apartment-3d.png), [900 px](screenshots/apartment-900.png), [render](screenshots/apartment-render.png). Linux/GPU/1080p/4K e catálogo fotográfico extenso não são comprovados por esses resultados. Os relatórios abaixo são históricos.

---

# Test report — ciclo 0.3.0

Executado em 2026-09-30 no mesmo host Windows/toolchain descrito abaixo: **18 casos/463 assertions**, CTest 6,30 s; FTS5/10.000 fixtures 1,7564 ms em concorrência. Build Release, ativação PBR/sol e persistência pela UI, inspeção de render Cycles CPU 1280×720/128, exportação PNG e recuperação passaram. Fonte e hashes dos seis mapas verificados; nenhum download em runtime.

Evidência e limites atuais em [CYCLE_03](CYCLE_03.md). Render final: [1280×720](screenshots/photoreal-kitchen.png), [workspace](screenshots/render-workspace.png). Presets 1080p/4K, GPU, Linux e equivalência a foto física permanecem sem validação. Os ciclos abaixo são históricos; screenshots de workspace foram atualizados.

---

# Test report — ciclo 0.2.0

Execução 2026-09-30 no mesmo host Windows/toolchain documentado abaixo. **17 casos/408 assertions passaram**; FTS5 com 10.000 fixtures: 0,5936 ms; CTest: 2,94 s. Build nativo, 25 thumbnails, UI/roundtrip/900 px, visualizador/exportação PNG, crash recovery e Cycles 960×540/64 CPU passaram. Render salvo pelo Blender aos 32,953 s em execução concorrente com testes. Falha sem câmera preservou a imagem anterior.

Evidência e comandos atuais: [CYCLE_02](CYCLE_02.md). Capturas: [1440 px](screenshots/native-ui.png), [1024 px](screenshots/native-ui-1024.png), [900 px](screenshots/native-ui-900.png), [render workspace](screenshots/render-workspace.png), [Cycles](screenshots/cycles-kitchen.png). Limites: sem prova Linux/GPU/1080p/4K, JPEG pela UI e aceitação integral da master spec. O relatório do ciclo 01 abaixo é histórico; as capturas foram atualizadas para o ciclo 02.

---

# Test report — ciclo 0.1.0

Execução local em 2026-09-29/30: Windows 11 Pro x86_64, Ryzen 5 5500 (6 cores/12 threads), 15,9 GiB de RAM. Toolchain isolado MSYS2 UCRT64: GCC 16.2.0, CMake 4.4.3, Ninja 1.13.2, Qt 6.11.2, OpenCASCADE 7.9.3, libzip 1.11.4, Catch2 3.16.0. Blender 5.2.1 LTS, executado como processo externo.

**Resultado: build nativo aprovado; 15 casos / 395 assertions aprovados; smoke de interface, encerramento forçado/recuperação e Cycles aprovados. Não é aceitação integral da master spec nem prova de suporte Linux.**

## Evidência executada

| Área | Exercício real | Resultado |
|---|---|---|
| Medidas | Expressões limitadas, vírgula decimal, rejeição de entradas inválidas | Passou |
| Comandos | Criação, UUIDs, undo/redo, alteração inválida preserva documento/histórico | Passou |
| Geometria | B-rep válido, volume de parede com recorte, abertura acompanha rotação da parede | Passou |
| Módulos | Largura de 947 mm, corpo de 18 mm, recomposição | Passou |
| Automações | União de tampo, recálculo por fontes, exclusão em cascata | Passou no conjunto reto coberto |
| Starter Library | Todas as 25 receitas distribuídas instanciadas; cada componente passa BRepCheck_Analyzer | Passou |
| Biblioteca | SQLite/FTS5, acentos, 10.000 fixtures, favoritos/recentes, consulta malformada | Passou |
| Persistência | Roundtrip, backup, validação antes da substituição, arquivo anterior preservado | Passou |
| Containers adversos | Caminho inesperado, ZIP truncado, UUID/órfão/ciclo inválido | Rejeitados |
| DXF | LINE, LWPOLYLINE reta, POLYLINE, ARC, CIRCLE; metros → mm; layers; save/open | Passou |
| Textura | JPG → PNG normalizado, deduplicação por SHA256, remoção do original e roundtrip | Passou |
| Integridade de assets | Bytes adulterados são rejeitados | Passou |
| Recuperação core | Cinco versões por UUID, dois projetos isolados, arquivo corrompido preservado/ignorado | Passou |
| Interface | Cliques desenham parede de 2.000 mm; biblioteca por duplo clique; eventos Qt de entrada/movimento/drop geram ghost e inserem a 1.000 × 1.000 mm; undo | Passou |
| Inspector | Clique em Aplicar com `753+59,5` → 812,5 mm; undo/redo; save/open idêntico | Passou |
| Crash recovery | Processo escritor cria autosave, recebe kill, novo processo usa diálogo nativo e restaura UUIDs/edição | Passou |
| Cycles | Snapshot de sólidos reais, processo Blender, CPU/denoise, PNG de 320 × 180 / 16 samples | Passou |
| Falha de render | Remoção da câmera causa exceção Python; aplicação informa Falhou; hash da imagem anterior permanece igual | Passou |
| Qualidade estática | Warnings habilitados, build sem warnings do C++; clang-format dry-run, Python py_compile, Bash syntax | Passou |
| Instalação em staging | `cmake --install` no Windows, recursos/licença copiados, smoke UI do executável instalado com DLLs do toolchain no PATH | Passou; não é instalador portátil nem QA Linux |

O teste de drop injeta eventos no viewport real e usa a biblioteca/engine reais; não substitui exploração manual de arraste entre janelas, múltiplos monitores ou plataformas. O smoke de crash testa encerramento **depois** de um autosave durável, sem simular perda de energia ou término durante escrita.

## Reproduzir neste host

```powershell
$env:PATH = "$env:USERPROFILE\.codex\tmp\lmx-tools\msys64\ucrt64\bin;$env:PATH"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure -V
./build/libremax-architect.exe --ui-smoke build/evidence
./build/libremax-architect.exe --recovery-smoke
./build/libremax-architect.exe --render-smoke build/render-evidence --blender 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe'
```

Logs locais: `build/evidence/core-report.txt`, `ui-report.txt`, `recovery-report.txt` e `build/render-evidence/render-report.txt`. Capturas reais versionadas: [1440 × 900](screenshots/native-ui.png), [1024 × 768](screenshots/native-ui-1024.png), [Cycles CPU 320 × 180](screenshots/cycles-kitchen.png). Os `.lmx` em `examples` são containers reais gerados pelo aplicativo.

## Correções encontradas pelos testes/revisão

- Temporário ZIP em Windows: uso de diretório temporário evitou bloqueio por handle de arquivo aberto.
- Cabeçalho `$INSUNITS` do DXF: pré-leitura resolveu unidade ignorada pelo parser de entidades.
- Transição SQLite/WAL: finalização da consulta de versão resolveu lock ativo.
- Render: arquivo temporário próprio, validação da resolução e cópia atômica impedem aceitar/reusar uma imagem antiga após erro. `--python-exit-code 1` propaga falhas do script.
- Recuperação: arquivos corrompidos deixaram de bloquear abertura; metadados da lista evitam reter todos os documentos em memória.
- Validação: famílias/dimensões decorativas, frente de vidro e metadados de material inválidos são rejeitados antes de aplicar o comando.
- Vista isométrica: opção Abrir vista permite inspecionar o interior; geometria do projeto/render permanece completa.

## Não comprovado

Linux/ARM64/Wayland nativo, AppImage/deb/Flatpak instalados, execução com rede desabilitada, crash durante ZIP write, perda de energia, timers de autosave ao longo de uma sessão extensa, escolha de múltiplas versões pela UI, importadores de malha/packs, fuzzing completo, GPU, JPG renderizado, qualidade 1080p/4K, vidro/espelho/textura em cenas de referência, todos os workflows da matriz e desempenho de cenas grandes. O Blender atual emite avisos de depreciação sobre `use_nodes` para Blender 6; não houve falha por esses avisos no 5.2.1.

CI Linux configurado, sem remoto e sem execução nesta sessão. [Matriz de paridade](VDMAX_ARCHITECT_PARITY.md) e [benchmark limitado](BENCHMARK_REPORT.md) complementam este relatório.
