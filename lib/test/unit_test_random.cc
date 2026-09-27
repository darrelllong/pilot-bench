/*
 * unit_test_random.cc
 * Unit tests for pilot_random.hpp
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
#include "gtest/gtest.h"
#include "pilot/pilot_random.hpp"
#include <vector>

using namespace pilot;
using namespace std;

namespace {

double normal_cdf(double x) {
    return 0.5 * erfc(-x / sqrt(2.0));
}

/**
 * The p-value of the Kolmogorov-Smirnov test of values that are uniform in
 * [0, 1] if the hypothesis is true, by the series of Kolmogorov with the
 * correction of Stephens for the sample size
 */
double ks_test(vector<double> u) {
    sort(u.begin(), u.end());
    const double n = static_cast<double>(u.size());
    double d = 0;
    for (size_t i = 0; i < u.size(); ++i) {
        d = max(d, (i + 1) / n - u[i]);
        d = max(d, u[i] - i / n);
    }
    const double lambda = (sqrt(n) + 0.12 + 0.11 / sqrt(n)) * d;
    double p = 0;
    for (int j = 1; j <= 100; ++j)
        p += 2 * ((j % 2) ? 1 : -1) * exp(-2.0 * j * j * lambda * lambda);
    return min(1.0, max(0.0, p));
}

} // namespace

TEST(RandomUnitTest, SplitMix64) {
    // the first outputs of the state 0, which are in many references
    uint64_t state = 0;
    ASSERT_EQ(0xe220a8397b1dcdafULL, splitmix64(&state));
    ASSERT_EQ(0x6e789e6aa1b965f4ULL, splitmix64(&state));
    ASSERT_EQ(0x06c45d188009454fULL, splitmix64(&state));
}

TEST(RandomUnitTest, TheWordsArePinned) {
    // A measurement can be repeated only if the generator gives the same
    // numbers. These have to stay as they are.
    pcg64_t a(static_cast<uint64_t>(0));
    ASSERT_EQ(1180971669125416571ULL, a.next_u64());
    ASSERT_EQ(4263147928687175688ULL, a.next_u64());
    ASSERT_EQ(14766038972118115940ULL, a.next_u64());
    ASSERT_EQ(455486646239554270ULL, a.next_u64());

    pcg64_t b(static_cast<uint64_t>(12345));
    ASSERT_EQ(11026926375082549866ULL, b.next_u64());
    ASSERT_EQ(8108686043434933465ULL, b.next_u64());
    ASSERT_EQ(6078212038498920979ULL, b.next_u64());
    ASSERT_EQ(17523412065927701865ULL, b.next_u64());

    pcg64_t c(static_cast<pcg64_t::u128>(7), static_cast<pcg64_t::u128>(11));
    ASSERT_EQ(6024511257967916890ULL, c.next_u64());
    ASSERT_EQ(9405712596061573158ULL, c.next_u64());

    // next_u32 is the high half of a word
    pcg64_t d(static_cast<uint64_t>(12345));
    ASSERT_EQ(static_cast<uint32_t>(11026926375082549866ULL >> 32), d.next_u32());
}

TEST(RandomUnitTest, TheUniformsArePinned) {
    pcg64_t a(static_cast<uint64_t>(12345));
    ASSERT_EQ(0.59777087658511041, a.unit_f64());
    ASSERT_EQ(0.43957275121475214, a.unit_f64());
    pcg64_t b(static_cast<uint64_t>(12345));
    ASSERT_EQ(0.71978637560737602, b.unit_f64_dense());
    ASSERT_EQ(0.48748651789046893, b.unit_f64_dense());
}

TEST(RandomUnitTest, Uniforms) {
    const size_t draws = 200000;
    pcg64_t rng(static_cast<uint64_t>(3));
    vector<double> unit, dense;
    for (size_t i = 0; i < draws; ++i) {
        unit.push_back(rng.unit_f64());
        dense.push_back(rng.unit_f64_dense());
        ASSERT_GE(unit.back(), 0);
        ASSERT_LT(unit.back(), 1);
        ASSERT_GE(dense.back(), 0);
        ASSERT_LT(dense.back(), 1);
    }
    ASSERT_GT(ks_test(unit), 0.001);
    ASSERT_GT(ks_test(dense), 0.001);
}

TEST(RandomUnitTest, TheTableHasEqualAreas) {
    const ziggurat_normal_t &z = ziggurat_normal_t::instance();
    const size_t layers = ziggurat_normal_t::LAYERS;
    // G. Marsaglia and W. W. Tsang have r = 3.6541528853610088 for 256 pieces
    ASSERT_NEAR(3.6541528853610088, z.x(0), 1e-13);
    ASSERT_NEAR(0.00492867323399, z.area(), 1e-13);
    ASSERT_EQ(0, z.x(layers - 1));
    ASSERT_EQ(1, z.f(layers - 1));
    for (size_t i = 1; i < layers; ++i)
        ASSERT_LT(z.x(i), z.x(i - 1)) << "x[" << i << "]";
    const double tolerance = 1e-10 * z.area();
    const double base = z.x(0) * z.f(0) + ziggurat_normal_t::tail_area(z.x(0));
    ASSERT_NEAR(z.area(), base, tolerance);
    for (size_t i = 1; i < layers; ++i) {
        const double piece = z.x(i - 1) * (z.f(i) - z.f(i - 1));
        ASSERT_NEAR(z.area(), piece, tolerance) << "piece " << i;
    }
    // The pieces cover the area under the density, which is sqrt(pi / 2),
    // and what of the rectangles is above it
    ASSERT_GT(layers * z.area(), sqrt(M_PI / 2));
    ASSERT_LT(layers * z.area(), 1.01 * sqrt(M_PI / 2));
}

TEST(RandomUnitTest, NormalVariates) {
    const size_t draws = 200000;
    const double sigmas = 5;
    pcg64_t rng(static_cast<pcg64_t::u128>(7), static_cast<pcg64_t::u128>(11));
    vector<double> values, uniforms;
    for (size_t i = 0; i < draws; ++i) {
        values.push_back(rng.normal());
        ASSERT_TRUE(std::isfinite(values.back()));
        uniforms.push_back(normal_cdf(values.back()));
    }
    ASSERT_GT(ks_test(uniforms), 0.001);

    const double n = static_cast<double>(draws);
    double mean = 0, variance = 0;
    for (double v : values) mean += v;
    mean /= n;
    for (double v : values) variance += (v - mean) * (v - mean);
    variance /= n;
    ASSERT_LT(abs(mean), sigmas / sqrt(n));
    ASSERT_LT(abs(variance - 1), sigmas * sqrt(2 / n));
    for (double threshold : {1.0, 2.0, 3.0, 4.0}) {
        size_t beyond = 0;
        for (double v : values)
            if (abs(v) > threshold) ++beyond;
        const double seen = beyond / n;
        const double want = 2 * normal_cdf(-threshold);
        const double se = sqrt(want * (1 - want) / n);
        ASSERT_LT(abs(seen - want), sigmas * se) << "|z| > " << threshold;
    }
}

TEST(RandomUnitTest, TheTailOfTheNormal) {
    // The variates beyond r are from another branch of the code. Given that
    // |Z| > r, P(|Z| > x) = erfc(x / sqrt(2)) / erfc(r / sqrt(2)).
    const size_t draws = 2000000;
    const double r = ziggurat_normal_t::instance().x(0);
    pcg64_t rng(static_cast<uint64_t>(1234));
    vector<double> uniforms;
    for (size_t i = 0; i < draws; ++i) {
        const double v = abs(rng.normal());
        if (v > r)
            uniforms.push_back(1 - erfc(v / sqrt(2.0)) / erfc(r / sqrt(2.0)));
    }
    // P(|Z| > r) is 2.6e-4
    ASSERT_GT(uniforms.size(), 300);
    ASSERT_GT(ks_test(uniforms), 0.001);
}

TEST(RandomUnitTest, ExponentialVariates) {
    const size_t draws = 200000;
    pcg64_t rng(static_cast<pcg64_t::u128>(13), static_cast<pcg64_t::u128>(17));
    vector<double> uniforms;
    double mean = 0;
    for (size_t i = 0; i < draws; ++i) {
        const double v = rng.exponential();
        ASSERT_GE(v, 0);
        mean += v;
        uniforms.push_back(-expm1(-v));
    }
    mean /= draws;
    ASSERT_GT(ks_test(uniforms), 0.001);
    ASSERT_LT(abs(mean - 1), 5 / sqrt(static_cast<double>(draws)));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
