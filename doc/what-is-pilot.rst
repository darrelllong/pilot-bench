.. include:: global-ref.rst

What is Pilot?
==============

The Pilot Benchmark Framework provides a tool (``bench``) and a library
(``libpilot``) to automate computer performance measurement tasks. The
project is designed to answer questions like these that arise often in
performance measurement:

*  How long should I run this benchmark to get a precise result?
*  Is this new scheduler really 3% faster than the old one, or is that a
   measurement error?
*  Which setting is faster for my database: 20 worker threads or 25?

Our design goals include:

*  Being as intelligent as possible so users do not have to undergo rigorous
   training in statistics or computer science.
*  Results must be statistically valid (accurate, precise, and repeatable).
*  Using the shortest possible time to reach a valid result.

To achieve these goals, Pilot provides the following functions:

*  Automatically deciding what statistical analysis method is appropriate based
   on system configuration, measurement requirements, and existing results.
*  Deciding the shortest benchmark duration needed to achieve a desired
   confidence interval width.
*  Saving test results in a standard format for sharing and comparing.
*  And much more.

To learn how to use Pilot, head to the :doc:`tutorial-list`.

Pilot is written in C++ for fast in-place analysis and is released under a
dual BSD 3-clause and GPLv2+ license. It compiles on major Linux
distributions and macOS.

For questions and discussion, please open an issue on
`GitHub <https://github.com/darrelllong/pilot-bench/issues>`_.


Why Should I Try Pilot?
-----------------------

Everyone needs to do performance measurement from time to time:
home users need to know their Internet speed, an engineer may need to
find the bottleneck of a system, a researcher may need to quantify the
performance improvement of a new algorithm. Yet not everyone has received
rigorous training in statistics and computer performance evaluation. That
is one of the reasons why we can find many incomplete or irreproducible
benchmark results, even in peer-reviewed scientific publications. A widely
reported study published in *Science* [osc:science15]_ found that 60% of
psychology experiments could not be replicated. Computer science cannot
afford to be complacent. Torsten Hoefler and Roberto Belli analyzed 95
papers from HPC-related conferences and found that most papers are flawed
in experimental design or analysis [hoefler:sc15]_.

Specifically, if any published number is derived from fewer than 20 samples,
or is presented as a mean without variance or confidence interval (CI), the
authors are likely doing it wrong. The following problems are common in
published results:

* **Imprecise**: the result may not be a good approximation of the true
  value; often caused by failing to account for CI width or not collecting
  enough samples.

* **Inaccurate**: the result may not reflect what you actually need to
  measure; often caused by a hidden bottleneck in the system.

* **Ignoring overhead**: not measuring or documenting the measurement and
  instrumentation overhead.

* **Presenting improvements without a p-value**: this makes it impossible
  to know how statistically reliable the improvements are.

Time is another limiting factor. People rarely allocate much time to
performance evaluation or tuning, yet few newly designed or deployed systems
can meet their expected performance targets without a careful tuning process.
A large part of that tuning process is spent running benchmark workloads to
construct performance models or test candidate configurations. Designers are
often given only a few days for these tasks and must rush by cutting corners,
leading to unreliable benchmark results and sub-optimal tuning decisions.

We want to improve this situation by producing statistically valid results in
the shortest possible time. We begin with a high-level overview of the
analytical methods needed to generate results that meet statistical
requirements, then design heuristic methods to automate and accelerate them.
We cover methods to measure and correct autocorrelation in samples, to use
the *t*-distribution to estimate optimal test duration from existing samples,
to detect the duration of warm-up and cool-down phases, to detect shifts of
the mean in samples, and to use the *t*-test to estimate the optimal test
duration for comparing benchmark results. In addition, we propose two new
algorithms: a simple linear performance model that accounts for warm-up and
cool-down phases using only total work amount and workload duration, and a
method to detect and measure the system's performance bottleneck while
keeping overhead within an acceptable range.

To encourage adoption of these methods, we implement them in Pilot, which
automates many performance evaluation tasks using real-time time-series
analysis. It helps users who lack deep statistics knowledge get good
performance results, and frees experienced researchers from manually
executing benchmarks so they can obtain scientifically correct results in
the shortest possible time.

Changes in This Fork
--------------------

This repository is a modernized fork of the original
`ASCAR Pilot <https://github.com/ascar-io/pilot-bench>`_ by Yan Li et al.
The following changes have been made:

* **CMake 4.x and Boost 1.74+ compatibility**: the build system has been
  updated to work with current toolchains. The ``boost_system`` library is
  no longer required (it became header-only in Boost 1.74). C++14 is now
  required.

* **Headless build mode (``WITH_TUI=OFF``)**: the text UI (curses/CDK) is
  now optional and disabled by default. This is the recommended mode for CI
  pipelines, remote hosts, containers, and scripted use. Pass
  ``-DWITH_TUI=ON`` only when the interactive text interface is needed.

* **``--confidence-level`` flag in ``run_program``**: the confidence level
  for CI calculations can now be set directly on the command line when
  running a benchmark program (default: 0.95).

* **Code and test quality improvements**: probe scripts are hardened,
  suppression-style wording has been removed from the codebase, and
  portability fixes have been applied.

.. [hoefler:sc15] Torsten Hoefler and Roberto Belli. Scientific
                  benchmarking of parallel computing systems. In
                  *Proceedings of the 2015 International Conference
                  for High Performance Computing, Networking, Storage
                  and Analysis (SC15)*, 2015.

.. [osc:science15] Open Science Collaboration. Estimating the
                   reproducibility of psychological
                   science. *Science*, 349(6251), 2015.
