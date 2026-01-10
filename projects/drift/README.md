# Drift — Rate & Trend Detection Monitor

**Module 4 of c-from-scratch**

> Module 3 proved health over time by composing existence and normality.
> Module 4 proves velocity toward failure using damped derivatives.

## What Is This?

A closed, total, deterministic state machine for detecting dangerous rates of change in scalar observation streams. Drift catches "silent failures" where the value is within bounds but moving toward a limit at dangerous velocity.

The core insight: **"Temperature is 45°C and normal"** tells you nothing about whether you'll be at critical in 30 seconds or 30 minutes.

## Contracts (Proven)

```
CONTRACT-1 (Bounded Slope):     |slope| > threshold → DRIFTING state
CONTRACT-2 (Noise Immunity):    Jitter < ε does not trigger DRIFTING
CONTRACT-3 (TTF Accuracy):      Time-to-failure within bounded error
CONTRACT-4 (Spike Resistance):  Single outlier shifts slope by ≤ α·spike
```

## Properties

| Property | Guaranteed |
|----------|------------|
| Memory | O(1) — 5 state variables |
| Time per step | O(1) — fixed operations |
| Determinism | Yes |
| Closure | Yes |
| Recoverability | Yes (via reset) |

## Quick Start

```bash
# Build everything
make

# Run the demo
make demo

# Run contract tests
make test
```

## Project Structure

```
drift/
├── include/
│   └── drift.h         # API and contracts
├── src/
│   ├── drift.c         # Implementation
│   └── main.c          # Demo
├── tests/
│   └── test_drift.c    # Contract test suite
├── lessons/
│   ├── 01-the-problem/
│   ├── 02-mathematical-model/
│   ├── 03-structs/
│   ├── 04-code/
│   ├── 05-testing/
│   └── 06-composition/
├── build/              # Compiled artifacts
├── Makefile
└── README.md
```

## The Damped Derivative

```
raw_slope = (x_t - x_{t-1}) / Δt
slope_t = α · raw_slope + (1 - α) · slope_{t-1}
```

This EMA of the derivative is noise-immune, O(1), and closed.

## API

```c
// Initialize
int drift_init(drift_fsm_t *d, const drift_config_t *cfg);

// Process one observation
int drift_update(drift_fsm_t *d, double value, uint64_t timestamp,
                 drift_result_t *result);

// Reset to initial state
void drift_reset(drift_fsm_t *d);

// Query functions
drift_state_t drift_state(const drift_fsm_t *d);
uint8_t drift_faulted(const drift_fsm_t *d);
uint8_t drift_is_drifting(const drift_fsm_t *d);
double drift_get_slope(const drift_fsm_t *d);
double drift_get_ttf(const drift_fsm_t *d);
```

## States

| State | Meaning |
|-------|---------|
| LEARNING | n < n_min, slope estimate not stable |
| STABLE | \|slope\| ≤ max_safe_slope |
| DRIFTING_UP | slope > max_safe_slope |
| DRIFTING_DOWN | slope < -max_safe_slope |
| FAULT | NaN/Inf or corruption detected |

## Configuration

```c
drift_config_t cfg = {
    .alpha = 0.1,              // EMA smoothing factor
    .max_safe_slope = 0.1,     // Drift threshold (units/ms)
    .upper_limit = 100.0,      // Physical ceiling for TTF
    .lower_limit = 0.0,        // Physical floor for TTF
    .n_min = 5,                // Learning period
    .max_gap = 5000,           // Max time gap (ms)
    .min_slope_for_ttf = 1e-6, // TTF validity threshold
    .reset_on_gap = 1          // Auto-reset on large gaps
};
```

## Test Results

```
╔════════════════════════════════════════════════════════════════╗
║           DRIFT Contract Test Suite                            ║
╚════════════════════════════════════════════════════════════════╝

Contract Tests:
  [PASS] CONTRACT-1: Bounded slope detection
  [PASS] CONTRACT-2: Noise immunity
  [PASS] CONTRACT-3: TTF accuracy
  [PASS] CONTRACT-4: Spike resistance

Invariant Tests:
  [PASS] INV-1: State always in valid domain
  [PASS] INV-2: (state ≠ LEARNING) → (n >= n_min)
  [PASS] INV-3: Fault implies FAULT state
  [PASS] INV-5: n increments monotonically
  [PASS] INV: Faulted input does not increment counter

Fuzz Tests:
  [PASS] Fuzz: 100000 random observations, invariants held
  [PASS] Fuzz: Fault injection handled safely

Results: 15/15 tests passed
```

## Applications

- **Thermal Runaway Detection**: Monitor battery temperature drift
- **Predictive Maintenance**: Track vibration trends toward failure
- **Battery SoC**: Estimate remaining runtime from discharge rate
- **Network Monitoring**: Detect latency trending toward SLA violation

## Safety Audit Challenge

> "Can you find a way to make the TTF calculation return a deceptive value during a rapid direction change?"

## License

MIT License - See LICENSE in repository root.

## Author

William Murray  
Copyright (c) 2026
