#!/usr/bin/env bash
# Unit test for Pilot CLI tool: a PI of type 1 (a ratio) is averaged by its
# harmonic mean, and a PI of type 0 by its arithmetic mean. The readings are
# 2 4 1 4 2 1 4 1 2 2 4 1 1 4 2 4 1 2 1 4: their harmonic mean is
# 20 / 11.75 = 1.70213 and their arithmetic mean is 2.35. Twenty readings
# are not enough for the session to converge: it ends when the mock
# benchmark has no more, and bench returns 12, which is expected here.
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

rm -f /tmp/pilot_mock_benchmark_harmonic_mean_round.txt
rc=0
./bench run_program -o ${RESULT_DIR}/h --ci-perc 0.9 --min-sample-size 10 --pi "rate,,0,1,1" \
    -- ./mock_benchmark_harmonic_mean.sh >"$TMPFILE" 2>&1 || rc=$?
[ "$rc" -eq 12 ]
grep -q "^0,20,1.70213," "${RESULT_DIR}/h/pi_results.csv"

rm -f /tmp/pilot_mock_benchmark_harmonic_mean_round.txt
rc=0
./bench run_program -o ${RESULT_DIR}/a --ci-perc 0.9 --min-sample-size 10 --pi "rate,,0,0,1" \
    -- ./mock_benchmark_harmonic_mean.sh >"$TMPFILE" 2>&1 || rc=$?
[ "$rc" -eq 12 ]
grep -q "^0,20,2.35," "${RESULT_DIR}/a/pi_results.csv"

# A reading of a ratio must be positive: the round fails with a message
ZERO="${RESULT_DIR}/zero.sh"
printf '#!/usr/bin/env bash\necho 0\n' >"$ZERO"
chmod +x "$ZERO"
rc=0
./bench run_program -o ${RESULT_DIR}/z --pi "rate,,0,1,1" -- "$ZERO" >"$TMPFILE" 2>&1 || rc=$?
[ "$rc" -eq 12 ]
grep -q "is a ratio (type 1), averaged by its harmonic mean, and its reading must be positive" "$TMPFILE"

# and so must the data of bench analyze -m 1
printf '2\n4\n0\n1\n' >"${RESULT_DIR}/zero.csv"
rc=0
./bench analyze -m 1 "${RESULT_DIR}/zero.csv" >"$TMPFILE" 2>&1 || rc=$?
[ "$rc" -eq 6 ]
grep -q "The harmonic mean needs positive data, and item 3 is 0" "$TMPFILE"
