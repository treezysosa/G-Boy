#!/bin/bash
set -euo pipefail
stage="$1"
output="$2"
root="$output/pkg-root"
mkdir -p "$root/Applications" "$root/Library/Audio/Plug-Ins/VST3" "$root/Library/Audio/Plug-Ins/Components" "$root/Library/Application Support/G Boy"
ditto "$stage/Standalone/G Boy.app" "$root/Applications/G Boy.app"
ditto "$stage/VST3/G Boy.vst3" "$root/Library/Audio/Plug-Ins/VST3/G Boy.vst3"
ditto "$stage/AU/G Boy.component" "$root/Library/Audio/Plug-Ins/Components/G Boy.component"
ditto "$stage/Factory Content" "$root/Library/Application Support/G Boy/Factory Content"
ditto "$stage/Documentation" "$root/Library/Application Support/G Boy/Documentation"
cp "$stage/Source.tar.gz" "$stage/BUILD-INFO.json" "$root/Library/Application Support/G Boy/"
for binary in "$root/Applications/G Boy.app/Contents/MacOS/G Boy" "$root/Library/Audio/Plug-Ins/VST3/G Boy.vst3/Contents/MacOS/G Boy" "$root/Library/Audio/Plug-Ins/Components/G Boy.component/Contents/MacOS/G Boy"; do
  lipo "$binary" -verify_arch arm64 x86_64
done
pkgbuild --root "$root" --identifier audio.gboy.instrument --version 0.6.0 --install-location / --ownership recommended "$output/G-Boy-0.6.0-rc.1-macOS.pkg"
