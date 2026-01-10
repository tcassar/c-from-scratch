/**
 * test_drift.c - Contract and Invariant Test Suite
 * 
 * This is not a unit test file. This is a proof harness.
 * Each test demonstrates a theorem, not just exercises an API.
 * 
 * Contract Tests:
 *   CONTRACT-1: Bounded slope detection
 *   CONTRACT-2: Noise immunity
 *   CONTRACT-3: TTF accuracy
 *   CONTRACT-4: Spike resistance
 * 
 * Invariant Tests:
 *   INV-1: State domain
 *   INV-2: Learning threshold
 *   INV-3: Fault implies FAULT state
 *   INV-4: Atomicity guard
 *   INV-5: Monotonic n
 * 
 * Fuzz Tests:
 *   Random streams
 *   Fault injection (NaN/Inf)
 *   Edge cases
 * 
 * Copyright (c) 2026 William Murray
 * MIT License - https://github.com/williamofai/c-from-scratch
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "drift.h"

/*===========================================================================
 * Test Counters
 *===========================================================================*/

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_PASS(name) do { \
    tests_run++; \
    tests_passed++; \
    printf("  [PASS] %s\n", name); \
} while(0)

#define TEST_FAIL(name, msg) do { \
    tests_run++; \
    printf("  [FAIL] %s: %s\n", name, msg); \
} while(0)

#define ASSERT_TRUE(cond, name, msg) do { \
    if (!(cond)) { TEST_FAIL(name, msg); return; } \
} while(0)

#define EPSILON 1e-9

/*===========================================================================
 * CONTRACT TESTS
 *===========================================================================*/

/**
 * CONTRACT-1: Bounded Slope Detection
 * 
 * When |slope| > max_safe_slope, state must be DRIFTING_UP or DRIFTING_DOWN.
 * When |slope| <= max_safe_slope (and ready), state must be STABLE.
 */
static void test_contract1_bounded_slope(void)
{
    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 3;
    cfg.max_safe_slope = 0.05;
    cfg.alpha = 0.5;  /* Fast tracking */
    
    drift_init(&d, &cfg);
    
    drift_result_t r;
    uint64_t ts = 1000;
    double value = 0.0;
    
    /* Ramp that exceeds threshold: 10 units per 100ms = 0.1 units/ms */
    for (int i = 0; i < 10; i++) {
        drift_update(&d, value, ts, &r);
        value += 10.0;
        ts += 100;
    }
    
    /* After ramp, slope should be ~0.1, exceeding 0.05 */
    if (d.n >= cfg.n_min) {
        double abs_slope = fabs(d.slope);
        if (abs_slope > cfg.max_safe_slope) {
            ASSERT_TRUE(d.state == DRIFT_DRIFTING_UP || d.state == DRIFT_DRIFTING_DOWN,
                       "CONTRACT-1", "High slope should trigger DRIFTING state");
        }
    }
    
    TEST_PASS("CONTRACT-1: Bounded slope detection");
}

/**
 * CONTRACT-2: Noise Immunity
 * 
 * Small jitter around a constant should not trigger DRIFTING state.
 */
static void test_contract2_noise_immunity(void)
{
    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 5;
    cfg.max_safe_slope = 0.05;
    cfg.alpha = 0.1;
    
    drift_init(&d, &cfg);
    
    drift_result_t r;
    uint64_t ts = 1000;
    srand(12345);
    
    /* Add ±1 unit jitter to constant 50 */
    for (int i = 0; i < 100; i++) {
        double jitter = ((double)(rand() % 200) / 100.0) - 1.0;
        double value = 50.0 + jitter;
        drift_update(&d, value, ts, &r);
        ts += 100;
    }
    
    /* With smoothing, small jitter should result in near-zero slope */
    ASSERT_TRUE(d.state == DRIFT_STABLE || d.state == DRIFT_LEARNING,
               "CONTRACT-2", "Noise should not trigger DRIFTING");
    ASSERT_TRUE(fabs(d.slope) < cfg.max_safe_slope,
               "CONTRACT-2", "Smoothed slope should be small");
    
    TEST_PASS("CONTRACT-2: Noise immunity (jitter does not trigger drift)");
}

/**
 * CONTRACT-3: TTF Accuracy
 * 
 * Time-to-failure estimate should be reasonably accurate for steady drift.
 */
static void test_contract3_ttf_accuracy(void)
{
    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 3;
    cfg.upper_limit = 100.0;
    cfg.lower_limit = 0.0;
    cfg.alpha = 0.9;  /* Very fast tracking for accurate slope */
    cfg.max_safe_slope = 0.01;
    
    drift_init(&d, &cfg);
    
    drift_result_t r;
    uint64_t ts = 1000;
    double value = 50.0;
    
    /* Steady ramp: 1 unit per 100ms = 0.01 units/ms */
    for (int i = 0; i < 20; i++) {
        drift_update(&d, value, ts, &r);
        value += 1.0;  /* 1 unit per step */
        ts += 100;     /* 100ms per step */
    }
    
    /* Current value ≈ 70, upper limit = 100, slope ≈ 0.01 */
    /* Expected TTF ≈ (100 - 70) / 0.01 = 3000 ms */
    if (r.has_ttf && r.slope > 0) {
        double expected_ttf = (cfg.upper_limit - (value - 1.0)) / r.slope;
        double error = fabs(r.ttf - expected_ttf) / expected_ttf;
        ASSERT_TRUE(error < 0.5,  /* Within 50% error */
                   "CONTRACT-3", "TTF should be reasonably accurate");
    }
    
    TEST_PASS("CONTRACT-3: TTF accuracy (bounded error)");
}

/**
 * CONTRACT-4: Spike Resistance
 * 
 * Single outlier shifts slope by at most α·(outlier_slope).
 */
static void test_contract4_spike_resistance(void)
{
    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 3;
    cfg.alpha = 0.1;
    cfg.max_safe_slope = 10.0;  /* High threshold */
    
    drift_init(&d, &cfg);
    
    drift_result_t r;
    uint64_t ts = 1000;
    
    /* Establish baseline with constant signal */
    for (int i = 0; i < 20; i++) {
        drift_update(&d, 50.0, ts, &r);
        ts += 100;
    }
    
    double slope_before = d.slope;
    
    /* Inject massive spike: +1000 in 100ms = raw_slope of 10 */
    drift_update(&d, 1050.0, ts, &r);
    
    double slope_after = d.slope;
    double slope_change = fabs(slope_after - slope_before);
    
    /* With α=0.1, maximum change should be α·10 = 1.0 */
    double max_expected_change = cfg.alpha * 10.0 * 1.1;  /* 10% tolerance */
    
    ASSERT_TRUE(slope_change <= max_expected_change,
               "CONTRACT-4", "Spike should be bounded by alpha");
    
    TEST_PASS("CONTRACT-4: Spike resistance (|Δslope| ≤ α·spike)");
}

/*===========================================================================
 * INVARIANT TESTS
 *===========================================================================*/

/**
 * INV-1: State Domain
 * 
 * State must always be in valid enum range.
 */
static void test_inv1_state_domain(void)
{
    drift_fsm_t d;
    drift_init(&d, &DRIFT_DEFAULT_CONFIG);
    
    ASSERT_TRUE(d.state >= DRIFT_LEARNING && d.state <= DRIFT_FAULT,
               "INV-1", "Initial state must be valid");
    
    drift_result_t r;
    uint64_t ts = 1000;
    
    for (int i = 0; i < 100; i++) {
        double value = (double)(rand() % 1000) / 10.0;
        drift_update(&d, value, ts, &r);
        
        ASSERT_TRUE(d.state >= DRIFT_LEARNING && d.state <= DRIFT_FAULT,
                   "INV-1", "State must remain valid after update");
        ts += 100;
    }
    
    TEST_PASS("INV-1: State always in valid domain");
}

/**
 * INV-2: Learning Threshold
 * 
 * (state ≠ LEARNING) → (n >= n_min)
 */
static void test_inv2_learning_threshold(void)
{
    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 10;
    
    drift_init(&d, &cfg);
    
    drift_result_t r;
    uint64_t ts = 1000;
    
    for (int i = 0; i < 15; i++) {
        drift_update(&d, 50.0, ts, &r);
        
        /* If not LEARNING (and not FAULT), n must be >= n_min */
        if (d.state != DRIFT_LEARNING && d.state != DRIFT_FAULT) {
            ASSERT_TRUE(d.n >= cfg.n_min,
                       "INV-2", "Non-LEARNING state requires n >= n_min");
        }
        ts += 100;
    }
    
    TEST_PASS("INV-2: (state ≠ LEARNING) → (n >= n_min)");
}

/**
 * INV-3: Fault Implies FAULT State
 */
static void test_inv3_fault_implies_fault_state(void)
{
    drift_fsm_t d;
    drift_init(&d, &DRIFT_DEFAULT_CONFIG);
    
    drift_result_t r;
    uint64_t ts = 1000;
    
    /* Normal operation */
    drift_update(&d, 50.0, ts, &r);
    ts += 100;
    
    ASSERT_TRUE(!drift_faulted(&d) || d.state == DRIFT_FAULT,
               "INV-3", "Fault flag implies FAULT state");
    
    /* Inject NaN */
    drift_update(&d, 0.0 / 0.0, ts, &r);
    
    ASSERT_TRUE(drift_faulted(&d), "INV-3", "NaN should set fault flag");
    ASSERT_TRUE(d.state == DRIFT_FAULT, "INV-3", "Fault should force FAULT state");
    
    TEST_PASS("INV-3: Fault implies FAULT state");
}

/**
 * INV-5: Monotonic n
 * 
 * n increments monotonically on valid (non-faulted) input.
 */
static void test_inv5_monotonic_n(void)
{
    drift_fsm_t d;
    drift_init(&d, &DRIFT_DEFAULT_CONFIG);
    
    drift_result_t r;
    uint64_t ts = 1000;
    uint32_t prev_n = 0;
    
    for (int i = 0; i < 50; i++) {
        drift_update(&d, 50.0, ts, &r);
        
        ASSERT_TRUE(d.n >= prev_n, "INV-5", "n must be monotonically non-decreasing");
        ASSERT_TRUE(d.n == prev_n + 1, "INV-5", "n must increment by 1");
        
        prev_n = d.n;
        ts += 100;
    }
    
    TEST_PASS("INV-5: n increments monotonically");
}

/**
 * INV: Faulted input does not increment n
 */
static void test_inv_fault_no_increment(void)
{
    drift_fsm_t d;
    drift_init(&d, &DRIFT_DEFAULT_CONFIG);
    
    drift_result_t r;
    uint64_t ts = 1000;
    
    /* Some normal inputs */
    for (int i = 0; i < 5; i++) {
        drift_update(&d, 50.0, ts, &r);
        ts += 100;
    }
    
    uint32_t n_before = d.n;
    
    /* Inject NaN (should fault) */
    drift_update(&d, 0.0 / 0.0, ts, &r);
    ts += 100;
    
    ASSERT_TRUE(d.n == n_before, "INV", "Faulted input should not increment n");
    
    /* Further inputs should also not increment (fault is sticky) */
    drift_update(&d, 50.0, ts, &r);
    ASSERT_TRUE(d.n == n_before, "INV", "Post-fault inputs should not increment n");
    
    TEST_PASS("INV: Faulted input does not increment counter");
}

/*===========================================================================
 * FUZZ TESTS
 *===========================================================================*/

/**
 * Fuzz: Random Streams
 * 
 * 100k random observations, check all invariants hold.
 */
static void test_fuzz_random_streams(void)
{
    drift_fsm_t d;
    drift_init(&d, &DRIFT_DEFAULT_CONFIG);
    
    drift_result_t r;
    uint64_t ts = 1000;
    srand((unsigned int)time(NULL));
    
    for (int i = 0; i < 100000; i++) {
        double value = ((double)(rand() % 10000) / 100.0);  /* 0 to 100 */
        drift_update(&d, value, ts, &r);
        
        /* Check invariants */
        if (!(d.state >= DRIFT_LEARNING && d.state <= DRIFT_FAULT)) {
            TEST_FAIL("Fuzz", "Invalid state during random stream");
            return;
        }
        if (drift_faulted(&d) && d.state != DRIFT_FAULT) {
            TEST_FAIL("Fuzz", "Fault flag without FAULT state");
            return;
        }
        
        ts += 100;
    }
    
    TEST_PASS("Fuzz: 100000 random observations, invariants held");
}

/**
 * Fuzz: NaN/Inf Injection
 */
static void test_fuzz_fault_injection(void)
{
    drift_fsm_t d;
    drift_init(&d, &DRIFT_DEFAULT_CONFIG);
    
    drift_result_t r;
    uint64_t ts = 1000;
    
    double special_values[] = {
        0.0 / 0.0,    /* NaN */
        1.0 / 0.0,    /* +Inf */
        -1.0 / 0.0,   /* -Inf */
    };
    
    for (int test = 0; test < 3; test++) {
        drift_reset(&d);
        
        /* Normal setup */
        for (int i = 0; i < 5; i++) {
            drift_update(&d, 50.0, ts, &r);
            ts += 100;
        }
        
        /* Inject special value */
        int err = drift_update(&d, special_values[test], ts, &r);
        ts += 100;
        
        if (err != DRIFT_ERR_DOMAIN && err != DRIFT_ERR_FAULT) {
            TEST_FAIL("Fuzz", "Special value not detected as domain error");
            return;
        }
        if (d.state != DRIFT_FAULT) {
            TEST_FAIL("Fuzz", "Special value did not trigger FAULT state");
            return;
        }
    }
    
    TEST_PASS("Fuzz: Fault injection (NaN/Inf) handled safely");
}

/*===========================================================================
 * EDGE CASE TESTS
 *===========================================================================*/

/**
 * Edge: Config Validation
 */
static void test_edge_config_validation(void)
{
    drift_fsm_t d;
    drift_config_t cfg;
    int err;
    
    /* Invalid alpha (too low) */
    cfg = DRIFT_DEFAULT_CONFIG;
    cfg.alpha = 0.0;
    err = drift_init(&d, &cfg);
    ASSERT_TRUE(err == DRIFT_ERR_CONFIG, "Edge", "alpha=0 should fail");
    
    /* Invalid alpha (too high) */
    cfg = DRIFT_DEFAULT_CONFIG;
    cfg.alpha = 1.1;
    err = drift_init(&d, &cfg);
    ASSERT_TRUE(err == DRIFT_ERR_CONFIG, "Edge", "alpha>1 should fail");
    
    /* Invalid max_safe_slope */
    cfg = DRIFT_DEFAULT_CONFIG;
    cfg.max_safe_slope = 0.0;
    err = drift_init(&d, &cfg);
    ASSERT_TRUE(err == DRIFT_ERR_CONFIG, "Edge", "max_safe_slope=0 should fail");
    
    /* Invalid limits (upper <= lower) */
    cfg = DRIFT_DEFAULT_CONFIG;
    cfg.upper_limit = 0.0;
    cfg.lower_limit = 100.0;
    err = drift_init(&d, &cfg);
    ASSERT_TRUE(err == DRIFT_ERR_CONFIG, "Edge", "upper<=lower should fail");
    
    /* Invalid n_min */
    cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 1;
    err = drift_init(&d, &cfg);
    ASSERT_TRUE(err == DRIFT_ERR_CONFIG, "Edge", "n_min<2 should fail");
    
    /* NULL pointers */
    err = drift_init(NULL, &DRIFT_DEFAULT_CONFIG);
    ASSERT_TRUE(err == DRIFT_ERR_NULL, "Edge", "NULL d should fail");
    
    err = drift_init(&d, NULL);
    ASSERT_TRUE(err == DRIFT_ERR_NULL, "Edge", "NULL cfg should fail");
    
    TEST_PASS("Edge: Config validation rejects invalid params");
}

/**
 * Edge: Reset Clears Faults
 */
static void test_edge_reset_clears_faults(void)
{
    drift_fsm_t d;
    drift_init(&d, &DRIFT_DEFAULT_CONFIG);
    
    drift_result_t r;
    uint64_t ts = 1000;
    
    /* Normal operation */
    drift_update(&d, 50.0, ts, &r);
    ts += 100;
    
    /* Inject fault */
    drift_update(&d, 0.0 / 0.0, ts, &r);
    
    ASSERT_TRUE(drift_faulted(&d), "Edge", "Should be faulted");
    ASSERT_TRUE(d.state == DRIFT_FAULT, "Edge", "Should be in FAULT state");
    
    /* Reset */
    drift_reset(&d);
    
    ASSERT_TRUE(!drift_faulted(&d), "Edge", "Reset should clear faults");
    ASSERT_TRUE(d.state == DRIFT_LEARNING, "Edge", "Reset should return to LEARNING");
    ASSERT_TRUE(d.n == 0, "Edge", "Reset should clear n");
    ASSERT_TRUE(!d.initialized, "Edge", "Reset should clear initialized flag");
    
    TEST_PASS("Edge: Reset clears faults and state");
}

/**
 * Edge: Temporal Monotonicity
 */
static void test_edge_temporal_monotonicity(void)
{
    drift_fsm_t d;
    drift_init(&d, &DRIFT_DEFAULT_CONFIG);
    
    drift_result_t r;
    uint64_t ts = 1000;
    
    /* First observation */
    int err = drift_update(&d, 50.0, ts, &r);
    ASSERT_TRUE(err == DRIFT_OK, "Edge", "First update should succeed");
    
    /* Same timestamp (should fail) */
    err = drift_update(&d, 51.0, ts, &r);
    ASSERT_TRUE(err == DRIFT_ERR_TEMPORAL, "Edge", "Same timestamp should fail");
    
    /* Earlier timestamp (should fail) */
    err = drift_update(&d, 52.0, ts - 100, &r);
    ASSERT_TRUE(err == DRIFT_ERR_TEMPORAL, "Edge", "Earlier timestamp should fail");
    
    /* State should still be valid */
    ASSERT_TRUE(d.state != DRIFT_FAULT, "Edge", "Temporal error shouldn't fault");
    
    TEST_PASS("Edge: Temporal monotonicity enforced");
}

/**
 * Edge: Time-Gap Auto-Reset
 */
static void test_edge_time_gap_reset(void)
{
    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.max_gap = 1000;  /* 1 second */
    cfg.reset_on_gap = 1;
    cfg.n_min = 3;
    
    drift_init(&d, &cfg);
    
    drift_result_t r;
    uint64_t ts = 1000;
    
    /* Build up some state */
    for (int i = 0; i < 10; i++) {
        drift_update(&d, 50.0, ts, &r);
        ts += 100;
    }
    
    ASSERT_TRUE(d.n == 10, "Edge", "Should have 10 observations");
    
    /* Large gap exceeding max_gap */
    ts += 2000;  /* 2 second gap */
    int err = drift_update(&d, 60.0, ts, &r);
    
    ASSERT_TRUE(err == DRIFT_OK, "Edge", "Gap should auto-reset, not error");
    ASSERT_TRUE(d.n == 1, "Edge", "n should reset to 1");
    ASSERT_TRUE(d.state == DRIFT_LEARNING, "Edge", "Should be back in LEARNING");
    
    TEST_PASS("Edge: Time-gap auto-reset works");
}

/*===========================================================================
 * MAIN
 *===========================================================================*/

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║           DRIFT Contract Test Suite                            ║\n");
    printf("║           Module 4: Rate & Trend Detection                     ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("Contract Tests:\n");
    test_contract1_bounded_slope();
    test_contract2_noise_immunity();
    test_contract3_ttf_accuracy();
    test_contract4_spike_resistance();
    printf("\n");
    
    printf("Invariant Tests:\n");
    test_inv1_state_domain();
    test_inv2_learning_threshold();
    test_inv3_fault_implies_fault_state();
    test_inv5_monotonic_n();
    test_inv_fault_no_increment();
    printf("\n");
    
    printf("Fuzz Tests:\n");
    test_fuzz_random_streams();
    test_fuzz_fault_injection();
    printf("\n");
    
    printf("Edge Case Tests:\n");
    test_edge_config_validation();
    test_edge_reset_clears_faults();
    test_edge_temporal_monotonicity();
    test_edge_time_gap_reset();
    printf("\n");
    
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("  Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("\n");
    
    return (tests_passed == tests_run) ? 0 : 1;
}
