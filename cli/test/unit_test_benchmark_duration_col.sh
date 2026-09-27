#!/usr/bin/env bash
# Test that Pilot CLI uses the round duration from the client program
# (--duration-col), not the duration it measures.
#
# Copyright (c) 2017-2019 Yan Li <yanli@tuneup.ai>. All rights reserved.
# The Pilot tool and library is free software; you can redistribute it
# and/or modify it under the terms of the GNU Lesser General Public
# License version 2.1 (not any other version) as published by the Free
# Software Foundation.
#
# The Pilot tool and library is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program in file lgpl-2.1.txt; if not, see
# https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
#
# Commit 033228934e11b3f86fb0a4932b54b2aeea5c803c and before were
# released with the following license:
# Copyright (c) 2015, 2016, University of California, Santa Cruz, CA, USA.
# Created by Yan Li <yanli@tuneup.ai>,
# Department of Computer Science, Baskin School of Engineering.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Storage Systems Research Center, the
#       University of California, nor the names of its contributors
#       may be used to endorse or promote products derived from this
#       software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# REGENTS OF THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
set -euo pipefail

TMPFILE=`mktemp`
OUTPUT_DIR=`mktemp -d -u`
OUTPUT_DIR2=`mktemp -d -u`
trap 'rm -rf "$TMPFILE" "$OUTPUT_DIR" "$OUTPUT_DIR2"' EXIT
./bench run_program --preset quick -d 1 --wps -w 100,2000 --session-limit 120 \
    -o ${OUTPUT_DIR} -- ./mock_benchmark_with_duration.sh %WORK_AMOUNT% >"$TMPFILE" 2>&1

# Every round duration has to be what the client program reported. The
# client program returns at once, so a round that Pilot measured would be far
# shorter than 1 s.
ROUNDS=`awk -F, 'NR > 1 { n++ } END { print n+0 }' ${OUTPUT_DIR}/rounds.csv`
[ "$ROUNDS" -gt 20 ]
awk -F, 'NR > 1 && $3 != sprintf("%.0f", (1 + $2 * 0.05) * 1000000000) { bad++ } END { exit bad+0 }' ${OUTPUT_DIR}/rounds.csv

# duration = 1 + work_amount / 20
grep -q "^WPS alpha: 1$" "$TMPFILE"
grep -q "^WPS v: 20$" "$TMPFILE"

# a column that the client program doesn't print
if ./bench run_program -d 2 --wps -w 100,2000 --session-limit 120 \
    -o ${OUTPUT_DIR2} -- ./mock_benchmark_with_duration.sh %WORK_AMOUNT% >"$TMPFILE" 2>&1; then
    exit 1
fi
grep -q "Cannot find the round duration (column 2)" "$TMPFILE"
rm -rf "${OUTPUT_DIR2}"
