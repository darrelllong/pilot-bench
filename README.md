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

Pilot is written in C++ for fast in-place analysis and is released under a
dual BSD 3-clause and GPLv2+ license.

This repository is a modernized fork of the original
[ASCAR Pilot](https://github.com/ascar-io/pilot-bench) by Yan Li et al.,
with CMake 4.x / Boost 1.74+ compatibility, an optional headless build mode,
and a `--confidence-level` flag in `run_program`. See
[What is Pilot?](doc/what-is-pilot.rst) for the full change list.

## Documentation

All documentation is in the [`doc/`](doc/) directory as reStructuredText,
readable directly on GitHub or buildable into HTML with Sphinx.

### Getting Started

| Document | Description |
|---|---|
| [What is Pilot?](doc/what-is-pilot.rst) | Overview, motivation, and changes in this fork |
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
| [Warm-up and Cool-Down Phase Detection](doc/features/warm-up-and-cool-down-phase-detection.rst) | EDM change-point detection and the WPS linear regression model |
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

If you use Pilot in your research, please cite the original paper:

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
