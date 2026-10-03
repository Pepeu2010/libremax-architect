# Build e plataformas

Fonte C++20, CMake ≥3.24, Ninja, Qt ≥6.4 (Core/Gui/Widgets/Sql/Concurrent/Test), OpenCASCADE, nlohmann/json, spdlog, libzip, FreeImage, OpenEXR ≥3 e Catch2. Preferir Catch2 3; aceitar 2.13 em distribuições com pacote antigo. Dependências não são baixadas pelo aplicativo em runtime.

Empacotamento usa Python ≥3.12 e curl para baixar os arquivos oficiais do motor,
sempre com SHA256 verificado. Windows 10/11 e runners usados já incluem curl;
o bootstrap Linux instala essa ferramenta de desenvolvimento.

Para usar sem compilar, veja [INSTALADOR](../INSTALADOR/README.md). O fluxo abaixo é para desenvolvimento.

## Reproduzir os instaladores

Compile uma cópia limpa da tag publicada com `-DLMX_DEPLOYMENT_BUILD=ON` e `-DLMX_BLENDER_RUNTIME_DIR` apontando para o motor preparado. A distribuição exige Blender incluído e recursos instalados em `share/libremax`, e gera executável Windows sem terminal. Não use a opção para abrir um binário solto na pasta de build.

No Windows, acrescente NSIS e Python ao toolchain UCRT64. Prepare o motor, configure e compile:

```powershell
python scripts/prepare-blender-runtime.py --platform windows --output build-install/blender-runtime
cmake -S . -B build-install/bin -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMX_DEPLOYMENT_BUILD=ON -DLMX_BLENDER_RUNTIME_DIR="$PWD/build-install/blender-runtime"
cmake --build build-install/bin --parallel 3
./scripts/package-windows.ps1 -ToolPrefix 'C:/caminho/msys64/ucrt64' -Python 'C:/caminho/msys64/ucrt64/bin/python.exe'
./scripts/verify-windows-installer.ps1 -Installer dist/windows/LibreMax-Architect-0.14.0-Windows-x64-Setup.exe -Gui
```

O verificador exige um usuário sem instalação/atalhos LibreMax existentes. Instala em uma pasta nova, limpa o PATH do processo de teste, verifica catálogo/SQLite/modelos/codecs/projeto e executa Cycles CPU usando o motor incluído automaticamente. Desinstala e confirma que um projeto permanece. `-Blender 'C:/caminho/blender.exe'` acrescenta um teste com motor externo. Os logs ficam em `build-install/installation-evidence`. Não modifica o PATH global.

No Ubuntu 24.04:

```bash
bash scripts/bootstrap.sh
python3 scripts/prepare-blender-runtime.py --platform linux --output build-linux-runtime
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DLMX_DEPLOYMENT_BUILD=ON -DLMX_BLENDER_RUNTIME_DIR="$PWD/build-linux-runtime"
cmake --build build-linux --parallel 3
ctest --test-dir build-linux --output-on-failure
cpack --config build-linux/CPackConfig.cmake -G DEB -B dist/linux
```

A [workflow dos instaladores](../.github/workflows/installers.yml) valida Windows e instala o `.deb` em um runner Ubuntu separado, sem checkout nem SDK do projeto, antes de publicar uma prévia. O Linux usa X11/XWayland; Mint 22 é alvo pela base Ubuntu, sem execução própria nesta entrega. DLLs Windows, licenças disponíveis e proveniência são descritas em [DEPENDENCIES](../INSTALADOR/windows/DEPENDENCIES.md).

Linux: `bash scripts/bootstrap.sh`, `bash scripts/build.sh`, `bash scripts/test.sh`. Qt SQL SQLite deve conter FTS5. O viewport inicial Linux usa X11/Xw_Window; em Wayland usar XWayland (`QT_QPA_PLATFORM=xcb`). Wayland nativo ainda não validado. Debian 12/Qt6.4, Debian 13, Ubuntu 24.04+ e Mint correspondentes precisam de execução real antes de declarados suportados.

Windows de validação: MSYS2 UCRT64 isolado em `~/.codex/tmp/lmx-tools/msys64`. Pacotes GCC, CMake, Ninja, Qt6, OpenCASCADE, libzip, Catch2, spdlog, nlohmann-json, FreeImage e OpenEXR. Não alterar PATH global; no PowerShell:

```powershell
$toolBin = "$env:USERPROFILE\.codex\tmp\lmx-tools\msys64\ucrt64\bin"
$env:PATH = "$toolBin;$env:PATH"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
./build/libremax-architect.exe --ui-smoke build/evidence
./build/libremax-architect.exe --recovery-smoke
./build/libremax-architect.exe --render-smoke build/render-evidence --blender 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe'
```

`--examples examples` gera cozinha/dormitório válidos. O smoke UI exige sessão gráfica/OpenGL; o core usa Qt offscreen. O smoke render exige Blender configurado; testa Cycles CPU em 320×180, não desempenho GPU ou render final 1080p.

`scripts/package.sh` prepara `.deb` e AppDir em Linux. AppImage requer linuxdeploy/Qt plugin/appimagetool previamente verificados. Manifest Flatpak é rascunho: faltam módulos das dependências e integração segura de Blender no sandbox. Não confundir configuração de empacotamento com pacote já testado.

`./scripts/package-source.ps1` gera no Windows `dist/source.tar.gz`, `starter-library.tar.gz`, `sample-projects.tar.gz` e `SHA256SUMS`, somente a partir do HEAD commitado. Arquivos de desenvolvimento não commitados e toolchains não entram nesses arquivos.

Para reproduzir as capturas e render da interface 0.2, use os comandos de [CYCLE_02](CYCLE_02.md): `--render-size 960x540 --render-samples 64` e UI smoke com `--preview-image`. O smoke padrão continua pequeno para integração rápida.

No ciclo 0.3, `starter-materials` é instalado com os recursos. `package-source.ps1` inclui `starter-materials.tar.gz`; exemplos incorporam seis mapas PBR e têm aproximadamente 14,5 MB por .lmx. Render 1280×720/128 e controles de céu/PBR estão em [CYCLE_03](CYCLE_03.md).

0.4 instala também `starter-models` e o pacote de fontes inclui `starter-models.tar.gz`. Rode `--assembly-smoke build/assembly-evidence` para a montagem nativa; abra `examples/apartamento.lmx` para experimentar. O bootstrap instala zipcmp/zipmerge/ziptool exigidos pelo CMake libzip e os pacotes de desenvolvimento TBB/FreeImage referenciados pelo OpenCASCADE da distribuição. O CI fixa clang-format 23.1.1, igual ao formatador usado localmente.

0.6 usa `build-modern` neste host para manter a versão de teste separada do executável anterior. `run-windows.ps1` prefere essa pasta quando o binário existe; `-BuildDirectory build` escolhe outra compilação explicitamente. Não recompilar o mesmo executável enquanto ele ou seus testes estiverem rodando no Windows. `--modern-smoke build-modern/modern-evidence` verifica os modelos detalhados, colocação e arquivo independente; `--experience-smoke build-modern/experience-evidence` verifica tutorial, novo projeto, salvamento e biblioteca inicial. [Resultados e limites](CYCLE_06.md).

0.8 usa FreeImage para HDR e OpenEXR diretamente para EXR FLOAT32. O bootstrap instala `libopenexr-dev`; no UCRT64 use `mingw-w64-ucrt-x86_64-openexr` e `mingw-w64-ucrt-x86_64-freeimage`. O pacote inclui `starter-environments`, e `--installation-smoke` importa o HDRI instalado e salva/reabre um projeto v2 portátil. `--environment-smoke build/environment-evidence --blender /caminho/blender` verifica quatro renders reais e a galeria EXR; no Linux execute com Xvfb/Mesa como na CI.
