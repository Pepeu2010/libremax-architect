# Blender incluído na instalação

A partir de 0.14, os instaladores Windows x64 e Ubuntu 24.04 amd64 incluem o
Blender **4.5.9 LTS oficial, sem modificações**, com Cycles, Python, bibliotecas
e licenças. O usuário instala o LibreMax e pode criar imagens sem baixar ou
selecionar outro executável.

O motor fica em `share/libremax/runtime/blender`, ao lado dos recursos do
aplicativo. A descoberta usa primeiro uma escolha explícita do usuário, depois
o motor incluído e, por último, instalações externas. A instalação e remoção
do LibreMax não alteram o Blender instalado separadamente. Os projetos e
preferências permanecem fora das pastas do aplicativo.

No Windows, a distribuição oficial contém também `blender.crt`,
`blender.shared` e o Python interno. Essas pastas completas acompanham o
instalador, além das DLLs Qt/OpenCASCADE do LibreMax. Não é necessário instalar
Python, SDKs ou Blender separadamente. O instalador funciona sem internet.

No Ubuntu, o `.deb` inclui o motor e declara dependências das bibliotecas do
sistema. Instale com `apt install ./arquivo.deb` ou o gerenciador de aplicativos;
o gerenciador resolve as dependências automaticamente. Se elas ainda não
estiverem disponíveis, a instalação precisa de acesso aos repositórios Ubuntu.
Depois da instalação, edição e render locais não exigem internet.

O empacotador Debian calcula as dependências nativas do aplicativo, do motor e
do Python. Quatro plugins opcionais de GPU permanecem incluídos, mas não tornam
obrigatória a instalação simultânea de drivers AMD, NVIDIA e Intel. CPU funciona
com as dependências normais do pacote; render GPU continua dependendo do driver
compatível com a placa escolhida. A varredura mantém as demais bibliotecas,
incluindo denoise CPU e módulos Python/MaterialX.

## Preparação e integridade

`scripts/blender-runtime.json` fixa versão, URLs oficiais e SHA256 por plataforma.
`scripts/prepare-blender-runtime.py` verifica o arquivo antes de extrair,
recusa diretórios de saída preenchidos e registra tamanho/hash de cada arquivo
em `libremax-runtime.json`. A distribuição completa preserva as licenças em
`license/`. Os builds de distribuição exigem `LMX_BLENDER_RUNTIME_DIR`; não
podem gerar uma release sem o motor preparado.

As Releases incluem `Blender-4.5.9-Source.tar.xz`, obtido da distribuição oficial
e conferido pelo hash fixado, junto às fontes LibreMax e aos checksums dos
instaladores. [Licenciamento Blender](https://www.blender.org/about/license/).

## Verificação de distribuição

O gate Windows instala o Setup em uma conta limpa, testa recursos e executa
Cycles CPU sem `--blender`, com PATH limitado ao sistema. Depois desinstala,
confirma a remoção do motor e preserva um projeto criado fora do aplicativo.
Um runner Ubuntu separado instala o `.deb` e repete descoberta, render, UI e
remoção. O teste de descoberta exige que o executável esteja dentro do pacote,
tenha a versão fixada e conserve o manifesto e as licenças.

O motor incluído aumenta o tamanho do download e o espaço ocupado. Blender 4.5
mantém o caminho CPU destinado ao i3-6006U/HD 520/8 GB. A inclusão do motor não
substitui o [teste físico de desempenho ainda pendente](MINIMUM_HARDWARE.md).

## Resultado da prévia 0.14

Fonte compilada: `5257f576bdc99b4fabc1666ae62ac77382038220`, tag
[`v0.14.0-preview.3`](https://github.com/Pepeu2010/libremax-architect/releases/tag/v0.14.0-preview.3).
[Instalação Windows e Ubuntu limpos](https://github.com/Pepeu2010/libremax-architect/actions/runs/37144046796)
passou com descoberta automática, render real do apartamento em CPU,
320 × 180/oito amostras, remoção do motor e preservação de projeto.
No Windows, o PATH do teste continha apenas pastas do sistema. No Ubuntu,
o `.deb` foi instalado em runner separado e também passou pela UI/biblioteca.

O build local passou em 43 testes/2.798 verificações, além de rejeitar checksum
inválido, saída já preenchida e distribuição sem motor. O filtro Debian manteve
as bibliotecas obrigatórias em 12 verificações de limites. A
[CI Linux completa](https://github.com/Pepeu2010/libremax-architect/actions/runs/37144046955)
também passou, incluindo renders reais nos motores 4.5 e 5.2.

Downloads publicados: Windows **481.352.906 bytes**; Ubuntu **587.061.390 bytes**.
Os digests GitHub dos sete assets correspondem aos manifestos combinado e de
plataforma. Os três manifestos foram baixados e seus hashes também conferidos.
Os instaladores publicados completos não foram baixados novamente no host;
a instalação real ocorreu nos gates de CI. O Setup local, separado desses
assets, também foi gerado e seu payload renderizou com PATH limpo.
