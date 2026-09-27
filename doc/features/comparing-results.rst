=================
Comparing Results
=================

The ability to compare benchmark results quickly and correctly was the
original motivation for Pilot. This is useful for system design and tuning,
for finding performance regressions during software development, and for
choosing the best parameters for storage or network systems. Without applying
correct statistical methods at runtime, most such comparisons are done
haphazardly — either terminated too early with too little data, or run far
longer than necessary.

Setup
-----

This section describes the statistical methods. What a session does with
them is described in `Stopping Criterion`_.

Suppose we have :math:`n` workloads whose results are directly comparable
(same unit, same scale), and we want to rank them. We may run new
measurements, or we may compare existing results. For existing results we
need three values per workload: the **mean**, the **subsession sample
size**, and the **subsession variance**. We use subsession quantities (see
:doc:`autocorrelation-detection-and-mitigation`) because i.i.d. samples are
a hard requirement for all the analyses in this section. Throughout this
section, all samples are subsession samples with negligible autocorrelation.

Comparing Two Results
---------------------

There are two cases when comparing two results A and B.

**Case 1: Non-overlapping confidence intervals.** If the CIs of A and B do
not overlap, we can conclude directly that one is greater than the other at
the chosen confidence level.

**Case 2: Overlapping confidence intervals.** When CIs overlap, we cannot
conclude directly — the true means could be equal. In this case Pilot uses
**Welch's unequal-variance** *t*-**test** [welch:biometrika47]_, an
adaptation of Student's *t*-test that is more reliable when the two samples
have unequal variances and unequal sizes. Both conditions are common in
system benchmarks.

Welch's *t*-test
~~~~~~~~~~~~~~~~

The null hypothesis is that there is no statistically significant difference
between A and B (i.e., :math:`\mu_A = \mu_B`). We compute the probability
(the *p*-value) of observing results at least as different as A and B if the
null hypothesis were true. A small *p*-value is evidence against the null
hypothesis.

Let :math:`\overline{x}` denote the sample mean, :math:`\sigma^2` the sample
variance, and :math:`n` the subsession sample size for each workload. The
test statistic is

.. math::

   t = \frac{\overline{x}_A - \overline{x}_B}
            {\sqrt{\dfrac{\sigma_A^2}{n_A} + \dfrac{\sigma_B^2}{n_B}}}.

The numerator is the difference of sample means. The denominator is the
standard error of that difference, pooled across the two samples without
assuming equal variance. A larger :math:`|t|` indicates that the observed
difference is larger relative to the measurement noise.

This statistic follows an approximate *t*-distribution, but not the standard
one — the degrees of freedom must be estimated because the two variances are
not assumed equal. The **Welch-Satterthwaite equation** gives the effective
degrees of freedom:

.. math::

   \nu =
     \frac{\left( \dfrac{\sigma_A^2}{n_A} + \dfrac{\sigma_B^2}{n_B} \right)^2}
          {\dfrac{\sigma_A^4}{n_A^2 (n_A - 1)} + \dfrac{\sigma_B^4}{n_B^2 (n_B - 1)}}.

:math:`\nu` lies between :math:`\min(n_A, n_B) - 1` and
:math:`n_A + n_B - 2`. When the variances are equal and the sample sizes are
equal it is :math:`n_A + n_B - 2`, the degrees of freedom of the standard
two-sample *t*-test.

The two-tailed *p*-value is then

.. math::

   p = 2 \cdot F(-|t|,\, \nu),

where :math:`F(x, \nu)` is the cumulative distribution function of the
*t*-distribution with :math:`\nu` degrees of freedom evaluated at :math:`x`.
Multiplying by 2 accounts for the fact that we care about differences in
either direction (A > B or A < B). A *p*-value below the threshold is taken
as evidence that A and B differ significantly.
``pilot_p_eq()`` calculates :math:`p` and the CI of
:math:`\overline{x}_A - \overline{x}_B`.

Required Sample Size
~~~~~~~~~~~~~~~~~~~~

The same statistic gives the sample size that a comparison needs. Let A be
a baseline that has been measured already, :math:`d = \overline{x}_A -
\overline{x}_B`, and :math:`t^*` the critical value of the
*t*-distribution for the required :math:`p`. Requiring
:math:`|t| \ge t^*` and solving for :math:`n_B`:

.. math::

   n_B \ge \frac{\sigma_B^2}
                 {\left( \dfrac{d}{t^*} \right)^2 - \dfrac{\sigma_A^2}{n_A}}.

The result is rounded up. If the denominator is not positive no sample size
is large enough: either the two means are the same, or the uncertainty of
the baseline alone is more than the required :math:`p` allows.
``pilot_optimal_sample_size_for_eq_test()`` returns ``ERR_NOT_ENOUGH_DATA``
in that case.

Stopping Criterion
------------------

What the library implements is the comparison of a running workload against
a baseline. The baseline is a mean, a subsession sample size, and a
subsession variance, set with ``pilot_set_baseline()`` or loaded with
``pilot_load_baseline_file()``. Once a baseline is set for a PI, the session
keeps running rounds until the number of unit readings exceeds
:math:`q \cdot n_B`, where :math:`q` is the subsession size and :math:`n_B`
is the required sample size above, or the minimum sample size if that is
greater. If :math:`n_B` cannot be calculated, the session tries to double
the number of unit readings. The required :math:`p` is 0.05. It cannot be
changed through the API.

.. warning::

   A workload whose mean is the same as the mean of the baseline never
   satisfies this criterion, because the null hypothesis cannot be
   rejected. The session then runs until the session duration limit or the
   work amount limit is reached. Set one of them when comparing against a
   baseline.

This applies to unit readings only. The comparison of readings and of WPS
is not implemented, and the ``bench`` command-line tool has no option for
setting a baseline.

.. note::

   The paper [li:mascots16]_ describes a more general algorithm that is
   **not implemented** in the library. It ranks :math:`n` workloads by
   running their rounds in turn until:

   1. There are enough samples to calculate CIs for all workloads.
   2. Each adjacent pair of CIs is either non-overlapping, or its
      *p*-value for the null hypothesis (:math:`\mu_A = \mu_B`) is below
      a threshold (usually 0.01).
   3. *(Optional but recommended)* Every CI is narrower than the required
      width. A tighter CI makes it easier to compare these results against
      new measurements in the future.

   The paper also takes the floor of :math:`\nu`; the library does not.

Choosing Work Amounts
---------------------

To minimize the number of rounds needed, Pilot chooses each round's work
amount so that round-start overhead is amortized over as much stable-phase
work as possible. Longer rounds reduce the fraction of time spent on startup
overhead and increase the number of samples per unit of wall-clock time.

.. [li:mascots16] Yan Li, Yash Gupta, Ethan L. Miller, and Darrell D. E.
                  Long. Pilot: A framework that understands how to do
                  performance benchmarks the right way. In *Proceedings
                  of the 24th International Symposium on Modeling,
                  Analysis, and Simulation of Computer and
                  Telecommunication Systems (MASCOTS 2016)*. IEEE, 2016.

.. [welch:biometrika47] B. L. Welch. The generalization of Student's
                        problem when several different population
                        variances are involved. *Biometrika*,
                        34(1–2):28–35, 1947.
