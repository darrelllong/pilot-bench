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

Ratios
~~~~~~

* **The mean method of a PI was not stored.** ``pilot_set_pi_info()``
  took the mean methods of the readings and the unit readings and did not
  store them, so every PI was averaged by its arithmetic mean, and a PI of
  type 1 (``--pi name,unit,column,1,...``) had its CI computed as for type
  0. The mean method of the readings is now stored. That of the unit
  readings is still not, because the unit readings are always averaged
  arithmetically; a result that reported another method would be false. The arithmetic mean of rates is greater than their harmonic mean
  unless they are all equal, so a throughput was overstated; when every
  reading is of the same work, the harmonic mean is the total work divided
  by the total time.

  *On upgrading:* a PI of type 1 is now averaged by its harmonic mean. A
  time per operation is not a ratio in this sense and must be type 0. A
  time declared as type 1 was averaged arithmetically until now, and will
  be averaged harmonically from now on, which is wrong for a time. A
  reading of a ratio must be positive, since the harmonic mean of data
  that include 0 is 0 and its interval is not defined: if one is not,
  ``bench`` ends the session (it returns 12), ``pilot_run_workload()``
  returns ``ERR_WRONG_PARAM``, ``pilot_import_benchmark_results()`` ends
  the program, and ``bench analyze -m 1`` returns 6.
* **A ratio is analysed through its reciprocals.** The harmonic mean of
  :math:`x` is :math:`H = 1/\bar y`, where :math:`\bar y` is the mean of
  :math:`y = 1/x`. The autocorrelation, the subsession size, and the CI are
  those of :math:`\bar y`, and the CI is mapped back:
  :math:`[1/(\bar y + d),\ 1/(\bar y - d)]`, where
  :math:`d = t\, s_y / \sqrt{h}`. It is not symmetric about :math:`H`;
  its width is reported, and it has no upper end, and an infinite width,
  when :math:`\bar y \le d`. The variance is reported as
  :math:`H^4 s_y^2`, by the delta method. The required sample size is the
  one for which the width of that interval is the required width. They
  were the variance and a symmetric CI of the subsession harmonic means
  about :math:`H`, which are not those of :math:`H`; ``bench analyze -m 1``
  reported them. The readings of a session did not use them, because of
  the defect above; the WPS analysis used the autocorrelation computed in
  this way (see below). When the width :math:`W` is finite, the ends of the
  interval can be recovered from :math:`H` and :math:`W`: they are
  :math:`1/(\bar y \pm d)`, with :math:`\bar y = 1/H` and
  :math:`d = W \bar y^2 / (1 + \sqrt{1 + W^2 \bar y^2})`.
* **A required sample size that is not finite is not taken for one.** When
  no finite number of readings can make the CI as narrow as required (for
  example an ordinary value whose mean is exactly 0 and a required width
  that is a percentage of the mean, so 0), the calculation converted an
  infinite number to an unsigned integer, which is undefined: on x86-64 it
  gave 0, and the PI was taken as satisfied at the least sample size. It
  is now reported as not enough data, and the PI is not satisfied.
* **The required sample size is calculated at the session's confidence
  level.** It was calculated for 95% whatever the level, so with
  ``--confidence-level`` above 0.95 a session could end with a CI wider
  than it required, and below 0.95 it took more readings than it needed.
* **The CI of the unit readings is at the session's confidence level.** It
  was at 95%.

The Autocorrelation Limit
~~~~~~~~~~~~~~~~~~~~~~~~~

* **The limit of** ``--ac`` **and of the presets is used for the readings.**
  The subsession size of the readings and of the unit readings, and so the
  number of readings a session needs, was always found with a limit of 0.1,
  whatever ``--ac`` or the preset set; the limit was logged, and was used
  only in the WPS analysis. For the readings, ``quick`` (0.8) and
  ``normal`` (0.2) therefore used the limit of ``strict`` (0.1), and
  ``--ac`` did not change the readings' limit under any preset.

  *On upgrading:* a session under ``quick`` or ``normal`` may now end with
  fewer readings or with more. On the same readings a wider limit never
  finds a larger subsession size, but the session can end at another
  number of readings, where it can. With negatively autocorrelated
  readings a wider limit can accept a subsession size of 1 where 0.1
  needed 2, and the alternation then makes the variance larger, so the
  session needs more readings. Results obtained before met the stricter
  limit of 0.1, and still do. To have 0.1, use ``--preset strict`` or
  ``--ac 0.1``. The change-point test uses 0.1 whatever the limit, as
  before.
* **A limit or confidence level that is not a number is rejected.**
  ``--ac nan`` passed the check of its range, and with this change would
  have kept a session from ever finding a subsession size;
  ``--confidence-level nan`` ended the program. ``bench analyze`` did not
  check ``--cl`` at all. ``pilot_set_autocorrelation_coefficient()`` and
  ``pilot_set_confidence_level()`` did not check their argument, and a
  change made during a session took effect only with the next reading;
  they now reject a value out of range, returning NaN, and have the
  analysis done again.

Reported Results
~~~~~~~~~~~~~~~~

* **A CI is not reported from an earlier analysis.** For the readings, the
  variance, autocorrelation, and CI of the optimal subsession were computed
  only when the required sample size was greater than 0, and otherwise kept
  the values of the last analysis that computed them; for the unit
  readings, only when it was 0 or more. A session that ended when no
  subsession size met the autocorrelation limit, for example at its time
  limit, reported that CI, of fewer readings, as its own, in
  ``pi_results.csv`` and in its summary; so did a session whose required
  sample size could not be calculated, for example of a mean of exactly 0.
  They are now computed whenever a subsession size has been found, and are
  NaN, with a subsession size of -1, when none has. For the unit readings
  the stale values were in the summary, the text interface, and the
  analytical result, but not in ``pi_results.csv``, which leaves those
  columns empty.
* **Fewer than 2 readings, or no unit readings.** Nothing was written to
  the analytical result, which held whatever its memory held: a single
  reading of 5 was reported with a mean of 0. The mean of a single reading
  is now the reading, and everything that cannot be calculated is NaN,
  with sizes of -1.

  *On upgrading:* ``pi_results.csv``, the quiet output of ``bench``, and
  the fields of the analytical result can now hold ``nan`` where there is
  no value.
* **A variance is formatted by the delta method**, :math:`f'(m)^2
  \operatorname{Var}(X)`, with :math:`f'` by a central difference, which is
  :math:`c^2 \operatorname{Var}(X)` for a format :math:`f(x) = a + c x`. It
  was the formatted mean times var / mean, which is
  :math:`c \operatorname{Var}(X)`, not :math:`c^2 \operatorname{Var}(X)`, for
  :math:`f(x) = c x`, so it was right only for the identity: the variance of
  every PI with a unit-converting format was wrong, and changes now by the
  factor :math:`c`. It did not hold for other formats either, such as the
  reciprocal one of ``lib/test/func_test_seq_write.cc``, and it was not a
  number when the mean was 0. The text interface computed it the same way, and now uses the
  analytical result.
* **The summary of the unit readings**, and the text interface, printed a
  subsession size and a required sample size of -1 as
  18446744073709551615, and then said that the sample size was smaller
  than the threshold. They now print -1 and say which of the two could not
  be calculated. The text interface's ratio of the subsession variance to
  the mean divided by the mean twice.
* **The progress line** printed an empty CI when the required sample size
  was not greater than 0; it now prints the CI whenever there is one.
* **The quiet output of** ``bench run_program`` tested the pointer to the
  numbers of readings, not the number of readings of each PI, so a PI with
  no readings printed its fields in place of empty ones.

WPS Analysis
~~~~~~~~~~~~

* **The subsession size of the rates is chosen from their reciprocals.**
  The WPS analysis chooses its subsession size from the autocorrelation of
  the per-round rates (work amount / duration) as a harmonic mean. That is
  now the autocorrelation of the subsession means of their reciprocals,
  the time per unit of work, about their mean (see Ratios, above). It was
  that of the subsession harmonic means about the harmonic mean of all the
  rounds, which is not their mean. The two coefficients are of different
  quantities, and either can be the larger, so the subsession size, and so
  the CI of :math:`v` and when a session with work amounts stops, can be
  smaller or larger than they were.

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
