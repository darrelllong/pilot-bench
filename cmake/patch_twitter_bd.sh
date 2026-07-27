#!/usr/bin/env bash
# Apply the pilot standalone-library patch to the Twitter BreakoutDetection
# checkout (ExternalProject PATCH_COMMAND runs inside the cloned source dir).
#
# Idempotent: ExternalProject may re-run the patch step on reconfigure, so an
# already-applied patch is detected (it reverses cleanly) and skipped.
set -euo pipefail

patch_file="$1"

if git apply --reverse --check "$patch_file" >/dev/null 2>&1; then
    # Patch is already in the tree.
    exit 0
fi

git apply "$patch_file"
