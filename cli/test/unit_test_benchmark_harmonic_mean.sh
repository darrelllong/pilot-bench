#!/usr/bin/env bash
# Unit test for Pilot CLI tool: a PI of type 1 (a ratio) is averaged by its
# harmonic mean, and a PI of type 0 by its arithmetic mean. The readings are
# 2 4 1 4 2 1 4 1 2 2 4 1 1 4 2 4 1 2 1 4. Under the default preset the
# ratio converges after 12 readings, whose harmonic mean is 12 / 7 = 1.71429
# (their arithmetic mean would be 2.33333); the ordinary value converges
# after 10, whose arithmetic mean is 2.3.
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
[ "$rc" -eq 0 ]
grep -q "^0,12,1.71429,1.71429," "${RESULT_DIR}/h/pi_results.csv"

rm -f /tmp/pilot_mock_benchmark_harmonic_mean_round.txt
rc=0
./bench run_program -o ${RESULT_DIR}/a --ci-perc 0.9 --min-sample-size 10 --pi "rate,,0,0,1" \
    -- ./mock_benchmark_harmonic_mean.sh >"$TMPFILE" 2>&1 || rc=$?
[ "$rc" -eq 0 ]
grep -q "^0,10,2.3,2.3," "${RESULT_DIR}/a/pi_results.csv"

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
