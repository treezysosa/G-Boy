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

The workflow compiles Windows VST3/standalone and macOS VST3/AU/standalone, runs core/hybrid/allocation tests, and runs a JUCE integration test for per-layer and matrix state recall, Phase 1 migration, sample-offset MIDI, pitch bend/CC1, missing files, real WAV loading and factory patch audibility. Final workflow results will be recorded after CI completes.

## Remaining checks and development

1. Run pluginval and interactive DAW tests: notes on multiple channels, import and automation during playback, rapid sample changes, project recall and offline export.
2. Measure full callback CPU, allocations and locks under hosts; run ThreadSanitizer on publication/loader teardown and keyboard events. Core allocation tests do not establish whole-plugin realtime guarantees.
3. Add sample-change crossfades, smooth generator/filter/enable changes and envelope-time edits, and listen under extreme automation. FM is not oversampled and can alias at high notes/indices.
4. Add root-note controls, sustain pedal, looping and key zones; validate sample relocation and missing-file recovery workflows.
5. Review the GUI at multiple DPI settings and window sizes, test devices and accessibility, and prepare signed installers after validation.
6. Phase 3 adds vocal chopping, dedicated pitch/time/formant processing and slice-to-MIDI. Granular synthesis alone is not a vocal workstation.
