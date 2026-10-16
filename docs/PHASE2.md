# Phase 2 implementation contract

`HybridEngine` owns four independent `LayerEngine` instances and a master gain. Each layer has 64 fixed voices, its own immutable sample pointer, ADSR, generator, filter, level, pan, tuning, LFO, macro, and four modulation routes. Render accepts arbitrary stereo buffer lengths and never allocates. The Phase 1 engine stays as a regression/reference renderer for the original demo.

Generator interface: one stereo frame per voice, plus reset on note-on. Modes are one-shot sampler, virtual analog, wavetable, two-operator FM, and four-overlap granular playback. Oscillators are procedural original code. Wavetables are built outside rendering. Filter modes are bypass, low-pass, band-pass, and high-pass, implemented with a topology-preserving state variable filter.

Modulation routes use fixed enums and bipolar amounts. Sources: off, LFO, amplitude envelope, velocity, mod wheel, note number, macro. Destinations: pitch, cutoff, gain, pan, grain position, oscillator shape/FM index. Routing is per voice. Modulation coefficients update every 16 samples; audio-rate envelopes and oscillator phase remain continuous. MIDI pitch bend is ±2 semitones per channel, mod wheel is per channel. Neither depends on internet access.

Processor integration: cache parameter atomic pointers at construction. Background loader owns decoding and retired sample deletion, using separate publication slots for each layer. Existing automation IDs `attack`, `decay`, `sustain`, `release`, `volume` are retained; the original ADSR IDs address layer 1 and volume remains master. Missing Phase 2 parameters in Phase 1 presets migrate to defaults, with layers 2–4 disabled.

UI integration: layer selector rebinds controls to that layer; engine, filter, enable, ADSR, tuning, level, pan, cutoff/resonance, shape, FM ratio, grain position/size, LFO and macro controls, and four source/destination/amount matrix rows. Sample import always targets the layer selected when the file chooser opened. No inactive future feature pages.

## Validation and remaining work

Core regression, five-mode/four-rate hybrid tests, all filter modes at cutoff/resonance extremes, missing/tiny samples, matrix routing into audio, buffer partition invariance, and zero-render-allocation tests have passed locally with GCC 14.2.0. An initial AddressSanitizer/UBSan hybrid run passed with leak checking disabled (the container is traced). Final CI checks include JUCE processor state migration and sample-offset MIDI. Windows/macOS Phase 2 build results will be recorded after the workflow completes.

Not included: user wavetable files, tempo-synced LFO, arbitrary drag/drop routing, unison, glide, looping/multisample zones, sample streaming, oversampled FM, granular transport/time stretching, or a full release-grade CPU/DAW validation suite. Generator/enable/sample switching can still click. Wavetable mip changes need listening tests under extreme pitch automation. GPU rendering is not enabled.
