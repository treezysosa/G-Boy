# Phase 5: library and browser

The BROWSER page loads actual samples, full patches, layer FX racks and pattern settings. Its factory collection has 29 functional entries: five procedural sample sources, ten hybrid patches, four synthetic-vocal patches, six rack settings and four patterns. Six new hybrid patches also appear in HYBRID. `gboy_generate_content` produces six additional original rendered samples; the complete installable factory-audio folder has twelve WAVs and a provenance manifest. Generated vowel textures are labelled synthetic, never human recordings.

## Workflow

- Search combines space-separated terms across names, categories, subcategories, descriptions, tags, authors, licenses, language, tradition and region. Filters select type/category/library/favorites. Empty stylistic categories have no fabricated content.
- IMPORT FILES accepts batches of WAV/AIFF/FLAC, `.gboy` and `.gbfx`; it copies files into the managed User Imports library. Drag multiple files onto BROWSER for the same operation. Other pages retain single-sample drag-and-drop.
- ADD FOLDER indexes originals recursively without moving them. It discovers samples and native patches/racks, up to 4096 assets per scan. Symlink files are skipped; files resolving outside the root are excluded. Re-add a folder to discover new files. CHECK FILES marks missing entries without deleting their metadata.
- LOAD TO LAYER loads samples and FX into the selected layer. Full patches replace the full instrument state. Pattern loading changes settings without starting playback. Root MIDI metadata is applied on sample load and saved with the DAW state.
- PREVIEW uses an independent sampler and background decoding/rendering. Samples and built-in hybrid/vocal patches can be previewed without replacing the patch. Built-in vocal previews audition the processed phrase; they are not a performance of every slice or harmony layer. Imported patch/rack previews require an adjacent `file.gboy.preview.wav`/`file.gbfx.preview.wav`, or a pack preview entry. Settings-only rack/pattern entries have no generated preview and the button is disabled. STOP PREVIEW and B / STOP end auditioning. Previews last at most three seconds, with fade-out and bounded normalization.
- Favorites and edited category/tags/author/license/provenance/root persist in the user index. Factory metadata is read-only. Acoustic similarity is available for analysed imported samples; it sorts by duration, RMS, zero-crossing brightness and estimated pitch. It is a deterministic distance, not an AI model or semantic search.
- SAVE PATCH saves the current `.gboy` state and adds a managed copy. Samples are not embedded. Allow pending sample edits to finish before saving.
- Select a linked/installed library in the library filter, then RELOCATE to its new root. Relative paths and asset IDs remain stable; favorites/tags survive. DAW/project state stores an asset ID plus a fallback path and root note. The sample worker resolves that ID after a move. Older states retain path-only recall. Rename individual files and they become missing; there is no fuzzy filename substitution.
- EXPORT LIBRARY creates a new folder pack from the selected user/installed library. Fill actual license/provenance first; the default unverified-import license cannot be exported as cleared content. Native patches may refer only to bundled samples or built-in sources. Export rewrites bundled references to relative paths; installation binds them to new library IDs. A patch with external unbundled dependencies is rejected. Exported packs are not a claim that user-entered license text has been legally verified.

## Analysis

Import and folder scans decode offline, on the library worker. Samples must be 8–192 kHz, at most 30 seconds, at most 128 MiB, and use a registered lossless format. The worker calculates waveform thumbnails, RMS/peak, relative silence bounds, onset candidates, normalized-autocorrelation root suggestions and onset-period tempo suggestions. Silence/uncertain sources have no invented pitch or tempo. Pitch has octave/complex-signal ambiguities; tempo has half/double-time ambiguities. Root is editable. Analysis does not destructively trim, slice or time-stretch the asset; those edits remain in VOCALS.

Automatically suggested tags describe acoustic properties only. Language, cultural tradition, region, human-performance status and production-style descriptions come from explicit metadata. These can be supplied through sidecars or pack entries. They are searched/displayed without inferring a uniform regional style.

## Content pack v1

INSTALL PACK selects a **folder**, not a ZIP archive. It must contain `gboy-library.json`:

```json
{
  "version": 1,
  "id": "unique-pack-identifier",
  "name": "Original recordings",
  "entries": [
    {
      "name": "Original phrase",
      "file": "samples/phrase.wav",
      "category": "VOCALS",
      "subcategory": "Spoken texture",
      "description": "Original commissioned performance",
      "tags": "spoken, dry",
      "root_midi": 60,
      "author": "Performer / producer credit",
      "license": "Your actual distribution license",
      "provenance": "Session date and source agreement reference",
      "human": "recorded with permission",
      "language": "Specific language supplied by performer",
      "tradition": "Specific tradition, if applicable",
      "region": "Specific context, if applicable",
      "preview": "previews/phrase.wav"
    }
  ]
}
```

Every entry needs a safe relative file path, license and provenance. Paths with traversal, absolute prefixes, colons/backslashes or external symlink targets are rejected. Preview paths obey the same containment rule. Duplicate relative paths and duplicate installed pack IDs are rejected. Files copy into a staging directory; the finished root is registered only after validation/copying succeeds. Failed installation removes staging and retains the existing index. No archive extraction, network request, DRM or third-party content downloader is involved.

An optional adjacent `sample.wav.gbmeta.json` or `patch.gboy.gbmeta.json` supplies the same entry metadata when adding a folder/importing. Imported preview sidecars use the conventional `.preview.wav` suffix. `.gboy` full presets and `.gbfx` racks are native formats; older JSON factory descriptors remain metadata, not preset files.

## Storage and threading

The profile lives under JUCE's user application-data directory in `G Boy/Library`. `index.json` is an atomically replaced version-1 JSON database. Managed files live under `Libraries/`. Plugin instances in one process share an index/worker for the same profile. Separate DAW/standalone processes are not coordinated: last writer wins, so simultaneous editing across processes is not supported.

Library scans, metadata persistence, install/export and analysis run on one library worker. Sample/vocal loading and preview rendering use the existing sample worker. The callback swaps immutable preview/sample buffers and retires old storage for deletion by the worker; preview rendering uses fixed 256-frame scratch arrays. The callback does not query the library manager. GUI search/snapshots run on the message thread; the browser checks a revision counter before copying an updated snapshot. The total index is capped at 10000 entries and each job queue at 64 operations. Heavy scans can delay other library tasks; streaming and cancelable progress UI are future work.

## Verification and limits

Local GCC tests and ASan/UBSan cover acoustic analysis at four rates, silence, tempo/onsets and all ten factory hybrid recipes. Native integration tests cover import, metadata/favorite persistence, search, missing files/relocation, install/export, duplicate/traversal rejection, sample root/asset recall, preview isolation and portable patch dependencies. Native results are recorded in [VALIDATION.md](VALIDATION.md) after CI.

No human recordings, regional-language performances, copyrighted game assets, semantic AI tagging, thousands of empty presets, long-sample streaming, automatic multisample/velocity zoning, installers, signatures or production DAW certification are included. Factory presets and generated audio form a small usable original collection. See the validation document for remaining release work.
