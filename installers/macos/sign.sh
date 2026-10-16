#!/bin/bash
set -euo pipefail
# Identities must already exist in the user's keychain. Never embeds credentials.
stage="$1"
application_identity="$2"
for bundle in "$stage/Standalone/G Boy.app" "$stage/VST3/G Boy.vst3" "$stage/AU/G Boy.component"; do
  codesign --force --options runtime --timestamp --sign "$application_identity" "$bundle"
  codesign --verify --strict --verbose=2 "$bundle"
done
# Build the PKG after signing payloads. Sign the resulting PKG using productsign,
# then submit with notarytool using an existing keychain profile; see docs/RELEASE.md.
