=====================================
Warm-up and Cool-Down Phase Detection
=====================================

Performance results are often used to predict the running time of future
workloads, and it is common practice to express performance with a single
number. For example, "the write throughput of this device is *X* MB/s."
This implicitly assumes a linear model:

.. math::

   \text{duration} = \frac{\text{work amount}}{\text{speed}}.

A linear model is simple and useful, but quoting only one number only
captures the device's *stable* performance. It is not adequate when the
measured PI is significantly affected by warm-up or cool-down phases.

.. _fig:surge-write:
.. figure:: figs/fbench_randomrw_c5_t20_throughput-marked.png
   :scale: 50 %

   Throughput of a multi-node random read-write workload, showing the
   setup phase, the warm-up phase caused by caching effects, and the
   cool-down phase caused by thread shutdown.

Most computer devices require a setup or warm-up phase before reaching
stable performance, as shown in :numref:`fig:surge-write`. If not accounted
for, these phases reduce the precision of the measurement. A common
workaround is to run the workload for a long time and hope the warm-up
effect is amortized — but when the duration of the warm-up phase is unknown,
there is no way to bound its actual impact on precision.

Pilot considers the following phases of a workload:

* **Setup phase**: steps that do not consume work — allocating memory,
  initializing variables, opening files, and so on.
* **Warm-up phase**: the system is performing work but has not yet reached
  stable performance (for example, caches are being populated).
* **Stable phase**: work is consumed at a consistent, stable rate.
* **Cool-down phase**: performance begins to drop before all work is
  complete. This is typical in multi-threaded workloads when some threads
  finish their share of work before others, reducing the number of active
  threads.

Collectively these are the *non-stable phases*. When a session has multiple
rounds, each round may or may not have its own non-stable phases, and their
durations may differ across rounds.

-----------------------------------------
Workloads that Can Provide Unit Readings
-----------------------------------------

When the workload provides unit readings (per-work-unit measurements), Pilot
can compute shifts in the UR mean to locate change-points, and uses those
change-points to separate URs into phases.

Pilot finds the change-points in two steps. The first step proposes
change-points and the second step keeps those that are significant.

**Proposing.** Let the readings be :math:`x_1, \ldots, x_n`. For a split
after the first :math:`\tau` readings, with means
:math:`\overline{x}_L` and :math:`\overline{x}_R` of the two sides, the
amount that the split takes from the sum of squares is

.. math::

   \frac{\tau\,(n - \tau)}{n} \left(\overline{x}_L - \overline{x}_R\right)^2.

The split that takes the most is a candidate. The same is done to each of
the two sides, and so on, until the sides are too short to be split. No
segment is shorter than 30 readings, so there can be no change-point in
fewer than 60.

**Testing.** A candidate is kept only if the two segments next to it are
different. The readings are usually autocorrelated, so Pilot first merges
them into subsession samples (see
:doc:`autocorrelation-detection-and-mitigation`). The subsession size is the
one that the longer of the two segments needs, because the longer segment
gives the better estimate of the autocorrelation. The subsession samples of
the two segments are then compared by the rank-sum test of Wilcoxon and of
Mann and Whitney.

Let the smaller segment have :math:`k` subsession samples and the other
:math:`m`. All :math:`k + m` samples are ranked together, and :math:`U` is
the sum of the ranks of the :math:`k`, less the least that it can be,
:math:`k(k+1)/2`. If the two segments are the same, every choice of
:math:`k` ranks among :math:`k + m` is as likely as every other, and the
number of choices that give :math:`U = u` is the coefficient of
:math:`x^u` of the Gaussian binomial coefficient

.. math::

   \binom{k+m}{k}_{\!x} = \prod_{i=1}^{k} \frac{1 - x^{m+i}}{1 - x^{i}},

so that

.. math::

   P(U \le u) = \frac{1}{\binom{k+m}{k}} \sum_{j=0}^{u} [x^j] \binom{k+m}{k}_{\!x}.

When all the samples are different, Pilot calculates this probability
exactly, and the *p*-value is twice the probability of the tail that
:math:`U` is in. The calculation takes
:math:`k\,u` steps. When that is more than two million steps Pilot uses
the normal approximation, of mean :math:`km/2` and variance
:math:`km(k+m+1)/12`, which in the tail is not less than the exact value.

The exact value matters. The least *p*-value that :math:`k` samples among
:math:`k + m` can have is :math:`2/\binom{k+m}{k}`, which for 3 among 100
is :math:`1.2 \times 10^{-5}`. The *t* approximation and the normal
approximation know nothing of this limit: for 3 samples that are all less
than 97 others, Welch's test on the ranks has
:math:`p = 5 \times 10^{-31}`. Autocorrelated readings wander, and a short
excursion is two or three subsession samples, so a test that uses such an
approximation takes excursions for changes.

**Values that are the same** have no order, so they have no ranks, and the
distribution above is not that of their rank sum. What Pilot does depends
on how much of the samples they are, which is measured by the factor by
which they make the variance of the rank sum less,

.. math::

   1 - \frac{\sum_g (t_g^3 - t_g)}{n^3 - n},

where :math:`t_g` is the size of the :math:`g`-th group of values that are
the same and :math:`n = k + m`.

* If the factor is 0.99 or more there are few such values. The ranks of a
  group are dealt between the two segments in the way that is the least in
  favor of a difference, which brings the rank sum as near to its
  expectation as the group allows. The *p*-value is then not less than that
  of any other way of dealing them.

* Otherwise there are many, as in readings that have few different values.
  The values are put in two classes, the lower and the higher, at the value
  that makes the two classes most equal in size, and the test is Fisher's
  exact test: if :math:`L` of the :math:`n` samples are in the lower class,
  the number of the :math:`k` that are in it has the hypergeometric
  distribution, and the *p*-value is the probability of all numbers that
  are as far from the expectation :math:`kL/n` as the one observed, or
  further. For the 0 and 1 of a success rate the two classes are the 0 and
  the 1, and this is the exact test of two proportions.

Neither uses random numbers. When values are the same there is no one
rank sum, and so no one exact *p*-value of it. What is true is this: the
*p*-value of the first rule is not less than that of any order in which
the values that are the same could be put, and the *p*-value of the second
is exact for the two classes. Neither rejects more often than its level
says. Samples that are all the same value have :math:`p = 1`.

Both rules cost power, because the first gives up the evidence of the
values that are the same and the second gives up the order within a class.
How much is in the tables below.

The candidate with the largest *p*-value is removed, the *p*-values of its
neighbors are calculated again because their segments have grown, and this
is repeated until every candidate that is left has a *p*-value of no more
than

.. math::

   \frac{\alpha}{n}, \qquad \alpha = 0.01.

The significance level is divided by :math:`n` because a candidate is at the
place where the two sides are most different, which is one of about
:math:`n` places. :math:`\alpha` can be changed with
``pilot_set_changepoint_significance_level()``.

If the test cannot be done for a candidate, for instance because the
autocorrelation is so high that a segment has fewer than two subsession
samples, the candidate is removed.

How Well It Works
~~~~~~~~~~~~~~~~~

The tests are exact, or on the safe side of it, if the subsession samples
are independent. Those of
autocorrelated readings are only nearly so, so the significance level does
not say how often a change-point is reported where there is none. We
measured it. The numbers are from ``lib/test/changepoint_rates.cc``, which
is built with Pilot and prints them; each is from 1000 samples. The
readings are made with ``pilot_random.hpp``, so the program prints the same
numbers everywhere.

Samples with no change in them, and the fraction in which a change-point was
reported:

.. list-table::
   :header-rows: 1
   :widths: 44 14 14 14 14

   * - Readings
     - n = 100
     - n = 300
     - n = 1000
     - n = 3000
   * - Independent, normal
     - 0.1%
     - 0.3%
     - 0.3%
     - 0.5%
   * - Independent, exponential
     - 0.1%
     - 0.2%
     - 0.4%
     - 0.3%
   * - Independent, lognormal (:math:`\sigma = 1`)
     - 0
     - 0.1%
     - 0.1%
     - 0.4%
   * - Independent, Cauchy
     - 0
     - 0
     - 0.1%
     - 0.1%
   * - Independent, five values
     - 0
     - 0
     - 0
     - 0.4%
   * - Independent, 0 or 1, half are 1
     - 0.2%
     - 0
     - 0.6%
     - 0.7%
   * - Independent, 0 or 1, 95% are 1
     - 0
     - 0
     - 0
     - 0.2%
   * - Normal and a sine of period 40
     - 0
     - 0
     - 0
     - 0
   * - Autoregressive, coefficient 0.5
     - 1.3%
     - 0.8%
     - 0.6%
     - 0.6%
   * - Autoregressive, coefficient 0.8
     - 1.8%
     - 1.0%
     - 0.5%
     - 0.1%
   * - Autoregressive, coefficient 0.9
     - 2.0%
     - 1.5%
     - 0.4%
     - 0

Samples of 500 readings of which the first 50 are a warm-up that is lower by
the given number of standard deviations, and the fraction in which a
change-point was reported within 20 readings of the 50th:

.. list-table::
   :header-rows: 1
   :widths: 44 14 14 14 14

   * - Noise
     - 0.5
     - 1
     - 2
     - 4
   * - Independent, normal
     - 20.7%
     - 95.9%
     - 97.2%
     - 96.2%
   * - Independent, exponential
     - 66.4%
     - 96.3%
     - 96.4%
     - 97.4%
   * - Independent, lognormal (:math:`\sigma = 1`)
     - 90.1%
     - 97.2%
     - 98.4%
     - 98.7%
   * - Autoregressive, coefficient 0.5
     - 1.4%
     - 25.1%
     - 80.1%
     - 84.5%
   * - Autoregressive, coefficient 0.8
     - 0.3%
     - 1.8%
     - 15.1%
     - 30.0%

The same for samples of 2000 readings of which the first 200 are the
warm-up:

.. list-table::
   :header-rows: 1
   :widths: 44 14 14 14 14

   * - Noise
     - 0.5
     - 1
     - 2
     - 4
   * - Independent, normal
     - 71.1%
     - 96.1%
     - 97.9%
     - 98.5%
   * - Independent, exponential
     - 83.5%
     - 96.9%
     - 98.5%
     - 98.3%
   * - Independent, lognormal (:math:`\sigma = 1`)
     - 90.1%
     - 97.3%
     - 99.0%
     - 98.9%
   * - Autoregressive, coefficient 0.5
     - 19.7%
     - 78.1%
     - 94.0%
     - 94.3%
   * - Autoregressive, coefficient 0.8
     - 0.9%
     - 20.1%
     - 66.5%
     - 74.7%

Readings that are 0 or 1, of which the fraction that is 1 changes, and the
fraction of samples in which a change-point was reported within 20 readings
of the change:

.. list-table::
   :header-rows: 1
   :widths: 34 22 22 22

   * - Fraction that is 1
     - Changes after
     - Of readings
     - Found
   * - 0.50 to 0.95
     - 300
     - 1000
     - 97.7%
   * - 0.80 to 0.95
     - 100
     - 500
     - 48.3%
   * - 0.85 to 0.95
     - 200
     - 1000
     - 35.8%
   * - 0.90 to 0.99
     - 300
     - 1000
     - 67.7%

Samples of 500 normal readings that are rounded to a multiple of a step,
which is given in standard deviations, of which the first 50 are a warm-up
that is lower by the given number of standard deviations. Readings that are
rounded to 0.001 have few values that are the same and are tested by the
first rule; those that are rounded to 0.67 or to 1 have about ten different
values and are tested by the second.

.. list-table::
   :header-rows: 1
   :widths: 40 20 20 20

   * - Step
     - 0.5
     - 1
     - 2
   * - Not rounded
     - 20.1%
     - 96.8%
     - 97.5%
   * - 0.001
     - 20.7%
     - 94.7%
     - 97.5%
   * - 0.2
     - 7.2%
     - 92.4%
     - 98.2%
   * - 0.33
     - 6.0%
     - 82.4%
     - 98.8%
   * - 0.67
     - 7.3%
     - 80.7%
     - 98.3%
   * - 1
     - 5.8%
     - 80.1%
     - 97.6%

Readings that have few different values, such as latencies in whole
milliseconds, lose a part of the power that readings that are not rounded
have. The exact distribution of the rank sum of samples that have values
that are the same, given which values are the same, would lose less; Pilot
does not calculate it yet.

A change that is small, or that is in strongly autocorrelated readings, is
often not reported. The mean of 50 readings of a process of coefficient 0.8
has the variance of the mean of about 6 independent readings. When a change
is not reported the warm-up stays in the readings.

In a session Pilot looks for change-points again and again as the readings
come, which gives it more than one chance to report one where there is
none. It looks after every round until there are 200 readings, and then
when there are 1% more readings than the last time
(``pilot_changepoint_detection_is_due()``). The table is of sessions of
readings that have no change, 1000 for every number. It has the
fraction of sessions in which a change-point was reported at any round of
the first 300 and of the first 1000, the fraction in which one was reported
at round 300 and at round 1000, and the greatest part of the readings that
a change-point left unused.

.. list-table::
   :header-rows: 1
   :widths: 34 13 13 13 13 14

   * - Readings
     - At any round, of 300
     - At any round, of 1000
     - At round 300
     - At round 1000
     - Most not used
   * - Independent, normal
     - 1.7%
     - 3.1%
     - 0.2%
     - 0.3%
     - 96.2%
   * - Independent, exponential
     - 1.6%
     - 2.4%
     - 0.1%
     - 0.2%
     - 96.4%
   * - Independent, lognormal (:math:`\sigma = 1`)
     - 0.7%
     - 1.6%
     - 0
     - 0.1%
     - 94.8%
   * - Independent, five values
     - 0.4%
     - 0.9%
     - 0
     - 0
     - 96.4%
   * - Independent, 0 or 1, 95% are 1
     - 0.3%
     - 0.6%
     - 0.1%
     - 0
     - 95.8%
   * - Autoregressive, coefficient 0.5
     - 7.6%
     - 11.3%
     - 1.0%
     - 0.1%
     - 96.0%
   * - Autoregressive, coefficient 0.8
     - 14.8%
     - 18.6%
     - 0.9%
     - 0.3%
     - 96.2%
   * - Autoregressive, coefficient 0.9
     - 18.6%
     - 22.8%
     - 1.1%
     - 0.7%
     - 94.8%

Most change-points that are reported where there is no change do not last:
one that is no longer significant when more readings have come is dropped,
and all the readings are used again. While it lasts, the readings before it
are not used, and the last column shows that this can be nearly all of
them. This costs time. It also makes the result less good than it says it
is. The readings that are used were chosen by looking at them, and the mean
of readings that were chosen so is further from the true mean than its CI
says. A session that ends while such a change-point lasts reports such a
mean. With readings that are autocorrelated this is not rare, and the
fraction of sessions in which it happens is in the fourth and fifth
columns.

Each number in these tables is a fraction of 1000 samples or sessions, so a
fraction of 1% has a standard error of 0.3%, and one of 10% has one of
0.9%.

The Stable Segment
~~~~~~~~~~~~~~~~~~

The change-points divide the readings into segments. It is common to see
change-points at the beginning and at the end of a workload (corresponding
to warm-up and cool-down). Pilot uses the following heuristic to identify
the stable segment: it must be the **longest segment** and must contain
**more than 50% of all samples**.

For the readings of rounds, as opposed to unit readings, Pilot uses the
readings after the last change-point. It looks for change-points in all the
readings every time. A change-point is first found when there are few
readings after it, and because no segment is shorter than 30 readings it is
found before where it is; the readings that come later put it right.

.. note::

   Until September 2026 Pilot used E-Divisive with Medians
   (EDM) [james:stat.ME14]_, which is what the paper [li:mascots16]_
   describes. As Pilot used it, EDM accepted a change-point if it made the
   goodness of fit greater by 25% of what it was, and what it was before the
   first change-point is zero, so a first change-point was always accepted.
   It reported a change-point in every one of 200 samples of 60 or more
   independent readings, and about 27 in 1000 readings. With the ``strict``
   preset, which needs 200 samples, a session never finished because the
   readings before each change-point were thrown away. EDM also compared
   medians, which put a change-point that is near the end of the readings
   as many as 29 readings away from the change.

.. _sec_wps_method:

-------------------------------------------
Workloads that Cannot Provide Unit Readings
-------------------------------------------

Some workloads cannot be meaningfully divided into units — for example,
command-line tools that report only a single aggregate result. Others would
require costly source-code changes to instrument individual work units.
For these cases, Pilot uses the **Work-per-second (WPS) Linear Regression
Method** to detect and remove non-stable phases.

The WPS method works best when:

* The work amount of the workload is adjustable (Pilot controls it via the
  ``%WORK_AMOUNT%`` macro).
* There is a linear relationship between work amount and workload duration.
* The durations of the setup, warm-up, and cool-down phases are reasonably
  stable across rounds.

If one or more conditions are violated, the WPS method will produce a wide
CI or high prediction error, making the problem visible. The method also
applies autocorrelation detection and subsession analysis, which makes it
tolerant of some measurement inconsistency.

The Linear Model
~~~~~~~~~~~~~~~~

Let:

* :math:`w` = work amount for one round
* :math:`t` = total duration of the round
* :math:`t_{\text{setup}}` = duration of the setup phase
* :math:`t_{\text{warmup}}` = duration of the warm-up phase
* :math:`t_{\text{stable}}` = duration of the stable phase
* :math:`t_{\text{cooldown}}` = duration of the cool-down phase
* :math:`w_{\text{warmup}}` = work consumed during the warm-up phase
* :math:`w_{\text{stable}}` = work consumed during the stable phase
* :math:`w_{\text{cooldown}}` = work consumed during the cool-down phase

The total duration and total work decompose as (the setup phase does not
consume work):

.. math::

   t &= t_{\text{setup}} + t_{\text{warmup}} + t_{\text{stable}} + t_{\text{cooldown}} \\
   w &= w_{\text{warmup}} + w_{\text{stable}} + w_{\text{cooldown}}

The stable-phase performance :math:`v_{\text{stable}}` is the quantity we
want to measure:

.. math::

   v_{\text{stable}} = \frac{w_{\text{stable}}}{t_{\text{stable}}}
   \quad \Longrightarrow \quad
   t_{\text{stable}} = \frac{w_{\text{stable}}}{v_{\text{stable}}}
                     = \frac{w - w_{\text{warmup}} - w_{\text{cooldown}}}{v_{\text{stable}}}.

Substituting into the total duration equation:

.. math::

   t &= t_{\text{setup}} + t_{\text{warmup}}
        + \frac{w - w_{\text{warmup}} - w_{\text{cooldown}}}{v_{\text{stable}}}
        + t_{\text{cooldown}} \\
     &= \underbrace{\left(t_{\text{setup}} + t_{\text{warmup}} + t_{\text{cooldown}}
        - \frac{w_{\text{warmup}} + w_{\text{cooldown}}}{v_{\text{stable}}}\right)}_{\alpha}
        + \frac{1}{v_{\text{stable}}} \cdot w.

This gives the linear model

.. math::

   t = \alpha + \frac{1}{v_{\text{stable}}} \cdot w,

where :math:`\alpha` is the intercept that lumps together all non-stable
overhead (setup time, warm-up time, cool-down time, minus the time that
would have been spent doing stable work during those phases). The slope is
:math:`1/v_{\text{stable}}`, the reciprocal of stable performance.

Given enough :math:`(w, t)` pairs from multiple rounds, Pilot estimates
:math:`\alpha` and :math:`v_{\text{stable}}` using `Ordinary Least Squares
regression <https://en.wikipedia.org/wiki/Least_squares>`_. To compute a
valid CI for :math:`v_{\text{stable}}` using the *t*-distribution, the
:math:`(w, t)` samples must be i.i.d. Pilot applies subsession analysis to
ensure this before running the regression (see
:doc:`autocorrelation-detection-and-mitigation`). A subsession sample is the
mean work amount and the mean duration of :math:`q` consecutive rounds. The
mean is used rather than the sum because the sum of :math:`q` rounds has an
intercept of :math:`q\alpha`.

The regression needs at least three subsession samples, and their work
amounts must not all be the same, because the slope is not defined when they
are. Pilot reports that there is not enough data in both cases.

The Confidence Interval of the Rate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Let :math:`\hat{\beta}` be the estimated slope, so that
:math:`\hat{v} = 1/\hat{\beta}`. With :math:`h` subsession samples the CI of
the slope is :math:`\hat{\beta} \pm \delta`, where

.. math::

   \delta = t^*_{h-2} \sqrt{\frac{\hat{\sigma}^2}{\sum_j (w_j - \overline{w})^2}},
   \qquad
   \hat{\sigma}^2 = \frac{1}{h-2} \sum_j \left(t_j - \hat{\alpha} - \hat{\beta} w_j\right)^2.

The CI of :math:`v_{\text{stable}}` comes from inverting the two ends:

.. math::

   \frac{1}{\hat{\beta} + \delta} \le v_{\text{stable}} \le \frac{1}{\hat{\beta} - \delta}.

It is not symmetric about :math:`\hat{v}`, and it exists only when
:math:`\hat{\beta} - \delta > 0`. When the CI of the slope contains zero the
data give :math:`v_{\text{stable}}` a lower bound but no upper bound. Pilot
then reports :math:`\hat{v}` without a CI
(``pilot_wps_warmup_removal_lr_method_p()`` returns
``ERR_NOT_ENOUGH_DATA_FOR_CI`` and sets the CI width to infinity). If the
WPS CI is required (``--wps``), the session goes on to run more rounds.

Choosing Work Amounts
~~~~~~~~~~~~~~~~~~~~~

Linear regression requires that:

* The spread of work amounts across rounds is large enough for the slope to
  be estimated accurately.
* The sample size is large enough.

Pilot generates a sequence of work amounts using a **midpoint bisection
algorithm**. Let :math:`(a, b)` be the valid work-amount range supplied by
the user. The first round uses the midpoint :math:`a + \frac{b-a}{2}`, which
divides the interval in two. Future rounds use the midpoints of the resulting
sub-intervals. This produces a deterministic sequence of distinct, spread-out
values without prior knowledge of how many rounds will be needed.

:numref:`fig:warm-up-removal-work-amounts` shows the first seven values in
this sequence.

.. _fig:warm-up-removal-work-amounts:
.. figure:: figs/warm-up-removal-work-amounts.png
   :scale: 50 %

   Work amounts for the first 7 rounds. Round 1 is the midpoint of
   :math:`a` and :math:`b`; Round 2 is the midpoint of :math:`a` and
   Round 1; Round 3 is the midpoint of Round 1 and :math:`b`; and so on.

Handling Short Rounds
~~~~~~~~~~~~~~~~~~~~~

If :math:`a = 0`, some early rounds may be too short to be meaningful
because they are dominated by non-stable overhead rather than stable-phase
work. Pilot measures the duration of each completed round. If a round is
shorter than the short round detection threshold (3, 10, or 20 seconds,
depending on the preset), it is recorded but excluded from every WPS
regression. Pilot then doubles the work amount of that round
and retries until the round is long enough, and updates :math:`a` to that
new minimum work amount.

Pacing Initial Rounds
~~~~~~~~~~~~~~~~~~~~~

A second problem arises when :math:`b` is very large: the midpoint bisection
sequence starts with a large work amount, making the first few rounds very
long and delaying the first result. To ensure Pilot provides a quick
(if rough) initial estimate, it uses the following heuristic:

Suppose the first round using work amount :math:`a` takes :math:`s` seconds.
We want each subsequent round to be :math:`k` seconds longer than the
previous, so the :math:`n`-th round lasts :math:`s + (n-1)k` seconds. The
total time for :math:`n` rounds is:

.. math::

   T = \sum_{i=1}^{n} [s + (i-1)k] = ns + \frac{k \cdot n(n-1)}{2}.

To get an initial result within :math:`T` seconds, solve for :math:`k`:

.. math::

   k = \frac{2(T - ns)}{n(n-1)}.

The preset target :math:`T` is 60 seconds. The number of rounds :math:`n`
should exceed 50 so that the central limit theorem applies [chen:hpca12]_.

Tracking the Intercept
~~~~~~~~~~~~~~~~~~~~~~

One additional complication: a round may end before the stable phase begins,
in which case the linear model does not apply to it. With the WPS method
Pilot cannot see the phases of a round, so it uses the estimated intercept.

When the estimate of :math:`\alpha` is negative, Pilot excludes the rounds
whose duration is not longer than :math:`|\alpha|` and runs the regression
again. It repeats this until the exclusion threshold stops changing. The
threshold starts from the short round detection threshold and never
decreases, so each regression uses a subset of the rounds of the one before
it, and the procedure ends after at most as many regressions as there are
rounds. If fewer than three rounds are left, Pilot reports that there is not
enough data. If the WPS CI is required (``--wps``), the session goes on to
run more rounds.

Rounds are excluded on account of :math:`\alpha` only while its estimate is
negative. The rounds that an earlier regression of the same analysis
excluded stay excluded.

.. note::

   This is what the library does. The paper [li:mascots16]_ describes the
   step differently: rounds whose work amount is smaller than
   :math:`\alpha` are removed, and :math:`a` is updated to :math:`\alpha`.

.. [chen:hpca12] Tianshi Chen, Yunji Chen, Qi Guo, Olivier Temam, Yue
                 Wu, and Weiwu Hu. Statistical performance comparisons
                 of computers. In *Proceedings of the 18th
                 International Symposium on High-Performance Computer
                 Architecture (HPCA-18)*. IEEE, 2012.

.. [li:mascots16] Yan Li, Yash Gupta, Ethan L. Miller, and Darrell D. E.
                  Long. Pilot: A framework that understands how to do
                  performance benchmarks the right way. In *Proceedings
                  of the 24th International Symposium on Modeling,
                  Analysis, and Simulation of Computer and
                  Telecommunication Systems (MASCOTS 2016)*. IEEE, 2016.

.. [james:stat.ME14] Nicholas A. James, Arun Kejariwal, and
                     David S. Matteson. Leveraging cloud data to
                     mitigate user experience from "breaking bad".
                     `arXiv:1411.7955 <https://arxiv.org/abs/1411.7955>`_,
                     2014.
