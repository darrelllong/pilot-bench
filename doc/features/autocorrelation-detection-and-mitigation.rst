========================================
Autocorrelation Detection and Mitigation
========================================

A benchmark session must run long enough to collect enough samples to
calculate the CI at the desired confidence level. The more samples collected,
the narrower the CI can be made. However, a crucial issue that is often
overlooked in published benchmark results is the **autocorrelation** among
samples.

What is Autocorrelation?
------------------------

Autocorrelation is the cross-correlation of a sequence of measurements with
itself at different time lags. Intuitively, high autocorrelation means that
earlier measurements can be used to predict later ones — the sequence has
"memory". This is a problem for statistical analysis because most standard
methods (including CI calculation) assume samples are independent of each
other. If samples are highly autocorrelated, collecting more of them does
*not* make the CI narrower in the way the formulas assume, no matter how
large the sample size becomes.

Most measurements in computer systems are autocorrelated because of the
stateful nature of the hardware and operating system. For example, most
systems use schedulers that allocate CPU time in discrete slices. Measurements
taken within a single time slice will be highly correlated with each other,
but may differ significantly between slices if the measurement interval is
shorter than the slice duration.

Measuring Autocorrelation
--------------------------

The autocorrelation coefficient of a sequence at lag :math:`\tau` is defined
as

.. math::

   R(\tau) = \frac{\operatorname{E}[(X_t - \mu)(X_{t+\tau} - \mu)]}{\sigma^2},

where :math:`\mu` is the population mean and :math:`\sigma^2` is the
population variance. Breaking this down:

* :math:`X_t` is the measurement at time :math:`t`.
* :math:`X_{t+\tau}` is the measurement :math:`\tau` steps later.
* The numerator is the expected value of the product of the deviations from
  the mean at two points separated by :math:`\tau` steps — this is the
  covariance at lag :math:`\tau`.
* Dividing by :math:`\sigma^2` normalizes the result to the range
  :math:`[-1, 1]`.

The autocorrelation coefficient :math:`R(\tau)` lies in :math:`[-1, 1]`:

* :math:`R(\tau) = 1` means measurements :math:`\tau` steps apart are
  perfectly positively correlated — knowing one tells you the other exactly.
* :math:`R(\tau) = -1` means perfectly negatively correlated.
* :math:`R(\tau) = 0` means no correlation at lag :math:`\tau`.

In practice, Pilot checks :math:`R(1)`, the lag-1 autocorrelation, which
captures correlation between adjacent samples. The range :math:`[-0.1, 0.1]`
is considered negligible autocorrelation and safe for CI
calculations [ferrari:78]_.

Subsession Analysis
-------------------

Subsession analysis [ferrari:78]_ is the method Pilot uses to reduce
autocorrelation in sample data. The idea is simple: instead of treating each
raw measurement as one independent sample, merge every :math:`n` consecutive
measurements into one subsession sample by averaging them.

More formally, if the original sequence is :math:`X_1, X_2, \ldots, X_N`,
then the :math:`n`-subsession sequence is

.. math::

   Y_k = \frac{1}{n} \sum_{i=(k-1)n+1}^{kn} X_i, \quad k = 1, 2, \ldots, \lfloor N/n \rfloor.

The subsession samples :math:`Y_k` are less correlated than the original
:math:`X_i` because each one averages over a longer time interval, smoothing
out short-range dependencies. The trade-off is that we end up with
:math:`\lfloor N/n \rfloor` subsession samples instead of :math:`N` original
samples — a smaller effective sample size.

Pilot applies subsession analysis as follows:

1. After collecting raw measurements and removing non-stable phases
   (warm-up/cool-down), compute :math:`R(1)` of the remaining samples.
2. If :math:`R(1)` is already in :math:`[-0.1, 0.1]`, the samples are
   sufficiently independent and no merging is needed (:math:`n = 1`).
3. Otherwise, increase :math:`n` and recompute :math:`R(1)` of the
   subsession samples. Repeat until the autocorrelation falls within the
   valid range.

The result is a set of subsession samples that are approximately i.i.d. and
can be used reliably for CI calculation.

.. [ferrari:78] Domenico Ferrari. *Computer Systems Performance
                Evaluation*. Prentice-Hall, 1978.
