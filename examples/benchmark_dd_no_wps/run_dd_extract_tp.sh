#!/usr/bin/env bash
# Run dd and extract the throughput in MB/s from dd's output. Works with the
# dd of Linux (GNU) and the dd of macOS and the BSDs.
# Author: Yan Li <yanli@tuneup.ai>
# This file is in public domain.
set -euo pipefail

if [ $# -lt 2 ]; then
    cat<<EOF
Usage: $0 output_file io_count
output_file:   the file to write to
io_count:      how many MBs to write
EOF
    exit 2
fi

OUTPUT_FILE=$1
IO_COUNT=$2

# GNU: "104857600 bytes (105 MB, 100 MiB) copied, 0.0452 s, 2.3 GB/s"
# BSD: "10485760 bytes transferred in 0.015194 secs (690127811 bytes/sec)"
# We calculate the throughput from the bytes and the seconds. The throughput
# that GNU dd prints has few digits and its unit changes.
LC_ALL=C dd if=/dev/zero of="$OUTPUT_FILE" bs=1048576 count="$IO_COUNT" 2>&1 | \
    awk '/ copied, /              { bytes = $1; secs = $(NF-3); found = 1 }
         / bytes transferred in / { bytes = $1; secs = $5;      found = 1 }
         END                      { if (!found || secs <= 0) exit 1
                                    printf "%.6f\n", bytes / secs / 1048576 }'
