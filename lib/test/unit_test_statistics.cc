/*
 * unit_test_statistics.cc: unit tests for statistics routines
 *
 * Copyright (c) 2017-2019 Yan Li <yanli@tuneup.ai>. All rights reserved.
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
 *
 * Commit 033228934e11b3f86fb0a4932b54b2aeea5c803c and before were
 * released with the following license:
 * Copyright (c) 2015, 2016, University of California, Santa Cruz, CA, USA.
 * Created by Yan Li <yanli@tuneup.ai>,
 * Department of Computer Science, Baskin School of Engineering.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Storage Systems Research Center, the
 *       University of California, nor the names of its contributors
 *       may be used to endorse or promote products derived from this
 *       software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OF THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <fstream>
#include "gtest/gtest.h"
#include "pilot/libpilot.h"
#include "pilot/pilot_random.hpp"
#include <vector>

using namespace pilot;
using namespace std;

nanosecond_type const ONE_SECOND = 1000000000LL;

/**
 * \details These sample response time are taken from [Ferrari78], page 79.
 */
const vector<double> g_response_time {
    1.21, 1.67, 1.71, 1.53, 2.03, 2.15, 1.88, 2.02, 1.75, 1.84, 1.61, 1.35, 1.43, 1.64, 1.52, 1.44, 1.17, 1.42, 1.64, 1.86, 1.68, 1.91, 1.73, 2.18,
    2.27, 1.93, 2.19, 2.04, 1.92, 1.97, 1.65, 1.71, 1.89, 1.70, 1.62, 1.48, 1.55, 1.39, 1.45, 1.67, 1.62, 1.77, 1.88, 1.82, 1.93, 2.09, 2.24, 2.16
};

/**
 * Sample data for testing binomial proportion confidence interval
 */
const vector<double> g_binary_sample {
    1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1
};

TEST(StatisticsUnitTest, CornerCases) {
    ASSERT_DEATH(pilot_subsession_auto_cov_p(g_response_time.data(), 1, 1, 0, ARITHMETIC_MEAN), "") << "Shouldn't be able to calculate covariance for one sample";
    // Shouldn't be able to calculate optimal subsession size for one sample
    ASSERT_EQ(-1, pilot_optimal_subsession_size_p(g_response_time.data(), 1, ARITHMETIC_MEAN));
}

TEST(StatisticsUnitTest, AutocorrelationCoefficient) {
    double sample_mean = pilot_subsession_mean_p(g_response_time.data(), g_response_time.size(), ARITHMETIC_MEAN);
    ASSERT_DOUBLE_EQ(1.756458333333333, sample_mean) << "Mean is wrong";

    ASSERT_DOUBLE_EQ(0.073474423758865273, pilot_subsession_var_p(g_response_time.data(), g_response_time.size(), 1, sample_mean, ARITHMETIC_MEAN)) << "Subsession mean is wrong";
    ASSERT_DOUBLE_EQ(0.046770566452423196, pilot_subsession_auto_cov_p(g_response_time.data(), g_response_time.size(), 1, sample_mean, ARITHMETIC_MEAN)) << "Coverance mean is wrong";
    ASSERT_DOUBLE_EQ(0.63655574361384437, pilot_subsession_autocorrelation_coefficient_p(g_response_time.data(), g_response_time.size(), 1, sample_mean, ARITHMETIC_MEAN)) << "Autocorrelation coefficient is wrong";

    ASSERT_DOUBLE_EQ(0.55892351761172487, pilot_subsession_autocorrelation_coefficient_p(g_response_time.data(), g_response_time.size(), 2, sample_mean, ARITHMETIC_MEAN)) << "Autocorrelation coefficient is wrong";

    ASSERT_DOUBLE_EQ(0.05264711174242424, pilot_subsession_var_p(g_response_time.data(), g_response_time.size(), 4, sample_mean, ARITHMETIC_MEAN)) << "Subsession var is wrong";
    ASSERT_DOUBLE_EQ(0.08230986644266707, pilot_subsession_autocorrelation_coefficient_p(g_response_time.data(), g_response_time.size(), 4, sample_mean, ARITHMETIC_MEAN)) << "Autocorrelation coefficient is wrong";

    ASSERT_DOUBLE_EQ(0.29157062128900485, pilot_subsession_confidence_interval_p(g_response_time.data(), g_response_time.size(), 4, .95, ARITHMETIC_MEAN));

    // testing binomial proportion confidence interval width
    ASSERT_DOUBLE_EQ(0.46566845477273205, pilot_subsession_confidence_interval_p(g_binary_sample.data(), g_binary_sample.size(), 1, .95, ARITHMETIC_MEAN, BINOMIAL_PROPORTION));

    size_t q = 4;
    ASSERT_DOUBLE_EQ(q, pilot_optimal_subsession_size_p(g_response_time.data(), g_response_time.size(), ARITHMETIC_MEAN));

    size_t opt_sample_size;
    ASSERT_EQ(true, pilot_optimal_sample_size_p(g_response_time.data(), g_response_time.size(), sample_mean * 0.1, ARITHMETIC_MEAN, &q, &opt_sample_size, SAMPLE_MEAN, .95, .1));
    ASSERT_EQ(4, q);
    ASSERT_EQ(34, opt_sample_size);
}

TEST(StatisticsUnitTest, HarmonicMean) {
    const vector<double> d {1.21, 1.67, 1.71, 1.53, 2.03, 2.15};
    double hm = pilot_subsession_mean_p(d.data(), d.size(), HARMONIC_MEAN);
    ASSERT_DOUBLE_EQ(1.6568334130160711, hm);
}

// A ratio is analysed through its reciprocals y = 1/x. The expected values
// were computed independently: ybar = mean(y), H = 1 / ybar, s_y^2 the sample
// variance of y; the variance is H^4 s_y^2; the interval is
// [1 / (ybar + d), 1 / (ybar - d)] with d = t(0.975, 5) s_y / sqrt(6), where
// t(0.975, 5) = 2.57058183564 was found by integrating the density; and the
// sample size for a width of 0.1 H is s_y^2 (t / d*)^2 with
// d* = (sqrt(1 + w^2 ybar^2) - 1) / w.
TEST(StatisticsUnitTest, HarmonicMeanInterval) {
    const vector<double> d {1.21, 1.67, 1.71, 1.53, 2.03, 2.15};
    double hm = pilot_subsession_mean_p(d.data(), d.size(), HARMONIC_MEAN);
    ASSERT_NEAR(0.12664694842376947, pilot_subsession_var_p(d.data(), d.size(), 1, hm, HARMONIC_MEAN), 1e-12);
    ASSERT_NEAR(0.78691891955, pilot_subsession_confidence_interval_p(d.data(), d.size(), 1, .95, HARMONIC_MEAN, SAMPLE_MEAN), 1e-8);
    size_t q, opt_sample_size;
    ASSERT_TRUE(pilot_optimal_sample_size_p(d.data(), d.size(), 0.1 * hm, HARMONIC_MEAN, &q, &opt_sample_size, SAMPLE_MEAN, .95, 1.0));
    ASSERT_EQ(1, q);
    ASSERT_EQ(123, opt_sample_size);

    // When the interval of the reciprocals reaches 0, the interval of the
    // harmonic mean has no upper end.
    const vector<double> wide {0.01, 100, 0.02, 50};
    ASSERT_TRUE(std::isinf(pilot_subsession_confidence_interval_p(wide.data(), wide.size(), 1, .95, HARMONIC_MEAN, SAMPLE_MEAN)));
}

// There are more WPS linear regression test cases in unit_test_readings_warmup_removal.cc

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegression1) {
    const double exp_alpha = 42;
    const double exp_v = 0.5;       // 0.5 work amount per second
    const vector<size_t> work_amount{50, 100, 150, 200, 250};
    const vector<double> error{20, -9, -18, -25, 30};
    vector<nanosecond_type> t;
    auto p_error = error.begin();
    double exp_ssr = 0;
    for_each(error.begin(), error.end(), [&exp_ssr](double e) {exp_ssr += e*e;});
    for (size_t c : work_amount) {
        t.push_back(ONE_SECOND * ((1.0 / exp_v) * c + exp_alpha + *(p_error++)));
    }
    double alpha = 0.0, v = 0.0, v_ci = 0.0, ssr = 0.0;
    pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), t.data(),
        1,  // autocorrelation_coefficient_limit
        0,  // duration threshold
        &alpha, &v, &v_ci, &ssr);
    ASSERT_NEAR(exp_ssr, ssr, 10);
    ASSERT_NEAR(44, alpha, 4);
    ASSERT_NEAR(exp_v, v, 0.1);
    ASSERT_NEAR(0.3016, v_ci, 0.01);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegression2) {
    // We generate some mock test data
    const size_t v_wu = 500;
    const size_t v_s  = 30;
    const size_t v_td = 15;
    const nanosecond_type t_su = 10;
    const nanosecond_type t_wu = 100;
    const nanosecond_type t_td = 30;
    vector<size_t> work_amount;
    vector<nanosecond_type> round_duration;
    ofstream of("output.csv");
    of << "work amount,round_duration" << endl;
    for(nanosecond_type t_s = 0; t_s < 2000; t_s += 50) {
        work_amount.push_back(v_wu * t_wu + v_s * t_s + v_td * t_td);
        round_duration.push_back(t_su + t_wu + t_td + t_s);
        of << work_amount.back() << "," << round_duration.back() << endl;
    }
    of.close();

    double alpha = 0.0, v = 0.0, v_ci = 0.0, ssr = 0.0;
    pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), round_duration.data(),
        1,  // autocorrelation_coefficient_limit
        0,  // duration threshold
        &alpha, &v, &v_ci, &ssr);
    ASSERT_NEAR(0, ssr, .001);
    ASSERT_NEAR(-1541.7 / ONE_SECOND, alpha, .1 / ONE_SECOND);
    ASSERT_NEAR(v_s * ONE_SECOND, v, .1);
    ASSERT_NEAR(0, v_ci, .001);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegression3) {
    // Real data test A. Four rounds are not enough for a CI: the 95% CI of
    // the slope is [-7.63e-09, 2.11e-08] s per work unit, which contains 0,
    // so v has a lower bound (47338412) but no upper bound.
    const vector<size_t> work_amount{429497000, 472446000, 515396000, 558346000};
    const vector<nanosecond_type> round_duration{4681140000, 5526190000, 5632120000, 5611980000};
    double alpha = 0.0, v = 0.0, v_ci = 0.0, ssr = 0.0;
    ASSERT_EQ(ERR_NOT_ENOUGH_DATA_FOR_CI,
              pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), round_duration.data(),
        1,  // autocorrelation_coefficient_limit
        0,  // duration threshold
        &alpha, &v, &v_ci, &ssr));
    ASSERT_NEAR(0.2059332, ssr, 0.001);
    ASSERT_NEAR(2.0296, alpha, .0001);
    ASSERT_NEAR(ONE_SECOND / 6.7485, v, 10000);
    ASSERT_TRUE(std::isinf(v_ci));
    ASSERT_GT(v_ci, 0);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegression4) {
    // Real data test B
    const vector<size_t> work_amount{429496729, 472446392, 515396064, 558345736, 601295408, 644245080, 687194752, 730144424, 773094096};
    const vector<nanosecond_type> round_duration{5731883327, 5235129386, 5321265550, 5860121124, 6040418744, 6513983890, 6623204911, 6828709974, 7455453108};
    double alpha = 0.0, v = 0.0, v_ci = 0.0, ssr = 0.0;
    pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), round_duration.data(),
        1,  // autocorrelation_coefficient_limit
        0,  // duration threshold
        &alpha, &v, &v_ci, &ssr);
    ASSERT_NEAR(0.59193307, ssr, 0.000001);
    ASSERT_NEAR(2694596476 / ONE_SECOND, alpha, 1);
    ASSERT_NEAR(172572240, v, 1);
    ASSERT_NEAR(141053262, v_ci, 1);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegressionSubsession1) {
    // Exact data that needs subsession size 2: every work amount is used
    // for two rounds in a row, so the naive v of adjacent rounds is
    // correlated. alpha and v must not depend on the subsession size.
    const double exp_alpha = 4;
    const double exp_v = 1.5;
    const vector<size_t> order{240, 180, 30, 90, 300, 60, 210, 360, 150, 270, 330, 120};
    vector<size_t> work_amount;
    vector<nanosecond_type> round_duration;
    for (size_t c : order) {
        for (int i = 0; i < 2; ++i) {
            work_amount.push_back(c);
            round_duration.push_back(ONE_SECOND * (exp_alpha + c / exp_v));
        }
    }
    double alpha = 0.0, v = 0.0, v_ci = 0.0, ssr = 0.0;
    size_t subsession_sample_size = 0;
    ASSERT_EQ(0, pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), round_duration.data(),
        0.1,  // autocorrelation_coefficient_limit
        0,    // duration threshold
        &alpha, &v, &v_ci, &ssr, NULL, &subsession_sample_size));
    ASSERT_EQ(work_amount.size() / 2, subsession_sample_size);
    ASSERT_NEAR(exp_alpha, alpha, 1e-6);
    ASSERT_NEAR(exp_v, v, 1e-6);
    ASSERT_NEAR(0, ssr, 1e-6);
    ASSERT_NEAR(0, v_ci, 1e-6);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegressionSubsession2) {
    // Noisy data that needs subsession size 2. The expected values are from
    // an ordinary least squares fit of the 12 subsession means.
    const vector<size_t> work_amount{60, 60, 150, 150, 120, 120, 30, 30, 210, 210, 240, 240,
                                     330, 330, 360, 360, 180, 180, 300, 300, 90, 90, 270, 270};
    const vector<nanosecond_type> round_duration{
        44544341237, 44580062425, 104189214938, 104543662366, 84539519333, 84890513031,
        24014113052, 23718539793, 143398394018, 143899644382, 163377596893, 162786447008,
        225626630123, 225570794444, 242575072201, 242463405771, 122569901265, 123172137059,
        204012938371, 204002214273, 65619180496, 65287643756, 184217413961, 183852193096};
    double alpha = 0.0, v = 0.0, v_ci = 0.0, ssr = 0.0;
    size_t subsession_sample_size = 0;
    ASSERT_EQ(0, pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), round_duration.data(),
        0.1,  // autocorrelation_coefficient_limit
        0,    // duration threshold
        &alpha, &v, &v_ci, &ssr, NULL, &subsession_sample_size));
    ASSERT_EQ(12, subsession_sample_size);
    ASSERT_NEAR(4.5624488, alpha, 1e-6);
    ASSERT_NEAR(1.5058144, v, 1e-6);
    ASSERT_NEAR(19.1507823, ssr, 1e-6);
    ASSERT_NEAR(0.0269967, v_ci, 1e-6);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegressionSubsession3) {
    // The data of Subsession2 plus one more round. 25 rounds can't be evenly
    // divided into subsessions of size 2, so the last round is left over. It
    // must not change alpha, v, or the CI. Only ssr, which is calculated from
    // all rounds, changes.
    const vector<size_t> work_amount{60, 60, 150, 150, 120, 120, 30, 30, 210, 210, 240, 240,
                                     330, 330, 360, 360, 180, 180, 300, 300, 90, 90, 270, 270,
                                     345};
    const vector<nanosecond_type> round_duration{
        44544341237, 44580062425, 104189214938, 104543662366, 84539519333, 84890513031,
        24014113052, 23718539793, 143398394018, 143899644382, 163377596893, 162786447008,
        225626630123, 225570794444, 242575072201, 242463405771, 122569901265, 123172137059,
        204012938371, 204002214273, 65619180496, 65287643756, 184217413961, 183852193096,
        235912345678};
    double alpha = 0.0, v = 0.0, v_ci = 0.0, ssr = 0.0;
    size_t subsession_sample_size = 0;
    ASSERT_EQ(0, pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), round_duration.data(),
        0.1,  // autocorrelation_coefficient_limit
        0,    // duration threshold
        &alpha, &v, &v_ci, &ssr, NULL, &subsession_sample_size));
    ASSERT_EQ(12, subsession_sample_size);
    ASSERT_NEAR(4.5624488, alpha, 1e-6);
    ASSERT_NEAR(1.5058144, v, 1e-6);
    ASSERT_NEAR(24.1593889, ssr, 1e-6);
    ASSERT_NEAR(0.0269967, v_ci, 1e-6);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegressionErrorOfFilteredRounds) {
    // Five rounds of duration = 42 + 2 * work_amount, and a round of 1 s
    // that the duration threshold filters out. The error is from the rounds
    // that the regression uses, so it is 0. With the short round it would
    // be (42 + 2 * 10 - 1)^2 = 3721.
    const vector<size_t> work_amount{50, 100, 10, 150, 200, 250};
    const vector<nanosecond_type> round_duration{142 * ONE_SECOND, 242 * ONE_SECOND, 1 * ONE_SECOND,
                                                 342 * ONE_SECOND, 442 * ONE_SECOND, 542 * ONE_SECOND};
    double alpha = 0.0, v = 0.0, v_ci = 0.0, ssr = 42, ssr_percent = 42;
    ASSERT_EQ(0, pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), round_duration.data(),
        1,               // autocorrelation_coefficient_limit
        2 * ONE_SECOND,  // duration threshold
        &alpha, &v, &v_ci, &ssr, &ssr_percent));
    ASSERT_NEAR(42, alpha, 1e-6);
    ASSERT_NEAR(0.5, v, 1e-9);
    ASSERT_NEAR(0, ssr, 1e-9);
    ASSERT_NEAR(0, ssr_percent, 1e-9);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegressionLongSubsession) {
    // 360 rounds of almost ten years each, duration = 4 + work_amount / 1.5.
    // Each work amount is used for 30 rounds in a row, which needs a
    // subsession size of 31. The sum of the durations of a subsession,
    // 9.7e18 ns, doesn't fit in nanosecond_type.
    const vector<size_t> order{240, 180, 30, 90, 300, 60, 210, 360, 150, 270, 330, 120};
    vector<size_t> work_amount;
    vector<nanosecond_type> round_duration;
    for (size_t c : order) {
        for (int i = 0; i < 30; ++i) {
            work_amount.push_back(468000000 + c * 1000);
            round_duration.push_back(ONE_SECOND * 4 + (work_amount.back() / 3) * (2 * ONE_SECOND));
        }
    }
    double alpha = 0.0, v = 0.0, v_ci = 0.0;
    size_t subsession_sample_size = 0;
    ASSERT_EQ(0, pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
        work_amount.data(), round_duration.data(),
        0.1,  // autocorrelation_coefficient_limit
        0,    // duration threshold
        &alpha, &v, &v_ci, NULL, NULL, &subsession_sample_size));
    ASSERT_EQ(work_amount.size() / 31, subsession_sample_size);
    ASSERT_GT(31.0 * round_duration[0], double(std::numeric_limits<nanosecond_type>::max()));
    ASSERT_NEAR(1.5, v, 1e-9);
    ASSERT_NEAR(4, alpha, 1e-3);
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegressionSameWorkAmount) {
    // The slope is not defined when all rounds have the same work amount.
    // The output must not be touched.
    const vector<nanosecond_type> round_duration{
        5731883327, 5235129386, 5321265550, 5860121124, 6040418744, 6513983890,
        6623204911, 6828709974, 7455453108, 6123456789, 5987654321, 6234567890};
    for (size_t wa : {size_t(0), size_t(3), size_t(429496729)}) {
        const vector<size_t> work_amount(round_duration.size(), wa);
        double alpha = 42, v = 42, v_ci = 42;
        ASSERT_EQ(ERR_NOT_ENOUGH_DATA,
                  pilot_wps_warmup_removal_lr_method_p(work_amount.size(),
            work_amount.data(), round_duration.data(),
            1,  // autocorrelation_coefficient_limit
            0,  // duration threshold
            &alpha, &v, &v_ci)) << "work amount " << wa;
        ASSERT_EQ(42, alpha);
        ASSERT_EQ(42, v);
        ASSERT_EQ(42, v_ci);
    }
}

TEST(StatisticsUnitTest, OrdinaryLeastSquareLinearRegressionSameLargeWorkAmount) {
    // The sum of these work amounts is greater than 2^53, so their mean has
    // a rounding error and the sum of the squares of the differences from
    // the mean is not 0 even though all of them are the same.
    const size_t rounds = 1000;
    vector<nanosecond_type> round_duration;
    uint32_t x = 20260927;
    for (size_t i = 0; i < rounds; ++i) {
        x = x * 1664525u + 1013904223u;
        // 30 s +- 0.5 s
        round_duration.push_back(30 * ONE_SECOND - ONE_SECOND / 2 + nanosecond_type(x % 1000000000u));
    }
    for (size_t wa : {size_t(132762829599805), size_t(501461895223897), size_t(992184428663883)}) {
        for (size_t n : {size_t(100), size_t(300), size_t(1000)}) {
            const vector<size_t> work_amount(n, wa);
            double alpha = 42, v = 42, v_ci = 42;
            ASSERT_EQ(ERR_NOT_ENOUGH_DATA,
                      pilot_wps_warmup_removal_lr_method_p(n,
                work_amount.data(), round_duration.data(),
                1,  // autocorrelation_coefficient_limit
                0,  // duration threshold
                &alpha, &v, &v_ci)) << "work amount " << wa << ", rounds " << n;
            ASSERT_EQ(42, alpha);
            ASSERT_EQ(42, v);
            ASSERT_EQ(42, v_ci);
        }
    }
}

TEST(StatisticsUnitTest, TestOfSignificance) {
    // Sample data from http://www.stat.yale.edu/Courses/1997-98/101/meancomp.htm.
    double mean_male = 98.105;
    double mean_female = 98.394;
    double var_male = pow(0.699, 2);
    double var_female = pow(0.743, 2);
    double ci_left = 0.0, ci_right = 0.0;
    size_t sample_size_male = 65;
    size_t sample_size_female = 65;
    double p = pilot_p_eq(mean_male, mean_female,
                          sample_size_male, sample_size_female,
                          var_male, var_female,
                          &ci_left, &ci_right);
    ASSERT_NEAR(0.024, p, 0.001);
    ASSERT_NEAR(-0.540, ci_left, 0.001);
    ASSERT_NEAR(-0.039, ci_right, 0.001);

    size_t opt_sample_size;
    ASSERT_EQ(0, pilot_optimal_sample_size_for_eq_test(mean_male,
            sample_size_male, var_male, mean_female, sample_size_female,
            var_female, p, &opt_sample_size));
    ASSERT_EQ(sample_size_female, opt_sample_size);
}

TEST(StatisticsUnitTest, OptimalSampleSizeForEqTestRoundsUp) {
    size_t opt_sample_size = 42;
    // d = -1, nu = 54.0159, t = 2.004866,
    // 4 / ((1 / 2.004866)^2 - 4 / 1000) = 16.34, which has to be rounded up
    ASSERT_EQ(0, pilot_optimal_sample_size_for_eq_test(100, 1000, 4,
            101, 50, 4, 0.05, &opt_sample_size));
    ASSERT_EQ(17, opt_sample_size);
}

TEST(StatisticsUnitTest, OptimalSampleSizeForEqTestBaselineNotPreciseEnough) {
    // The variance of the mean of the baseline is 25 / 10 = 2.5, and
    // (d / t)^2 = (1 / 2.241338)^2 = 0.199. No sample size is large enough.
    size_t opt_sample_size = 42;
    ASSERT_EQ(ERR_NOT_ENOUGH_DATA, pilot_optimal_sample_size_for_eq_test(100, 10, 25,
            101, 50, 4, 0.05, &opt_sample_size));
    ASSERT_EQ(42, opt_sample_size);
}

TEST(StatisticsUnitTest, OptimalSampleSizeForEqTestSameMean) {
    size_t opt_sample_size = 42;
    ASSERT_EQ(ERR_NOT_ENOUGH_DATA, pilot_optimal_sample_size_for_eq_test(100, 1000, 4,
            100, 50, 4, 0.05, &opt_sample_size));
    ASSERT_EQ(42, opt_sample_size);
}

TEST(StatisticsUnitTest, TestOfSignificanceNoVariance) {
    // The degree of freedom is 0/0 when neither sample has any variance
    double ci_left = 42, ci_right = 42;
    ASSERT_EQ(0, pilot_p_eq(100, 101, 10, 10, 0, 0, &ci_left, &ci_right));
    ASSERT_EQ(-1, ci_left);
    ASSERT_EQ(-1, ci_right);
    ASSERT_EQ(1, pilot_p_eq(100, 100, 10, 10, 0, 0, &ci_left, &ci_right));
    ASSERT_EQ(0, ci_left);
    ASSERT_EQ(0, ci_right);

    size_t opt_sample_size = 42;
    ASSERT_EQ(0, pilot_optimal_sample_size_for_eq_test(100, 10, 0,
            101, 10, 0, 0.05, &opt_sample_size));
    ASSERT_EQ(0, opt_sample_size);
    opt_sample_size = 42;
    ASSERT_EQ(ERR_NOT_ENOUGH_DATA, pilot_optimal_sample_size_for_eq_test(100, 10, 0,
            100, 10, 0, 0.05, &opt_sample_size));
    ASSERT_EQ(42, opt_sample_size);
}

TEST(StatisticsUnitTest, TestSpeedOfLight) {
    // Test data from http://math.arizona.edu/~ghystad/chapter12.pdf
    vector<double> vel1{850, 740, 900,1070, 930, 850, 950, 980, 980, 880,1000, 980, 930, 650, 760,
                        810,1000,1000, 960, 960, 960, 940, 960, 940, 880, 800, 850, 880, 900, 840,
                        830, 790, 810, 880, 880, 830, 800, 790, 760, 800, 880, 880, 880, 860, 720,
                        720, 620, 860, 970, 950, 880, 910, 850, 870, 840, 840, 850, 840, 840, 840,
                        890, 810, 810, 820, 800, 770, 760, 740, 750, 760, 910, 920, 890, 860, 880,
                        720, 840, 850, 850, 780, 890, 840, 780, 810, 760, 810, 790, 810, 820, 850,
                        870, 870, 810, 740, 810, 940, 950, 800, 810, 870};
    vector<double> vel2{883, 816, 778, 796, 682, 711, 611, 599,1051, 781, 578, 796, 774, 820, 772,
                        696, 573, 748, 748, 797, 851, 809, 723};
    double mean1 = pilot_subsession_mean_p(vel1.data(), vel1.size(), ARITHMETIC_MEAN);
    double var1 = pilot_subsession_var_p(vel1.data(), vel1.size(), 1, mean1, ARITHMETIC_MEAN);
    double mean2 = pilot_subsession_mean_p(vel2.data(), vel2.size(), ARITHMETIC_MEAN);
    double var2 = pilot_subsession_var_p(vel2.data(), vel2.size(), 1, mean2, ARITHMETIC_MEAN);
    ASSERT_NEAR(pow(79.0105478191, 2), var1, 0.0001);
    ASSERT_NEAR(pow(107.114618526, 2), var2, 0.0001);
    ASSERT_NEAR(27.754, pilot_calc_deg_of_freedom(var1, var2, vel1.size(), vel2.size()), 0.001);
    double ci_left = 0.0, ci_right = 0.0;
    ASSERT_NEAR(0.0003625357,
                pilot_p_eq(mean1, mean2, vel1.size(), vel2.size(), var1, var2, &ci_left, &ci_right, 0.99),
                0.0000000001);
    ASSERT_NEAR(30.67544, ci_left, 0.00001);
    ASSERT_NEAR(161.68977, ci_right, 0.00001);
}

TEST(StatisticsUnitTest, TestChangeInMean1) {
    vector<double> data;
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    for (int i = 0; i < 30; ++i)
        data.push_back(5.1);
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    int *changepoints;
    size_t cp_n;
    ASSERT_EQ(0, pilot_changepoint_detection(data.data(), data.size(), &changepoints, &cp_n));
    ASSERT_EQ(2, cp_n);
    ASSERT_EQ(30, changepoints[0]);
    ASSERT_EQ(60, changepoints[1]);
    pilot_free(changepoints);
}

TEST(StatisticsUnitTest, FindDominantSegment) {
    vector<double> data;
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    for (int i = 0; i < 30; ++i)
        data.push_back(5.1);
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    size_t begin, end;
    ASSERT_EQ(ERR_NO_DOMINANT_SEGMENT, pilot_find_dominant_segment(data.data(), data.size(), &begin, &end));

    data.clear();
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    for (int i = 0; i < 130; ++i)
        data.push_back(5.1);
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    ASSERT_EQ(0, pilot_find_dominant_segment(data.data(), data.size(), &begin, &end));
    ASSERT_EQ(30, begin);
    ASSERT_EQ(160, end);
}

static size_t count_changepoints(const vector<double> &data) {
    int *changepoints = NULL;
    size_t cp_n = 42;
    EXPECT_EQ(0, pilot_changepoint_detection(data.data(), data.size(), &changepoints, &cp_n));
    pilot_free(changepoints);
    return cp_n;
}

TEST(StatisticsUnitTest, NoChangepointInIndependentReadings) {
    // E-Divisive with Medians, which we used before, reported a changepoint
    // in every sample of 60 or more readings
    for (size_t n : {60, 100, 300, 1000}) {
        for (uint64_t seed = 1; seed <= 25; ++seed) {
            pcg64_t rng(static_cast<uint64_t>(seed * 1000 + n));
            vector<double> normal, exponential, lognormal;
            for (size_t i = 0; i < n; ++i) {
                normal.push_back(100 + 5 * rng.normal());
                exponential.push_back(rng.exponential());
                lognormal.push_back(exp(rng.normal()));
            }
            ASSERT_EQ(0, count_changepoints(normal)) << "normal, n " << n << ", seed " << seed;
            ASSERT_EQ(0, count_changepoints(exponential)) << "exponential, n " << n << ", seed " << seed;
            ASSERT_EQ(0, count_changepoints(lognormal)) << "lognormal, n " << n << ", seed " << seed;
        }
    }
}

TEST(StatisticsUnitTest, FewChangepointsInAutocorrelatedReadings) {
    // First-order autoregressive processes that have no change. A changepoint
    // is reported in about 0.5% of such samples; this test allows 2 in 50.
    size_t samples = 0, reported = 0;
    for (double rho : {0.5, 0.8}) {
        for (uint64_t seed = 1; seed <= 25; ++seed) {
            pcg64_t rng(static_cast<uint64_t>(seed * 77 + static_cast<uint64_t>(rho * 10)));
            vector<double> data;
            double x = rng.normal();
            for (size_t i = 0; i < 1200; ++i) {
                x = rho * x + sqrt(1 - rho * rho) * rng.normal();
                // the first 200 are for the process to forget where it began
                if (i >= 200)
                    data.push_back(100 + 5 * x);
            }
            ++samples;
            if (count_changepoints(data) > 0)
                ++reported;
        }
    }
    ASSERT_EQ(50, samples);
    ASSERT_LE(reported, 2);
}

TEST(StatisticsUnitTest, NoChangepointInReadingsOfFewValues) {
    // The readings of a success rate are 0 or 1. Thirty 1 in a row, which
    // happen in one series of 30 out of five, are a segment that has no
    // variance, and a test that takes them for different from the rest
    // reports a changepoint. It ended a session with a success rate of 1
    // and a CI of 0.
    for (size_t n : {300, 1000, 3000}) {
        for (uint64_t seed = 1; seed <= 20; ++seed) {
            pcg64_t rng(static_cast<uint64_t>(seed * 31 + n));
            vector<double> success, half, five;
            for (size_t i = 0; i < n; ++i) {
                success.push_back(rng.unit_f64() < 0.95 ? 1 : 0);
                half.push_back(rng.unit_f64() < 0.5 ? 1 : 0);
                five.push_back(static_cast<double>(rng.next_u64() % 5));
            }
            ASSERT_EQ(0, count_changepoints(success)) << "95% 1, n " << n << ", seed " << seed;
            ASSERT_EQ(0, count_changepoints(half)) << "50% 1, n " << n << ", seed " << seed;
            ASSERT_EQ(0, count_changepoints(five)) << "five values, n " << n << ", seed " << seed;
        }
    }

    // a change of the success rate is a change
    vector<double> data;
    pcg64_t rng(static_cast<uint64_t>(5));
    for (size_t i = 0; i < 1000; ++i)
        data.push_back(rng.unit_f64() < (i < 300 ? 0.5 : 0.95) ? 1 : 0);
    size_t loc = 0;
    ASSERT_EQ(0, pilot_find_one_changepoint(data.data(), data.size(), &loc));
    ASSERT_NEAR(300, loc, 20);
}

TEST(StatisticsUnitTest, RankSumTest) {
    // k values that are all less than the m others: there are C(k + m, k)
    // ways to deal the ranks, and two of them are as far from the middle.
    // Welch's test on the ranks has 8.9e-31, 5e-31, and 1.1e-10 for the first
    // three, which let excursions of two or three subsession samples pass
    // for changepoints.
    struct {
        size_t k, m;
        double p;
    } separated[] = {{2, 98, 2.0 / 4950}, {3, 97, 2.0 / 161700}, {3, 30, 2.0 / 5456},
                     {5, 50, 2.0 / 3478761}, {1, 9, 2.0 / 10}, {2, 2, 2.0 / 6}};
    for (auto &c : separated) {
        vector<double> a, b;
        for (size_t i = 0; i < c.k; ++i) a.push_back(static_cast<double>(i));
        for (size_t i = 0; i < c.m; ++i) b.push_back(1000 + static_cast<double>(i));
        ASSERT_NEAR(c.p, pilot_rank_sum_test(a.data(), a.size(), b.data(), b.size()), 1e-9 * c.p)
            << c.k << " against " << c.m;
        // the order of the samples, and which of them is the greater
        ASSERT_NEAR(c.p, pilot_rank_sum_test(b.data(), b.size(), a.data(), a.size()), 1e-9 * c.p);
        for (double &x : a) x += 5000;
        ASSERT_NEAR(c.p, pilot_rank_sum_test(a.data(), a.size(), b.data(), b.size()), 1e-9 * c.p);
    }

    // U = 17 of 8 among 108: P(U <= 17) is 1025 ways of 352025629371
    {
        vector<double> a{1, 2, 3, 4, 5, 6, 7, 25};
        vector<double> b;
        for (int r = 8; r <= 108; ++r)
            if (25 != r) b.push_back(r);
        ASSERT_EQ(100, b.size());
        ASSERT_NEAR(2 * 1025.0 / 352025629371.0,
                    pilot_rank_sum_test(a.data(), a.size(), b.data(), b.size()), 1e-20);
    }

    // samples that are not different
    {
        vector<double> a{1, 4, 5, 8, 9, 12};
        vector<double> b{2, 3, 6, 7, 10, 11};
        ASSERT_GT(pilot_rank_sum_test(a.data(), a.size(), b.data(), b.size()), 0.9);
    }

    // large samples, where the normal approximation is used: 1000 against
    // 1000 that are greater by one standard deviation of N(0, 1)
    {
        pcg64_t rng(static_cast<uint64_t>(2));
        vector<double> a, b;
        for (int i = 0; i < 1000; ++i) {
            a.push_back(rng.normal());
            b.push_back(1 + rng.normal());
        }
        const double p = pilot_rank_sum_test(a.data(), a.size(), b.data(), b.size());
        ASSERT_LT(p, 1e-50);
        ASSERT_GE(p, 0);
    }

    // an empty sample
    {
        vector<double> a{1, 2, 3};
        ASSERT_EQ(1, pilot_rank_sum_test(a.data(), a.size(), a.data(), 0));
    }
}

TEST(StatisticsUnitTest, RankSumTestOfValuesThatAreTheSame) {
    // 30 readings of 1 against 970 readings of which 49 are 0. Welch's test
    // on the ranks, of which one side has no variance, has 4e-12. Fisher's
    // exact test has 0.3945.
    {
        vector<double> a(30, 1.0), b;
        for (int i = 0; i < 970; ++i)
            b.push_back(i % 20 ? 1 : 0);
        ASSERT_NEAR(0.394502734614766, pilot_rank_sum_test(a.data(), a.size(), b.data(), b.size()), 1e-12);
        ASSERT_NEAR(0.394502734614766, pilot_rank_sum_test(b.data(), b.size(), a.data(), a.size()), 1e-12);
    }

    // samples that are the same cannot be different, whatever their sizes
    for (size_t k : {2, 18, 41, 55, 100}) {
        for (size_t m : {2, 212, 746, 2746}) {
            vector<double> c(k, 3.14), d(m, 3.14);
            ASSERT_EQ(1, pilot_rank_sum_test(c.data(), c.size(), d.data(), d.size()))
                << k << " against " << m;
        }
    }

    // 300 readings of which half are 0 against 700 of which 35 are
    {
        vector<double> e, f;
        for (int i = 0; i < 300; ++i) e.push_back(i % 2);
        for (int i = 0; i < 700; ++i) f.push_back(i % 20 ? 1 : 0);
        const double p = pilot_rank_sum_test(e.data(), e.size(), f.data(), f.size());
        ASSERT_NEAR(4.92895298666787e-59, p, 1e-68);
    }

    // small tables, of which the p-values are 62/2261 and 13/63
    {
        vector<double> a{0, 0, 0, 0, 0, 0, 0, 1, 1, 1};
        vector<double> b{0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
        ASSERT_NEAR(62.0 / 2261, pilot_rank_sum_test(a.data(), a.size(), b.data(), b.size()), 1e-14);
        vector<double> c{0, 0, 0, 0, 1};
        vector<double> d{0, 1, 1, 1, 1};
        ASSERT_NEAR(13.0 / 63, pilot_rank_sum_test(c.data(), c.size(), d.data(), d.size()), 1e-14);
    }

    // Few values are the same: the ranks of those that are the same are
    // dealt in the way that is the least in favor of a difference. Here 10
    // is in both samples, and a gets the greater of its two ranks, so a has
    // the ranks 1 to 9 and 11 of 110: U = 1, of which there are 2 ways.
    {
        vector<double> a{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        vector<double> b{10};
        for (int i = 11; i < 110; ++i) b.push_back(i);
        ASSERT_EQ(100, b.size());
        const double ways = 46897636623981.0;   // C(110, 10)
        ASSERT_NEAR(2 * 2 / ways, pilot_rank_sum_test(a.data(), a.size(), b.data(), b.size()), 1e-25);
    }

    // a value that is not a number
    {
        vector<double> a{1, 2, std::numeric_limits<double>::quiet_NaN()};
        vector<double> b{4, 5, 6, 7};
        ASSERT_EQ(1, pilot_rank_sum_test(a.data(), a.size(), b.data(), b.size()));
    }
}

TEST(StatisticsUnitTest, ChangepointOfReadingsThatAreNotNumbers) {
    vector<double> data;
    pcg64_t rng(static_cast<uint64_t>(9));
    for (size_t i = 0; i < 1000; ++i)
        data.push_back((i < 100 ? 3 : 0) + rng.normal());
    ASSERT_EQ(1, count_changepoints(data));
    // a reading that is not a number: no detection, and no crash
    data[500] = std::numeric_limits<double>::quiet_NaN();
    ASSERT_EQ(0, count_changepoints(data));
    data[500] = std::numeric_limits<double>::infinity();
    ASSERT_EQ(0, count_changepoints(data));

    // readings of which the variance is greater than any double
    data.clear();
    for (size_t i = 0; i < 1000; ++i)
        data.push_back(1e300 * (1 + 0.1 * rng.normal()) * (i < 100 ? 1.5 : 1));
    int *changepoints = NULL;
    size_t cp_n = 0;
    ASSERT_EQ(0, pilot_changepoint_detection(data.data(), data.size(), &changepoints, &cp_n));
    pilot_free(changepoints);
}

TEST(StatisticsUnitTest, ChangepointOfWarmUpAndCoolDown) {
    // 50 readings of warm-up that are lower by 2 standard deviations, 400
    // stable readings, and 50 readings of cool-down that are lower by 3
    size_t found_both = 0;
    const size_t samples = 20;
    for (uint64_t seed = 1; seed <= samples; ++seed) {
        pcg64_t rng(static_cast<uint64_t>(seed));
        vector<double> data;
        for (size_t i = 0; i < 500; ++i) {
            double level = 100;
            if (i < 50) level -= 2 * 5;
            if (i >= 450) level -= 3 * 5;
            data.push_back(level + 5 * rng.normal());
        }
        int *changepoints = NULL;
        size_t cp_n = 0;
        ASSERT_EQ(0, pilot_changepoint_detection(data.data(), data.size(), &changepoints, &cp_n));
        bool ok = (2 == cp_n) && abs(changepoints[0] - 50) <= 5 && abs(changepoints[1] - 450) <= 5;
        pilot_free(changepoints);
        if (!ok) continue;
        ++found_both;

        size_t begin = 0, end = 0;
        ASSERT_EQ(0, pilot_find_dominant_segment(data.data(), data.size(), &begin, &end));
        ASSERT_NEAR(50, begin, 5);
        ASSERT_NEAR(450, end, 5);
        size_t loc = 0;
        ASSERT_EQ(0, pilot_find_one_changepoint(data.data(), data.size(), &loc));
        ASSERT_NEAR(450, loc, 5);
    }
    // they are found in about 97% of such samples
    ASSERT_GE(found_both, samples - 2);
}

TEST(StatisticsUnitTest, ChangepointOfLargeReadings) {
    // The readings are about 1e12 and the change is 20. The place of the
    // change must not be lost in the digits.
    vector<double> data;
    pcg64_t rng(static_cast<uint64_t>(7));
    for (size_t i = 0; i < 300; ++i)
        data.push_back(1e12 + (i < 100 ? 0 : 20) + 5 * rng.normal());
    size_t loc = 0;
    ASSERT_EQ(0, pilot_find_one_changepoint(data.data(), data.size(), &loc));
    ASSERT_NEAR(100, loc, 3);
}

TEST(StatisticsUnitTest, ChangepointNeedsTwoSegments) {
    // no segment is shorter than MIN_CHANGEPOINT_DETECTION_SAMPLE_SIZE
    vector<double> data;
    for (int i = 0; i < 29; ++i)
        data.push_back(1.1);
    for (int i = 0; i < 30; ++i)
        data.push_back(5.1);
    ASSERT_EQ(0, count_changepoints(data));
    data.push_back(5.1);
    int *changepoints = NULL;
    size_t cp_n = 0;
    ASSERT_EQ(0, pilot_changepoint_detection(data.data(), data.size(), &changepoints, &cp_n));
    ASSERT_EQ(1, cp_n);
    // the change is at 29 but the first segment has to have 30 readings
    ASSERT_EQ(30, changepoints[0]);
    pilot_free(changepoints);

    // the same reading all the time
    data.assign(500, 3.14);
    ASSERT_EQ(0, count_changepoints(data));
}

TEST(StatisticsUnitTest, FindChangepoint) {
    vector<double> data;
    size_t loc;
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    ASSERT_EQ(ERR_NO_CHANGEPOINT, pilot_find_one_changepoint(data.data(), data.size(), &loc));
    for (int i = 0; i < 30; ++i)
        data.push_back(5.1);
    ASSERT_EQ(0, pilot_find_one_changepoint(data.data(), data.size(), &loc));
    ASSERT_EQ(30, loc);
    for (int i = 0; i < 30; ++i)
        data.push_back(1.1);
    ASSERT_EQ(0, pilot_find_one_changepoint(data.data(), data.size(), &loc));
    ASSERT_EQ(60, loc);
}

int main(int argc, char **argv) {
    PILOT_LIB_SELF_CHECK;
    // we only display fatals because errors are expected in some test cases
    pilot_set_log_level(lv_fatal);

    // Use a deterministic death-test backend across environments.
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
