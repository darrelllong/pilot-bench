# Pilot Benchmark Framework

<img src="assets/pilot.jpg" width="50%" />

The Pilot Benchmark Framework provides a tool (`bench`) and a library
(`libpilot`) to automate computer performance measurement. It answers
questions like:

* How long should I run this benchmark to get a precise result?
* Is this new scheduler really 3% faster, or is that measurement noise?
* Which is faster for my database: 20 worker threads or 25?

Design goals:

* Be as intelligent as possible — users should not need a statistics background.
* Results must be statistically valid: accurate, precise, and repeatable.
* Reach a valid result in the shortest possible time.

Pilot is written in C++ for fast in-place analysis and is released under the
GNU Lesser General Public License version 2.1. Commit 0332289, of January
2017, and the commits before it were released under a BSD 3-clause license.
See [LICENSE](LICENSE).

Pilot has been revised since the paper of 2016 that describes it. Defects of
its statistical methods have been fixed, and some of what it does now is not
what the paper describes. See [Revisions Since 2016](#revisions-since-2016).

## Revisions Since 2016

Pilot was written by Yan Li at the Storage Systems Research Center of UC
Santa Cruz in 2015 and 2016, and is described in the paper of 2016 that is
cited below. He maintained it as
[ASCAR Pilot](https://github.com/ascar-io/pilot-bench) until 2019. This
repository continues it. The revisions are by Darrell Long, from 2026.

Pilot is stand-alone. It needs a C++ compiler, CMake, and Boost. The build
downloads googletest for the tests. The text interface, the Lua prompt, and
the Python binding are optional and need their own libraries.

### What Has Been Revised

| Subject | Revision |
|---|---|
| Build | Builds with CMake 4 and Boost 1.74 and later, with Make or Ninja. The text interface is optional (`WITH_TUI`). A parallel build from scratch works. |
| `bench` | `--confidence-level`. `--duration-col` is read; it was parsed and then not used. A session that has no `--pi` has to finish its WPS analysis. Columns may be separated by a comma and a space. |
| Documentation | Rewritten, with a command-line reference and a changelog. |

### What Has Been Fixed

These are defects of the statistical methods or of the measurement. They
were in Pilot before the revisions began.

| Defect | Consequence | Fix |
|---|---|---|
| Change-point detection accepted a first change-point whatever the readings were. | A change-point was reported in every one of 200 samples of 60 or more independent readings that had none. The readings before a change-point are not used, so a session that needed more than about 60 readings worked with the last 30 to 60 only, and a session of the `strict` preset could not finish. | The detection has been replaced. In one analysis of independent readings that have no change it reports a change-point in fewer than 1% of samples, and it finds a warm-up that is lower by one standard deviation in 96% of them. A session does the analysis many times, and readings that are autocorrelated are harder; [the description of the detection](doc/features/warm-up-and-cool-down-phase-detection.rst) has the numbers for both. |
| The clock for the round duration, and for each call of the function of `simple_runner`, moved in steps of 10 ms (with Boost 1.92 on macOS). | A call that took less than 10 ms was measured as 0 or as 10 ms. | `std::chrono::steady_clock`. |
| The WPS regression used the sums of the rounds of a subsession, not their means. | With a subsession size `q` greater than 1, the intercept was `q` times what it is and the CI of the rate was `q` times too wide. | The means. |
| The threshold that leaves rounds out of the WPS regression was set from a number of seconds and used as a number of nanoseconds. | Rounds that were too short were in the regression. | The units, and the threshold no longer falls to zero. |
| The WPS regression returned success when all the rounds had the same work amount. | Results that were not numbers were taken for results. | It is reported as not enough data. |
| The CI of the rate was the difference of the inverted ends of the CI of the slope, also when the CI of the slope contained zero. | A rate that has no upper bound was reported with a CI, which could satisfy the required width. | The rate is reported without a CI. |
| The sample size for a comparison was a negative number converted to an unsigned integer when no sample size was large enough. | On some machines a session stopped at the least sample size as if the comparison had been decided. | It is reported as not enough data. |
| The comparison functions ended the program when neither sample had any variance. | | Fixed. |
| `pilot_set_pi_info()` did not store the mean method of a PI. | Every PI declared as a ratio (type 1: throughput, speed), from `bench` and from the library, was averaged by its arithmetic mean, and its CI was computed as for an ordinary value. The arithmetic mean of rates is greater than their harmonic mean unless they are all equal, so a throughput was overstated. | It is stored. |
| The variance, autocorrelation and CI of a harmonic mean were those of the subsession harmonic means about the harmonic mean, and the CI was symmetric about it. | `bench analyze -m 1` reported them. The readings of a session did not use them, because of the defect above; the WPS analysis did, to choose its subsession size. | A ratio is analysed through its reciprocals, and its CI is not symmetric about the mean. A reading of a ratio must be positive. |
| The required sample size was calculated for 95% confidence whatever the confidence level of the session. | With `--confidence-level` above 0.95 a session could end with a CI wider than it required; below 0.95 it took more readings than it needed. | The session's confidence level is used. |
| The CI of the unit readings was at 95% confidence whatever the confidence level. | | The session's confidence level is used. |
| A required sample size that is not finite, when no number of readings can make the CI as narrow as required (for example a mean of exactly 0 and a width that is a percentage of it), was converted to an unsigned integer, which is undefined. | On x86-64 it gave 0, and the PI was taken as satisfied at the least sample size. | It is reported as not enough data. |

[The changelog](doc/changelog.rst) has all of them, and what to look out for
when upgrading.

### Where Pilot Differs from the Paper

The paper describes Pilot as it was in 2016. Pilot now differs from it in
these ways.

| Subject | The paper | Pilot now |
|---|---|---|
| Change-point detection | E-Divisive with Medians. | Binary segmentation proposes change-points, and the rank-sum test on subsession samples, or Fisher's exact test if many of them are the same, decides which to keep. |
| Rounds left out of the WPS regression | Rounds whose work amount is smaller than the intercept. | Rounds that are shorter than the short round detection threshold, and rounds that are shorter than the intercept when it is negative. |
| Comparing results | Ranks several workloads by running their rounds in turn. | Compares one workload against a baseline. |
| Threshold of the *p*-value of a comparison | Usually 0.01. | 0.05. |

### How the Revisions Are Checked

A defect of a statistical method cannot be found by reading its results,
which look like results. It is found by giving the method readings of which
the answer is known. `lib/test/changepoint_rates.cc` does this for the
change-point detection, and
[the description of the detection](doc/features/warm-up-and-cool-down-phase-detection.rst)
has its numbers. The readings are from `pilot_random.hpp`, a random number
generator that gives the same numbers with every compiler, so the numbers
can be reproduced.

## Documentation

All documentation is in the [`doc/`](doc/) directory as reStructuredText,
readable directly on GitHub or buildable into HTML with Sphinx.

### Getting Started

| Document | Description |
|---|---|
| [What is Pilot?](doc/what-is-pilot.rst) | Overview, motivation, and changes in this fork |
| [Changelog](doc/changelog.rst) | What has changed, where Pilot differs from the paper, and what to look out for when upgrading |
| [Tutorial: Benchmarking C++ Functions](doc/tutorials/Measuring-the-duration-of-running-CPP-functions.rst) | Use `libpilot` to measure function duration with full statistical rigor |
| [Tutorial: Command-Line Benchmarking](doc/tutorials/Using-Pilot-to-run-a-command-line-benchmark-job.rst) | Use `bench` to drive any CLI program, with a full `dd` example |

### Installation and Building

| Document | Description |
|---|---|
| [Build from Source](doc/build.rst) | CMake options, build modes, Python binding |
| [Install on Linux](doc/install-linux.rst) | Linux build instructions |
| [Install on macOS](doc/install-mac.rst) | macOS build instructions |

### Concepts and Statistical Background

| Document | Description |
|---|---|
| [Statistics 101](doc/features/statistics-101.rst) | Accuracy, precision, repeatability, and confidence intervals explained |
| [Terms and Definitions](doc/features/terms.rst) | PI, session, round, work amount, unit reading, WPS |
| [Autocorrelation Detection and Mitigation](doc/features/autocorrelation-detection-and-mitigation.rst) | Why autocorrelation matters and how subsession analysis fixes it |
| [Deciding Optimal Session Length](doc/features/deciding-optimal-session-length.rst) | How Pilot decides when to stop collecting data |
| [Warm-up and Cool-Down Phase Detection](doc/features/warm-up-and-cool-down-phase-detection.rst) | Change-point detection and the WPS linear regression model |
| [Comparing Results](doc/features/comparing-results.rst) | Welch's t-test and the algorithm for ranking benchmark results |

### Command-Line Reference

| Document | Description |
|---|---|
| [Command-Line Reference](doc/reference.rst) | All flags and options for `bench run_program`, `bench analyze`, and `bench detect_changepoint_edm`; preset table; confidence level vs. CI width explained |

## Build from Source

Pilot supports two build modes:

* `WITH_TUI=OFF` (default): headless CLI, no curses/CDK dependency.
* `WITH_TUI=ON`: adds the optional curses/CDK text UI.

**Headless build (recommended for CI, containers, scripted use):**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DWITH_TUI=OFF
cmake --build build -j
```

The resulting binary is `build/cli/bench`.

**With text UI:**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DWITH_TUI=ON
cmake --build build -j
```

Requires curses development headers and libraries.

## Quick Start

```bash
# Benchmark a C++ function — link libpilot and call simple_runner()
g++ -std=c++14 -O2 -lpilot -o my_bench my_bench.cc
./my_bench

# Run a command-line benchmark (dd example, Option 1 — recommended)
bench run_program -d 0 --wps -w 1,5000 \
    -- ./run_dd.sh /tmp/io_test %WORK_AMOUNT%

# Analyze an existing CSV of measurements
bench analyze --preset normal data.csv
```

See the [tutorials](doc/tutorial-list.rst) for full worked examples.

## Development

We partially follow [Google's C++ Style Guide](https://google.github.io/styleguide/cppguide.html),
with the exception that we use four spaces for indentation rather than two.

## Acknowledgments

This is a research project from the [Storage Systems Research
Center](http://www.ssrc.ucsc.edu/) at [UC Santa Cruz](http://ucsc.edu).
Supported in part by the National Science Foundation under awards
IIP-1266400, CCF-1219163, CNS-1018928, CNS-1528179, by the Department of
Energy under award DE-FC02-10ER26017/DESC0005417, by a Symantec Graduate
Fellowship, by a grant from Intel Corporation, and by industrial members of
the [Center for Research in Storage Systems](http://www.crss.ucsc.edu/).
Any opinions, findings, and conclusions expressed in this material are those
of the author(s) and do not necessarily reflect the views of the sponsors.

## Citation

If you use Pilot in your research, please cite the original paper. Pilot has
been revised since, so say which commit you used.

```bibtex
@inproceedings{li:mascots16,
  author    = {Yan Li and Yash Gupta and Ethan L. Miller and Darrell D. E. Long},
  title     = {Pilot: A Framework that Understands How to Do Performance
               Benchmarks the Right Way},
  booktitle = {Proceedings of the IEEE 24th International Symposium on
               Modeling, Analysis, and Simulation of Computer and
               Telecommunication Systems (MASCOTS'16)},
  year      = {2016},
  publisher = {IEEE},
}
```
