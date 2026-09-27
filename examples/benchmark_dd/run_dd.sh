#!/usr/bin/env bash
# Run dd and extract duration from dd's output. Works with the dd of Linux
# (GNU) and the dd of macOS and the BSDs.
# Author: Yan Li <yanli@tuneup.ai>
# This file is in public domain.
set -euo pipefail

OUTPUT_FILE=$1
IO_COUNT=$2

# GNU: "104857600 bytes (105 MB, 100 MiB) copied, 0.0452 s, 2.3 GB/s"
# BSD: "104857600 bytes transferred in 0.015194 secs (690127811 bytes/sec)"
LC_ALL=C dd if=/dev/zero of="$OUTPUT_FILE" bs=1048576 count="$IO_COUNT" 2>&1 | \
    awk '/ copied, /              { print $(NF-3); found = 1 }
         / bytes transferred in / { print $5;      found = 1 }
         END                      { exit !found }'
