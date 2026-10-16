# Phase 3 vocal workstation contract

Vocal edits use a worker-side phase vocoder with independent pitch and duration controls, cepstral spectral-envelope shifting/preservation, reverse and a small offline effect chain. Original decoded samples remain separate from rendered buffers, so edits are nondestructive and do not compound. DSP has no network dependency. Render controls debounce for 150ms; they are not sample-accurate realtime effects. The current sound remains playable until rendering completes.

A sixth generator, Vocal, adds MIDI slice playback. Off maps the whole phrase chromatically; grid-8, grid-16 and onset modes map slices from MIDI 60 upward at their original pitch, with per-layer tuning and pitch bend available. Slice bounds are embedded in each immutable rendered Sample. The audio thread only plays precomputed buffers. The UI draws slice markers and can export a standard MIDI file representing the slice sequence at 120 BPM.

Existing parameter IDs and generator values 0–4 are retained. New per-layer controls store pitch, duration, formant, formant preservation, reverse, slice mode and effect selection. State version 3 reconstructs rendered content from the original source and these settings; older states migrate with neutral vocal settings. A dedicated vocal editor page reuses the layer selection, file import and save/load workflow.

Factory vocal sources are original synthetic harmonic/formant textures. No recorded human performances, downloaded dialogue, voice models or artist imitations are supplied. Presets demonstrate pitched chops, reverse atmospheres, telephone/lo-fi/stutter processing and layered harmony.

## Practical limits

The worker uses a Hann-window radix-2 FFT phase vocoder and cepstral low-quefrency envelope estimation. Pitch changes combine compensated stretching with band-limited spectral output and final interpolation. The formant envelope is approximate; transient smearing, stereo image changes and extreme-shift artifacts need listening tests. The audio callback does not execute FFTs. Whole-phrase MIDI and layer tuning still resample buffers; offline formant preservation applies only to the dedicated pitch edit.

Grid/onset slicing is automatic, with no manual-boundary editor yet. Telephone, lo-fi and fixed-120-BPM stutter are rendered effects. Full time-synced FX routing, realtime vocal retuning and recorded vocal libraries are not part of this phase. External source files must remain available for recall. GUI and DAW playback require interactive validation beyond CI.

## Local checks

GCC builds pass with C++20 and warnings treated as errors. Tests cover FFT reconstruction, independent pitch/time at 44.1/48/96/192 kHz, spectral formant movement and preservation, reverse, slice detection/mapping, three effects, missing/short/silent data, cancellation and zero allocations in all six playback generators. Native JUCE integration, Windows/macOS builds, all five suites and factory generation passed in [run 38914202728](https://github.com/treezysosa/G-Boy/actions/runs/38914202728). Integration checks cover vocal-state reconstruction, original source labelling, duration, exported MIDI events, export without audio callbacks and audibility of all four presets. See VALIDATION.md for binaries and test scope.
