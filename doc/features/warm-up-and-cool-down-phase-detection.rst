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

Multiple change-point detection is a challenging problem, particularly when
we cannot assume anything about the distribution of the error or the
underlying process, and when the algorithm must support online updates.
After evaluating several methods, we found that **E-Divisive with Medians
(EDM)** [james:stat.ME14]_, published by Matteson and James in 2014, best
fits our requirements.

EDM is:

* **Non-parametric**: it works on the mean and variance of the data without
  assuming a particular distribution.
* **Robust**: it performs well on data drawn from a wide range of
  distributions, including non-normal ones.
* **Fast**: the initial computation is :math:`O(n \log n)` and each
  subsequent update is :math:`O(\log n)`.

EDM outputs a list of change-points that divide the time series into
segments. It is common to see multiple change-points at the beginning and
end of a workload (corresponding to warm-up and cool-down). Pilot uses the
following heuristic to identify the stable segment: it must be the **longest
segment** and must contain **more than 50% of all samples**. This
effectively removes any number of non-stable phases at the start and end of
the data.

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
:doc:`autocorrelation-detection-and-mitigation`).

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
shorter than a preset lower bound (typically 1 second), it is recorded but
excluded from analysis. Pilot then doubles the work amount of that round
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

One additional complication: the midpoint bisection may produce work amounts
smaller than :math:`\alpha` (the total non-stable work), in which case the
stable phase is absent from the round and the linear model does not apply.
Pilot tracks the estimated :math:`\alpha` after each round. Any round whose
work amount is less than the current estimate of :math:`\alpha` is excluded
from analysis, and :math:`a` is updated to the current :math:`\alpha`.

.. [chen:hpca12] Tianshi Chen, Yunji Chen, Qi Guo, Olivier Temam, Yue
                 Wu, and Weiwu Hu. Statistical performance comparisons
                 of computers. In *Proceedings of the 18th
                 International Symposium on High-Performance Computer
                 Architecture (HPCA-18)*. IEEE, 2012.

.. [james:stat.ME14] Nicholas A. James, Arun Kejariwal, and
                     David S. Matteson. Leveraging cloud data to
                     mitigate user experience from "breaking bad".
                     `arXiv:1411.7955 <https://arxiv.org/abs/1411.7955>`_,
                     2014.
