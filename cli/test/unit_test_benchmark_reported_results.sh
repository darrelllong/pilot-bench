#!/usr/bin/env bash
# Unit test for Pilot CLI tool: the variance and the CI of the optimal
# subsession are computed whenever a subsession size has been found, also
# when the required sample size is 0 or cannot be calculated, and are not
# those of an earlier analysis.
#
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
set -euo pipefail

RESULT_DIR=`mktemp -d`
trap 'rm -rf "$RESULT_DIR"' EXIT
TMPFILE=`mktemp`
export PILOT_MOCK_SEQUENCE_ROUND_FILE="${RESULT_DIR}/round"

# Readings that are all 1, of a binomial PI with no least sample size: the
# required sample size is 0, the session is satisfied, and the CI is 0.
export PILOT_MOCK_SEQUENCE="1 1 1 1 1 1 1 1 1 1"
rc=0
./bench run_program -o ${RESULT_DIR}/ones --min-sample-size 0 --ci-perc 0.1 --pi "success,,0,2,1" \
    -- ./mock_benchmark_sequence.sh >"$TMPFILE" 2>&1 || rc=$?
[ "$rc" -eq 0 ]
grep -q "\[PI 0\] Reading optimal subsession size: 1" "$TMPFILE"
grep -q "^0,[0-9]*,1,1,0,0,0,0," "${RESULT_DIR}/ones/pi_results.csv"

# A single reading: its mean is the reading, and the rest cannot be
# calculated, so it is NaN, not what the memory held.
rm -f "$PILOT_MOCK_SEQUENCE_ROUND_FILE"
export PILOT_MOCK_SEQUENCE="5"
rc=0
./bench run_program -o ${RESULT_DIR}/one --pi "value,,0,0,1" \
    -- ./mock_benchmark_sequence.sh >"$TMPFILE" 2>&1 || rc=$?
[ "$rc" -eq 12 ]
grep -q "^0,1,5,5,nan,nan,nan,nan," "${RESULT_DIR}/one/pi_results.csv"

# 40 integers whose mean is exactly 0: a subsession size of 1 is found, but
# no sample size can make the CI a percentage of 0 wide, so the session runs
# out of readings. The CI is that of all 40, t(0.975, 39) s / sqrt(40) x 2,
# with s^2 = SSQ / 39; it is not NaN, nor that of fewer readings.
rm -f "$PILOT_MOCK_SEQUENCE_ROUND_FILE"
export PILOT_MOCK_SEQUENCE="7 8 -9 2 -1 -7 -10 -9 -4 5 9 -9 6 7 9 2 9 -6 10 9 -8 -4 -9 10 4 10 -5 -7 -5 -9 3 -7 10 -10 1 -6 -1 7 -2 0"
rc=0
./bench run_program -o ${RESULT_DIR}/zero --pi "value,,0,0,1" \
    -- ./mock_benchmark_sequence.sh >"$TMPFILE" 2>&1 || rc=$?
[ "$rc" -eq 12 ]
grep -q "\[PI 0\] Reading optimal subsession size: 1" "$TMPFILE"
grep -q "^0,40,0,0,51.4359,51.4359,4.58736,4.58736," "${RESULT_DIR}/zero/pi_results.csv"
