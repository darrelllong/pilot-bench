.. include:: ../global-ref.rst

Statistics 101
==============

This is not meant to be a full course in statistics. Instead we aim to
provide just enough background to understand and use Pilot effectively,
while keeping it accessible and providing links to further reading where
appropriate.

Performance Measurement and Benchmarking
-----------------------------------------

**Performance measurement** is concerned with how to measure the
performance of a running program or system, while *benchmarking* is more
about issuing a specific workload to a system in order to measure its
performance. High-quality performance measurement and benchmarking are
important for nearly everyone who works with computer systems — from
researchers to system administrators to consumers. Performance evaluation
is critical for computer scientists studying new algorithms, for system
designers making architectural decisions, and for administrators verifying
that systems meet Service-Level Agreements.

Performance measurement and benchmarking are similar but not identical. We
can usually control how benchmarks are run, but measurement sometimes needs
to be done on applications or systems over which we have little control. The
discussion below applies to both, and we use these two terms interchangeably
unless otherwise stated.

Benchmarks must meet several requirements to be useful:

* **Accuracy**: the results must reflect the performance property being
  measured.
* **Precision**: the results must be a good approximation of the true value
  with quantified error.
* **Repeatability**: multiple runs of the same benchmark under the same
  conditions should produce reasonably similar results.
* **Measured overhead**: the instrumentation overhead must be measured and
  documented.
* **Comparability**: results intended for publication must include enough
  hardware and software information for others to compare results from a
  similar environment.
* **Replicability**: others must be able to reproduce the results from
  scratch.

People often use terms like *accuracy* and *precision* loosely. Here we
define them carefully. Readers who want more background on statistical
concepts such as mean, variance, and sample size may find Ferrari's textbook
a useful reference [ferrari:78]_.

**Accuracy** reflects whether the results actually measure what the user
wants to measure. A benchmark usually exercises many components of the
system simultaneously. When we need to measure a specific property — such as
I/O bandwidth — the benchmark must be designed so that no other component
(CPU, RAM, network) is the limiting factor. This requires careful
experimental design, monitoring of related components during the benchmark,
and verification that the intended component is the bottleneck.

**Precision** is related to accuracy but is a distinct concept. Precision
describes how close a measured value is to the true population parameter.
In statistical terms, precision is quantified by the **confidence interval
(CI)**. If the CI of a throughput mean :math:`\mu` is :math:`C` at the 95%
confidence level, then there is a 95% probability that the true system
throughput lies within the interval

.. math::

   \left[\mu - \frac{C}{2},\ \mu + \frac{C}{2}\right].

A narrower CI means higher precision. In practice, CIs are typically
stated at the 95% confidence level. Presenting a single performance number
— such as "the write throughput of this disk is 100 MB/s" — without a CI
is statistically misleading: it gives no indication of how reliable or
repeatable that number is.

The CI is computed from the sample standard deviation :math:`s`, the sample
size :math:`n`, and the *t*-distribution critical value :math:`t^*` at the
desired confidence level:

.. math::

   C = 2 \cdot t^* \cdot \frac{s}{\sqrt{n}}.

This formula shows that to halve the CI width you must quadruple the sample
size — collecting more data has diminishing returns, which is one reason why
knowing *how many samples are enough* matters so much.

**Repeatability** is critical because the goal of most benchmarks is to
predict the performance of future workloads — which is only meaningful if
results are consistent. There are two kinds of errors that undermine
repeatability:

* **Systematic errors** arise when you are not measuring what you intend to
  measure — for instance, because the benchmark design is flawed or a hidden
  bottleneck prevents the target property from being exercised. Even if
  results look plausible, they may not be repeatable in a different
  environment.

* **Random errors** are caused by noise outside our control. They lead to
  non-repeatable measurements when the sample size is insufficient, or when
  samples are not independent and identically distributed (`i.i.d.`_).

Scientifically valid benchmarking requires substantial knowledge of both the
computer system and statistics. It is not surprising that many vendors
publish misleading benchmark results, and that many peer-reviewed research
publications suffer from poor execution of performance
measurement [hoefler:sc15]_.

Getting Results Fast
--------------------

The traditional approach to benchmarking — run for as long as possible and
hope the law of large numbers smooths out errors — is no longer viable. In
practice, engineers often have only one or two days to benchmark a newly
installed cluster or storage system before they must report results. Modern
distributed systems can have hundreds or thousands of tunable parameters,
and each candidate configuration requires its own benchmark run. The shorter
each benchmark is, the more configurations can be evaluated, leading to
better-tuned systems.

Existing analytical software such as `R <https://www.r-project.org/>`_ is
either too heavyweight to embed in an application, hard to integrate (R is a
large GPL-licensed package), or requires complex scripting to use its
analysis functions.

Pilot addresses this by providing a lightweight C++ library and command-line
tool that automates most analytical tasks in computer performance evaluation.
It does not introduce new statistical methods; instead it identifies the most
practical methods for performance measurement and implements heuristics to
automate and accelerate them.

.. [ferrari:78] Domenico Ferrari. *Computer Systems Performance
                Evaluation*. Prentice-Hall, 1978.

.. [hoefler:sc15] Torsten Hoefler and Roberto Belli. Scientific
                  benchmarking of parallel computing systems. In
                  *Proceedings of the 2015 International Conference
                  for High Performance Computing, Networking, Storage
                  and Analysis (SC15)*, 2015.
