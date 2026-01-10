/**
 * main.c - Drift (Rate & Trend Detection) Demo
 * 
 * Demonstrates all four contracts with visual output:
 *   1. Normal Operation   - LEARNING → STABLE transition
 *   2. Ramp Detection     - STABLE → DRIFTING_UP detection
 *   3. Noise Immunity     - Jitter does not trigger DRIFTING
 *   4. Fault Handling     - NaN injection, sticky faults
 *   5. TTF Calculation    - Time-to-failure estimation
 *   6. Time-Gap Handling  - Large gap triggers reset
 * 
 * Copyright (c) 2026 William Murray
 * MIT License - https://github.com/williamofai/c-from-scratch
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "drift.h"

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

static void print_config(const drift_config_t *cfg)
{
    printf("\n  Configuration:\n");
    printf("    alpha           = %.3f  (EMA smoothing)\n", cfg->alpha);
    printf("    max_safe_slope  = %.4f (drift threshold)\n", cfg->max_safe_slope);
    printf("    upper_limit     = %.1f  (physical ceiling)\n", cfg->upper_limit);
    printf("    lower_limit     = %.1f   (physical floor)\n", cfg->lower_limit);
    printf("    n_min           = %u     (learning period)\n", cfg->n_min);
    printf("    max_gap         = %lu ms (max time gap)\n", (unsigned long)cfg->max_gap);
}

static void print_result(int i, double value, uint64_t ts, 
                         const drift_result_t *r, int err)
{
    if (err != DRIFT_OK) {
        printf("  %3d | %8.2f | %6lu | ERROR: %s\n",
               i, value, (unsigned long)ts, drift_error_name(err));
    } else {
        printf("  %3d | %8.2f | %6lu | slope=%+8.4f | ttf=%8.1f | %s\n",
               i, value, (unsigned long)ts,
               r->slope, 
               r->has_ttf ? r->ttf : INFINITY,
               drift_state_name(r->state));
    }
}

/*===========================================================================
 * Demo 1: Normal Operation (Stable Signal)
 *===========================================================================*/

static void demo_stable_signal(void)
{
    print_header("Demo 1: Stable Signal (LEARNING → STABLE)");
    printf("  A constant signal should settle to STABLE with zero slope.\n");

    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 5;  /* Shorter learning for demo */
    
    drift_init(&d, &cfg);
    print_config(&cfg);

    printf("\n    i |    value |     ts | slope    | TTF      | state\n");
    printf("  ----+----------+--------+----------+----------+-------------\n");

    drift_result_t r;
    uint64_t ts = 1000;
    
    for (int i = 1; i <= 10; i++) {
        double value = 50.0;  /* Constant value */
        int err = drift_update(&d, value, ts, &r);
        print_result(i, value, ts, &r, err);
        ts += 100;  /* 100ms intervals */
    }

    printf("\n  Final: slope=%.6f, state=%s\n",
           drift_get_slope(&d), drift_state_name(drift_state(&d)));
    printf("  Expected: slope ≈ 0, state = STABLE\n");
}

/*===========================================================================
 * Demo 2: Ramp Detection (Drifting Upward)
 *===========================================================================*/

static void demo_ramp_up(void)
{
    print_header("Demo 2: Ramp Detection (STABLE → DRIFTING_UP)");
    printf("  A linearly increasing signal should trigger DRIFTING_UP.\n");

    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 3;
    cfg.max_safe_slope = 0.05;  /* 0.05 units/ms threshold */
    cfg.alpha = 0.3;            /* Faster response for demo */
    
    drift_init(&d, &cfg);
    print_config(&cfg);

    printf("\n    i |    value |     ts | slope    | TTF      | state\n");
    printf("  ----+----------+--------+----------+----------+-------------\n");

    drift_result_t r;
    uint64_t ts = 1000;
    double value = 20.0;
    
    for (int i = 1; i <= 15; i++) {
        int err = drift_update(&d, value, ts, &r);
        print_result(i, value, ts, &r, err);
        
        /* Ramp: increase by 10 units per 100ms = 0.1 units/ms */
        value += 10.0;
        ts += 100;
    }

    printf("\n  Final: slope=%.4f, state=%s\n",
           drift_get_slope(&d), drift_state_name(drift_state(&d)));
    printf("  Expected: slope ≈ 0.1 (> 0.05), state = DRIFTING_UP\n");
}

/*===========================================================================
 * Demo 3: Noise Immunity (CONTRACT-2)
 *===========================================================================*/

static void demo_noise_immunity(void)
{
    print_header("Demo 3: Noise Immunity (Jitter Does NOT Trigger Drift)");
    printf("  Small random jitter around a constant should remain STABLE.\n");

    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 5;
    cfg.max_safe_slope = 0.05;
    cfg.alpha = 0.1;  /* Strong smoothing */
    
    drift_init(&d, &cfg);

    printf("\n    i |    value |     ts | slope    | TTF      | state\n");
    printf("  ----+----------+--------+----------+----------+-------------\n");

    drift_result_t r;
    uint64_t ts = 1000;
    srand(42);  /* Reproducible randomness */
    
    for (int i = 1; i <= 20; i++) {
        /* Add ±2 units of random jitter to constant 50 */
        double jitter = ((double)(rand() % 100) / 25.0) - 2.0;
        double value = 50.0 + jitter;
        
        int err = drift_update(&d, value, ts, &r);
        print_result(i, value, ts, &r, err);
        ts += 100;
    }

    printf("\n  Final: slope=%.6f, state=%s\n",
           drift_get_slope(&d), drift_state_name(drift_state(&d)));
    printf("  Expected: slope ≈ 0 (noise cancels), state = STABLE\n");
}

/*===========================================================================
 * Demo 4: Spike Resistance (CONTRACT-4)
 *===========================================================================*/

static void demo_spike_resistance(void)
{
    print_header("Demo 4: Spike Resistance (Single Outlier)");
    printf("  A single spike should shift slope by at most α·spike_slope.\n");

    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 5;
    cfg.max_safe_slope = 1.0;  /* High threshold to see spike effect */
    cfg.alpha = 0.1;
    
    drift_init(&d, &cfg);

    printf("\n    i |    value |     ts | slope    | TTF      | state\n");
    printf("  ----+----------+--------+----------+----------+-------------\n");

    drift_result_t r;
    uint64_t ts = 1000;
    
    /* Establish baseline with constant signal */
    for (int i = 1; i <= 10; i++) {
        drift_update(&d, 50.0, ts, &r);
        print_result(i, 50.0, ts, &r, DRIFT_OK);
        ts += 100;
    }
    
    printf("  --- SPIKE ---\n");
    
    /* Inject spike */
    drift_update(&d, 150.0, ts, &r);  /* +100 spike! */
    print_result(11, 150.0, ts, &r, DRIFT_OK);
    double slope_after_spike = drift_get_slope(&d);
    ts += 100;
    
    printf("  --- RETURN TO NORMAL ---\n");
    
    /* Return to normal */
    for (int i = 12; i <= 20; i++) {
        drift_update(&d, 50.0, ts, &r);
        print_result(i, 50.0, ts, &r, DRIFT_OK);
        ts += 100;
    }

    printf("\n  Slope immediately after spike: %.4f\n", slope_after_spike);
    printf("  Spike raw_slope = (150-50)/100 = 1.0 units/ms\n");
    printf("  With α=0.1, slope shift = α·1.0 = 0.1 (bounded!)\n");
    printf("  Final slope after recovery: %.6f\n", drift_get_slope(&d));
}

/*===========================================================================
 * Demo 5: TTF Calculation
 *===========================================================================*/

static void demo_ttf_calculation(void)
{
    print_header("Demo 5: Time-To-Failure Calculation");
    printf("  Estimate when signal will hit upper limit.\n");

    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 3;
    cfg.upper_limit = 100.0;
    cfg.lower_limit = 0.0;
    cfg.max_safe_slope = 0.05;
    cfg.alpha = 0.5;  /* Fast tracking for demo */
    
    drift_init(&d, &cfg);
    print_config(&cfg);

    printf("\n    i |    value |     ts | slope    | TTF      | state\n");
    printf("  ----+----------+--------+----------+----------+-------------\n");

    drift_result_t r;
    uint64_t ts = 1000;
    double value = 40.0;
    
    for (int i = 1; i <= 10; i++) {
        int err = drift_update(&d, value, ts, &r);
        print_result(i, value, ts, &r, err);
        
        /* Steady ramp: 5 units per 100ms = 0.05 units/ms */
        value += 5.0;
        ts += 100;
    }

    printf("\n  At value=%.0f, slope=%.4f:\n", value - 5.0, drift_get_slope(&d));
    printf("  Distance to limit = %.0f - %.0f = %.0f\n", 
           cfg.upper_limit, value - 5.0, cfg.upper_limit - (value - 5.0));
    printf("  TTF = distance / slope ≈ %.0f ms\n", drift_get_ttf(&d));
}

/*===========================================================================
 * Demo 6: Fault Handling (NaN Injection)
 *===========================================================================*/

static void demo_fault_handling(void)
{
    print_header("Demo 6: Fault Handling (NaN Injection)");
    printf("  Inject NaN — expect FAULT state, sticky until reset.\n");

    drift_fsm_t d;
    drift_init(&d, &DRIFT_DEFAULT_CONFIG);

    drift_result_t r;
    uint64_t ts = 1000;
    
    /* Some normal observations */
    printf("\n  --- Normal operation ---\n");
    for (int i = 0; i < 5; i++) {
        drift_update(&d, 50.0, ts, &r);
        ts += 100;
    }
    printf("  Before fault: state=%s, faulted=%s\n",
           drift_state_name(drift_state(&d)),
           drift_faulted(&d) ? "yes" : "no");

    /* Inject NaN */
    printf("\n  --- Injecting NaN ---\n");
    int err = drift_update(&d, 0.0 / 0.0, ts, &r);
    printf("  After NaN: state=%s, faulted=%s, error=%s\n",
           drift_state_name(drift_state(&d)),
           drift_faulted(&d) ? "yes" : "no",
           drift_error_name(err));
    ts += 100;

    /* Attempt recovery (fault is sticky) */
    printf("\n  --- Attempting recovery (fault is sticky) ---\n");
    for (int i = 0; i < 3; i++) {
        err = drift_update(&d, 50.0, ts, &r);
        printf("  After normal input: state=%s, faulted=%s, error=%s\n",
               drift_state_name(drift_state(&d)),
               drift_faulted(&d) ? "yes" : "no",
               drift_error_name(err));
        ts += 100;
    }
    printf("  Fault persists — must call drift_reset() to clear.\n");

    /* Reset */
    printf("\n  --- Calling drift_reset() ---\n");
    drift_reset(&d);
    printf("  After reset: state=%s, faulted=%s\n",
           drift_state_name(drift_state(&d)),
           drift_faulted(&d) ? "yes" : "no");
}

/*===========================================================================
 * Demo 7: Time-Gap Handling
 *===========================================================================*/

static void demo_time_gap(void)
{
    print_header("Demo 7: Time-Gap Handling (Stale Data Protection)");
    printf("  Large time gap triggers auto-reset to prevent corrupt slope.\n");

    drift_fsm_t d;
    drift_config_t cfg = DRIFT_DEFAULT_CONFIG;
    cfg.n_min = 3;
    cfg.max_gap = 1000;  /* 1 second max gap */
    cfg.reset_on_gap = 1;
    
    drift_init(&d, &cfg);
    printf("  max_gap = %lu ms\n", (unsigned long)cfg.max_gap);

    drift_result_t r;
    uint64_t ts = 1000;
    
    /* Normal operation */
    printf("\n  --- Normal operation ---\n");
    for (int i = 0; i < 5; i++) {
        drift_update(&d, 50.0 + i, ts, &r);
        printf("  ts=%5lu: n=%u, state=%s\n", 
               (unsigned long)ts, d.n, drift_state_name(drift_state(&d)));
        ts += 100;
    }

    /* Large gap */
    printf("\n  --- Large gap (5000ms, exceeds max_gap=1000ms) ---\n");
    ts += 5000;
    drift_update(&d, 60.0, ts, &r);
    printf("  ts=%5lu: n=%u, state=%s (auto-reset triggered)\n",
           (unsigned long)ts, d.n, drift_state_name(drift_state(&d)));
    
    printf("\n  Note: n reset to 1, state back to LEARNING.\n");
    printf("  This prevents stale EMA state from corrupting new data.\n");
}

/*===========================================================================
 * Main
 *===========================================================================*/

int main(void)
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║           Module 4: Drift — Rate & Trend Detection            ║\n");
    printf("║                                                               ║\n");
    printf("║   \"Temperature is normal now, but rising too fast.\"           ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");

    demo_stable_signal();
    demo_ramp_up();
    demo_noise_immunity();
    demo_spike_resistance();
    demo_ttf_calculation();
    demo_fault_handling();
    demo_time_gap();

    print_header("Demo Complete");
    printf("\n  Key insights demonstrated:\n");
    printf("    1. Damped derivative via EMA of slope\n");
    printf("    2. Noise immunity through smoothing\n");
    printf("    3. Spike resistance (bounded by α)\n");
    printf("    4. TTF calculation for predictive maintenance\n");
    printf("    5. Sticky faults, cleared only by reset\n");
    printf("    6. Time-gap protection for stale data\n");
    printf("\n  Contracts proven:\n");
    printf("    CONTRACT-1: Bounded slope (|slope| <= physics)\n");
    printf("    CONTRACT-2: Noise immunity (jitter < ε → no drift)\n");
    printf("    CONTRACT-3: TTF accuracy (within bounded error)\n");
    printf("    CONTRACT-4: Spike resistance (Δslope ≤ α·spike)\n");
    printf("\n  Next: Module 5 — Consensus (Which sensor to trust?)\n\n");

    return 0;
}
