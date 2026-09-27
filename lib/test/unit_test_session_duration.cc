/*
 * unit_test_session_duration.cc: unit tests for calculating the optimal
 * session duration
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

// This test cases needs DEBUG
#undef NDEBUG

#include <algorithm>
#include <chrono>
#include <cstring>
#include "gtest/gtest.h"
#include <memory>
#include "pilot/libpilot.h"
#include <vector>
#include "workload.hpp"

using namespace pilot;
using namespace std;

static size_t g_work_amount_limit;
static const int g_num_of_pi = 1;
static const vector<size_t> *g_expected_work_amount_per_round = NULL;
static const vector<int> *g_required_sample_size_per_round = NULL;

int mock_workload_func(const pilot_workload_t *wl,
                       size_t round,
                       size_t work_amount,
                       pilot_malloc_func_t *lib_malloc_func,
                       size_t *num_of_work_unit,
                       double ***unit_readings,
                       double **readings,
                       nanosecond_type *round_duration, void *data) {
    assert(g_work_amount_limit >= work_amount);
    assert(g_expected_work_amount_per_round);
    assert((*g_expected_work_amount_per_round)[round] == work_amount);

    *num_of_work_unit = work_amount;

    // store unit_readings
    *unit_readings = (double**)lib_malloc_func(sizeof(double*) * g_num_of_pi);
    for (int piid = 0; piid < g_num_of_pi; ++piid) {
        size_t memsize = sizeof(double) * work_amount;
        (*unit_readings)[piid] = (double*)lib_malloc_func(memsize);
        // the readings here are irrelevant so we just fill in some number
        for (size_t i = 0; i < work_amount; ++i)
            (*unit_readings)[piid][i] = 42.42;
    }

    // store readings
    assert(NULL == *readings);
    *readings = (double*)lib_malloc_func(sizeof(double*) * g_num_of_pi);
    for (int i = 0; i < g_num_of_pi; ++i)
        (*readings)[i] = 42.42;

    return 0;
}

ssize_t mock_calc_required_ur_func(const pilot_workload_t* wl, int piid) {
    size_t test_rounds = wl->rounds_;
    if (test_rounds >= g_required_sample_size_per_round->size()) {
        cerr << __func__ << "(): more rounds than expected" << endl;
        abort();
    }
    return (*g_required_sample_size_per_round)[test_rounds];
}

void test_opt_session_duration(size_t work_amount_limit,
                               const vector<size_t> &expected_work_amount_per_round,
                               const vector<int> &required_sample_size_per_round) {
    shared_ptr<pilot_workload_t> wl(pilot_new_workload("Test workload"), pilot_destroy_workload);
    pilot_set_short_workload_check(wl.get(), false);
    pilot_set_workload_func(wl.get(), &mock_workload_func);
    pilot_set_work_amount_limit(wl.get(), work_amount_limit);
    pilot_set_num_of_pi(wl.get(), 1);
    pilot_set_pi_info(wl.get(), 0, "TestPI", "tick", NULL, NULL,
                      false,  /* reading must satisfy */
                      true);  /* unit readings must satisfy */
    pilot_set_calc_required_unit_readings_func(wl.get(), &mock_calc_required_ur_func);
    pilot_set_wps_analysis(wl.get(), NULL, false, false);
    pilot_set_short_round_detection_threshold(wl.get(), 0);
    pilot_set_warm_up_removal_method(wl.get(), NO_WARM_UP_REMOVAL);

    g_expected_work_amount_per_round = &expected_work_amount_per_round;
    g_required_sample_size_per_round = &required_sample_size_per_round;
    g_work_amount_limit = work_amount_limit;

    int res = pilot_run_workload(wl.get());
    if (0 != res) {
        cerr << "pilot_run_workload() returned " << res << endl;
        abort();
    }

    if (wl->rounds_ != g_expected_work_amount_per_round->size()) {
        cerr << __func__ << "(): not enough rounds" << endl;
        abort();
    }

    g_expected_work_amount_per_round = NULL;
    g_required_sample_size_per_round = NULL;
}

TEST(PilotRunWorkloadTest, CalculatingOptimalSessionDuration) {
    // Tests if pilot_run_workload() can correctly assign work_amount
    // for each round. In the following case, the work_amount_limit is
    // set to 50. The first round's work amount
    // (expected_work_amount_per_round[0]) is expected to be 1 (trial
    // run). Our mock workload func would return 1 sample (sample to
    // work amount ratio is 1:1 here.) The test case uses the
    // calc_readings_required_func hook to tell pilot_run_workload()
    // that a total of 15 samples are required (as in
    // required_sample_size_per_round[0]). Now we have 1 sample so 14
    // more samples are required. We expect the next round to have
    // 14*(1+.2)=12 samples (as in expected_work_amount_per_round[1],
    // .2 is the ratio of safety buffer), but
    // round_work_amount_to_avg_amount_limit_ defaults to 5 so the
    // work amount limit for next round is 1*5 = 5. We need 15-1-5=9
    // samples for the third round, and trunc(9*1.2) = 10.
    pilot_set_log_level(lv_warning);
    test_opt_session_duration(
            50,              // work_amount_limit
            { 1,  5, 10},    // expected_work_amount_per_round
            {15, 15, 15, 15});   // required_sample_size_per_round
    //! TODO: a test case whose average num of work units per work amount != 1
    //! TODO: a test case for multiple PIs that each of which has a different work_unit_per_amount ratio
}

TEST(PilotRunWorkloadTest, TestCalcNextRoundWorkAmountFromWPS) {
    pilot_workload_t *wl = pilot_new_workload("Test workload");
    size_t wa_limit = 1000;
    pilot_set_work_amount_limit(wl, wa_limit);
    pilot_set_wps_analysis(wl, NULL, true, true);
    pilot_set_init_work_amount(wl, 0);
    pilot_set_session_desired_duration(wl, 60);
    wl->adjusted_min_work_amount_ = 10;
    wl->round_work_amounts_.push_back(10);
    wl->round_durations_.push_back(1 * ONE_SECOND);
    wl->rounds_++;
    // See [Li16] Equation (5)
    double k = (2.0 * 60 - 2 * 1 * 10) / (10 * 10 - 10);
    size_t wa_slice_size = size_t(k * 10 / 1);
    size_t wa;
    ASSERT_TRUE(calc_next_round_work_amount_from_wps(wl, &wa));
    ASSERT_EQ(10 + wa_slice_size, wa);

    // more mock rounds
    wl->round_work_amounts_.push_back(365);
    wl->round_durations_.push_back(ONE_SECOND);
    wl->rounds_++;
    ASSERT_TRUE(calc_next_round_work_amount_from_wps(wl, &wa));
    ASSERT_EQ(((365 - 10) / wa_slice_size + 1) * wa_slice_size + 10, wa);
    wl->round_work_amounts_.push_back(999);
    wl->round_durations_.push_back(ONE_SECOND);
    wl->rounds_++;
    wa_slice_size /= 2;
    ASSERT_TRUE(calc_next_round_work_amount_from_wps(wl, &wa));
    ASSERT_EQ(10 + wa_slice_size, wa);
    wl->round_work_amounts_.push_back(10 + wa_slice_size);
    wl->round_durations_.push_back(ONE_SECOND);
    wl->rounds_++;
    ASSERT_TRUE(calc_next_round_work_amount_from_wps(wl, &wa));
    ASSERT_EQ(10 + wa_slice_size * 2, wa);
    pilot_destroy_workload(wl);
}

static void add_mock_round(pilot_workload_t *wl, size_t work_amount, double duration_in_sec) {
    wl->round_work_amounts_.push_back(work_amount);
    wl->round_durations_.push_back(nanosecond_type(duration_in_sec * ONE_SECOND));
    wl->rounds_++;
}

static pilot_workload_t* new_mock_wps_workload(void) {
    pilot_workload_t *wl = pilot_new_workload("Test workload");
    pilot_set_work_amount_limit(wl, 1000);
    pilot_set_init_work_amount(wl, 0);
    pilot_set_wps_analysis(wl, NULL, true, true);
    pilot_set_autocorrelation_coefficient(wl, 1);
    pilot_set_short_round_detection_threshold(wl, 1);
    return wl;
}

static shared_ptr<pilot_analytical_result_t> mock_wps_analysis(pilot_workload_t *wl) {
    // make sure the analysis is done again
    wl->raw_data_changed_time_ = chrono::steady_clock::now();
    return shared_ptr<pilot_analytical_result_t>(pilot_analytical_result(wl, NULL),
                                                 pilot_free_analytical_result);
}

const vector<size_t> g_mock_wps_work_amounts{30, 300, 60, 270, 90, 240, 120, 210, 150, 180};

TEST(PilotRunWorkloadTest, WPSLeavesOutShortRounds) {
    pilot_workload_t *wl = new_mock_wps_workload();
    // Ten rounds of duration = 4 + work_amount / 1.5, and two rounds that are
    // shorter than the short round detection threshold. A regression of all
    // 12 rounds has alpha 1.5395 and v 1.4742.
    const double exp_alpha = 4;
    const double exp_v = 1.5;
    for (size_t i = 0; i < g_mock_wps_work_amounts.size(); ++i) {
        size_t wa = g_mock_wps_work_amounts[i];
        add_mock_round(wl, wa, exp_alpha + wa / exp_v);
        if (1 == i || 5 == i)
            add_mock_round(wl, 3, 0.9);
    }
    // The second analysis has the result of the first one to start from. It
    // must not let the short rounds back in.
    for (int i = 0; i < 2; ++i) {
        auto ar = mock_wps_analysis(wl);
        ASSERT_TRUE(ar->wps_has_data);
        ASSERT_EQ(10, ar->wps_subsession_sample_size);
        ASSERT_NEAR(exp_alpha, ar->wps_alpha, 1e-6);
        ASSERT_NEAR(exp_v, ar->wps_v, 1e-6);
    }
    pilot_destroy_workload(wl);
}

TEST(PilotRunWorkloadTest, WPSLeavesOutRoundsShorterThanNegativeAlpha) {
    pilot_workload_t *wl = new_mock_wps_workload();
    // Ten rounds of duration = -5 + work_amount / 1.5, and two rounds of 3 s
    // and 3.5 s that didn't reach the stable phase. A regression of all 12
    // rounds has alpha -3.4316 and v 1.5168. With the two rounds left out
    // alpha is -5.
    const double exp_alpha = -5;
    const double exp_v = 1.5;
    for (size_t i = 0; i < g_mock_wps_work_amounts.size(); ++i) {
        size_t wa = g_mock_wps_work_amounts[i];
        add_mock_round(wl, wa, exp_alpha + wa / exp_v);
        if (2 == i)
            add_mock_round(wl, 6, 3);
        if (6 == i)
            add_mock_round(wl, 9, 3.5);
    }
    for (int i = 0; i < 2; ++i) {
        auto ar = mock_wps_analysis(wl);
        ASSERT_TRUE(ar->wps_has_data);
        ASSERT_EQ(10, ar->wps_subsession_sample_size);
        ASSERT_NEAR(exp_alpha, ar->wps_alpha, 1e-6);
        ASSERT_NEAR(exp_v, ar->wps_v, 1e-6);
    }
    pilot_destroy_workload(wl);
}

TEST(PilotRunWorkloadTest, WPSNotEnoughRoundsLongerThanNegativeAlpha) {
    pilot_workload_t *wl = new_mock_wps_workload();
    // Ten rounds of duration = -100 + work_amount / 1.5, which are from 10 s
    // to 120 s. Only two of them are longer than -alpha.
    const double exp_alpha = -100;
    const double exp_v = 1.5;
    for (size_t wa : {225, 330, 165, 270, 195, 240, 315, 180, 255, 210}) {
        add_mock_round(wl, wa, exp_alpha + wa / exp_v);
    }
    auto ar = mock_wps_analysis(wl);
    ASSERT_FALSE(ar->wps_has_data);
    pilot_destroy_workload(wl);
}

TEST(PilotRunWorkloadTest, WPSPerfectFit) {
    pilot_workload_t *wl = new_mock_wps_workload();
    // A CI width of 0 is a valid result. It is not the same as no CI (-1).
    for (size_t wa : g_mock_wps_work_amounts) {
        add_mock_round(wl, wa, 4 + wa / 1.5);
    }
    auto ar = mock_wps_analysis(wl);
    ASSERT_TRUE(ar->wps_has_data);
    ASSERT_GE(ar->wps_v_ci, 0);
    ASSERT_NEAR(0, ar->wps_v_ci, 1e-6);
    char *summary = pilot_text_workload_summary(wl);
    ASSERT_NE(nullptr, strstr(summary, "WPS v CI: "));
    ASSERT_EQ(nullptr, strstr(summary, "Not enough data for WPS v CI"));
    pilot_free_text_dump(summary);
    pilot_destroy_workload(wl);
}

TEST(PilotRunWorkloadTest, WPSNoUpperBoundOfV) {
    pilot_workload_t *wl = new_mock_wps_workload();
    // The data of StatisticsUnitTest.OrdinaryLeastSquareLinearRegression3
    const vector<size_t> work_amount{429497000, 472446000, 515396000, 558346000};
    const vector<nanosecond_type> round_duration{4681140000, 5526190000, 5632120000, 5611980000};
    pilot_set_work_amount_limit(wl, 1000000000);
    for (size_t i = 0; i < work_amount.size(); ++i) {
        wl->round_work_amounts_.push_back(work_amount[i]);
        wl->round_durations_.push_back(round_duration[i]);
        wl->rounds_++;
    }
    auto ar = mock_wps_analysis(wl);
    ASSERT_TRUE(ar->wps_has_data);
    ASSERT_NEAR(ONE_SECOND / 6.7485, ar->wps_v, 10000);
    ASSERT_EQ(-1, ar->wps_v_ci);
    ASSERT_EQ(-1, ar->wps_v_ci_formatted);
    char *summary = pilot_text_workload_summary(wl);
    ASSERT_NE(nullptr, strstr(summary, "Not enough data for WPS v CI"));
    pilot_free_text_dump(summary);
    pilot_destroy_workload(wl);
}

int main(int argc, char **argv) {
    PILOT_LIB_SELF_CHECK;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
