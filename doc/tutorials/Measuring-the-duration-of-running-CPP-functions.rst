Measuring the Duration of Running C++ Functions
===============================================

Programmers often need to benchmark functions quickly. Most ad hoc approaches
— timing a single run, averaging a few trials, eyeballing the result — produce
unreliable and irreproducible numbers. ``libpilot`` handles the statistical
details automatically so you get scientifically valid results without needing
a statistics background.

All tutorials assume you have already compiled Pilot from source or installed
a binary package. See the :doc:`../build` page if you have not done so.

Sample 1: Benchmarking a Hash Function
---------------------------------------

We want to measure how long a hash function takes to process 1 GB of data.
The function below (courtesy of `Daniel Lemire
<http://lemire.me/blog/2015/10/22/faster-hashing-without-effort/>`_) hashes a
contiguous memory region:

.. code-block:: cpp

   #include <pilot/libpilot.h>
   #include <cstdlib>

   // Use global variables so the compiler cannot optimize them away.
   static const size_t kDataSize = 1024UL * 1024 * 1024;  // 1 GB
   static char   g_buf[kDataSize];
   static int    g_hash;

   static int hash_func_one() {
       g_hash = 1;
       for (size_t i = 0; i < kDataSize; ++i)
           g_hash = 31 * g_hash + g_buf[i];
       return 0;  // 0 means success
   }

   int main() {
       // Fill with deterministic pseudo-random data.
       for (size_t i = 0; i < kDataSize; ++i)
           g_buf[i] = static_cast<char>(i * 42);

       simple_runner(hash_func_one);
       return 0;
   }

There are only two requirements on the workload function:

1. Include ``<pilot/libpilot.h>``.
2. Pass the function to ``simple_runner()``. The function must take no
   parameters and return 0 on success. Wrap any function that does not
   match this signature in a thin wrapper.

To compile and run (assuming Pilot is installed or built from source):

.. code-block:: bash

   # If Pilot is installed system-wide:
   g++ -std=c++14 -O2 -lpilot -o benchmark_hash benchmark_hash.cc
   ./benchmark_hash

   # If building from the repository source without installing:
   g++ -std=c++14 -O2 \
       -I /path/to/pilot-bench/lib/interface_include \
       -L /path/to/pilot-bench/build/lib \
       -lpilot -Wl,-rpath,/path/to/pilot-bench/build/lib \
       -o benchmark_hash benchmark_hash.cc
   ./benchmark_hash

Alternatively, go to the ``examples/`` directory in the repository and run
``./make.sh`` to compile all examples at once.

What Pilot Does For You
------------------------

``simple_runner`` runs the workload for as many iterations as needed to
narrow the confidence interval (CI) of the measured duration to within 10%
of the sample mean. Here is why that matters.

The execution time of a function is a **random variable**: your computer runs
billions of instructions per second across many concurrent processes, so the
exact time any single function call takes varies. To characterize a random
variable reliably you need:

1. **Samples that are independent and identically distributed
   (**\ `i.i.d. <https://en.wikipedia.org/wiki/Independent_and_identically_distributed_random_variables>`_\ **)**.
   If consecutive measurements are correlated (autocorrelation), the
   standard CI formula underestimates the true uncertainty.

2. **A sufficiently large sample size**. The
   `law of large numbers <https://en.wikipedia.org/wiki/Law_of_large_numbers>`_
   requires hundreds of independent samples before the sample mean reliably
   approximates the true expected value.

On a high level, Pilot does the following:

1. **Checks for i.i.d.** by computing the autocorrelation coefficient of
   the sample sequence. If autocorrelation is too high, it applies subsession
   analysis to merge correlated adjacent samples into fewer, less-correlated
   ones (see :doc:`autocorrelation-detection-and-mitigation`).

2. **Computes the confidence interval (CI)** of the sample mean. The CI
   :math:`C` at the 95% confidence level is:

   .. math::

      C = 2 \cdot t^*_{0.975,\,\nu} \cdot \frac{s}{\sqrt{n}},

   where :math:`s` is the sample standard deviation, :math:`n` the sample
   size, and :math:`t^*_{0.975,\,\nu}` the 97.5th percentile of the
   *t*-distribution with :math:`\nu` degrees of freedom.

3. **Estimates the required sample size** using the *t*-distribution to
   determine how many samples are needed to meet the CI-width target.

4. **Detects and removes warm-up and cool-down phases** using change-point
   detection (see :doc:`warm-up-and-cool-down-phase-detection`).

5. **Runs just long enough** to collect the necessary samples, then stops.

.. _interpreting-the-benchmark-output:

Interpreting the Benchmark Output
-----------------------------------

By default, ``simple_runner()`` prints a log to stdout. Here is a walkthrough
of a typical run.

**Round startup:**

.. code-block:: none

   [2016-07-01 20:40:08] <info> Starting workload round 0 with work_amount 1
   [2016-07-01 20:40:10] <info> Finished workload round 0

Pilot runs the workload in *rounds*. In each round it calls the workload
function a number of times equal to the *work amount*. Each individual
function call produces one *unit reading* (UR). Round 0 uses work amount 1
to get an initial estimate of how long one function call takes.

**Change-point detection:**

.. code-block:: none

   [2016-07-01 20:40:10] <info> Running changepoint detection on UR data
   [2016-07-01 20:40:10] <info> Skipping non-stable phases detection because
     we have fewer than 24 URs in the last round. Ingesting all URs.
   [2016-07-01 20:40:10] <info> Ingested 1 URs from last round

Change-point detection needs at least 24 URs to identify phase boundaries.
Round 0 only has 1, so all URs are accepted. Pilot has ingested 1 unit reading.

**First summary:**

.. code-block:: none

   [2016-07-01 20:40:10] <info> 0 | Duration: R has no summary, UR has no summary

After round 0, there is not yet enough data to compute a summary for either
*Readings* (R, one per round) or *Unit Readings* (UR, one per function call).
For this benchmark we focus on UR statistics.

**Minimum work amount:**

.. code-block:: none

   [2016-07-01 20:40:10] <info> Setting adjusted_min_work_amount to 1

Pilot checks that each round is long enough to avoid being dominated by
scheduling overhead. ``adjusted_min_work_amount`` is the minimum work amount
that keeps the round above the short-round threshold (default: 1 second). Here
it is already satisfied.

**Not enough data yet:**

.. code-block:: none

   [2016-07-01 20:40:10] <info> [PI 0] doesn't have enough information for
     calculating required sample size

Pilot needs roughly 10–20 samples before it can estimate the sample size
required to reach the CI-width target.

**After a few more rounds:**

.. code-block:: none

   [2016-07-01 20:40:21] <info> 3 | Duration: R has no summary, UR m1.423 c0.04202 v0.0007472

After round 3, the UR statistics are:

* ``m1.423`` — sample mean: 1.423 seconds per function call.
* ``c0.04202`` — CI width at 95%: 0.04202 seconds. There is a 95%
  probability that the true expected value lies in
  :math:`[1.423 - 0.021,\; 1.423 + 0.021] = [1.402,\; 1.444]` seconds.
* ``v0.0007472`` — sample variance.

The CI is already about 3% of the mean, which looks good — but we also need
enough samples:

.. code-block:: none

   [2016-07-01 20:40:21] <info> [PI 0] required unit readings sample size 200
     (required sample size 200 x subsession size 1)

Pilot has determined that at least **200 independent samples** are required
before the sample mean is trustworthy by the law of large numbers. Even with
a narrow CI, a small sample size means the mean estimate is unreliable.

**Work amount limiting:**

.. code-block:: none

   [2016-07-01 20:40:21] <info> Limiting next round's work amount to 10
     (no more than 5 times the average round work amount)

Early in the session, Pilot limits the work amount growth to avoid committing
to a very long round based on insufficient data. As confidence in the
estimates grows, it allows larger work amounts.

Pilot continues running rounds until both the CI-width target and the minimum
sample size are satisfied. How long this takes depends on the variance of your
workload and the load on your machine. You can stop early if a rough answer
is sufficient — a few dozen samples is enough for a quick comparison; let it
run for hours or days when making long-term decisions.

Sample 2: Benchmarking Sequential Write Throughput
---------------------------------------------------

A more complete example is available at
``lib/test/func_test_seq_write.cc`` in the repository. It measures
sequential write I/O throughput to a file you specify, using ``libpilot``
for all statistical analysis. Build it from source (it is compiled as part
of the library tests) and run it without arguments to see usage.

You can plot the CSV output with ``lib/test/plot_seq_write_throughput.py``
(requires Python 3 and matplotlib).

Troubleshooting
---------------

**Error: "Running at max_work_amount_ still cannot meet round duration
requirement. Please increase the max work amount upper limit."**

Pilot enforces a minimum round duration (default: 1 second) to ensure
rounds are not dominated by startup overhead. If this error occurs, the
workload is too fast to produce rounds that long with the current maximum
work amount. Increase ``max_work_amount`` in the workload configuration, or
wrap the function call in a loop so each invocation takes longer.
