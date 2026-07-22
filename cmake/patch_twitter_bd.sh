#!/usr/bin/env bash
# Apply a patch to the current source tree, idempotently.
#
# Used as an ExternalProject PATCH_COMMAND (see lib/CMakeLists.txt), which can
# re-run when the build tree is reconfigured; applying an already-applied patch
# must therefore be a no-op rather than an error.
#
# Usage: patch_twitter_bd.sh <patch-file>   (run from the source dir to patch)
set -euo pipefail

patch_file="$1"

# If the patch already applies cleanly in reverse, it has been applied — skip.
if patch -p1 -R -f --dry-run < "$patch_file" >/dev/null 2>&1; then
    echo "patch_twitter_bd: already applied, skipping"
    exit 0
fi

patch -p1 -f < "$patch_file"
