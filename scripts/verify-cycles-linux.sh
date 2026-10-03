#!/usr/bin/env bash
# Integration fixture: download the official, checksummed engine; never replace Cycles with a mock.
set -euo pipefail
cd "$(dirname "$0")/.."
app="${1:-./build/libremax-architect}"
evidence="${2:-build/queue-evidence}"
runtime="build/blender-runtime"
archive="blender-5.2.1-linux-x64.tar.xz"
mkdir -p "$runtime" "$evidence"
curl --fail --location --retry 3 "https://download.blender.org/release/Blender5.2/$archive" -o "$runtime/$archive"
printf '%s  %s\n' a31f524fa99a527d3d52b7f5aaa68c34e1a19d5a1c9473f79c5cc610fd5b10e9 "$runtime/$archive" | sha256sum --check -
tar -xf "$runtime/$archive" -C "$runtime"
engine="$(realpath "$runtime/blender-5.2.1-linux-x64/blender")"
xvfb-run -a env QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 "$app" --queue-smoke "$evidence" --blender "$engine" | tee "$evidence/queue-report.txt"
test -f "$evidence/edited-project.lmx"
test -f "$evidence/render-gallery.png"
xvfb-run -a env QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 "$app" --environment-smoke "$evidence/environment" --blender "$engine" | tee "$evidence/environment-report.txt"
test -f "$evidence/environment/portable-hdri.lmx"
test -f "$evidence/environment/exr-native-preview.png"
