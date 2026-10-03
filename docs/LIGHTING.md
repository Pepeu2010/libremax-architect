# Iluminação nativa — fontes 0.10

Em **Iluminação → Nova luz**, escolha spot, fita LED, painel de luz, ponto ou sol. A luz começa no centro do cômodo atual, próxima do teto. A posição pode ser ajustada ao criar ou nas propriedades. O alvo inicial fica abaixo da fonte; direção/alvo e giro ficam nos ajustes avançados. Se não houver cômodo, usa a posição inicial do editor.

Escolha Quente (2700 K), Aconchegante (3000 K), Neutra (4000 K), Luz do dia (5000 K), Fria (6500 K), Outra temperatura (1000–12000 K) ou Cor escolhida. Kelvin usa a conversão nativa do Blender, conferida na API do Blender 5.2.1 instalado; a fita LED usa o mesmo RGB linear do motor. [API oficial de luz](https://docs.blender.org/api/5.3/bpy.types.Light.html). O manual diferencia potência luminosa radiométrica do consumo elétrico da lâmpada: [luzes no Blender](https://docs.blender.org/manual/en/dev/render/lights/light_object.html).

**Brilho** controla a intensidade na imagem; não é o consumo elétrico de uma lâmpada comercial. Área/ponto/spot usam a potência radiométrica do Cycles; sol usa irradiância. LED usa fluxo radiométrico dividido pela área e por pi. Os valores iniciais são sugestões, ajustáveis pela prévia; não substituem um cálculo luminotécnico de projeto executivo.

| Tipo | Conversão real | Controles |
|---|---|---|
| Ponto | POINT | Brilho, cor/Kelvin e raio em centímetros |
| Spot | SPOT | Brilho, cor/Kelvin, raio, ângulo e suavidade do feixe |
| Painel | AREA | Retângulo/redonda/quadrada, comprimento e largura, alvo e giro |
| LED | Uma malha plana emissiva contínua | Comprimento, largura, brilho, cor/Kelvin, alvo e giro |
| Sol | SUN | Intensidade, cor/Kelvin, ângulo de sombra e direção |

LED emite apenas pela frente, orientada para o alvo; o verso absorve luz. Não é uma coleção de pontos, nem uma imagem simulando brilho. São quatro vértices e uma face. O script do tradutor acompanha o instalador e cada cópia de render na fila, preservando a repetição do pedido. As luminárias decorativas do catálogo continuam separadas das fontes de luz.

Os marcadores amarelos são auxiliares selecionáveis do editor; não entram na malha exportada nem substituem a iluminação da foto. O painel estreito coloca rótulos acima dos controles, com rolagem vertical. Os ajustes passam pelo histórico de Desfazer/Refazer.

![Criar fita LED no diálogo nativo](screenshots/lighting-led-dialog.png)

![Marcador LED e propriedades em janela de 900 pixels](screenshots/lighting-led-inspector-900.png)

## Projeto portátil e compatibilidade

A nova iluminação exige `.lmx` versão **3**, para impedir que versões antigas abram Kelvin/LED sem reproduzir o resultado. A promoção acontece ao criar ou editar uma luz. Use **Salvar como** para manter uma cópia em arquivo separado. Abra projetos v1/v2 normalmente. HDRI, EXR e snapshots mantêm v3 sem rebaixá-la para v2. Abra arquivos v3 no LibreMax 0.10+; versões anteriores recusam o formato. Não existe conversão automática de volta para versões antigas.

## Evidência

Windows Release: **37 testes / 2.233 verificações** passaram. O teste nativo passou com cinco diálogos de criação, edição Kelvin/medidas, raio/ângulo/formato/giro, undo/redo, layout de 900 pixels e projeto v3 salvo/reaberto. Projetos antigos com direção de luz nula continuam aceitos; o editor e o Cycles usam a direção vertical para baixo nesse caso.

O fluxo pela interface produziu **sete imagens reais Cycles CPU**, 160 × 90 / 16 amostras, no Blender 5.2.1 LTS: LED 3000 K, LED 6500 K, LED desligado, ponto, spot, área retangular e sol. Os descritores vieram dos objetos reais do Blender. Raio de 7 cm virou 0,07 m; painel 120 × 35 cm virou 1,2 × 0,35 m; sombra do sol 0,75 grau virou aproximadamente 0,01309 radiano. O último render foi EXR com prévia integrada.

Na cena controlada, a média RGB foi LED quente **34,01 / 22,30 / 8,56**, LED frio **26,06 / 25,33 / 25,42**, e LED desligado **0,084 / 0,084 / 0,084**. A comparação prova emissão e alteração da cor refletida. Não mede a qualidade de apresentação de um apartamento nem superioridade frente a concorrentes.

![LED quente calculado no Cycles](screenshots/lighting-led-warm.png)
![LED frio calculado no Cycles](screenshots/lighting-led-cold.png)

Continuar editando a temperatura durante os renders preservou os snapshots originais. Os renders v3 e EXR permaneceram v3. Resultados locais: `build-lighting/lighting-release-report.txt` e `build-lighting/native-release`.

O mesmo projeto LED produziu uma imagem 320 × 180 / 16 amostras pelo pipeline QProcess na RX 7600 deste Windows. O Blender confirmou GPU/HIP sem fallback; uma falha posterior preservou a imagem anterior. `build-lighting/hip-report.txt` registra o resultado. Isso verifica essa fonte e esse hardware, sem ampliar a conclusão para GPUs não testadas.

A [CI Linux 0.10](https://github.com/Pepeu2010/libremax-architect/actions/runs/37125774451) passou com núcleo, formato, smokes nativos, fila, HDRI/EXR e as sete imagens de iluminação Cycles CPU/Mesa/Xvfb. A [CI dos instaladores](https://github.com/Pepeu2010/libremax-architect/actions/runs/37125785735) passou: Windows com PATH sem SDK e os 148 modelos; `.deb` instalado em runner Ubuntu 24.04 novo com biblioteca, três modos, colocação, salvamento e desinstalação preservando projetos. O pacote Linux em staging também executou os sete renders. No host Windows, o payload empacotado executou criação/edição e sete imagens CPU, além do LED HIP, com PATH sem SDK. Essas verificações usam a revisão `7c5a662e85a3b35c672f2065dbf459baf47b1fad`.

## Apartamento em 4K

Os instaladores e o código da revisão verificada estão na [Release 0.10.0-rc.1](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.10.0-rc.1). Os hashes dos seis assets remotos coincidiram com os arquivos locais; os dois binários vieram da CI acima. [Checksums e relatório](TEST_REPORT.md).

O apartamento moderno existente, com áreas em 4000 K e uma fita LED de teto em 3000 K, gerou **3840 × 2160**, denoise, 512 amostras máximas/adaptive sampling e 12 reflexões via `RenderJob`/QProcess. O motor confirmou GPU/HIP sem fallback. O tempo informado pelo Blender foi **08:08,98**; não inclui toda a preparação nativa. Driver RX 7600: 32.0.32015.2008. O teste negativo seguinte preservou o hash da imagem, `a4bc54b4911ef7576a2395a0eb72de606bba65b79c8ef412dc8fa0db1d610165`.

![Apartamento em 4K calculado pelo Cycles HIP](screenshots/lighting-apartment-4k.png)

Este é um teste dos parâmetros de apresentação e saída 4K pelo pipeline, não do gesto de selecionar Final na galeria. Não verifica todas as câmeras, materiais, GPUs nem equivalência a fotografia. O sofá e alguns móveis continuam simples; a tonalidade quente e os acabamentos precisam de direção de cena. A maior resolução não acrescenta detalhes ausentes nos modelos.

Reprodução no host Windows, a partir da raiz do repositório:

```powershell
./build-render/libremax-architect.exe --lighting-smoke build-lighting/native-release --blender 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe'
python docs/fixtures/lighting-apartment-4k.py
./build-render/libremax-architect.exe --render-smoke build-lighting/apartment-4k --blender 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --render-device AUTO --expect-render-gpu --render-size 3840x2160 --render-samples 512 --render-project build-lighting/apartamento-iluminado.lmx
```

O gerador prepara somente a cena de aceitação; o leitor de produção valida o container antes do render. A amostra não acompanha o catálogo do instalador. Há outros requisitos abertos da master spec: materiais/mapas adicionais, HDRI blur, câmera/enquadramento, instâncias/tesselação, seleção Final pela interface e hardware físico adicional.
