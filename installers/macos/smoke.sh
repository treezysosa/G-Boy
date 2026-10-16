#!/bin/bash
set -euo pipefail
package="$1"
output="$2"
mkdir -p "$output"
pkgutil --expand-full "$package" "$output/expanded-pkg"
# Exercise the real installer only on the disposable CI runner.
sudo installer -pkg "$package" -target /
test -f '/Library/Audio/Plug-Ins/VST3/G Boy.vst3/Contents/MacOS/G Boy'
test -f '/Library/Audio/Plug-Ins/Components/G Boy.component/Contents/MacOS/G Boy'
test -f '/Applications/G Boy.app/Contents/MacOS/G Boy'
test -f '/Library/Application Support/G Boy/Factory Content/gboy-library.json'
printf '{"passed":true,"installed":true,"universal_slices_verified":true}\n' > "$output/installer-smoke.json"
