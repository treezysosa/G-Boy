# Validation — supplied Linux workspace

- GCC 14.2.0 available. CMake, JUCE and native Windows/macOS SDKs unavailable.
- Dependency downloads did not complete. No JUCE compilation, standalone GUI launch, plugin scan, DAW recall, or operating-system compatibility claim has been verified.
- Core compiles using C++20 with `-Wall -Wextra -Werror -Wpedantic`.
- Core tests pass across 4 sample rates (44100, 48000, 96000, 192000) and 4 block sizes (1, 32, 256, 1024): silence, audible MIDI response, finite bounded output, release to silence, voice-overload exercise and panic.
- Four factory sounds checked for nonzero energy and expected lengths.
- AddressSanitizer and UndefinedBehaviorSanitizer pass with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer cannot run in this traced environment; leak detection is unverified.
- Content generator compiles with warnings treated as errors. Python wave reader confirms four stereo 44.1 kHz / 132300-frame factory WAVs and a stereo 48 kHz / 144000-frame demo.
- GitHub Actions workflow is supplied but has not been run.

## Next development steps

1. Run Windows and macOS builds; fix any JUCE integration errors before calling the plugin build verified.
2. Run pluginval and DAW smoke tests: notes with block offsets, multiple channels, import during playback, sample changes, bad/missing files, presets and project recall. Measure callback allocations, locks and CPU usage under load.
3. Verify keyboard queue, sample publication and loader teardown under ThreadSanitizer and sustained load. Validate state calls under each host's threading behavior.
4. Add sample-change crossfades and envelope parameter smoothing, root-note controls and sustain/pitch bend; test behavior with offline rendering and extreme automation.
5. Capture and review GUI at multiple DPI settings, audio-device behavior and accessibility. Build signed Windows installer and signed/notarized macOS packages after validation.
6. Begin Phase 2 with four-layer interfaces, filters and modulation only after the Phase 1 host checks pass.
