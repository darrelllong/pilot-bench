#!/usr/bin/env bash
# Mock benchmark that prints, on each run, the next of the numbers in
# $PILOT_MOCK_SEQUENCE, and fails when there are no more. Progress is
# stored in $PILOT_MOCK_SEQUENCE_ROUND_FILE.
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
read -r -a DATA <<<"$PILOT_MOCK_SEQUENCE"
ROUND=0
[ -f "$PILOT_MOCK_SEQUENCE_ROUND_FILE" ] && ROUND=$(cat "$PILOT_MOCK_SEQUENCE_ROUND_FILE")
if [ "$ROUND" -ge ${#DATA[@]} ]; then
    rm -f "$PILOT_MOCK_SEQUENCE_ROUND_FILE"
    exit 1
fi
echo "${DATA[$ROUND]}"
echo $((ROUND + 1)) >"$PILOT_MOCK_SEQUENCE_ROUND_FILE"
