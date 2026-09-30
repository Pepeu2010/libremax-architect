#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$(uname -s)" != Linux ]]; then
  echo "Linux packages require a Linux build and QA." >&2
  exit 1
fi
ctest --test-dir build --output-on-failure
cpack --config build/CPackConfig.cmake -G DEB -B dist
cmake --install build --prefix "$PWD/dist/AppDir/usr"
if [[ -n "${LMX_LINUXDEPLOY:-}" && -n "${LMX_APPIMAGETOOL:-}" ]]; then
  export QMAKE="${LMX_QMAKE:-/usr/lib/qt6/bin/qmake}"
  "$LMX_LINUXDEPLOY" --appdir dist/AppDir --executable dist/AppDir/usr/bin/libremax-architect \
    --desktop-file resources/libremax-architect.desktop --icon-file resources/libremax.svg --plugin qt
  "$LMX_APPIMAGETOOL" dist/AppDir dist/LibreMax-Architect-x86_64.AppImage
else
  echo "AppDir prepared. Provide vetted linuxdeploy, its Qt plugin and appimagetool to create AppImage."
fi
git archive --format=tar.gz --output=dist/source.tar.gz HEAD
find dist -maxdepth 1 -type f ! -name SHA256SUMS -exec sha256sum {} + > dist/SHA256SUMS
