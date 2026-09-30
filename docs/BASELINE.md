# Baseline — 2026-09-29

- Workspace: vazio, exceto Git; nenhum código, teste, roadmap ou recurso Marceneiro a remover.
- Branch inicial `master`, sem commits, sem remote configurado.
- Host Windows; WSL ausente; CMake/Ninja/GCC/Qt/OpenCASCADE ausentes no PATH.
- Blender instalado em `C:/Program Files/Blender Foundation/Blender 5.2`; versão e capacidades devem ser verificadas por execução.
- Toolchain de validação isolado em `~/.codex/tmp/lmx-tools`, sem alterar PATH global ou instalar WSL.
- Não é possível compilar estado anterior ou rodar testes anteriores: não existem.
- Pacotes AppImage/.deb e execução Linux ainda exigem runner Linux. Fonte e CI serão preparados, sem inventar artefatos.
