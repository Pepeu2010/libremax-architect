# Alvo mínimo obrigatório — i3-6006U, HD Graphics 520, 8 GB

Requisito confirmado pelo usuário: o LibreMax deve permitir montar um apartamento e gerar imagens em um notebook com **Intel Core i3-6006U, Intel HD Graphics 520 e 8 GB de RAM**. Esse computador passa a ser o alvo mínimo de aceitação do produto. Isso é um requisito; ainda não é uma afirmação de teste físico concluído.

O [i3-6006U](https://www.intel.com/content/www/us/en/products/sku/91157/intel-core-i36006u-processor-3m-cache-2-00-ghz/specifications.html) tem dois núcleos, quatro threads e GPU Skylake. A GPU compartilha a memória do sistema. Suporte OpenGL depende também do driver instalado.

## Caminho implementado na versão 0.12

- Sem preferência anterior, CPUs com até quatro threads começam em **Leve**: sem mapas de imagem na vista, sem MSAA e com um worker de miniaturas. A escolha manual continua sendo preservada.
- O editor usa OpenCASCADE/OpenGL. Modelos, medidas, materiais e texturas do projeto não são removidos pelo modo Leve; o render conserva os detalhes.
- Fotos usam **Cycles CPU** nessa máquina. A HD 520 não é um dispositivo de render Cycles oneAPI compatível; [oneAPI exige GPUs Intel Arc compatíveis](https://docs.blender.org/UATEST/manual/en/4.5/render/cycles/gpu_rendering.html). AUTO pode recorrer à CPU; escolher CPU evita a procura por backends GPU.
- O processo Blender usa prioridade reduzida e `max(1, min(8, threads disponíveis - 2))` threads de render. Em quatro threads disponíveis, solicita duas, preservando recursos para editar. Esse limite não é um limite de memória do processo.
- O aplicativo aceita **Blender 4.5 LTS+**. Para esse notebook, use 4.5 LTS. As [exigências oficiais](https://www.blender.org/download/requirements/) mantêm suporte Intel Broadwell ou posterior no 4.5; a linha atual começa em Kaby Lake. O i3 dual-core continua abaixo da indicação atual de quatro núcleos do Blender; execução CPU neste modelo precisa de comprovação física.
- Em Blender 4.5, Céu natural usa Nishita. Blender 5+ mantém Multiple Scattering. HDRI incorporado, materiais PBR, Kelvin, LED, PNG/JPEG/EXR e denoise permanecem disponíveis. Os dois modelos de céu podem produzir iluminação diferente.
- Uma imagem por vez, barra de progresso, tempo decorrido, previsão do motor e cancelamento. Comece pelo modo **Rápido, 640 × 360** para ajustar câmera/luzes antes de calcular uma imagem maior. O render final pode levar bastante tempo nessa CPU; não há duração garantida.

## Aceitação no notebook

A fonte 0.13 também reutiliza sólidos ao mover/girar cópias e malhas no Cycles,
com preservação de modelos e texturas. Isso reduz trabalho e pacotes repetidos;
não substitui as medições de fluidez e RAM abaixo. [Provas e limites](SHARED_GEOMETRY.md).

Antes de declarar suporte confirmado, executar no hardware real:

1. Instalar e abrir o aplicativo com driver Intel funcional; registrar sistema, versão do driver e resolução da tela.
2. Confirmar início em Leve, catálogo completo, busca e criação de cômodo.
3. Abrir `examples/apartamento-moderno.lmx`; mover, girar, encaixar e editar móveis em planta e 3D. Registrar fluidez e picos de RAM durante órbita/arraste e carregamento.
4. Salvar e reabrir o projeto, com geometria e materiais preservados.
5. Selecionar Blender 4.5 LTS e CPU, renderizar Rápido; verificar imagem válida, progresso/estimativa, edição durante cálculo e cancelamento.
6. Calcular uma imagem de apresentação em 1920 × 1080, registrar tempo e pico de RAM, verificar finalização e preservação da imagem anterior após falha.

Sem crash, falta de memória ou bloqueio prolongado da edição nessas cenas. Cenas maiores e texturas mais pesadas exigem avaliação própria. Não extrapolar testes em Ryzen/Radeon, afinidade de CPU ou Mesa para resultados na HD 520.

## Evidência atual

O Blender oficial **4.5.9 LTS Windows x64**, baixado em versão portátil, teve SHA256 conferido com o manifesto oficial: `41da973b9bf95bb312cbeff4d1982feb13259b43c821686b9bafea4dfe5477cf`. Produziu a imagem real do apartamento moderno com céu Nishita, 27 mapas PBR, CPU, denoise, 320 × 180 / oito amostras. Uma falha posterior preservou a imagem. Essa prova foi feita no host Ryzen/RX 7600, não no i3/HD 520.

Fila e galeria, quatro imagens HDRI/EXR e sete renders de iluminação passaram com Blender 4.5.9 CPU no Windows. Progresso, estimativa positiva, cancelamento e duração persistida passaram. Biblioteca de 175 miniaturas, três modos e montagem passaram; Blender 5.2.1 também manteve o render CPU do apartamento. Núcleo: 40 testes / 2.269 verificações. A [CI Linux](https://github.com/Pepeu2010/libremax-architect/actions/runs/37130831318) passou com os dois motores e confirmou início automático em Leve num runner com duas CPUs lógicas. A [CI dos instaladores](https://github.com/Pepeu2010/libremax-architect/actions/runs/37130831328) passou em Windows e Ubuntu instalado em runner novo. A [prévia 0.12](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.12.0-preview.1) está publicada. O teste físico acima permanece pendente. [Relatório](TEST_REPORT.md), [biblioteca e modos de edição](MODEL_LIBRARIES_PERFORMANCE.md).

A [prévia 0.13](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.13.0-preview.1) acrescenta geometria compartilhada, seleção independente e pacotes de render menores. Os 43 testes, UI/montagem e comparação contra o leitor anterior passaram em Windows; a CI Linux confirmou ambos os motores e a instalação Windows/Ubuntu passou. Isso reduz trabalho repetido, mas continua sem medir o notebook físico. [Provas e limites](SHARED_GEOMETRY.md).
