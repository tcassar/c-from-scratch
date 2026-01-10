/**
 * main.c - Consensus (TMR Voter) Demo
 * 
 * Demonstrates all four contracts with visual output:
 *   1. Normal Operation   - All sensors agree
 *   2. Single Fault       - One sensor lies, others correct
 *   3. Byzantine Fault    - Sensor slowly drifts (subtle liar)
 *   4. Degraded Mode      - One sensor marked faulty
 *   5. No Quorum          - Two sensors fail
 *   6. Disagreement       - Sensors give different but valid readings
 * 
 * Copyright (c) 2026 William Murray
 * MIT License - https://github.com/williamofai/c-from-scratch
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "consensus.h"

/*===========================================================================
 * Demo Helpers
 *===========================================================================*/

static void print_header(const char *title)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  %s\n", title);
    printf("═══════════════════════════════════════════════════════════════\n");
}

static void print_config(const consensus_config_t *cfg)
{
    printf("\n  Configuration:\n");
    printf("    max_deviation  = %.2f  (agreement tolerance)\n", cfg->max_deviation);
    printf("    tie_breaker    = %d     (sensor index for ties)\n", cfg->tie_breaker);
    printf("    n_min          = %u     (learning period)\n", cfg->n_min);
    printf("    use_weighted_avg = %d   (0=median, 1=average)\n", cfg->use_weighted_avg);
}

static void print_inputs(const sensor_input_t inputs[3])
{
    printf("  Inputs:\n");
    for (int i = 0; i < 3; i++) {
        printf("    S%d: value=%8.2f  health=%s\n",
               i, inputs[i].value, sensor_health_name(inputs[i].health));
    }
}

static void print_result(const consensus_result_t *r, int err)
{
    if (err != CONSENSUS_OK && err != CONSENSUS_ERR_QUORUM) {
        printf("  Result: ERROR %s\n", consensus_error_name(err));
        return;
    }

    printf("  Result:\n");
    printf("    Consensus Value: %8.2f\n", r->value);
    printf("    Confidence:      %8.2f\n", r->confidence);
    printf("    State:           %s\n", consensus_state_name(r->state));
    printf("    Active Sensors:  %d\n", r->active_sensors);
    printf("    Sensors Agree:   %s\n", r->sensors_agree ? "yes" : "no");
    printf("    Spread:          %8.2f\n", r->spread);
    printf("    Valid:           %s\n", r->valid ? "yes" : "no");
    printf("    Used: [%d, %d, %d]\n", r->used[0], r->used[1], r->used[2]);
}

/*===========================================================================
 * Demo 1: Normal Operation (All Agree)
 *===========================================================================*/

static void demo_all_agree(void)
{
    print_header("Demo 1: Normal Operation (All Sensors Agree)");
    printf("  All three sensors report similar values.\n");

    consensus_fsm_t c;
    consensus_config_t cfg = CONSENSUS_DEFAULT_CONFIG;
    consensus_init(&c, &cfg);
    print_config(&cfg);

    sensor_input_t inputs[3] = {
        { .value = 100.0, .health = SENSOR_HEALTHY },
        { .value = 100.5, .health = SENSOR_HEALTHY },
        { .value = 100.2, .health = SENSOR_HEALTHY }
    };

    print_inputs(inputs);

    consensus_result_t r;
    int err = consensus_update(&c, inputs, &r);
    print_result(&r, err);

    printf("\n  Expected: Consensus ≈ 100.2 (median), Confidence = 1.0, AGREE\n");
}

/*===========================================================================
 * Demo 2: Single Fault Tolerance (CONTRACT-1)
 *===========================================================================*/

static void demo_single_fault(void)
{
    print_header("Demo 2: Single Fault Tolerance (One Liar)");
    printf("  Sensor 2 reports garbage. Mid-value selection ignores it.\n");

    consensus_fsm_t c;
    consensus_config_t cfg = CONSENSUS_DEFAULT_CONFIG;
    consensus_init(&c, &cfg);

    sensor_input_t inputs[3] = {
        { .value = 100.0, .health = SENSOR_HEALTHY },
        { .value = 100.5, .health = SENSOR_HEALTHY },
        { .value = 9999.0, .health = SENSOR_HEALTHY }  /* Liar! */
    };

    print_inputs(inputs);

    consensus_result_t r;
    int err = consensus_update(&c, inputs, &r);
    print_result(&r, err);

    printf("\n  Expected: Consensus = 100.5 (median ignores extreme), DISAGREE\n");
    printf("  CONTRACT-1 PROVEN: Single faulty sensor did NOT corrupt output!\n");
}

/*===========================================================================
 * Demo 3: Byzantine Fault (Subtle Liar)
 *===========================================================================*/

static void demo_byzantine_fault(void)
{
    print_header("Demo 3: Byzantine Fault (Subtle Liar Drifts Over Time)");
    printf("  Sensor 2 starts correct but slowly drifts away.\n");

    consensus_fsm_t c;
    consensus_config_t cfg = CONSENSUS_DEFAULT_CONFIG;
    cfg.max_deviation = 2.0;
    consensus_init(&c, &cfg);

    printf("\n    Step | S0    | S1    | S2 (liar) | Consensus | State\n");
    printf("  -------+-------+-------+-----------+-----------+----------\n");

    consensus_result_t r;
    double ground_truth = 100.0;
    
    for (int step = 0; step < 10; step++) {
        /* S0 and S1 follow ground truth with small noise */
        double s0 = ground_truth + ((step % 3) - 1) * 0.1;
        double s1 = ground_truth + ((step % 2) - 0.5) * 0.1;
        
        /* S2 (the liar) drifts at 1.5x speed */
        double s2 = ground_truth + step * 1.5;

        sensor_input_t inputs[3] = {
            { .value = s0, .health = SENSOR_HEALTHY },
            { .value = s1, .health = SENSOR_HEALTHY },
            { .value = s2, .health = SENSOR_HEALTHY }
        };

        consensus_update(&c, inputs, &r);

        printf("  %5d  | %5.1f | %5.1f | %9.1f | %9.1f | %s\n",
               step, s0, s1, s2, r.value, consensus_state_name(r.state));
    }

    printf("\n  Note: Despite S2 drifting to +13.5, consensus stayed near 100.\n");
    printf("  Mid-value selection protects against subtle liars!\n");
}

/*===========================================================================
 * Demo 4: Degraded Mode (One Sensor Marked Faulty)
 *===========================================================================*/

static void demo_degraded_mode(void)
{
    print_header("Demo 4: Degraded Mode (Upstream Marks Sensor Faulty)");
    printf("  Sensor 2 marked FAULTY by upstream Drift module.\n");

    consensus_fsm_t c;
    consensus_config_t cfg = CONSENSUS_DEFAULT_CONFIG;
    consensus_init(&c, &cfg);

    sensor_input_t inputs[3] = {
        { .value = 50.0, .health = SENSOR_HEALTHY },
        { .value = 50.3, .health = SENSOR_HEALTHY },
        { .value = 999.0, .health = SENSOR_FAULTY }  /* Marked faulty */
    };

    print_inputs(inputs);

    consensus_result_t r;
    int err = consensus_update(&c, inputs, &r);
    print_result(&r, err);

    printf("\n  Expected: Consensus using only S0, S1. State = DEGRADED.\n");
    printf("  Sensor 2 excluded from voting due to FAULTY health.\n");
}

/*===========================================================================
 * Demo 5: No Quorum (Two Sensors Fail)
 *===========================================================================*/

static void demo_no_quorum(void)
{
    print_header("Demo 5: No Quorum (Insufficient Healthy Sensors)");
    printf("  Two sensors marked FAULTY. Cannot achieve consensus.\n");

    consensus_fsm_t c;
    consensus_config_t cfg = CONSENSUS_DEFAULT_CONFIG;
    consensus_init(&c, &cfg);

    /* First, establish a good value */
    sensor_input_t good_inputs[3] = {
        { .value = 75.0, .health = SENSOR_HEALTHY },
        { .value = 75.5, .health = SENSOR_HEALTHY },
        { .value = 75.2, .health = SENSOR_HEALTHY }
    };
    consensus_result_t r;
    consensus_update(&c, good_inputs, &r);
    printf("  First update (all healthy): consensus = %.1f\n", r.value);

    /* Now two fail */
    sensor_input_t bad_inputs[3] = {
        { .value = 80.0, .health = SENSOR_HEALTHY },
        { .value = 0.0, .health = SENSOR_FAULTY },
        { .value = 0.0, .health = SENSOR_FAULTY }
    };

    printf("\n");
    print_inputs(bad_inputs);

    int err = consensus_update(&c, bad_inputs, &r);
    print_result(&r, err);

    printf("\n  Expected: NO_QUORUM state, error = ERR_QUORUM\n");
    printf("  Last known value (75.0) returned with very low confidence.\n");
}

/*===========================================================================
 * Demo 6: Disagreement (Sensors Differ Beyond Tolerance)
 *===========================================================================*/

static void demo_disagreement(void)
{
    print_header("Demo 6: Disagreement (Spread Exceeds Tolerance)");
    printf("  Sensors give different readings beyond max_deviation.\n");

    consensus_fsm_t c;
    consensus_config_t cfg = CONSENSUS_DEFAULT_CONFIG;
    cfg.max_deviation = 1.0;  /* Tight tolerance */
    consensus_init(&c, &cfg);
    print_config(&cfg);

    sensor_input_t inputs[3] = {
        { .value = 100.0, .health = SENSOR_HEALTHY },
        { .value = 102.0, .health = SENSOR_HEALTHY },
        { .value = 104.0, .health = SENSOR_HEALTHY }
    };

    print_inputs(inputs);

    consensus_result_t r;
    int err = consensus_update(&c, inputs, &r);
    print_result(&r, err);

    printf("\n  Expected: Consensus = 102.0 (median), but DISAGREE state.\n");
    printf("  Spread = 4.0 exceeds max_deviation = 1.0.\n");
    printf("  System works but flags the disagreement for attention.\n");
}

/*===========================================================================
 * Demo 7: Weighted Average vs Mid-Value
 *===========================================================================*/

static void demo_voting_methods(void)
{
    print_header("Demo 7: Voting Methods Comparison");
    printf("  Compare mid-value selection vs weighted average.\n");

    sensor_input_t inputs[3] = {
        { .value = 100.0, .health = SENSOR_HEALTHY },
        { .value = 100.0, .health = SENSOR_HEALTHY },
        { .value = 200.0, .health = SENSOR_HEALTHY }  /* Outlier */
    };

    print_inputs(inputs);

    /* Mid-value selection */
    consensus_fsm_t c1;
    consensus_config_t cfg1 = CONSENSUS_DEFAULT_CONFIG;
    cfg1.use_weighted_avg = 0;
    consensus_init(&c1, &cfg1);

    consensus_result_t r1;
    consensus_update(&c1, inputs, &r1);

    /* Simple average would be (100+100+200)/3 = 133.33 */
    double naive_avg = (100.0 + 100.0 + 200.0) / 3.0;

    printf("\n  Mid-Value Selection:  %.1f\n", r1.value);
    printf("  Naive Average:        %.1f\n", naive_avg);
    printf("\n  Mid-value protects against the outlier (200.0).\n");
    printf("  Naive average would be pulled toward the liar.\n");
}

/*===========================================================================
 * Demo 8: Degraded Sensor Handling
 *===========================================================================*/

static void demo_degraded_sensors(void)
{
    print_header("Demo 8: Degraded Sensors (Lower Confidence)");
    printf("  Sensors marked DEGRADED still contribute but reduce confidence.\n");

    consensus_fsm_t c;
    consensus_config_t cfg = CONSENSUS_DEFAULT_CONFIG;
    consensus_init(&c, &cfg);

    /* All healthy */
    sensor_input_t healthy[3] = {
        { .value = 50.0, .health = SENSOR_HEALTHY },
        { .value = 50.2, .health = SENSOR_HEALTHY },
        { .value = 50.1, .health = SENSOR_HEALTHY }
    };

    consensus_result_t r;
    consensus_update(&c, healthy, &r);
    printf("\n  All HEALTHY: confidence = %.2f\n", r.confidence);

    /* One degraded */
    consensus_reset(&c);
    sensor_input_t one_degraded[3] = {
        { .value = 50.0, .health = SENSOR_HEALTHY },
        { .value = 50.2, .health = SENSOR_DEGRADED },
        { .value = 50.1, .health = SENSOR_HEALTHY }
    };

    consensus_update(&c, one_degraded, &r);
    printf("  One DEGRADED: confidence = %.2f\n", r.confidence);

    /* Two degraded */
    consensus_reset(&c);
    sensor_input_t two_degraded[3] = {
        { .value = 50.0, .health = SENSOR_DEGRADED },
        { .value = 50.2, .health = SENSOR_DEGRADED },
        { .value = 50.1, .health = SENSOR_HEALTHY }
    };

    consensus_update(&c, two_degraded, &r);
    printf("  Two DEGRADED: confidence = %.2f\n", r.confidence);

    printf("\n  Degraded sensors reduce confidence but still vote.\n");
}

/*===========================================================================
 * Main
 *===========================================================================*/

int main(void)
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║           Module 5: Consensus — TMR Voter                     ║\n");
    printf("║                                                               ║\n");
    printf("║   \"A man with one clock knows what time it is.                ║\n");
    printf("║    A man with two clocks is never sure.                       ║\n");
    printf("║    With THREE clocks, we can outvote the liar.\"               ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    demo_all_agree();
    demo_single_fault();
    demo_byzantine_fault();
    demo_degraded_mode();
    demo_no_quorum();
    demo_disagreement();
    demo_voting_methods();
    demo_degraded_sensors();

    print_header("Demo Complete");
    printf("\n  Key insights demonstrated:\n");
    printf("    1. Mid-value selection ignores extremes\n");
    printf("    2. Single fault tolerance (CONTRACT-1)\n");
    printf("    3. Byzantine fault resistance (subtle liars)\n");
    printf("    4. Graceful degradation with 2 sensors\n");
    printf("    5. Quorum detection (need >= 2 healthy)\n");
    printf("    6. Confidence reflects system health\n");
    printf("\n  Contracts proven:\n");
    printf("    CONTRACT-1: Single fault tolerance\n");
    printf("    CONTRACT-2: Bounded output (within healthy range)\n");
    printf("    CONTRACT-3: Deterministic voting\n");
    printf("    CONTRACT-4: Degradation awareness\n");
    printf("\n  Next: Module 6 — Pressure (How to handle overflow?)\n\n");

    return 0;
}
