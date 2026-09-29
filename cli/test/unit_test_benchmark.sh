#!/usr/bin/env bash
# Unit test for Pilot CLI tool run benchmark
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

# bench saves the results in the current directory if it is not given one
RESULT_DIR=`mktemp -d`
trap 'rm -rf "$RESULT_DIR"' EXIT

TMPFILE=`mktemp`
# Under the default preset, quick, the autocorrelation limit is 0.8. The
# first 10 readings have a lag-1 autocorrelation of 0.25, so the subsession
# size is 1, and they need 6 readings for a CI of 30% of the mean, fewer
# than the least sample size, so the session ends at 10: mean 1.779,
# variance 0.0750989, CI 2 t(0.975, 9) s / sqrt(10) = 0.392075.
check() {
    grep -q "response time: R m1.779 c0.3921 v0.0751" "$TMPFILE"
    grep -q "\[PI 0\] Reading mean: 1.779 ms" "$TMPFILE"
    grep -q "\[PI 0\] Reading CI: 0.3921 ms" "$TMPFILE"
    grep -q "\[PI 0\] Reading variance: 0.0751 ms" "$TMPFILE"
    grep -q "\[PI 0\] Reading optimal subsession size: 1" "$TMPFILE"

    # test quiet mode
    rm -f /tmp/pilot_mock_benchmark_round.txt
    OUTPUT_DIR=`mktemp -d -u`
    ./bench run_program --ci-perc 0.3 --min-sample-size 10 --pi "response time,ms,0,0,1:delay time,ms,1,0" \
        --quiet -o ${OUTPUT_DIR} \
        -- ./mock_benchmark.sh >"$TMPFILE" 2>&1
    # we don't directly compare the output with an expected file, because the
    # session_duration on the first line will always be different
    grep -q "0,1.779,0.392075,0.0750989,0,1.779,0.392075,0.0750989," "$TMPFILE"
    grep -q "1,2.779,0.392075,0.0750989,0,2.779,0.392075,0.0750989"  "$TMPFILE"
    # even we are running in quiet mode log should still contain <debug> information
    grep -q '<debug>' "${OUTPUT_DIR}/session_log.txt"
    # check correctness of saved results
    grep -q "0,10,1.779,1.779,0.0750989,0.0750989,0.392075,0.392075,0" "${OUTPUT_DIR}/pi_results.csv"
    grep -q "1,10,2.779,2.779,0.0750989,0.0750989,0.392075,0.392075,0" "${OUTPUT_DIR}/pi_results.csv"

    # TODO: add more checks here
}

rm -f /tmp/pilot_mock_benchmark_round.txt
./bench run_program -o ${RESULT_DIR}/r --ci-perc 0.3 --min-sample-size 10 --pi "response time,ms,0,0,1" \
    -- ./mock_benchmark.sh >"$TMPFILE" 2>&1
check
# Test valid-rc option
rm "$TMPFILE"
rm -f /tmp/pilot_mock_benchmark_round.txt
./bench run_program -o ${RESULT_DIR}/r --ci-perc 0.3 --min-sample-size 10 --pi "response time,ms,0,0,1" --valid-rc 0 --valid-rc 1\
    -- ./mock_benchmark.sh -r >"$TMPFILE" 2>&1
check

# --ac sets the autocorrelation limit, here 0.1. Subsession sizes 1 to 3 of
# the first 44 readings have autocorrelations of 0.61, 0.56 and 0.47, and 4
# has 0.082, so the subsession size is 4 and the session needs 44 readings:
# mean 1.72477, subsession variance 0.0446593, CI 0.283944.
rm -f /tmp/pilot_mock_benchmark_round.txt
./bench run_program -o ${RESULT_DIR}/ac --ci-perc 0.3 --min-sample-size 10 --ac 0.1 --pi "response time,ms,0,0,1" \
    -- ./mock_benchmark.sh >"$TMPFILE" 2>&1
grep -q "Rounds: 44" "$TMPFILE"
grep -q "\[PI 0\] Reading optimal subsession size: 4" "$TMPFILE"
grep -q "0,44,1.72477,1.72477,0.0446593,0.0446593,0.283944,0.283944,0" "${RESULT_DIR}/ac/pi_results.csv"

# A limit or a confidence level that is not a number is rejected
rc=0
./bench run_program --ac nan --pi "response time,ms,0,0,1" -- ./mock_benchmark.sh >"$TMPFILE" 2>&1 || rc=$?
[ "$rc" -eq 2 ]
rc=0
./bench run_program --confidence-level nan --pi "response time,ms,0,0,1" -- ./mock_benchmark.sh >"$TMPFILE" 2>&1 || rc=$?
[ "$rc" -eq 2 ]
