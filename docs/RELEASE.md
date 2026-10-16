# G Boy 0.6.0 Release Candidate 1

Phase 6 provides release engineering and automated verification. This candidate is
not labelled a signed, DAW-certified production release. `RELEASE-REPORT.json`
records automated gates and leaves the signing/manual-DAW gates open.

[Windows installer + portable](https://github.com/treezysosa/G-Boy/actions/runs/38939414968/artifacts/12063271221) · [macOS universal PKG + portable](https://github.com/treezysosa/G-Boy/actions/runs/38939414968/artifacts/12062369174). All eleven native suites, both pluginval strictness-5 runs, installed macOS auval and installer smoke checks passed in the [validated workflow](https://github.com/treezysosa/G-Boy/actions/runs/38939414968).

## Installing and removing

Windows x64: run `G-Boy-0.6.0-rc.1-Windows-Setup.exe`. It installs the standalone in
Program Files, VST3 in Common Files/VST3, and the original factory pack under
ProgramData/G Boy/Factory Content. Add that factory folder using BROWSER > INSTALL
PACK to copy/index the twelve WAVs and fourteen native presets with rendered
previews. The instrument's built-in collection already works without that pack.
Uninstall using Windows Apps or the supplied uninstaller. User libraries under
AppData/G Boy/Library are retained; the installer never deletes that directory.
The MSVC runtime is statically linked; no redistributable download is needed.

macOS 11 or newer: run `G-Boy-0.6.0-rc.1-macOS.pkg`. Universal arm64/x86_64 VST3/AU
bundles go under /Library/Audio/Plug-Ins, the standalone under /Applications, and
factory content/documentation/source under /Library/Application Support/G Boy.
Only release-owned paths are installed. A PKG has no automatic uninstaller;
remove the G Boy app/bundles and support folder explicitly if desired, preserving
the per-user G Boy/Library data. The current candidate has no Developer ID
signature/notarization. Gatekeeper may require explicit local approval. Do not
mistake an ad-hoc build signature for a trusted Developer ID signature.

Portable archives contain the same bundles/content, installer, documentation,
corresponding source, validation evidence and SHA256SUMS.txt. Download checksums
are in DOWNLOAD-SHA256SUMS.txt. Installer payloads are fixed before validation;
validation reports/checksums are attached outside the installer afterward.

## Automated gates

The GitHub workflow builds Windows x64 and macOS universal binaries. It runs:

1. All audio, hybrid, vocal, FX, sequencer, analysis, library and processor tests.
2. State v1–v5 fixture migration to v6, future-version rejection, malformed/random
   state handling, parameter bounds and finite-value validation.
3. Full callback C++ new/delete instrumentation at four sample rates and four
   buffer sizes, plus concurrent sample publication/parameter editing. This does
   not instrument C malloc/free, operating-system locks or all DAW host paths.
4. Ten native GUI snapshots: five pages at minimum/maximum window sizes. This
   renders real components; it does not replace usability, DPI or accessibility
   testing on physical screens/DAWs.
5. A fixed 48-voice/three-layer/two-master-FX benchmark with median/p95/max timing.
   Reports are measurements of that runner, not hard realtime guarantees.
6. Tracktion pluginval 1.0.4, strictness 5, against the built VST3 on both systems.
   The official version is pinned; the downloaded tool's SHA-256 and command are
   recorded. macOS AU also runs Apple's auval after real PKG installation.
7. Windows silent install/uninstall with a user-profile preservation marker, and
   macOS PKG expansion/installation plus lipo verification of both architectures.
8. ASan/UBSan core tests on Linux, including leak checking on the CI runner.
9. Packaging tests that reject missing/unsafe assets and prevent finalization if
   validation gates fail. Each distributed factory entry carries provenance.

Mac universal slices are verified in each shipped bundle. Native arm64 and the
architecture used by the downloaded validator are logged; this is not a test on
physical Intel hardware. No claim of compatibility with every listed DAW is made.

## State and reliability

State version 6 accepts versions 1–5 while retaining stable parameter IDs, ranges
and plugin identifiers. Missing parameters use defaults. Unknown IDs are ignored;
non-finite/malformed values are rejected and finite out-of-range values clamped.
Future versions and invalid roots are rejected before applying state. Sample root
notes and library asset IDs/fallback paths retain Phase 5 recall semantics.
Imported library numeric metadata is bounded before conversion/display. Library
work checks for shutdown between files/copy operations; the sample worker checks
shutdown while awaiting library readiness.

The callback meter reports smoothed callback time divided by the buffer duration
and current active voices. Benchmark and processor reports include peak values.
Timing instrumentation and coefficient caching allocate no C++ heap storage in
rendering. Stereo filters share computed coefficients while keeping independent
channel state; bypassed voice filters skip coefficient computation.

## Rebuilding the accompanying source

Extract Source.tar.gz. It contains G-Boy and the complete JUCE 8.0.6 source used by
the build, without Git history or generated audio/binaries. Packaging utilities require Python 3.9 or newer. Build with CMake 3.22+
and a C++20 compiler:

```sh
cmake -S G-Boy -B build -DJUCE_SOURCE_DIR=/absolute/path/to/JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

For universal macOS add `'-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'` and
`-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0`. Generate WAVs with gboy_generate_content;
run gboy_plugin_tests `--export-presets /absolute/path/to/G-Boy/factory-content`
to write fourteen playable native patches and their real three-second previews.
Read LICENSE-BINARY.txt and THIRD-PARTY.md for the binary/source license split.

## Signing with the owner's certificates

No certificates, passwords or developer identities are embedded or generated.
Sign Windows payload EXE/VST3 files **before** compiling the installer, then sign
and verify the installer using a certificate already in the Windows certificate
store:

```powershell
installers/windows/sign.ps1 -CertificateThumbprint YOUR_THUMBPRINT -Files @('path/to/G Boy.exe','path/to/G Boy.vst3')
# Build installer, then sign it with the same script.
```

For macOS, sign the staged bundles with an existing Developer ID Application
identity, rebuild the PKG, sign using a Developer ID Installer identity, submit
with an existing notarytool keychain profile, and staple/verify the result:

```sh
bash installers/macos/sign.sh /absolute/stage 'Developer ID Application: YOUR ID'
# Rebuild PKG from the signed stage.
productsign --sign 'Developer ID Installer: YOUR ID' unsigned.pkg signed.pkg
xcrun notarytool submit signed.pkg --keychain-profile YOUR_PROFILE --wait
xcrun stapler staple signed.pkg
pkgutil --check-signature signed.pkg
spctl --assess --type install --verbose=2 signed.pkg
```

Re-run plugin validation and installer smoke tests after signing. Rebuild the
archive inventory/checksums for the signed files and record actual evidence;
the unsigned CI report must not be relabelled as signed.

## Production gate still requiring local evidence

Complete [DAW-MATRIX.csv](DAW-MATRIX.csv) on actual licensed hosts/devices. Check
notes/sustain/transport, dense automation, changing files during playback,
multiple instances, save/reopen, offline/real-time export equality, long sessions,
audio-device changes, UI scaling and installer upgrades. Unsupported sustain,
looping/key zones, streaming, MIDI learn/undo and format omissions remain listed
in README; Phase 6 does not silently add those systems.

Known quality limits: type/filter/sample switches can click, nonlinear/FM paths
are not oversampled, vocal processing is offline/heuristic, convolution uses a
short sparse factory IR, and cross-process library editing is not coordinated.
Address these as required by the intended release quality bar before declaring
production readiness. Pro Tools requires separately licensed AAX work; no AAX
or CLAP compatibility is claimed by these artifacts.
