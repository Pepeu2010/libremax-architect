# Execução 0.17 — a moldura da câmera acompanha a foto

O editor agora usa a perspectiva da câmera persistente. A moldura delimita
a imagem em paisagem, retrato, quadrado e resolução personalizada.
Lente, sensor, deslocamentos, posição, alvo, inclinação e recorte são traduzidos
pelo mesmo contrato para OpenCASCADE e Blender. A moldura usa o tamanho real
do canvas, incluindo escala de tela, e conserva a composição ao redimensionar.

**Ver e ajustar enquadramento** permite compor pelo mouse. **Guardar enquadramento**
gera uma operação de histórico; enviar uma imagem guarda primeiro o ajuste
pendente. Alterar a câmera pelas propriedades atualiza a moldura e não permite
que um ajuste anterior sobrescreva essa alteração. O tutorial explica o fluxo.
O render continua em outro processo; a moldura não calcula iluminação ao vivo.
[Guia de uso](CAMERA_FRAMING.md).

## Evidência no Windows

- Build Release e formato C++ passaram. Núcleo: **54 testes / 3.194 verificações**.
- `--camera-smoke` passou com Blender **4.5.9 LTS e 5.2.1 LTS**: nove pontos
  em quatro formatos, comparados com a projeção nativa e com
  `bpy_extras.object_utils.world_to_camera_view` do próprio Blender.
- Maior diferença nativa: aproximadamente **1,2 × 10⁻¹³ pixel** antes da
  conversão para coordenadas inteiras. O teste exige no máximo 10⁻⁶ pixel.
  A comparação Blender, que também inclui distância em metros, ficou abaixo
  de **6 × 10⁻⁷**; o teste exige 10⁻⁵.
- Cinco imagens Cycles CPU reais por execução, oito amostras: 320×180,
  180×320, 240×240, 320×240 e 320×240. Dimensões e variação visível foram
  verificadas; a quinta usa o botão nativo de envio com ajuste ainda pendente.
- Botões visíveis em 900 pixels, roda/arrasto/giro, edição das propriedades,
  guardar, Desfazer/Refazer e `.lmx` portátil passaram. Escala de tela 150%
  também passou no teste de composição e envio.
- `--ui-smoke` passou: 203 miniaturas, encaixe real de sofá/relógio,
  seleção de cópias, três modos de edição e inspector compacto sem overflow.
- O render 480×270/32 foi comparado ao leitor arquivado 0.12: diferença média
  de canal **0,010885/255**, com cerca de **0,00077%** dos pixels acima de
  24/255. A câmera padrão preserva a aparência anterior nessa cena controlada.

Provas locais: `build-render/camera-017-release-45/`,
`build-render/camera-017-release-52/`, `build-render/camera-017-release-dpi150/`,
`build-render/camera-017-archived-parity/` e `build-render/ui-017/`.
A captura compacta é do aplicativo nativo compilado.

## Gates e limites

A [CI Linux](https://github.com/Pepeu2010/libremax-architect/actions/runs/37163354243)
inclui o teste com os dois Blenders e Mesa; a
[CI dos instaladores](https://github.com/Pepeu2010/libremax-architect/actions/runs/37163354206)
executa o teste nos pacotes Windows/Ubuntu. Ambas usam a fonte
`c3d68484d933a1765228a025386874afff859cec`. A CI Linux completa e os gates
do pacote Linux/Ubuntu instalado passaram. As capturas do pacote Linux foram
inspecionadas: foto visível e controles compactos acessíveis. O gate Windows
ainda precisa concluir; os instaladores publicados permanecem na prévia 0.16.

Essa prova não cobre todas as GPUs/driver, navegação a pé, vistas divididas,
gizmos ou desempenho em apartamentos grandes. A cena de calibração usa sólidos
simples; não comprova equivalência fotográfica ao VDMax. O teste físico
i3-6006U/HD 520/8 GB e a comparação direta continuam pendentes. A master spec
e a meta de superar o VDMax permanecem em andamento.
