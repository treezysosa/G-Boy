# G Boy — Phase 2 (0.2.0)

A C++20 sample instrument with a JUCE 8 interface. Source foundation, not a validated production release. Windows VST3/Standalone and macOS VST3/AU/Standalone targets are configured. Phase 1 compiled and passed its tests on GitHub Windows and macOS runners. Phase 2 adds the hybrid engine described below; see docs/VALIDATION.md for its build status.

Implemented: 64-voice stereo sampler, linear interpolation, MIDI note-on/off and channel-aware release, oldest-voice stealing, ADSR, smoothed master gain, soft output limiting, four original generated sounds, on-screen keyboard, hold-to-preview A button, B panic, sample drag/drop and asynchronous decoding, factory selector, save/load `.gboy` presets and host project state, resizable handheld-style GUI, waveform and output meter, modern/retro palettes. Parameters are exposed to DAW automation. Imported samples use MIDI 60 as root. WAV/AIFF/FLAC support comes from JUCE's basic format readers.


## Phase 2 hybrid instrument

Use LAYER 1–4 to edit a layer, enable it, and choose Sampler, Virtual analog, Wavetable, FM or Granular. Each layer has its own sample import, ADSR, level/pan/tuning, filter, LFO, macro and four modulation routes. Only layer 1 is enabled initially. There are 64 voices per layer (up to 256 layer voices for 64 played notes); dense patches cost more CPU and no adaptive CPU governor is implemented.

- **Virtual analog:** continuous sine → PolyBLEP saw → PolyBLEP pulse morph.
- **Wavetable:** three original frames with interpolation and six harmonic-limited mip levels. User wavetable import is not implemented.
- **FM:** two sine operators, ratio control and shape/index control. No oversampling; high modulation indices and high notes can alias.
- **Granular:** four overlapping Hann-window grains per voice, position and grain size controls; pitch follows MIDI and grains wrap around sample boundaries. This is not a full time-stretch or vocal formant engine.
- **Filter:** per-voice stereo topology-preserving state variable filter with bypass/LP/BP/HP modes and resonance.
- **Modulation:** four source/destination/bipolar-amount rows per layer. Sources: LFO, ADSR, velocity, CC1 mod wheel, note, macro. Targets: pitch (12 semitones per unit), cutoff (4 octaves), gain, pan, grain position and oscillator shape/FM index. Control rate is one update per 16 samples with smoothing. The LFO is free-running in Hz; there is no tempo sync or drag/drop routing yet.
- **MIDI:** per-channel ±2-semitone pitch bend and CC1. Hardware MIDI uses sample offsets; on-screen keys are queued for the next audio block.

The VA/WT/FM shape control affects those modes; FM ratio affects FM, grain controls affect Granular, and cutoff/resonance require an enabled filter. This keeps a single compact panel while retaining separate parameter values for every layer.

The Hybrid patches menu loads four original configurations: Handheld Stack, Glacier Grains, Indigo Bass and FM Prism. It replaces the entire patch. The factory-sample menu and IMPORT affect only the selected layer. SAVE/LOAD applies to all four layers. Phase 1 `.gboy` presets migrate with extra layers disabled and new parameters reset to defaults; the original parameter IDs and plugin ID remain unchanged. Phase 2 uses different layer gain staging and a constant-power pan law, so old patches are not guaranteed to match Phase 1 loudness exactly. Sample paths are not embedded or relocated automatically. Wait for sample loading to finish before playback or offline export; a host load-completion barrier is not implemented.

A reference sampler remains in `src/audio/Engine.h` for Phase 1 regression checks; `src/audio/HybridEngine.h` is the live plugin engine. Listen to `factory-content/Phase2-Demo.wav` for a real offline four-layer render.

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

One background loader decodes samples and publishes a pending immutable buffer for each of the four independent layers. The callback swaps it at a block boundary and retires the previous buffer for worker deletion. No sample decoding, buffer destruction, or engine allocation occurs in rendering. UI notes use a fixed-capacity SPSC queue; overflow requests panic. MIDI supplied by the DAW is rendered at its sample offsets. GUI and host state use JUCE services; their realtime behavior still requires host validation.

Presets store all four layers, their matrix routes, and absolute user sample paths, not embedded audio. Keep samples in place when reopening projects. Missing or invalid samples retain the previous sound and show an error. Loading is limited to 30 seconds and stereo output to bound memory. Saving while a sample request is pending captures the last successfully decoded source for that layer. Sample changes stop current voices and can click; gain, tuning, cutoff, pan, shape and modulation transitions are smoothed. Generator/filter changes, enable switching and envelope-time edits are not fully click-free yet.

## Validation and scope

See [validation](docs/VALIDATION.md) for actual test results and remaining checks. Later phases (vocal workstation, full FX rack, sequencer, searchable libraries) are not implemented or displayed as working controls. MIDI 2.0, sustain pedal, looping, streaming, root detection, custom root selection, CLAP, AAX, GPU rendering, undo, MIDI learn, installers and release signing are not included. Pro Tools support needs AAX and its licensed SDK; VST3 does not provide native Pro Tools support.

Before distribution, review JUCE's AGPLv3/commercial licensing requirements. Repository source is MIT; generated factory content is CC0-1.0. No Nintendo assets, game recordings or human vocal recordings are included.
