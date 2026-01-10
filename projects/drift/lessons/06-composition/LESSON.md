# c-from-scratch — Module 4: Drift

## Lesson 6: Composition, Guarantees, and the Path Forward

---

## What We Built

### Module 1 — Pulse
**Proves existence in time.**
- Input: Timestamps
- Output: ALIVE / DEAD
- Guarantees: Bounded detection latency

### Module 2 — Baseline
**Proves normality in value.**
- Input: Scalar values
- Output: STABLE / DEVIATION + z-score
- Guarantees: Convergence, sensitivity, spike resistance

### Module 3 — Timing
**Proves health over time.**
- Input: Events
- Output: HEALTHY / UNHEALTHY
- Guarantees: Composition of existence + normality

### Module 4 — Drift
**Proves velocity toward failure.**
- Input: Values + timestamps
- Output: STABLE / DRIFTING + slope + TTF
- Guarantees: Noise immunity, bounded detection, spike resistance

---

## Why Composition Matters

All modules share these properties:

| Property | Meaning |
|----------|---------|
| Closed | State depends only on previous state + input |
| Total | Always returns a valid result |
| Bounded | O(1) memory, O(1) compute |
| Deterministic | Same inputs → same outputs |
| Contract-defined | Behaviour is specified, not implied |

**This makes composition safe.**

---

## The Drift Composition Pipeline

```
sensor_value → Drift → (slope, TTF) → Decision
```

Combined with other modules:

```
sensor → Baseline (is value normal?)
      ↘
        → Combined Health
      ↗
sensor → Drift (is value trending toward failure?)
```

### What You Can Detect

| Failure Mode | Detected By |
|--------------|-------------|
| Value out of range | Baseline |
| Value trending to limit | Drift |
| Heartbeat stopped | Pulse |
| Timing anomaly | Timing |
| Silent drift | Drift |
| Sudden spike | Baseline + Drift |

---

## The Safety Audit Challenge

For Module 4, challenge your contributors:

> **"Can you find a way to make the TTF calculation return a deceptive value during a rapid direction change?"**

Consider:
- What happens when slope crosses zero?
- What if upper_limit == current value?
- What if dt is very small?

This is how we harden safety-critical code.

---

## Where Drift Scales Next

### Application 1: Thermal Runaway Detection
```c
drift_config_t battery_cfg = {
    .alpha = 0.2,
    .max_safe_slope = 0.1,  /* °C/second */
    .upper_limit = 60.0,    /* Critical temp */
    .lower_limit = -20.0
};
```

### Application 2: Predictive Maintenance
Monitor bearing vibration. When slope indicates drift toward failure threshold, schedule maintenance before breakdown.

### Application 3: Battery SoC Estimation
Track discharge rate to predict remaining runtime.

### Application 4: Network Latency Monitoring
Detect when response times are trending toward SLA violation.

---

## The Complete Module Map

```
┌─────────────────────────────────────────────────────────────────┐
│                    MODULE 4: DRIFT                              │
├─────────────────────────────────────────────────────────────────┤
│  Lesson 1: The Problem                                          │
│  ├── Why thresholds miss "silent drift"                         │
│  ├── Why raw slope amplifies noise                              │
│  └── The damped derivative insight                              │
│                                                                 │
│  Lesson 2: The Mathematical Model                               │
│  ├── State: (slope, x_{t-1}, t_{t-1}, n, q)                    │
│  ├── Damped derivative: slope = α·raw + (1-α)·slope            │
│  └── TTF calculation                                            │
│                                                                 │
│  Lesson 3: Structs & State Encoding                             │
│  ├── drift_fsm_t field-by-field                                 │
│  ├── What is NOT stored                                         │
│  └── Configuration and result structures                        │
│                                                                 │
│  Lesson 4: Code as Mathematical Transcription                   │
│  ├── Temporal validation (monotonic time-gate)                  │
│  ├── Time-gap protection                                        │
│  └── EMA implementation                                         │
│                                                                 │
│  Lesson 5: Testing & Hardening                                  │
│  ├── Contract tests (bounded slope, noise immunity)             │
│  ├── Invariant tests                                            │
│  └── Fuzz tests (100k+ observations)                            │
│                                                                 │
│  Lesson 6: Composition & Applications                           │
│  ├── Combined health monitoring                                 │
│  └── Predictive maintenance use cases                           │
├─────────────────────────────────────────────────────────────────┤
│  DELIVERABLES                                                   │
│  ├── drift.h      (API + contracts)                             │
│  ├── drift.c      (implementation)                              │
│  ├── main.c       (demo)                                        │
│  └── test_drift.c (proof harness)                               │
└─────────────────────────────────────────────────────────────────┘
```

---

## The Framework Evolution

```
Phase 1: "The William Show" (Modules 1-3)     ✓ Complete
Phase 2: "The Framework" (Modules 4-6 + SPEC) ← Module 4 done
Phase 3: "The Ecosystem" (Contributors build)
Phase 4: "The Standard" (Industry adoption)
```

### Next: Module 5 — Consensus

> **"Which sensor should I trust?"**

When you have three temperature sensors and they disagree:
- Sensor A: 45°C
- Sensor B: 47°C
- Sensor C: 85°C (faulty)

How do you decide? Module 5 implements voting algorithms, TMR, and fault-tolerant averaging.

---

## Final Takeaway

> **Good systems don't guess. They prove.**

Module 4 proves velocity toward failure using damped derivatives.

Together with Baseline (normality) and Timing (health), you now have a complete toolkit for:
- Is the value normal? (Baseline)
- Is it moving toward failure? (Drift)
- Is the system alive and responsive? (Pulse, Timing)

**End of Module 4: Drift — Rate & Trend Detection**
