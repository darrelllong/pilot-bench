/*
 * changepoint_rates.cc: how often the changepoint detection is right
 *
 * The test that decides if a changepoint is kept assumes more than is true
 * of readings, so its significance level does not say how often a
 * changepoint is reported where there is none. This program measures it, and
 * it measures how often a change that is there is found. The numbers in
 * doc/features/warm-up-and-cool-down-phase-detection.rst are from it.
 *
 * It makes its readings with pilot_random.hpp, so it prints the same numbers
 * everywhere.
 *
 * usage: changepoint_rates [series [sessions [significance_level]]]
 *
 * Copyright (c) 2026, Darrell Long. All rights reserved.
 * The Pilot tool and library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License version 2.1 (not any other version) as published by the Free
 * Software Foundation.
 *
 * The Pilot tool and library is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program in file lgpl-2.1.txt; if not, see
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "pilot/libpilot.h"
#include "pilot/pilot_random.hpp"

using namespace pilot;
using namespace std;

static vector<double> autoregressive(pcg64_t &rng, size_t n, double rho) {
    // the first readings are for the process to forget where it began
    const size_t burn_in = 500;
    const double scale = sqrt(1 - rho * rho);
    double x = rng.normal();
    vector<double> out;
    out.reserve(n);
    for (size_t i = 0; i < n + burn_in; ++i) {
        x = rho * x + scale * rng.normal();
        if (i >= burn_in)
            out.push_back(x);
    }
    return out;
}

static vector<double> make_series(pcg64_t &rng, const string &kind, size_t n) {
    if ("ar 0.5" == kind) return autoregressive(rng, n, 0.5);
    if ("ar 0.8" == kind) return autoregressive(rng, n, 0.8);
    if ("ar 0.9" == kind) return autoregressive(rng, n, 0.9);
    vector<double> x(n);
    for (size_t i = 0; i < n; ++i) {
        if ("normal" == kind) x[i] = rng.normal();
        else if ("exponential" == kind) x[i] = rng.exponential();
        else if ("lognormal" == kind) x[i] = exp(rng.normal());
        else if ("cauchy" == kind) x[i] = rng.normal() / rng.normal();
        else if ("0 or 1, 95% 1" == kind) x[i] = rng.unit_f64() < 0.95 ? 1 : 0;
        else if ("0 or 1, 50% 1" == kind) x[i] = rng.unit_f64() < 0.5 ? 1 : 0;
        else if ("five values" == kind) x[i] = static_cast<double>(rng.next_u64() % 5);
        else if ("periodic" == kind) x[i] = rng.normal() + sin(2 * M_PI * i / 40.0);
        else {
            fprintf(stderr, "unknown kind %s\n", kind.c_str());
            exit(2);
        }
    }
    return x;
}

static vector<int> detect(const double *x, size_t n) {
    int *cp = NULL;
    size_t m = 0;
    vector<int> r;
    if (0 == pilot_changepoint_detection(x, n, &cp, &m))
        r.assign(cp, cp + m);
    pilot_free(cp);
    return r;
}

//! the standard deviation of the distribution of the readings
static double sd_of(const string &kind) {
    // exp(N(0, 1)) has the variance (e - 1) e
    if ("lognormal" == kind) return sqrt((exp(1.0) - 1) * exp(1.0));
    // the others that we shift have the variance 1
    return 1;
}

int main(int argc, char **argv) {
    PILOT_LIB_SELF_CHECK;
    pilot_set_log_level(lv_no_show);
    const size_t series = argc > 1 ? atol(argv[1]) : 1000;
    const size_t sessions = argc > 2 ? atol(argv[2]) : 1000;
    if (argc > 3)
        pilot_set_changepoint_significance_level(atof(argv[3]));
    const double level = pilot_set_changepoint_significance_level(0.5);
    pilot_set_changepoint_significance_level(level);
    uint64_t seed = 20260927;

    printf("significance level %g, %zu series for every number\n\n", level, series);
    printf("A changepoint is reported in a series that has none (%%)\n");
    printf("%-16s %8s %8s %8s %8s\n", "readings", "n=100", "n=300", "n=1000", "n=3000");
    for (const char *kind : {"normal", "exponential", "lognormal", "cauchy", "five values",
                             "0 or 1, 50% 1", "0 or 1, 95% 1", "periodic",
                             "ar 0.5", "ar 0.8", "ar 0.9"}) {
        printf("%-16s", kind);
        for (size_t n : {100, 300, 1000, 3000}) {
            pcg64_t rng(++seed);
            size_t hit = 0;
            for (size_t s = 0; s < series; ++s) {
                vector<double> x = make_series(rng, kind, n);
                if (!detect(x.data(), n).empty()) ++hit;
            }
            printf(" %8.1f", 100.0 * hit / series);
            fflush(stdout);
        }
        printf("\n");
    }

    for (size_t config = 0; config < 2; ++config) {
        const size_t n = config ? 2000 : 500;
        const size_t length = config ? 200 : 50;
        printf("\nA warm-up of %zu of %zu readings that is lower by a number of standard\n"
               "deviations is found within 20 readings of where it is (%%)\n", length, n);
        printf("%-16s %8s %8s %8s %8s\n", "noise", "0.5", "1", "2", "4");
        for (const char *kind : {"normal", "exponential", "lognormal", "ar 0.5", "ar 0.8"}) {
            printf("%-16s", kind);
            for (double shift : {0.5, 1.0, 2.0, 4.0}) {
                pcg64_t rng(++seed);
                size_t hit = 0;
                const double scale = sd_of(kind);
                for (size_t s = 0; s < series; ++s) {
                    vector<double> x = make_series(rng, kind, n);
                    for (size_t i = 0; i < length; ++i) x[i] -= shift * scale;
                    for (int c : detect(x.data(), n)) {
                        if (labs(static_cast<long>(c) - static_cast<long>(length)) <= 20) {
                            ++hit;
                            break;
                        }
                    }
                }
                printf(" %8.1f", 100.0 * hit / series);
                fflush(stdout);
            }
            printf("\n");
        }
    }

    printf("\nReadings that are 0 or 1, of which the fraction that is 1 changes, and the\n"
           "change is found within 20 readings of where it is (%%)\n");
    struct {
        double before, after;
        size_t at, n;
    } rate_changes[] = {{0.50, 0.95, 300, 1000}, {0.80, 0.95, 100, 500},
                        {0.85, 0.95, 200, 1000}, {0.90, 0.99, 300, 1000}};
    for (auto &c : rate_changes) {
        pcg64_t rng(++seed);
        size_t hit = 0;
        for (size_t s = 0; s < series; ++s) {
            vector<double> x(c.n);
            for (size_t i = 0; i < c.n; ++i)
                x[i] = rng.unit_f64() < (i < c.at ? c.before : c.after) ? 1 : 0;
            for (int cp : detect(x.data(), c.n)) {
                if (labs(static_cast<long>(cp) - static_cast<long>(c.at)) <= 20) {
                    ++hit;
                    break;
                }
            }
        }
        printf("%.2f to %.2f after %zu of %zu readings %8.1f\n", c.before, c.after, c.at, c.n,
               100.0 * hit / series);
        fflush(stdout);
    }

    printf("\nA warm-up of 50 of 500 normal readings that are rounded to a multiple of a\n"
           "step, given in standard deviations, is found within 20 readings of where\n"
           "it is (%%)\n");
    printf("%-16s %8s %8s %8s\n", "step", "0.5", "1", "2");
    for (double step : {0.0, 0.001, 0.2, 0.33, 0.67, 1.0}) {
        if (0 == step)
            printf("%-16s", "not rounded");
        else
            printf("%-16g", step);
        for (double shift : {0.5, 1.0, 2.0}) {
            pcg64_t rng(++seed);
            size_t hit = 0;
            for (size_t s = 0; s < series; ++s) {
                vector<double> x(500);
                for (size_t i = 0; i < x.size(); ++i) {
                    x[i] = rng.normal() - (i < 50 ? shift : 0);
                    if (step > 0) x[i] = floor(x[i] / step + 0.5) * step;
                }
                for (int cp : detect(x.data(), x.size())) {
                    if (labs(static_cast<long>(cp) - 50) <= 20) {
                        ++hit;
                        break;
                    }
                }
            }
            printf(" %8.1f", 100.0 * hit / series);
            fflush(stdout);
        }
        printf("\n");
    }

    printf("\nSessions of readings that have no change, in which the detection is done\n"
           "when Pilot does it (%zu sessions for every number)\n", sessions);
    printf("%-16s %26s %26s %13s\n", "", "reported at any round (%)", "reported at the last (%)", "most that is");
    printf("%-16s %12s %13s %12s %13s %13s\n", "readings", "300 rounds", "1000 rounds", "300 rounds", "1000 rounds", "not used (%)");
    for (const char *kind : {"normal", "exponential", "lognormal", "five values", "0 or 1, 95% 1",
                             "ar 0.5", "ar 0.8", "ar 0.9"}) {
        pcg64_t rng(++seed);
        size_t any300 = 0, any1000 = 0, last300 = 0, last1000 = 0;
        size_t most_not_used = 0;
        for (size_t s = 0; s < sessions; ++s) {
            vector<double> x = make_series(rng, kind, 1000);
            bool any = false;
            bool now = false;
            size_t last_r = 0;
            for (size_t r = 2 * MIN_CHANGEPOINT_DETECTION_SAMPLE_SIZE; r <= 1000; ++r) {
                if (pilot_changepoint_detection_is_due(r, last_r)) {
                    size_t loc = 0;
                    now = (0 == pilot_find_one_changepoint(x.data(), r, &loc));
                    last_r = r;
                    // the part of the readings that is not used, in 1/1000
                    if (now) most_not_used = max(most_not_used, 1000 * loc / r);
                }
                if (now) any = true;
                if (300 == r) {
                    if (any) ++any300;
                    if (now) ++last300;
                }
                if (1000 == r) {
                    if (any) ++any1000;
                    if (now) ++last1000;
                }
            }
        }
        printf("%-16s %12.1f %13.1f %12.1f %13.1f %13.1f\n", kind,
               100.0 * any300 / sessions, 100.0 * any1000 / sessions,
               100.0 * last300 / sessions, 100.0 * last1000 / sessions,
               most_not_used / 10.0);
        fflush(stdout);
    }
    return 0;
}
