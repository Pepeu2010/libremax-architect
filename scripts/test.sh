#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
ctest --test-dir build --output-on-failure --timeout 120
if [[ "${LMX_UI_TEST:-0}" == 1 ]]; then
  ./build/libremax-architect --ui-smoke build/evidence
  ./build/libremax-architect --recovery-smoke
fi
if [[ -n "${LMX_BLENDER:-}" ]]; then
  ./build/libremax-architect --render-smoke build/render-evidence --blender "$LMX_BLENDER"
fi
