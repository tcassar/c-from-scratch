/**
 * test_consensus.c - Contract and Invariant Test Suite
 * 
 * This is not a unit test file. This is a proof harness.
 * Each test demonstrates a theorem, not just exercises an API.
 * 
 * Contract Tests:
 *   CONTRACT-1: Single fault tolerance
 *   CONTRACT-2: Bounded output
 *   CONTRACT-3: Deterministic voting
 *   CONTRACT-4: Degradation awareness
 * 
 * Invariant Tests:
 *   INV-1: State domain
 *   INV-2: Agreement implies conditions
 *   INV-3: No quorum implies few sensors
 *   INV-4: Fault implies FAULT state
 * 
 * Byzantine Fault Tests:
 *   Subtle liars, timing attacks, value manipulation
 * 
 * Copyright (c) 2026 William Murray
 * MIT License - https://github.com/williamofai/c-from-scratch
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "consensus.h"

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

static int double_eq(double a, double b) {
    return fabs(a - b) < EPSILON;
}

/*===========================================================================
 * CONTRACT TESTS
 *===========================================================================*/

/**
 * CONTRACT-1: Single Fault Tolerance
 * 
 * One faulty sensor does not corrupt the output.
 */
static void test_contract1_single_fault_tolerance(void)
{
    consensus_fsm_t c;
    consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

    /* S2 is a massive outlier */
    sensor_input_t inputs[3] = {
        { .value = 100.0, .health = SENSOR_HEALTHY },
        { .value = 100.2, .health = SENSOR_HEALTHY },
        { .value = 99999.0, .health = SENSOR_HEALTHY }  /* Liar */
    };

    consensus_result_t r;
    consensus_update(&c, inputs, &r);

    /* Mid-value selection should give 100.2 (median) */
    ASSERT_TRUE(fabs(r.value - 100.2) < 0.01, "CONTRACT-1",
               "Consensus should be median, ignoring outlier");
    ASSERT_TRUE(r.value < 200.0, "CONTRACT-1",
               "Output must be bounded by sane sensors");

    TEST_PASS("CONTRACT-1: Single fault tolerance (outlier ignored)");
}

/**
 * CONTRACT-1b: Single Fault Tolerance with Negative Outlier
 */
static void test_contract1b_negative_outlier(void)
{
    consensus_fsm_t c;
    consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

    /* S0 is a massive negative outlier */
    sensor_input_t inputs[3] = {
        { .value = -99999.0, .health = SENSOR_HEALTHY },  /* Liar */
        { .value = 50.0, .health = SENSOR_HEALTHY },
        { .value = 50.5, .health = SENSOR_HEALTHY }
    };

    consensus_result_t r;
    consensus_update(&c, inputs, &r);

    ASSERT_TRUE(r.value > 0.0, "CONTRACT-1b",
               "Negative outlier should be ignored");
    ASSERT_TRUE(fabs(r.value - 50.0) < 1.0, "CONTRACT-1b",
               "Consensus should be near healthy values");

    TEST_PASS("CONTRACT-1b: Negative outlier ignored");
}

/**
 * CONTRACT-2: Bounded Output
 * 
 * Consensus is always within the range of healthy inputs.
 */
static void test_contract2_bounded_output(void)
{
    consensus_fsm_t c;
    consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

    srand(42);
    for (int trial = 0; trial < 100; trial++) {
        double v0 = (rand() % 1000) / 10.0;
        double v1 = (rand() % 1000) / 10.0;
        double v2 = (rand() % 1000) / 10.0;

        sensor_input_t inputs[3] = {
            { .value = v0, .health = SENSOR_HEALTHY },
            { .value = v1, .health = SENSOR_HEALTHY },
            { .value = v2, .health = SENSOR_HEALTHY }
        };

        consensus_result_t r;
        consensus_update(&c, inputs, &r);

        double min_val = fmin(v0, fmin(v1, v2));
        double max_val = fmax(v0, fmax(v1, v2));

        ASSERT_TRUE(r.value >= min_val - EPSILON, "CONTRACT-2",
                   "Consensus must be >= min(inputs)");
        ASSERT_TRUE(r.value <= max_val + EPSILON, "CONTRACT-2",
                   "Consensus must be <= max(inputs)");
    }

    TEST_PASS("CONTRACT-2: Bounded output (100 random trials)");
}

/**
 * CONTRACT-3: Deterministic Voting
 * 
 * Same inputs always produce the same output.
 */
static void test_contract3_deterministic(void)
{
    sensor_input_t inputs[3] = {
        { .value = 10.0, .health = SENSOR_HEALTHY },
        { .value = 20.0, .health = SENSOR_HEALTHY },
        { .value = 15.0, .health = SENSOR_HEALTHY }
    };

    double results[10];
    for (int i = 0; i < 10; i++) {
        consensus_fsm_t c;
        consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

        consensus_result_t r;
        consensus_update(&c, inputs, &r);
        results[i] = r.value;
    }

    /* All results must be identical */
    for (int i = 1; i < 10; i++) {
        ASSERT_TRUE(double_eq(results[i], results[0]), "CONTRACT-3",
                   "Same inputs must produce same output");
    }

    TEST_PASS("CONTRACT-3: Deterministic (10 identical trials)");
}

/**
 * CONTRACT-4: Degradation Awareness
 * 
 * Confidence decreases with fewer healthy sensors.
 */
static void test_contract4_degradation_awareness(void)
{
    /* All 3 healthy */
    consensus_fsm_t c1;
    consensus_init(&c1, &CONSENSUS_DEFAULT_CONFIG);
    sensor_input_t inputs1[3] = {
        { .value = 50.0, .health = SENSOR_HEALTHY },
        { .value = 50.1, .health = SENSOR_HEALTHY },
        { .value = 50.2, .health = SENSOR_HEALTHY }
    };
    consensus_result_t r1;
    consensus_update(&c1, inputs1, &r1);
    double conf_3 = r1.confidence;

    /* Only 2 healthy */
    consensus_fsm_t c2;
    consensus_init(&c2, &CONSENSUS_DEFAULT_CONFIG);
    sensor_input_t inputs2[3] = {
        { .value = 50.0, .health = SENSOR_HEALTHY },
        { .value = 50.1, .health = SENSOR_HEALTHY },
        { .value = 999.0, .health = SENSOR_FAULTY }
    };
    consensus_result_t r2;
    consensus_update(&c2, inputs2, &r2);
    double conf_2 = r2.confidence;

    ASSERT_TRUE(conf_3 > conf_2, "CONTRACT-4",
               "Confidence must decrease with fewer healthy sensors");
    ASSERT_TRUE(r2.state == CONSENSUS_DEGRADED, "CONTRACT-4",
               "State must be DEGRADED with 2 sensors");

    TEST_PASS("CONTRACT-4: Degradation awareness (confidence decreases)");
}

/*===========================================================================
 * INVARIANT TESTS
 *===========================================================================*/

/**
 * INV-1: State Domain
 */
static void test_inv1_state_domain(void)
{
    consensus_fsm_t c;
    consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

    ASSERT_TRUE(c.state >= CONSENSUS_INIT && c.state <= CONSENSUS_FAULT,
               "INV-1", "Initial state must be valid");

    srand(123);
    for (int i = 0; i < 100; i++) {
        sensor_input_t inputs[3];
        for (int j = 0; j < 3; j++) {
            inputs[j].value = (rand() % 1000) / 10.0;
            inputs[j].health = rand() % 3;  /* HEALTHY, DEGRADED, or FAULTY */
        }

        consensus_result_t r;
        consensus_update(&c, inputs, &r);

        ASSERT_TRUE(c.state >= CONSENSUS_INIT && c.state <= CONSENSUS_FAULT,
                   "INV-1", "State must remain valid");
    }

    TEST_PASS("INV-1: State always in valid domain");
}

/**
 * INV-2: Agreement implies sufficient sensors and low spread
 */
static void test_inv2_agree_conditions(void)
{
    consensus_fsm_t c;
    consensus_config_t cfg = CONSENSUS_DEFAULT_CONFIG;
    cfg.max_deviation = 1.0;
    consensus_init(&c, &cfg);

    sensor_input_t inputs[3] = {
        { .value = 50.0, .health = SENSOR_HEALTHY },
        { .value = 50.3, .health = SENSOR_HEALTHY },
        { .value = 50.1, .health = SENSOR_HEALTHY }
    };

    consensus_result_t r;
    consensus_update(&c, inputs, &r);

    if (c.state == CONSENSUS_AGREE) {
        ASSERT_TRUE(r.active_sensors >= 2, "INV-2",
                   "AGREE requires >= 2 active sensors");
        ASSERT_TRUE(r.spread <= cfg.max_deviation, "INV-2",
                   "AGREE requires spread <= max_deviation");
    }

    TEST_PASS("INV-2: AGREE implies conditions met");
}

/**
 * INV-3: No quorum implies few sensors
 */
static void test_inv3_no_quorum(void)
{
    consensus_fsm_t c;
    consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

    sensor_input_t inputs[3] = {
        { .value = 50.0, .health = SENSOR_HEALTHY },
        { .value = 0.0, .health = SENSOR_FAULTY },
        { .value = 0.0, .health = SENSOR_FAULTY }
    };

    consensus_result_t r;
    int err = consensus_update(&c, inputs, &r);

    ASSERT_TRUE(c.state == CONSENSUS_NO_QUORUM, "INV-3",
               "Must be NO_QUORUM with only 1 healthy");
    ASSERT_TRUE(err == CONSENSUS_ERR_QUORUM, "INV-3",
               "Must return ERR_QUORUM");
    ASSERT_TRUE(r.active_sensors < 2, "INV-3",
               "Active sensors must be < 2");

    TEST_PASS("INV-3: NO_QUORUM implies < 2 healthy sensors");
}

/**
 * INV-4: Fault detection
 */
static void test_inv4_reentrancy_fault(void)
{
    /* Note: Reentrancy is hard to test without threads.
     * We test the fault flag mechanism instead. */
    
    consensus_fsm_t c;
    consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

    /* Manually set fault flag to test behavior */
    c.fault_reentry = 1;
    c.state = CONSENSUS_FAULT;

    sensor_input_t inputs[3] = {
        { .value = 50.0, .health = SENSOR_HEALTHY },
        { .value = 50.1, .health = SENSOR_HEALTHY },
        { .value = 50.2, .health = SENSOR_HEALTHY }
    };

    consensus_result_t r;
    int err = consensus_update(&c, inputs, &r);

    ASSERT_TRUE(err == CONSENSUS_ERR_FAULT, "INV-4",
               "Faulted module should return ERR_FAULT");
    ASSERT_TRUE(consensus_faulted(&c), "INV-4",
               "Fault flag should be set");

    /* Reset should clear */
    consensus_reset(&c);
    ASSERT_TRUE(!consensus_faulted(&c), "INV-4",
               "Reset should clear faults");

    TEST_PASS("INV-4: Fault flags work correctly");
}

/*===========================================================================
 * BYZANTINE FAULT TESTS
 *===========================================================================*/

/**
 * Byzantine: Subtle Liar (Slow Drift)
 */
static void test_byzantine_slow_drift(void)
{
    consensus_fsm_t c;
    consensus_config_t cfg = CONSENSUS_DEFAULT_CONFIG;
    cfg.max_deviation = 5.0;
    consensus_init(&c, &cfg);

    double ground_truth = 100.0;
    double max_error = 0.0;

    for (int step = 0; step < 50; step++) {
        /* S0 and S1 follow truth */
        double s0 = ground_truth + (step % 2) * 0.1;
        double s1 = ground_truth - (step % 2) * 0.1;
        
        /* S2 drifts away slowly */
        double s2 = ground_truth + step * 0.5;

        sensor_input_t inputs[3] = {
            { .value = s0, .health = SENSOR_HEALTHY },
            { .value = s1, .health = SENSOR_HEALTHY },
            { .value = s2, .health = SENSOR_HEALTHY }
        };

        consensus_result_t r;
        consensus_update(&c, inputs, &r);

        double error = fabs(r.value - ground_truth);
        if (error > max_error) max_error = error;
    }

    /* With mid-value selection, error should be bounded by healthy sensors */
    ASSERT_TRUE(max_error < 1.0, "Byzantine",
               "Slow drift should not corrupt consensus significantly");

    TEST_PASS("Byzantine: Slow drift resisted (max error < 1.0)");
}

/**
 * Byzantine: Oscillating Liar
 */
static void test_byzantine_oscillating(void)
{
    consensus_fsm_t c;
    consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

    double ground_truth = 50.0;

    for (int step = 0; step < 100; step++) {
        /* S0 and S1 are stable */
        double s0 = ground_truth;
        double s1 = ground_truth + 0.1;
        
        /* S2 oscillates wildly */
        double s2 = (step % 2 == 0) ? ground_truth + 1000 : ground_truth - 1000;

        sensor_input_t inputs[3] = {
            { .value = s0, .health = SENSOR_HEALTHY },
            { .value = s1, .health = SENSOR_HEALTHY },
            { .value = s2, .health = SENSOR_HEALTHY }
        };

        consensus_result_t r;
        consensus_update(&c, inputs, &r);

        /* Consensus should always be near ground truth */
        double error = fabs(r.value - ground_truth);
        ASSERT_TRUE(error < 1.0, "Byzantine",
                   "Oscillating liar should not affect consensus");
    }

    TEST_PASS("Byzantine: Oscillating liar resisted");
}

/**
 * Byzantine: Two Liars (should fail gracefully)
 */
static void test_byzantine_two_liars(void)
{
    consensus_fsm_t c;
    consensus_config_t cfg = CONSENSUS_DEFAULT_CONFIG;
    cfg.max_deviation = 1.0;
    consensus_init(&c, &cfg);

    /* S0 is honest, S1 and S2 are coordinated liars */
    sensor_input_t inputs[3] = {
        { .value = 100.0, .health = SENSOR_HEALTHY },  /* Honest */
        { .value = 200.0, .health = SENSOR_HEALTHY },  /* Liar */
        { .value = 200.0, .health = SENSOR_HEALTHY }   /* Liar */
    };

    consensus_result_t r;
    consensus_update(&c, inputs, &r);

    /* With 2 liars, consensus may be corrupted - this is expected!
     * TMR only tolerates 1 fault. But state should be DISAGREE. */
    ASSERT_TRUE(r.state == CONSENSUS_DISAGREE, "Byzantine",
               "Two liars should cause DISAGREE state");
    ASSERT_TRUE(r.spread > cfg.max_deviation, "Byzantine",
               "Spread should exceed tolerance");

    TEST_PASS("Byzantine: Two liars detected as DISAGREE");
}

/*===========================================================================
 * EDGE CASE TESTS
 *===========================================================================*/

/**
 * Edge: NaN Input Handling
 */
static void test_edge_nan_input(void)
{
    consensus_fsm_t c;
    consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

    sensor_input_t inputs[3] = {
        { .value = 50.0, .health = SENSOR_HEALTHY },
        { .value = 50.1, .health = SENSOR_HEALTHY },
        { .value = 0.0 / 0.0, .health = SENSOR_HEALTHY }  /* NaN */
    };

    consensus_result_t r;
    int err = consensus_update(&c, inputs, &r);

    /* NaN sensor should be excluded, not crash */
    ASSERT_TRUE(err == CONSENSUS_OK, "Edge",
               "NaN input should be excluded, not error");
    ASSERT_TRUE(r.active_sensors == 2, "Edge",
               "NaN sensor should not count as active");

    TEST_PASS("Edge: NaN input handled gracefully");
}

/**
 * Edge: All Identical Values
 */
static void test_edge_all_identical(void)
{
    consensus_fsm_t c;
    consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

    sensor_input_t inputs[3] = {
        { .value = 42.0, .health = SENSOR_HEALTHY },
        { .value = 42.0, .health = SENSOR_HEALTHY },
        { .value = 42.0, .health = SENSOR_HEALTHY }
    };

    consensus_result_t r;
    consensus_update(&c, inputs, &r);

    ASSERT_TRUE(double_eq(r.value, 42.0), "Edge",
               "Identical inputs should give exact value");
    ASSERT_TRUE(double_eq(r.spread, 0.0), "Edge",
               "Identical inputs should have zero spread");
    ASSERT_TRUE(r.sensors_agree, "Edge",
               "Identical inputs should agree");
    ASSERT_TRUE(double_eq(r.confidence, 1.0), "Edge",
               "Perfect agreement should give confidence 1.0");

    TEST_PASS("Edge: All identical values handled");
}

/**
 * Edge: Config Validation
 */
static void test_edge_config_validation(void)
{
    consensus_fsm_t c;
    consensus_config_t cfg;
    int err;

    /* Invalid max_deviation */
    cfg = CONSENSUS_DEFAULT_CONFIG;
    cfg.max_deviation = 0.0;
    err = consensus_init(&c, &cfg);
    ASSERT_TRUE(err == CONSENSUS_ERR_CONFIG, "Edge",
               "max_deviation=0 should fail");

    cfg.max_deviation = -1.0;
    err = consensus_init(&c, &cfg);
    ASSERT_TRUE(err == CONSENSUS_ERR_CONFIG, "Edge",
               "max_deviation<0 should fail");

    /* Invalid tie_breaker */
    cfg = CONSENSUS_DEFAULT_CONFIG;
    cfg.tie_breaker = 5;
    err = consensus_init(&c, &cfg);
    ASSERT_TRUE(err == CONSENSUS_ERR_CONFIG, "Edge",
               "tie_breaker>2 should fail");

    /* NULL pointers */
    err = consensus_init(NULL, &CONSENSUS_DEFAULT_CONFIG);
    ASSERT_TRUE(err == CONSENSUS_ERR_NULL, "Edge",
               "NULL fsm should fail");

    err = consensus_init(&c, NULL);
    ASSERT_TRUE(err == CONSENSUS_ERR_NULL, "Edge",
               "NULL config should fail");

    TEST_PASS("Edge: Config validation works");
}

/**
 * Edge: Reset Clears State
 */
static void test_edge_reset(void)
{
    consensus_fsm_t c;
    consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

    /* Do some updates */
    sensor_input_t inputs[3] = {
        { .value = 50.0, .health = SENSOR_HEALTHY },
        { .value = 50.1, .health = SENSOR_HEALTHY },
        { .value = 50.2, .health = SENSOR_HEALTHY }
    };
    consensus_result_t r;
    consensus_update(&c, inputs, &r);
    consensus_update(&c, inputs, &r);

    /* Reset */
    consensus_reset(&c);

    ASSERT_TRUE(c.state == CONSENSUS_INIT, "Edge",
               "Reset should return to INIT");
    ASSERT_TRUE(c.n == 0, "Edge",
               "Reset should clear counter");
    ASSERT_TRUE(!consensus_faulted(&c), "Edge",
               "Reset should clear faults");

    TEST_PASS("Edge: Reset clears state correctly");
}

/*===========================================================================
 * FUZZ TESTS
 *===========================================================================*/

/**
 * Fuzz: Random Inputs
 */
static void test_fuzz_random(void)
{
    consensus_fsm_t c;
    consensus_init(&c, &CONSENSUS_DEFAULT_CONFIG);

    srand((unsigned int)time(NULL));

    for (int i = 0; i < 100000; i++) {
        sensor_input_t inputs[3];
        for (int j = 0; j < 3; j++) {
            inputs[j].value = ((double)(rand() % 100000) - 50000) / 100.0;
            inputs[j].health = rand() % 3;
        }

        consensus_result_t r;
        consensus_update(&c, inputs, &r);

        /* Check invariants */
        if (!(c.state >= CONSENSUS_INIT && c.state <= CONSENSUS_FAULT)) {
            TEST_FAIL("Fuzz", "Invalid state during random test");
            return;
        }
    }

    TEST_PASS("Fuzz: 100000 random inputs, invariants held");
}

/*===========================================================================
 * MAIN
 *===========================================================================*/

int main(void)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║           CONSENSUS Contract Test Suite                        ║\n");
    printf("║           Module 5: Triple Modular Redundancy                  ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    printf("Contract Tests:\n");
    test_contract1_single_fault_tolerance();
    test_contract1b_negative_outlier();
    test_contract2_bounded_output();
    test_contract3_deterministic();
    test_contract4_degradation_awareness();
    printf("\n");

    printf("Invariant Tests:\n");
    test_inv1_state_domain();
    test_inv2_agree_conditions();
    test_inv3_no_quorum();
    test_inv4_reentrancy_fault();
    printf("\n");

    printf("Byzantine Fault Tests:\n");
    test_byzantine_slow_drift();
    test_byzantine_oscillating();
    test_byzantine_two_liars();
    printf("\n");

    printf("Edge Case Tests:\n");
    test_edge_nan_input();
    test_edge_all_identical();
    test_edge_config_validation();
    test_edge_reset();
    printf("\n");

    printf("Fuzz Tests:\n");
    test_fuzz_random();
    printf("\n");

    printf("══════════════════════════════════════════════════════════════════\n");
    printf("  Results: %d/%d tests passed\n", tests_passed, tests_run);
    printf("══════════════════════════════════════════════════════════════════\n");
    printf("\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
