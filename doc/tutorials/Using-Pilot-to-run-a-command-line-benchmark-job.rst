Using Pilot to Run a Command-Line Benchmark Job
===============================================

Pilot's command-line tool, ``bench``, can drive any program that writes
its results to standard output as CSV. This tutorial walks through using
``dd`` as a concrete I/O benchmark, illustrating the two main approaches.

All examples assume you have built Pilot from source. The ``bench`` binary is
at ``build/cli/bench`` relative to the repository root. Add it to your
``PATH`` or use the full path.

.. code-block:: bash

   # From the repository root after building:
   export PATH="$PWD/build/cli:$PATH"


The ``dd`` Benchmark
--------------------

``dd`` is a common tool for measuring raw I/O throughput. A basic invocation
writes 100 MB to a file:

.. code-block:: bash

   # Linux (bs=1M, output on stderr)
   dd if=/dev/zero of=/tmp/io_test bs=1M count=100

   # macOS (bs=1m, output on stderr)
   dd if=/dev/zero of=/tmp/io_test bs=1m count=100

On Linux the stderr output looks like:

.. code-block:: none

   100+0 records in
   100+0 records out
   104857600 bytes (105 MB, 100 MiB) copied, 0.123456 s, 849 MB/s

On macOS:

.. code-block:: none

   100+0 records in
   100+0 records out
   104857600 bytes transferred in 0.123456 secs (849274880 bytes/sec)

Running ``dd`` once gives you one data point with no statistical context.
Pilot fixes this by running many rounds with varying I/O amounts and
computing a proper confidence interval.

There are two ways to feed results from ``dd`` to Pilot. **Option 1
(recommended)** passes the round *duration* to Pilot and lets it do WPS
linear regression with automatic warm-up/cool-down detection. **Option 2**
passes throughput directly; it is simpler but slower to converge and misses
the warm-up phase correction.

Option 1: Analyzing the Round Duration (Recommended)
------------------------------------------------------

The WPS method (see :doc:`../features/warm-up-and-cool-down-phase-detection`)
needs the round duration and work amount for each run. Write a wrapper script
that runs ``dd`` with a variable I/O count and prints only the duration in
seconds:

**Linux** (``examples/benchmark_dd/run_dd.sh``):

.. code-block:: bash

   #!/usr/bin/env bash
   set -euo pipefail

   OUTPUT_FILE="$1"
   IO_COUNT="$2"

   dd if=/dev/zero of="$OUTPUT_FILE" bs=1M count="$IO_COUNT" 2>&1 | \
       awk '/bytes.*copied/ { print $NF }'
   # Prints the elapsed time in seconds, e.g.: 0.123456

**macOS** (``examples/benchmark_dd/run_dd_mac.sh``):

.. code-block:: bash

   #!/usr/bin/env bash
   set -euo pipefail

   OUTPUT_FILE="$1"
   IO_COUNT="$2"

   dd if=/dev/zero of="$OUTPUT_FILE" bs=1m count="$IO_COUNT" 2>&1 | \
       awk '/bytes transferred/ { print $5 }'
   # Prints the elapsed time in seconds, e.g.: 0.123456

Make the script executable:

.. code-block:: bash

   chmod +x run_dd.sh   # or run_dd_mac.sh on macOS

Test it manually first to confirm it prints a single number:

.. code-block:: bash

   ./run_dd.sh /tmp/io_test 100
   # Expected output: a single decimal number, e.g.: 0.132891

Now run Pilot:

.. code-block:: bash

   bench run_program \
       -d 0 \
       --wps \
       -w 1,5000 \
       -- ./run_dd.sh /tmp/io_benchmark %WORK_AMOUNT%

What each flag means:

``run_program``
    The ``bench`` subcommand for running an external program.

``-d 0``
    Column 0 of the script's CSV output is the round duration in seconds.
    ``bench`` expects the workload to print one or more comma-separated
    values per run; here there is only one value so it is column 0.

``--wps``
    Enable WPS (Work-Per-Second) analysis. Pilot will run the script with
    different I/O amounts across rounds, fit a linear model
    :math:`t = \alpha + w / v_{\text{stable}}`, and extract
    :math:`v_{\text{stable}}` (stable throughput) along with its CI.

``-w 1,5000``
    The valid work-amount range in MB. Pilot will try I/O amounts between
    1 MB and 5000 MB. Adjust the upper bound to match your device: faster
    devices (NVMe SSDs) need larger work amounts to ensure each round is
    long enough to escape the warm-up phase; slower devices (spinning disk,
    network storage) need less.

``--``
    Separates Pilot's flags from the workload command.

``./run_dd.sh /tmp/io_benchmark %WORK_AMOUNT%``
    The workload command. ``%WORK_AMOUNT%`` is a macro that Pilot replaces
    with the work amount it has chosen for each round.

**Setting the confidence level** (this fork only):

.. code-block:: bash

   bench run_program \
       -d 0 --wps -w 1,5000 \
       --confidence-level 0.99 \
       -- ./run_dd.sh /tmp/io_benchmark %WORK_AMOUNT%

The ``--confidence-level`` flag (default: 0.95) controls the confidence
level used when computing CIs. Use 0.99 for stricter statistical guarantees.

Pilot will print log messages and summary statistics as it runs. See
:ref:`Interpreting the benchmark output
<interpreting-the-benchmark-output>` for a detailed walkthrough.

Option 2: Analyzing Throughput Directly
-----------------------------------------

If you cannot control the work amount (for example, the tool does not accept
a configurable I/O size), you can pass throughput values directly. Write a
wrapper that extracts the throughput from ``dd``'s output:

**Linux** (``examples/benchmark_dd_no_wps/run_dd_extract_tp.sh``):

.. code-block:: bash

   #!/usr/bin/env bash
   set -euo pipefail

   OUTPUT_FILE="$1"
   IO_COUNT="$2"

   dd if=/dev/zero of="$OUTPUT_FILE" bs=1M count="$IO_COUNT" 2>&1 | \
       awk '/bytes.*copied/ { gsub(/[^0-9.]/, "", $(NF-1)); print $(NF-1) }'
   # Prints throughput in MB/s, e.g.: 849.3

**macOS** (``examples/benchmark_dd_no_wps/run_dd_extract_tp_mac.sh``):

.. code-block:: bash

   #!/usr/bin/env bash
   set -euo pipefail

   OUTPUT_FILE="$1"
   IO_COUNT="$2"

   dd if=/dev/zero of="$OUTPUT_FILE" bs=1m count="$IO_COUNT" 2>&1 | \
       awk '/bytes transferred/ { print $7 / $5 / 1048576 }'
   # Prints throughput in MB/s: bytes/sec ÷ 1048576

Test the script manually:

.. code-block:: bash

   chmod +x run_dd_extract_tp.sh
   ./run_dd_extract_tp.sh /tmp/io_test 5000
   # Expected output: a single number representing MB/s

Run Pilot with this script:

.. code-block:: bash

   bench run_program \
       --pi "throughput,MB/s,0,1,1" \
       -- ./run_dd_extract_tp.sh /tmp/io_test 5000

The ``--pi`` flag specifies the Performance Index to measure. The format is:

.. code-block:: none

   name,unit,column,type,must_satisfy

``name``
    Display name for the PI (e.g., ``throughput``).

``unit``
    Display unit (e.g., ``MB/s``). Used only for display.

``column``
    Zero-based column index in the workload's CSV output.

``type``
    ``0`` for an ordinary value (time, bytes, count); ``1`` for a ratio
    (throughput, speed). The type determines the correct mean calculation
    method: ratios use the harmonic mean; ordinary values use the arithmetic
    mean.

``must_satisfy``
    ``1`` if this PI's CI must meet the width requirement before the session
    ends; ``0`` to record data without requiring it to converge.

Multiple PIs can be specified by separating them with ``:``:

.. code-block:: none

   --pi "read_tp,MB/s,0,1,1:write_tp,MB/s,1,1,1"

Run ``bench run_program --help`` to see the full option reference.

**Note**: with Option 2, Pilot cannot vary the work amount per round (it is
fixed to whatever the script uses), so it cannot apply the WPS linear model.
It simply collects a series of throughput readings and computes their CI.
This means:

* You must choose an I/O size large enough that each round is dominated by
  stable-phase work (not warm-up/cool-down). A round shorter than 10 seconds
  is usually too short.
* Convergence is slower: without varying work amounts, the WPS regression
  cannot remove non-stable overhead, so more samples are needed to reach the
  same CI width.

For most practical cases, **Option 1 is the right choice**.

Quick Reference
---------------

.. code-block:: bash

   # Option 1 — duration-based, WPS analysis (recommended)
   bench run_program -d 0 --wps -w 1,5000 \
       -- ./run_dd.sh /tmp/io_test %WORK_AMOUNT%

   # Option 1 — with custom confidence level
   bench run_program -d 0 --wps -w 1,5000 --confidence-level 0.99 \
       -- ./run_dd.sh /tmp/io_test %WORK_AMOUNT%

   # Option 2 — throughput-based, no WPS
   bench run_program --pi "throughput,MB/s,0,1,1" \
       -- ./run_dd_extract_tp.sh /tmp/io_test 5000

   # Analyze an existing CSV file
   bench analyze --preset normal data.csv

   # Detect change-points in a time series
   bench detect_changepoint_edm --csv-file data.csv --field 0
