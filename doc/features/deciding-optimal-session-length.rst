===============================
Deciding Optimal Session Length
===============================

A benchmark session consists of many rounds. After each round, Pilot
recomputes the CI for all PIs. The session ends when the CIs of all PIs
reach their target width. Because each round can include non-stable phases
(warm-up, cool-down) that do not contribute valid samples, Pilot aims to
**maximize the length of each round and minimize the total number of rounds**.
Longer rounds mean more stable-phase samples per round and fewer round-start
overheads overall.

However, Pilot cannot simply begin with the maximum work amount. In storage
benchmarks, the maximum work amount might be the full capacity of a device,
requiring dozens of hours to complete a single round. In network benchmarks,
the maximum work amount could be unlimited. Starting a session at maximum
work amount risks making the user wait an impractically long time before
seeing any result. Instead, Pilot begins with a few short trial rounds to
estimate the duration-to-work-amount ratio, then increases the work amount
systematically.

Workloads that Provide Unit Readings
-------------------------------------

When the workload produces unit readings — individual per-work-unit
measurements — Pilot treats each unit reading as one sample. A single round
can yield hundreds or thousands of samples. For example, a sequential write
workload that writes 500 MB in 1 MB increments produces 500 throughput
measurements if the workload records the duration of each write.

Pilot processes the unit readings as follows:

1. **Non-stable phase removal**: run change-point detection (EDM) on the
   unit reading sequence to find and discard warm-up and cool-down phases
   (see :doc:`warm-up-and-cool-down-phase-detection`).
2. **Autocorrelation reduction**: apply subsession analysis to the remaining
   samples until they are approximately i.i.d.
   (see :doc:`autocorrelation-detection-and-mitigation`).
3. **CI calculation**: compute the CI of the sample mean using the
   *t*-distribution. If the CI width has not yet reached the target, run
   another round.

Because each round can provide many samples, this path tends to converge
quickly to the desired CI width.

Workloads that Cannot Provide Unit Readings
--------------------------------------------

These workloads are handled using the WPS (Work-Per-Second) method described
in :ref:`sec_wps_method`. The WPS method runs the workload at varying work
amounts, fits a linear model to relate work amount to total round duration,
and extracts the stable-phase performance from the regression estimate. It
also applies non-stable phase detection and subsession analysis, and decides
the optimal number of rounds needed to reach the desired CI width.

The WPS method is appropriate for command-line tools and other workloads that
report only a single aggregate result per run rather than per-unit timings.
