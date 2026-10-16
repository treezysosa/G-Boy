# Validation

## Baseline

Phase 1 built successfully on Windows and macOS via [GitHub run 38900088017](https://github.com/treezysosa/G-Boy/actions/runs/38900088017). VST3 and standalone targets were compiled on both systems, and AU on macOS; the core test suite passed. No interactive DAW validation was performed.

## Phase 2 local verification

GCC 14.2.0 in the supplied Linux workspace. CMake and JUCE remain unavailable locally, so native plugin builds use GitHub Actions.

- Original sampler regression tests pass across four sample rates (44100, 48000, 96000, 192000) and four block sizes (1, 32, 256, 1024).
- Hybrid tests pass: all five generators at all four rates, note/release behavior, 64 voices per layer, four-layer mixing, enable and pan isolation, missing/tiny samples and filter stability at extreme cutoff/resonance settings.
- Response tests verify 440 Hz oscillator tuning, a one-octave modulation shift, and low/band/high-pass frequency attenuation.
- Every implemented modulation source changes rendered audio. Matrix destination summing and variable-buffer render equivalence are checked.
- A separate allocation-counting test reports zero heap allocations in hybrid rendering, voice allocation/stealing, filters, grains and modulation. This covers the engine, not every DAW/JUCE host path.
- Hybrid AddressSanitizer/UndefinedBehaviorSanitizer tests pass with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer is unavailable under the container's tracing; leak detection is unverified.
- Core, hybrid, allocation and content-generator targets compile with `-Wall -Wextra -Werror -Wpedantic`.
- Original content and Phase 1/Phase 2 demos are rendered to stereo PCM WAV files by `gboy_generate_content`.

## Phase 2 CI verification

The workflow compiles Windows VST3/standalone and macOS VST3/AU/standalone, runs core/hybrid/allocation tests, and runs a JUCE integration test for per-layer and matrix state recall, Phase 1 migration, sample-offset MIDI, pitch bend/CC1, missing files, real WAV loading and factory patch audibility. [GitHub run 38903075715](https://github.com/treezysosa/G-Boy/actions/runs/38903075715) successfully compiled all targets on Windows and macOS. All **4/4 tests passed on each platform**: `audio_core`, `hybrid_core`, `realtime_allocations`, and `plugin_processor`. Factory-content generation and artifact upload succeeded on both.

Built source commit: `2ce46e5e7dd63079f224c6d0162fb7ec0c881ada`.

- [Windows VST3 + standalone](https://github.com/treezysosa/G-Boy/actions/runs/38903075715/artifacts/12046589328)
- [macOS VST3 + AU + standalone](https://github.com/treezysosa/G-Boy/actions/runs/38903075715/artifacts/12046479521)

These are development artifacts, not signed installers. Interactive GUI, DAW compatibility, actual audio devices and Intel/universal macOS deployment remain unverified. Parameter recall was tested using tight floating-point tolerances, since normalized range conversion can introduce rounding. The engine itself is allocation-tested; the full host callback has not been instrumented.

## Phase 3 local verification

- The new `vocal_core` suite passes with GCC C++20 and warnings treated as errors.
- FFT roundtrip and pitch/time independence are tested at 44.1, 48, 96 and 192 kHz. Pitch octaves change frequency without changing length; 1.6x duration retains the source tone frequency.
- Spectral-envelope peak measurements verify formant movement and preservation compensation, in addition to waveform-difference checks.
- Reverse, telephone/lo-fi/stutter effects, grid/onset slicing, distinct slice keys/ends, silence, very short signals and cancellation pass.
- Existing sampler and hybrid regressions pass. The allocation suite passes across all six playback generators, including Vocal.
- Vocal AddressSanitizer/UBSan tests pass with leak checking disabled for the traced workspace.
- Factory generation produces a 3.2-second original synthetic vowel phrase and a 4-second processed Phase 3 demo. Both are stereo 44.1 kHz PCM WAV files with provenance metadata.
- The native integration suite now tests rendered vocal duration, state reconstruction, source labelling, slice MIDI parsing, stopped-audio export and vocal preset audibility. [GitHub run 38914202728](https://github.com/treezysosa/G-Boy/actions/runs/38914202728) successfully built the Windows VST3/standalone and macOS VST3/AU/standalone targets. All **5/5 test suites passed on each platform**: `audio_core`, `hybrid_core`, `realtime_allocations`, `vocal_core`, and `plugin_processor`. Factory generation and artifact upload also passed.

Built source commit: `08bf8d7042c8afe526184c1ad96c796dcfe24dc7`.

- [Windows Phase 3 build](https://github.com/treezysosa/G-Boy/actions/runs/38914202728/artifacts/12052222034)
- [macOS Phase 3 build](https://github.com/treezysosa/G-Boy/actions/runs/38914202728/artifacts/12052575532)

Phase 3 is a development build. Pitch/time/formant edits are worker-rendered, not sample-accurate live processors. Whole-phrase MIDI and layer tuning resample audio. Native GUI interaction, DAWs, external audio devices, human-recording quality, Intel/universal macOS deployment and signing remain unverified. Spectral-envelope tests establish implemented behavior, not commercial-quality vocal processing.

## Phase 4 verification

- Local GCC C++20 builds pass the effects and sequencer suites with warnings treated as errors. ASan/UBSan checks pass for both cores with leak checking disabled in the traced workspace.
- All 27 effect choices (Off plus 26 effects) produce finite bounded output at 44.1, 48, 96 and 192 kHz. Dedicated tests cover exact Off/bypass, delay impulse timing, limiter ceiling, zero-width mono and sparse convolution tails.
- Scheduler checks cover timing, gate releases, swing, probability, ratchets, scales, arps/octaves/chords, latch, chaining, stop and seek.
- Allocation instrumentation covers all 20 active effect slots across every type and fixed-capacity chord scheduling; zero heap allocations were observed in these engine paths. This does not certify the full JUCE/DAW callback.
- JUCE integration covers rack save/load/order, sequenced audio without MIDI, MIDI-file note/release pairing, full FX/pattern state recall, legacy neutral defaults and simulated host PPQ/stop. The clock uses the rate passed to preparation; direct processor preparation does not depend on the wrapper populating JUCE's base sample-rate field.
- [GitHub run 38921725091](https://github.com/treezysosa/G-Boy/actions/runs/38921725091) compiled Windows VST3 and standalone successfully. All **7/7 test suites passed** on Windows: audio, hybrid, realtime allocations, vocal, effects, sequencer and processor integration. Factory audio generation and artifact upload passed. macOS is queued as of this documentation update.

Built source commit: `59cee1e5f1e64603980cbeb0e523b294d5a6d5b2`.

[Download Windows Phase 4 VST3 + standalone](https://github.com/treezysosa/G-Boy/actions/runs/38921725091/artifacts/12055376800). The package includes generated factory audio. macOS results and any eventual artifact appear on the linked workflow run.

Phase 4 remains a development build. Interactive GUI/DAW behavior, listening quality, CPU budgets, whole-callback realtime properties, signing and Intel/universal macOS deployment remain unverified. Convolution uses a generated sparse 64-tap response; imported long IRs, external audio sidechain, MIDI drag-export and realtime MIDI output are not implemented. Nonlinear processing is not oversampled. See [Phase 4 contract](PHASE4.md) for the precise feature scope.

## Phase 5 verification

- GCC 14.2 C++20 analysis tests pass with `-Wall -Wextra -Werror -Wpedantic` at 44.1, 48, 96 and 192 kHz. Tests verify A3 pitch/root estimates, trim bounds, RMS/peak, silence/empty input, onset/120-BPM suggestions, similarity identity and audible finite output from all ten hybrid recipes.
- Analysis ASan/UBSan checks pass with `ASAN_OPTIONS=detect_leaks=0`; leak detection remains unavailable under tracing. Existing hybrid and allocation suites also pass locally after integration.
- The content generator compiles with warnings as errors and generates all twelve factory samples plus prior demos. The folder-pack manifest names actual generated files with CC0 provenance; six new sounds are original two-layer hybrid renders.
- New native `library_manager` tests cover Unicode WAV copying, pitch analysis, all-term search, favorites, metadata persistence, missing/relocated folders, pack export/install, duplicate/traversal rejection, preview isolation, sample asset/root project recall, portable sample+patch dependencies, repeated patch-save overwrite and offline-preview cancellation. [GitHub run 38926652429](https://github.com/treezysosa/G-Boy/actions/runs/38926652429) built Windows VST3/standalone and macOS VST3/AU/standalone. All **9/9 suites passed on each platform**: `audio_core`, `hybrid_core`, `realtime_allocations`, `vocal_core`, `effects_core`, `library_analysis`, `sequencer_core`, `plugin_processor`, and `library_manager`. Factory generation and artifact upload passed on both.

Built source commit: `48c9a6c7cfa27c0d784a821a4864eae4aad4861d`.

- [Windows Phase 5 VST3 + standalone](https://github.com/treezysosa/G-Boy/actions/runs/38926652429/artifacts/12057232159)
- [macOS Phase 5 VST3 + AU + standalone](https://github.com/treezysosa/G-Boy/actions/runs/38926652429/artifacts/12057471626)

The browser/index workflow is separate from the audio callback. Native GUI interaction, DAW compatibility, filesystem races under rapid edits, cross-process index coordination, real-world analysis quality, long-library CPU budgets, whole-callback realtime guarantees, signing and installers remain unverified. See [Phase 5](PHASE5.md) for precise storage, manifest, workflow and feature limits.

## Remaining checks and development

1. Run pluginval and interactive DAW tests: notes on multiple channels, import and automation during playback, rapid sample changes, project recall and offline export.
2. Measure full callback CPU, allocations and locks under hosts; run ThreadSanitizer on publication/loader teardown and keyboard events. Core allocation tests do not establish whole-plugin realtime guarantees.
3. Add sample-change crossfades, smooth generator/filter/enable changes and envelope-time edits, and listen under extreme automation. FM is not oversampled and can alias at high notes/indices.
4. Add root-note controls, sustain pedal, looping and key zones; validate sample relocation and missing-file recovery workflows.
5. Review the GUI at multiple DPI settings and window sizes, test devices and accessibility, and prepare signed installers after validation.
6. Phase 6 focuses on plugin validation, performance/DAW tests, installers, signing and release packaging. Phase 3 vocal processing remains offline and needs listening/DAW tests on actual licensed recordings.
