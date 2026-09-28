#!/usr/bin/env bash
# Mock benchmark for testing that Pilot CLI averages a PI of type 1 (a
# ratio) by its harmonic mean. It prints one rate on each run. Progress is
# stored at /tmp/pilot_mock_benchmark_harmonic_mean_round.txt.
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
DATA=(2 4 1 4 2 1 4 1 2 2 4 1 1 4 2 4 1 2 1 4)

ROUND_FILE=/tmp/pilot_mock_benchmark_harmonic_mean_round.txt

if [ -f $ROUND_FILE ]; then
    ROUND=`cat $ROUND_FILE`
else
    ROUND=0
fi
if [ $ROUND -ge ${#DATA[@]} ]; then
    rm "$ROUND_FILE"
    exit 1
fi

echo ${DATA[$ROUND]}

ROUND=`expr $ROUND + 1`
echo $ROUND >$ROUND_FILE
