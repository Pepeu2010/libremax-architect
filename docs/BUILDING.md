# Build e plataformas

Fonte C++20, CMake ≥3.24, Ninja, Qt ≥6.4 (Core/Gui/Widgets/Sql/Concurrent/Test), OpenCASCADE, nlohmann/json, spdlog, libzip e Catch2. Preferir Catch2 3; aceitar 2.13 em distribuições com pacote antigo. Dependências não são baixadas pelo aplicativo em runtime.

Linux: `bash scripts/bootstrap.sh`, `bash scripts/build.sh`, `bash scripts/test.sh`. Qt SQL SQLite deve conter FTS5. O viewport inicial Linux usa X11/Xw_Window; em Wayland usar XWayland (`QT_QPA_PLATFORM=xcb`). Wayland nativo ainda não validado. Debian 12/Qt6.4, Debian 13, Ubuntu 24.04+ e Mint correspondentes precisam de execução real antes de declarados suportados.

Windows de validação: MSYS2 UCRT64 isolado em `~/.codex/tmp/lmx-tools/msys64`. Pacotes GCC, CMake, Ninja, Qt6, OpenCASCADE, libzip, Catch2, spdlog e nlohmann-json. Não alterar PATH global; no PowerShell:

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
