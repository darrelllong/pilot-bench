=========
Changelog
=========

Pilot changes over time. This page records what has changed since the
paper [li:mascots16]_ and since the original
`ASCAR Pilot <https://github.com/ascar-io/pilot-bench>`_, why, and what a
user who upgrades has to look out for.

.. contents::
   :local:
   :depth: 1


Where Pilot Differs from the Paper
----------------------------------

.. list-table::
   :header-rows: 1
   :widths: 24 38 38

   * - Subject
     - The paper
     - Pilot now
   * - Change-point detection
     - E-Divisive with Medians (EDM).
     - Binary segmentation proposes change-points, and the rank-sum test
       on the subsession samples, or Fisher's exact test if many of them
       are the same, decides which to keep. See
       :doc:`features/warm-up-and-cool-down-phase-detection`.
   * - Rounds that are left out of the WPS regression
     - Rounds whose work amount is smaller than :math:`\alpha`.
     - Rounds that are shorter than the short round detection threshold,
       and, while the estimate of :math:`\alpha` is negative, rounds that
       are shorter than :math:`|\alpha|`.
   * - Comparing results
     - Ranks :math:`n` workloads by running their rounds in turn.
     - Compares the unit readings of one workload against a baseline. The
       ranking algorithm is not implemented. See
       :doc:`features/comparing-results`.
   * - Threshold of the *p*-value of a comparison
     - Usually 0.01.
     - 0.05.
   * - Degrees of freedom of Welch's test
     - The floor of the Welch-Satterthwaite value.
     - The Welch-Satterthwaite value.


September 2026
--------------

Change-Point Detection
~~~~~~~~~~~~~~~~~~~~~~

EDM, as Pilot used it, accepted a change-point if it made the goodness of
fit greater by 25% of what it was. Before the first change-point the
goodness of fit is zero, so a first change-point was always accepted. In
200 samples each of 60, 100, 300, and 1000 independent readings that had no
change in them, it reported a change-point in every sample, and about 27 in
1000 readings.

Pilot does not use the readings before the last change-point. A session
that needed more than about 60 readings therefore worked with the last 30
to 60 of them only. The results of such sessions were calculated correctly
from the readings that were used, but the sessions ran for longer than they
had to, and a session that needed more than 60 subsession samples, as every
session of the ``strict`` preset does, could not finish.

EDM also compared medians. The median of a segment does not move until half
of the segment is different, so the best split of a change that is near the
end of the readings was as many as 29 readings away from the change.

The method that replaces it, how often it reports a change-point where
there is none, and how often it finds one that is there, are in
:doc:`features/warm-up-and-cool-down-phase-detection`. The numbers there
are printed by ``lib/test/changepoint_rates.cc``.

Two methods were tried in its place before this one, and were not
released. What was wrong with them is recorded here because both looked
right.

* The first compared the ranks of the subsession samples by Welch's
  *t*-test. Its *p*-values were wrong by many orders of magnitude when a
  segment had two or three subsession samples: for 3 samples that are all
  less than 97 others it had :math:`5 \times 10^{-31}`, and the least that
  the *p*-value can be is :math:`1.2 \times 10^{-5}`. It took the
  excursions of autocorrelated readings for changes, and it reported a
  change-point in readings of 0 and 1 whenever 30 of them in a row were the
  same. One session ended with a success rate of 1 and a CI of 0, of
  readings of which 95% were 1.

* The second used the rank-sum test with its exact distribution, and put
  the values that were the same in an order at random before it ranked
  them. The random numbers depended on the sizes of the two samples only,
  so that the same samples would always have the same *p*-value. But then
  the same sizes always had the same order, which is a bias and not a
  chance: for some sizes the test rejected at 5% in every sample of
  readings that had no difference, and two samples that were all the same
  value could have :math:`p = 2.5 \times 10^{-6}`.

Both were found by giving the test samples of which the answer was known.

Pilot now looks for change-points in all the readings. It used to look in
the readings after the last change-point only, so a change-point that was
found early, and before where it was, stayed there. It looks after every
round until there are 200 readings, and then when there are 1% more than
the last time.

The Twitter BreakoutDetection library is no longer used, and is no longer
downloaded and patched by the build.

WPS Analysis
~~~~~~~~~~~~

* **Subsession samples are means.** They were the sums of :math:`q` rounds,
  of which the intercept is :math:`q\alpha`. With a subsession size greater
  than 1, :math:`\alpha` was :math:`q` times what it is, and the CI of
  :math:`v` was :math:`q` times too wide. :math:`v` was not affected.
* **The duration threshold is in nanoseconds.** It was set from
  :math:`\alpha`, which is in seconds, so an :math:`\alpha` of -3.5 s made a
  threshold of 3 ns. It also fell to 0 once there was a result, which let
  the rounds that are too short into the regression.
* **The same work amount in every round** is reported as not enough data.
  The regression returned success with results that were not numbers.
* **A rate that has no upper bound.** When the CI of the slope contains
  zero, :math:`v` has no upper bound. Pilot now reports :math:`v` without a
  CI; ``wps_v_ci`` is -1. It used to report the difference of the inverted
  ends of the CI of the slope, a number that means nothing and that could
  satisfy the required width.
* **The errors** ``wps_err`` and ``wps_naive_v_err`` are calculated from the
  rounds that the regression uses. Their ``_percent`` values are fractions
  of the sum of the durations in seconds.
* **Sums** of work amounts and of durations are in floating point. The sum
  of the durations that a workload reports does not have to fit in a 64-bit
  integer.

Comparison
~~~~~~~~~~

* The sample size that a comparison needs is rounded up, not down.
* When no sample size is large enough, because the means are the same or
  because the baseline alone is too uncertain,
  ``pilot_optimal_sample_size_for_eq_test()`` returns
  ``ERR_NOT_ENOUGH_DATA``. It used to convert a negative number to an
  unsigned integer, which gave 0 on some machines and a very large number on
  others.
* ``pilot_p_eq()`` and ``pilot_optimal_sample_size_for_eq_test()`` no longer
  end the program when neither sample has any variance.

Measurement
~~~~~~~~~~~

Pilot measured the round duration, and the duration of each call of the
function of ``simple_runner``, with the wall clock time of
``boost::timer::cpu_timer``. With Boost 1.92 on macOS that clock moves in
steps of 10 ms, so a call that took less than 10 ms was measured as 0 or as
10 ms. Pilot now uses ``std::chrono::steady_clock``.

``bench``
~~~~~~~~~

* ``--duration-col`` is read. It was parsed, and required for ``--wps``,
  but the round duration was always the time that Pilot measured for
  running the workload program.
* Without ``--pi``, the WPS analysis has to converge whether ``--wps`` is
  set or not, and ``--duration-col`` and ``--work-amount`` are required.
  Such a session used to run one round and finish with exit code 0 and no
  result.
* A line is split into columns by commas or by whitespace, and whitespace
  next to a comma is part of the separator. See :doc:`reference`.
* ``wps_analysis.csv`` had 12 column names and 11 values.
* The example scripts for ``dd`` work with the ``dd`` of Linux and of macOS
  and print what the tutorial says they print.

Library
~~~~~~~

* ``pilot_random.hpp`` is new: PCG64 seeded through SplitMix64, uniform
  variates, and the ziggurat of Marsaglia and Tsang for normal variates.
  Pilot needs random numbers to make readings of which the answer is known.
  The distributions of the C++ standard library give different numbers
  with different compilers, so a measurement that uses them cannot be
  repeated.
* ``pilot_rank_sum_test()`` is new. It is the rank-sum test, with Fisher's
  exact test for samples that have many values that are the same.
* ``pilot_set_changepoint_significance_level()`` and
  ``pilot_changepoint_detection_is_due()`` are new.
* ``pilot_set_next_round_work_amount_hook()`` and ``pilot_get_log_level()``
  are exported. A program that called them could not be linked with the
  shared library.
* ``pilot_est_sample_var_dist_unknown()`` is removed from ``libpilot.h``. It
  was never defined.

Build
~~~~~

* The tests depend on gtest, so a parallel build from scratch does not
  compile them before gtest is downloaded.
* The project can be built with Ninja.
* The ``patch`` program is not required.

What Is Left to Do
~~~~~~~~~~~~~~~~~~

These are known, and are not defects of what Pilot reports.

* **Readings that have few different values.** The change-point detection
  finds fewer changes in them than it could, because the two rules for
  values that are the same give up a part of the evidence. The exact
  distribution of the rank sum, given which values are the same, would
  lose less.
* **Readings that are strongly autocorrelated.** In a session of such
  readings a change-point is reported where there is none more often than
  in one analysis, because the analysis is done again and again. Most such
  change-points do not last, but while one lasts most of the readings can
  be left unused. A subsession size that is chosen with a margin would
  make this less frequent and would find fewer of the changes that are
  there.
* **The comparison of more than two results**, which the paper describes,
  is not implemented.

What to Look Out For When Upgrading
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. **Column numbers.** ``1, 2`` has two columns; it had three. A column
   number that was chosen to get past the empty columns has to be changed.
#. **``--duration-col``** is used. A workload that prints something that is
   not the duration in seconds in that column now gets wrong durations, or
   fails.
#. **Sessions that compare against a baseline.** A workload whose mean is
   the same as the mean of the baseline runs until the session duration
   limit or the work amount limit. On some machines it used to stop at the
   minimum sample size.
#. **Sessions finish with fewer rounds** when they need more than about 60
   readings, because the readings are no longer thrown away.
#. **``wps_v_ci`` can be -1** when ``wps_has_data`` is true.
#. **``--percent``** of ``bench detect_changepoint_edm``, and the
   ``percent`` and ``degree`` parameters of the change-point functions, are
   not used.

.. [li:mascots16] Yan Li, Yash Gupta, Ethan L. Miller, and Darrell D. E.
                  Long. Pilot: A framework that understands how to do
                  performance benchmarks the right way. In *Proceedings
                  of the 24th International Symposium on Modeling,
                  Analysis, and Simulation of Computer and
                  Telecommunication Systems (MASCOTS 2016)*. IEEE, 2016.
