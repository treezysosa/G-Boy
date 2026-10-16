# G Boy — 0.6.0 Release Candidate 1

C++20/JUCE hybrid instrument, sampler and offline vocal workstation. Phase 6 adds release-candidate installers, universal macOS builds, state migration, callback monitoring, native reliability tests, external validation and source/license packaging. See [release engineering](docs/RELEASE.md) for installation, reproducible builds, signing commands and the remaining production gates. All eleven native suites pass on Windows and macOS; pluginval strictness 5, installed macOS AU validation and installer smoke checks passed. [Windows installer/portable package](https://github.com/treezysosa/G-Boy/actions/runs/38939414968/artifacts/12063271221) · [macOS universal PKG/portable package](https://github.com/treezysosa/G-Boy/actions/runs/38939414968/artifacts/12062369174). These are unsigned candidates; owner signing/notarization and the manual DAW matrix remain open.

Implemented: 64-voice stereo sampler, linear interpolation, MIDI note-on/off and channel-aware release, oldest-voice stealing, ADSR, smoothed master gain, soft output limiting, four original generated sounds, on-screen keyboard, hold-to-preview A button, B panic, sample drag/drop and asynchronous decoding, factory selector, save/load `.gboy` presets and host project state, resizable handheld-style GUI, waveform and output meter, modern/retro palettes. Parameters are exposed to DAW automation. Direct imports use MIDI 60 as root; browser loads apply editable analysed/declared root metadata. WAV/AIFF/FLAC support comes from JUCE's basic format readers.


## Phase 6 release candidate

Windows x64 uses VST3/standalone; macOS universal uses VST3/AU/standalone. The release pipeline generates installers, portable archives, checksums, benchmarks, GUI snapshots and corresponding source. It runs pluginval strictness 5 and macOS auval and exercises installer payloads. Fourteen real native factory patches with audio previews expand the installable factory pack to 26 assets. Artifacts remain unsigned; actual DAW certification is a separate, explicitly recorded gate.

## Phase 5 library and browser

**BROWSER** searches 29 real factory entries and imported content by name, category, type, library, tags, author, license and supplied cultural/language metadata. Favorites and user metadata persist. Batch import copies samples/presets/racks; Add Folder links existing content. Waveform thumbnails, root/tempo suggestions, silence bounds and onset analysis run in the background. Similarity sorts analysed samples by acoustic features; it is not an AI or semantic model.

Load samples/FX into the selected layer, or load complete patches. Preview uses a separate player, preserving the current patch. Factory expansion adds Quiet Storm Keys, Velvet Night Pad, Cassette Bass, Neon Arcade, Haunted Glass and Bedroom Haze. All are original synthesis; generated textures are not recorded humans.

Install folder packs with `gboy-library.json`; each asset requires license/provenance metadata and safe relative paths. Export user libraries after filling actual licensing metadata. Exported full patches require their samples to be bundled and use relative references. Relocate linked/installed libraries to recover moved files while retaining IDs, tags and favorites; project recall resolves sample IDs before falling back to saved paths. Rename recovery is explicit, not guessed.

[Phase 5 documentation](docs/PHASE5.md) covers controls, manifest/sidecar schema, storage, preview behavior, analysis limits and portability. `factory-content/gboy-library.json` installs the twelve generated factory WAVs after running the content generator. Factory JSON preset descriptors are metadata; functional patch entries use built-in recipes or native `.gboy` files.

## Phase 4 effects and sequencer

**FX:** edit the selected layer's four serial slots, or select **Master rack**. Choose any of 26 effect types, adjust wet/dry and the three labelled physical-unit controls, bypass a slot, and use UP/DN to change order. SAVE RACK / LOAD RACK writes `.gbfx` files; full `.gboy` presets include every rack and modulation route. Six rack settings include Init, Tape Dream, Crushed Cassette, Shimmer Cloud, Pump Delay and Factory Room.

Implemented effects: single-band parametric EQ, compressor, peak limiter, saturation, hard/fold distortion, bitcrusher, sample-rate reduction, tape/wow, vinyl hiss/crackles, chorus, flanger, four-stage phaser, tremolo, auto-pan, stereo delay, ping-pong delay, algorithmic reverb, sparse factory-IR convolution, shimmer reverb, filter sweep, comb resonator, ring modulation, transient shaper, mid/side width, beat-clock ducking and delay-head pitch/vibrato modulation. Controls smooth over approximately 10ms. Type/order changes reset slot state and are not fully click-free. Vinyl intentionally generates noise even with no notes. The limiter is not an oversampled true-peak limiter; nonlinear effects are not oversampled. Pitch modulation has creative wet-path delay rather than host-compensated retuning. Convolution uses a generated 64-tap sparse room FIR with adjustable spread/decay; imported long IRs and partitioned convolution are not implemented.

**Modulation:** HYBRID's matrix now has an **FX override** selector per row: use the existing voice destination, or route to layer FX mix/primary amount or master FX mix/primary amount. Existing destination IDs/ranges remain unchanged for older automation. FX modulation is shared across slots in a rack, evaluated per render chunk (up to 256 samples) and smoothed. Envelope uses the largest active layer voice; velocity/note/wheel use that layer's most recent note/channel. Master modulation sums the layer routes. Rack files store slot settings; matrix routing belongs to the full patch.

**SEQ:** enable the sequencer, then use **Host sync** with DAW play, or turn Host sync off and press **Run** for the internal clock. Standalone falls back to the internal clock if no host PPQ is available. Enabled sequencing replaces direct MIDI notes. Step mode uses the lowest held note as root, or Root MIDI when no notes are held. Arp modes require held/latched keys; Chord mode plays held keys (up to eight), or a root major triad. Incoming MIDI channels/velocities are retained. B / STOP disables sequencing and releases notes.

- Four editable banks, each with 64 stored steps; active lengths 16/32/64, with eighth/sixteenth/thirty-second-note divisions.
- 16-column piano-roll pages: click/drag to set/enable notes, right-click to disable, and edit the bottom velocity bars. The grid shows ±12 semitones; the selected-step pitch control supports ±24. Inactive steps beyond the current length are grey.
- Per-step velocity, gate, probability and 1–4 ratchets; swing delays odd steps while preserving each pair's duration.
- Up/down/up-down arps, 1–3 octaves, latch, chord mode, five scale choices and key locking (0=C, 1=C#, etc.).
- Optional chaining cycles A→B→C→D from the selected bank; the bank menu selects the starting/edit bank. Playhead status shows the sounding bank and step.
- Four original pattern presets and seeded randomization of the selected bank. Random probability repeats deterministically for the same seed and absolute beat position.
- EXPORT PATTERN MIDI writes 960-PPQN MIDI with tempo, swing, gates, ratchets, probability, scale and chaining, using the playback scheduler. Export starts at beat zero; arp export uses a root major triad instead of live held input. Disable internal sequencing when playing the exported MIDI back into G Boy. File export is implemented; drag-export and realtime MIDI output are not.

Host sync requires usable PPQ information. Transport stops/seeks release generated notes; effects can continue their tails. The plugin reports an estimated finite tail (capped at 600 seconds); continuous noise and externally held notes are not finite tails. No external sidechain bus, automatic CPU governor or performance guarantee at maximum FX/polyphony has been implemented. The rack and sequencer need interactive DAW and listening validation before release.

`factory-content/Phase4-Demo.wav` is a real sequenced two-layer render with ping-pong delay and reverb. The factory sparse-room response is included with original provenance metadata. See [Phase 4](docs/PHASE4.md) for the module contract and [validation](docs/VALIDATION.md) for results.

## Phase 3 vocal workstation

Select **VOCALS** to edit the selected layer. IMPORT accepts your original or licensed WAV/AIFF/FLAC recording. **SYNTHETIC VOWEL SOURCE** loads an original generated vowel phrase, explicitly labelled as synthetic. Choose the **Vocal** generator for slice keys, or use Sampler/Granular to play the processed buffer in their normal modes.

- **Pitch:** ±12 semitones, rendered independently of duration.
- **Duration ratio:** 0.5–2.0, rendered independently of pitch. This is a phase-vocoder algorithm, not a speed-only resampler.
- **Formant:** ±12 semitones of cepstral spectral-envelope movement. Preserve formants compensates for the *rendered pitch control*. It is approximate, with audible artifacts possible; it does not clone a voice.
- **Reverse:** reverses the rendered phrase. Changes always start from the cached original, never from a previously processed result.
- **Chops:** whole phrase, 8/16 equal slices, or energy-onset detection (up to 32). Slice markers and MIDI note numbers appear on the waveform; slices map from MIDI 60 upward and retain their base pitch. Layer tuning and bend can transpose them. Whole-phrase key tracking and layer tuning use conventional resampling, so live MIDI transposition still changes duration/formants; preservation applies to the offline pitch edit.
- **Effects:** clean, telephone band filtering/saturation, lo-fi sample hold/quantization, and stutter at a fixed 120 BPM. These are rendered effects, separate from the realtime FX rack.
- **EXPORT SLICE MIDI:** writes a standard `.mid` sequence at 120 BPM using detected/rendered slice timing. Import the file into a DAW MIDI track routed to G Boy. Realtime MIDI output and drag-export are not implemented.

Vocal controls are stored in plugin/preset state and exposed to the host. They debounce for 150ms and render on the worker, so they are **offline edits, not sample-accurate live automation**. Wait for RENDERING / LOADING to finish before playing or bouncing the new version. The previous buffer stays playable meanwhile. Publishing a render stops that layer's voices and can click. Imports are limited to 30 seconds at 8–192 kHz; stretching can double the buffer length. High-rate, four-layer patches can require hundreds of MB of memory; one worker bounds concurrent render work.

The vocal menu provides Pixel Chops, Reverse Aura, Radio Cuts and Vowel Harmony, using only synthetic vowel content. SAVE/LOAD recalls all layers and reconstructs processed buffers from their source files. Keep external recordings at their saved paths. Older presets migrate with neutral vocal settings. Slices are automatic; manual boundary editing, phase-locked stereo/transient preservation, tempo-synced chopping, real-time formant-preserving MIDI retuning, and human vocal libraries remain future work.

Listen to `factory-content/Phase3-Demo.wav` for a rendered example. See [Phase 3 implementation](docs/PHASE3.md) and [validation](docs/VALIDATION.md).

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

Presets store all four layers, their matrix routes, sample root notes, library asset IDs when available, and fallback absolute user sample paths, not embedded audio. Keep samples in place when reopening projects. Missing or invalid samples retain the previous sound and show an error. Loading is limited to 30 seconds and stereo output to bound memory. Saving while a sample request is pending captures the last successfully decoded source for that layer. Sample changes stop current voices and can click; gain, tuning, cutoff, pan, shape and modulation transitions are smoothed. Generator/filter changes, enable switching and envelope-time edits are not fully click-free yet.

## Validation and scope

See [validation](docs/VALIDATION.md) for actual test results and remaining checks. Phase 5 library/browser features are implemented; production validation and release packaging remain future work. MIDI 2.0, sustain pedal, looping, streaming, root detection, custom root selection, CLAP, AAX, GPU rendering, undo, MIDI learn, installers and release signing are not included. Pro Tools support needs AAX and its licensed SDK; VST3 does not provide native Pro Tools support.

Before distribution, review JUCE's AGPLv3/commercial licensing requirements. Repository source is MIT; generated factory content is CC0-1.0. No Nintendo assets, game recordings or human vocal recordings are included.
