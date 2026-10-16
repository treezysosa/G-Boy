# G Boy — Phase 1 (0.1.0)

A C++20 sample instrument with a JUCE 8 interface. Source foundation, not a validated production release. Windows VST3/Standalone and macOS VST3/AU/Standalone targets are configured. No Windows or macOS binary has been built in the supplied Linux workspace.

Implemented: 64-voice stereo sampler, linear interpolation, MIDI note-on/off and channel-aware release, oldest-voice stealing, ADSR, smoothed master gain, soft output limiting, four original generated sounds, on-screen keyboard, hold-to-preview A button, B panic, sample drag/drop and asynchronous decoding, factory selector, save/load `.gboy` presets and host project state, resizable handheld-style GUI, waveform and output meter, modern/retro palettes. Parameters are exposed to DAW automation. Imported samples use MIDI 60 as root. WAV/AIFF/FLAC support comes from JUCE's basic format readers.

## Windows build

Install Visual Studio 2022 with **Desktop development with C++**, Windows SDK, CMake and Git. In PowerShell, winget can install the tools (Visual Studio's installer must finish its workload installation):

```powershell
winget install --id Microsoft.VisualStudio.2022.Community --exact --override "--wait --passive --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
winget install --id Kitware.CMake --exact
winget install --id Git.Git --exact
```

Open Developer PowerShell for VS 2022, enter this repository, then:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

First configuration fetches JUCE tag 8.0.6, which bundles VST3 support. For offline builds obtain JUCE separately, then configure with `-DJUCE_SOURCE_DIR=C:/SDKs/JUCE`. Outputs: `build/GBoy_artefacts/Release/VST3/G Boy.vst3` and `build/GBoy_artefacts/Release/Standalone/G Boy.exe`. Copy the **whole** `.vst3` bundle to `C:/Program Files/Common Files/VST3/` and rescan in the DAW. Standalone: select the audio device/MIDI input in its settings menu, then use a MIDI controller or the on-screen keys.

## macOS build

```sh
xcode-select --install
brew install cmake git
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

AU is added only on macOS. Install the `.component` bundle into `~/Library/Audio/Plug-Ins/Components/` and VST3 into `~/Library/Audio/Plug-Ins/VST3/`. Signing/notarization is not implemented. The CI macOS job builds its native architecture; universal builds require the command above and validation on both architectures.

## Core-only builds and content

No JUCE or audio hardware is required for the DSP tests:

```sh
cmake -S . -B build-core -DGBOY_BUILD_PLUGIN=OFF
cmake --build build-core
ctest --test-dir build-core --output-on-failure
./build-core/gboy_generate_content
```

If CMake is absent, run:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Wpedantic -Isrc tests/CoreTests.cpp -o /tmp/gboy-tests
/tmp/gboy-tests
g++ -std=c++20 -O2 -Isrc tools/GenerateContent.cpp -o /tmp/gboy-generate
/tmp/gboy-generate
```

`factory-content/Phase1-Demo.wav` is an actual offline MIDI chord render. Factory sounds are also generated in memory at startup; the plugin does not require external factory WAV files. JSON preset files describe factory content; they are not `.gboy` preset files. Save user presets from the GUI.

## State and threading

One background loader decodes samples and publishes a pending immutable buffer. The callback swaps it at a block boundary and retires the previous buffer for worker deletion. No sample decoding, buffer destruction, or engine allocation occurs in rendering. UI notes use a fixed-capacity SPSC queue; overflow requests panic. MIDI supplied by the DAW is rendered at its sample offsets. GUI and host state use JUCE services; their realtime behavior still requires host validation.

Presets store parameter values and an absolute user sample path, not embedded audio. Keep samples in place when reopening projects. Missing or invalid samples retain the previous sound and show an error. Loading is limited to 30 seconds and stereo output to bound memory. Saving while a sample request is pending captures the last successfully decoded source. Sample changes stop current voices and can click; parameter changes other than master gain are not fully smoothed yet.

## Validation and scope

See [validation](docs/VALIDATION.md) for actual test results and remaining checks. Later phases (four layers, vocal workstation, FX, sequencer, searchable libraries) are not implemented or displayed as working controls. MIDI 2.0, sustain pedal, pitch bend, looping, streaming, root detection, custom root selection, CLAP, AAX, GPU rendering, undo, MIDI learn, installers and release signing are not included. Pro Tools support needs AAX and its licensed SDK; VST3 does not provide native Pro Tools support.

Before distribution, review JUCE's AGPLv3/commercial licensing requirements. Repository source is MIT; generated factory content is CC0-1.0. No Nintendo assets, game recordings or human vocal recordings are included.
