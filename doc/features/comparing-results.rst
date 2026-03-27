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

   \nu = \left\lfloor
     \frac{\left( \dfrac{\sigma_A^2}{n_A} + \dfrac{\sigma_B^2}{n_B} \right)^2}
          {\dfrac{\sigma_A^4}{n_A^2 (n_A - 1)} + \dfrac{\sigma_B^4}{n_B^2 (n_B - 1)}}
   \right\rfloor.

:math:`\nu` lies between :math:`\min(n_A, n_B) - 1` and
:math:`n_A + n_B - 2`. When both variances are equal it reduces to the
pooled degrees of freedom of the standard two-sample *t*-test.

The two-tailed *p*-value is then

.. math::

   p = 2 \cdot F(t,\, \nu),

where :math:`F(t, \nu)` is the cumulative distribution function of the
*t*-distribution with :math:`\nu` degrees of freedom evaluated at :math:`t`.
Multiplying by 2 accounts for the fact that we care about differences in
either direction (A > B or A < B). A *p*-value below the threshold (typically
0.01) is taken as evidence that A and B differ significantly.

Pilot also uses the first equation to compute the optimal subsession sample
size needed to achieve a target CI width — collecting just enough data to
make a reliable comparison.

Stopping Criterion
------------------

The comparison algorithm runs until all of the following conditions are met:

1. There are enough samples to calculate CIs for all workloads.
2. Each adjacent pair of CIs is either non-overlapping, or its *p*-value
   for the null hypothesis (:math:`\mu_A = \mu_B`) is below the threshold
   (default 0.01).
3. *(Optional but recommended)* Every CI is narrower than the required
   width. A tighter CI makes it easier to compare these results against new
   measurements in the future.

Choosing Work Amounts
---------------------

To minimize the number of rounds needed, Pilot chooses each round's work
amount so that round-start overhead is amortized over as much stable-phase
work as possible. Longer rounds reduce the fraction of time spent on startup
overhead and increase the number of samples per unit of wall-clock time.

.. [welch:biometrika47] B. L. Welch. The generalization of Student's
                        problem when several different population
                        variances are involved. *Biometrika*,
                        34(1–2):28–35, 1947.
