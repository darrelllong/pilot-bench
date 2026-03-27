Command-Line Reference
======================

The ``bench`` binary provides three subcommands. Run ``bench <subcommand> --help``
at any time to see the full option list for that subcommand.

.. contents::
   :local:
   :depth: 1


bench run_program
-----------------

Run an external program as a benchmark workload and measure one or more
Performance Indices (PIs) from its output.

.. code-block:: none

   bench run_program [options] -- <program> [args...]

The ``--`` is required to separate Pilot's options from the workload command.

**Macros** substituted in the workload command before each round:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Macro
     - Replaced with
   * - ``%WORK_AMOUNT%``
     - The work amount Pilot has chosen for this round
   * - ``%RESULT_DIR%``
     - A per-round directory for storing any output files

Options
~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Flag
     - Short
     - Description
   * - ``--help``
     -
     - Print the help message and exit.
   * - ``--ac <value>``
     - ``-a``
     - Autocorrelation limit. ``value`` must be in ``(0, 1]``; the acceptable
       range is set to ``[-value, value]``. Overrides the preset. See
       :doc:`features/autocorrelation-detection-and-mitigation`.
   * - ``--ci <value>``
     - ``-c``
     - Required CI width as an *absolute* value. Set to ``-1`` to disable.
       If both ``--ci`` and ``--ci-perc`` are specified, the narrower
       constraint applies.
   * - ``--ci-perc <value>``
     -
     - Required CI width as a *percentage of the mean* (e.g. ``0.10`` for
       10%). Set to ``-1`` to disable. Overrides the preset's default CI
       width.
   * - ``--confidence-level <value>``
     -
     - Probability that the true mean lies within the reported CI. Must be
       in ``(0, 1)``. Default: ``0.95``. Lower values (e.g. ``0.90``)
       converge in fewer rounds; higher values (e.g. ``0.99``) require more.
   * - ``--duration-col <col>``
     - ``-d``
     - Zero-based column index of the round duration (in seconds) in the
       workload's CSV output. Required for WPS analysis (``--wps``).
   * - ``--env <NAME=VALUE>``
     -
     - Set an environment variable for the workload process only. Useful for
       ``LD_PRELOAD`` and similar variables that should not affect Pilot
       itself. May be repeated.
   * - ``--min-sample-size <n>``
     - ``-m``
     - Minimum required subsession sample size before the session can end.
       Overrides the preset default.
   * - ``--output-dir <path>``
     - ``-o``
     - Directory for storing per-round result files.
   * - ``--pi <spec>``
     - ``-p``
     - Performance Index specification. Format::

         name,unit,column,type,must_satisfy[:name,unit,column,type,must_satisfy...]

       Fields:

       - ``name`` — display name (may be empty)
       - ``unit`` — display unit, e.g. ``MB/s`` (may be empty)
       - ``column`` — zero-based column index in workload CSV output
       - ``type`` — ``0`` ordinary value (time, bytes); ``1`` ratio
         (throughput, speed) uses harmonic mean; ``2`` binary (0 or 1) uses
         binomial CI
       - ``must_satisfy`` — ``1`` if this PI's CI must converge before the
         session ends; ``0`` to record only

       Multiple PIs are separated by ``:``.
   * - ``--preset <mode>``
     -
     - Statistical rigor preset. Sets autocorrelation limit, CI width,
       minimum sample size, and round duration threshold together. See
       `Presets`_ below.
   * - ``--quiet``
     - ``-q``
     - Suppress informational output.
   * - ``--session-limit <seconds>``
     - ``-s``
     - Stop with exit code 13 if the session exceeds this duration.
       Default: unlimited.
   * - ``--valid-rc <code>``
     -
     - Accept this return code from the workload as success. Default: only
       ``0`` is accepted. May be repeated for multiple codes.
   * - ``--verbose``
     - ``-v``
     - Print debug information.
   * - ``--work-amount <min,max>``
     - ``-w``
     - Valid work-amount range. Pilot will vary the work amount between
       ``min`` and ``max`` across rounds. Required when using ``--wps``.
   * - ``--wps``
     -
     - Enable Work-Per-Second linear regression analysis. Requires
       ``--duration-col`` and ``--work-amount``. Detects and removes
       warm-up and cool-down phases automatically. See
       :doc:`features/warm-up-and-cool-down-phase-detection`.


bench analyze
-------------

Analyze an existing CSV file of measurements and report the mean, CI,
autocorrelation, and required sample size.

.. code-block:: none

   bench analyze [options] <csv-file>

Use ``-`` as the filename to read from stdin.

Options
~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Flag
     - Short
     - Description
   * - ``--help``
     -
     - Print the help message and exit.
   * - ``--ac <value>``
     - ``-a``
     - Autocorrelation limit in ``(0, 1]``. Range set to ``[-value, value]``.
       Overrides the preset.
   * - ``--cl <value>``
     - ``-c``
     - Confidence level for CI calculation. Must be in ``(0, 1)``.
       Default: ``0.95``.
   * - ``--field <n>``
     - ``-f``
     - Zero-based column index of the field to analyze. Default: ``0``.
   * - ``--ignore-lines <n>``
     - ``-i``
     - Skip the first ``n`` lines of the file (e.g. to skip a header row).
   * - ``--mean-method <n>``
     - ``-m``
     - Mean calculation method. ``0`` = arithmetic mean (default);
       ``1`` = harmonic mean (use for throughput and speed ratios).
   * - ``--preset <mode>``
     -
     - Statistical rigor preset. See `Presets`_ below.
   * - ``--quiet``
     - ``-q``
     - Suppress informational output.
   * - ``--verbose``
     - ``-v``
     - Print debug information.


bench detect_changepoint_edm
-----------------------------

Run E-Divisive with Medians (EDM) change-point detection on a time series
and print the detected change-point indices.

.. code-block:: none

   bench detect_changepoint_edm [options]

Options
~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 15 55

   * - Flag
     - Short
     - Description
   * - ``--help``
     -
     - Print the help message and exit.
   * - ``--csv-file <file>``
     - ``-c``
     - Input CSV file. Use ``-`` for stdin.
   * - ``--field <n>``
     - ``-f``
     - Zero-based column index of the time series to analyze.
   * - ``--ignore-lines <n>``
     - ``-i``
     - Skip the first ``n`` lines of the file.
   * - ``--percent <value>``
     - ``-p``
     - Penalization constant controlling sensitivity to new change-points.
       Specifies the minimum fractional increase in the goodness-of-fit
       statistic required to accept an additional change-point. Default:
       ``0.25`` (25% increase required). Lower values detect more
       change-points; higher values detect fewer.
   * - ``--quiet``
     - ``-q``
     - Suppress informational output.
   * - ``--verbose``
     - ``-v``
     - Print debug information.


.. _presets:

Presets
-------

The ``--preset`` flag sets a bundle of statistical requirements at once.
Individual flags (``--ac``, ``--ci-perc``, ``--min-sample-size``) override
the preset's defaults.

.. list-table::
   :header-rows: 1
   :widths: 12 18 18 20 32

   * - Preset
     - Autocorrelation limit
     - CI width (% of mean)
     - Min. sample size
     - Round duration threshold
   * - ``quick``
     - ±0.8
     - 20%
     - 30
     - 3 s
   * - ``normal``
     - ±0.2
     - 10%
     - 50
     - 10 s
   * - ``strict``
     - ±0.1
     - 10%
     - 200
     - 20 s

Default: ``quick``.

**Guidance:**

- ``quick`` is suitable for rapid iteration and sanity checks. Its
  autocorrelation limit of ±0.8 is lenient; results may understate variance
  on correlated workloads.
- ``normal`` is appropriate for most general benchmarking. It matches common
  expectations for reproducible results.
- ``strict`` follows Ferrari's recommendation [ferrari:78]_ for negligible
  autocorrelation and meets the sample-size requirements for the central
  limit theorem to hold reliably [chen:hpca12]_. Use this for
  publication-quality results.

The round duration threshold applies only when ``--work-amount`` is set. It
is the minimum round duration Pilot will accept; rounds shorter than this
are excluded from analysis and the work amount is increased.

Confidence Level vs. CI Width
------------------------------

These two flags control related but distinct aspects of the stopping criterion:

**Confidence level** (``--confidence-level`` in ``run_program``, ``--cl`` in
``analyze``) sets the *probability* that the true mean lies within the
reported interval. A 95% CI means: if you ran the benchmark many times, 95%
of the computed intervals would contain the true mean.

**CI width** (``--ci-perc`` or ``--ci``) sets the *required width* of that
interval. The session continues until the CI is narrower than this threshold.

Together they determine how much data Pilot collects:

.. list-table::
   :header-rows: 1
   :widths: 22 22 56

   * - Confidence level
     - CI width
     - Interpretation
   * - 0.95 (default)
     - 10% of mean
     - 95% sure the true mean is within ±5% of the measured mean
   * - 0.99
     - 10% of mean
     - 99% sure — requires more rounds than the above
   * - 0.95
     - 5% of mean
     - Tighter bound — also requires more rounds

The relationship between sample size :math:`n`, CI width :math:`C`, and
confidence level follows from the t-distribution:

.. math::

   C = 2 \cdot t^*_{\alpha/2,\,n-1} \cdot \frac{s}{\sqrt{n}}

where :math:`s` is the sample standard deviation and :math:`t^*` is the
critical value at the chosen confidence level. Halving :math:`C` requires
roughly four times as many samples.

.. [ferrari:78] Domenico Ferrari. *Computer Systems Performance
                Evaluation*. Prentice-Hall, 1978.

.. [chen:hpca12] Tianshi Chen et al. Statistical performance comparisons
                 of computers. *HPCA-18*, IEEE, 2012.
