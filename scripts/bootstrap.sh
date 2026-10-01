#!/usr/bin/env bash
set -euo pipefail
if [[ "$(uname -s)" != Linux ]]; then
  echo "Use Ubuntu/Mint/Debian Linux for this bootstrap. Windows: see docs/BUILDING.md." >&2
  exit 1
fi
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build pkg-config qt6-base-dev libqt6sql6-sqlite \
  libocct-foundation-dev libocct-modeling-data-dev libocct-modeling-algorithms-dev \
  libocct-visualization-dev libocct-data-exchange-dev libzip-dev zipcmp zipmerge ziptool nlohmann-json3-dev \
  libspdlog-dev catch2 libtbb-dev libfreeimage-dev libgl1-mesa-dev libx11-dev clang-format clang-tidy python3
echo "Install Blender 5.2 LTS separately and choose its executable in the Render panel."
