# c-from-scratch — Module 4: Drift

## Lesson 4: Code as Mathematical Transcription

> "Good code is not clever. Correct code is inevitable."

---

## Core Idea

> The code is not an implementation choice. It is the **only possible implementation** that satisfies the contracts.

Every line exists because removing it would violate a guarantee.

---

## The Shape of drift.c

At the highest level, `drift.c` has exactly these responsibilities:

1. Reject invalid configurations
2. Detect and isolate faults
3. Handle first observation (no slope yet)
4. Validate temporal constraints
5. Update state (damped derivative)
6. Compute TTF
7. Apply FSM transitions

**Nothing else.**

---

## 1. Total Functions: No Undefined Behaviour

**Every public function must be total.**

```c
int drift_update(drift_fsm_t *d, double value, uint64_t timestamp,
                 drift_result_t *result)
```

This function is total:
- Always returns a valid error code
- Always populates result (if not NULL)
- Never divides by zero
- Never reads uninitialised memory
- Never leaves the FSM inconsistent

---

## 2. Reentrancy Guard (INV-4)

```c
if (d->in_step) {
    d->fault_reentry = 1;
    d->state = DRIFT_FAULT;
    return DRIFT_ERR_FAULT;
}
d->in_step = 1;
/* ... work ... */
d->in_step = 0;
```

This enforces atomicity: no concurrent execution.

---

## 3. Input Validation (INV-3)

```c
if (!isfinite(value)) {
    d->fault_fp = 1;
    d->state = DRIFT_FAULT;
    d->in_step = 0;
    return DRIFT_ERR_DOMAIN;
}
```

Faults are semantic failures. NaN or Inf must not silently corrupt state.

---

## 4. First Observation Handling

```c
if (!d->initialized) {
    d->last_value = value;
    d->last_time = timestamp;
    d->initialized = 1;
    d->n = 1;
    /* No slope computation possible yet */
    d->in_step = 0;
    return DRIFT_OK;
}
```

We need two points to compute a slope. The first observation just stores data.

---

## 5. Temporal Validation (Monotonic Time-Gate)

```c
if (timestamp <= d->last_time) {
    d->in_step = 0;
    return DRIFT_ERR_TEMPORAL;
}
```

Timestamps must be strictly increasing. Out-of-order or duplicate timestamps indicate clock problems.

---

## 6. Time-Gap Protection

```c
uint64_t delta_t = timestamp - d->last_time;

if (delta_t > d->cfg.max_gap) {
    if (d->cfg.reset_on_gap) {
        /* Auto-reset and continue */
        drift_reset(d);
        d->last_value = value;
        d->last_time = timestamp;
        d->n = 1;
        d->in_step = 0;
        return DRIFT_OK;
    } else {
        d->in_step = 0;
        return DRIFT_ERR_TEMPORAL;
    }
}
```

Large gaps make the EMA state stale. Either reset or reject.

---

## 7. The Core Update (Damped Derivative)

```c
/* From Lesson 2 mathematical model */
double dt = (double)delta_t;
double dx = value - d->last_value;

/* raw_slope = Δx / Δt */
double raw_slope = dx / dt;

/* Check for overflow */
if (!isfinite(raw_slope)) {
    d->fault_overflow = 1;
    d->state = DRIFT_FAULT;
    d->in_step = 0;
    return DRIFT_ERR_OVERFLOW;
}

/* slope_t = α · raw_slope + (1-α) · slope_{t-1} */
double new_slope = (d->cfg.alpha * raw_slope) + 
                   ((1.0 - d->cfg.alpha) * d->slope);

if (!isfinite(new_slope)) {
    d->fault_overflow = 1;
    d->state = DRIFT_FAULT;
    d->in_step = 0;
    return DRIFT_ERR_OVERFLOW;
}

d->slope = new_slope;
```

**This is a literal transcription of the math.** No reordering, no "optimisation."

---

## 8. TTF Calculation

```c
double ttf = INFINITY;
uint8_t has_ttf = 0;

if (d->slope > d->cfg.min_slope_for_ttf) {
    /* Drifting toward upper limit */
    double distance = d->cfg.upper_limit - value;
    if (distance > 0) {
        ttf = distance / d->slope;
        has_ttf = isfinite(ttf) && ttf > 0;
    }
} else if (d->slope < -d->cfg.min_slope_for_ttf) {
    /* Drifting toward lower limit */
    double distance = value - d->cfg.lower_limit;
    if (distance > 0) {
        ttf = distance / fabs(d->slope);
        has_ttf = isfinite(ttf) && ttf > 0;
    }
}

d->ttf = ttf;
```

TTF is only computed when slope is significant and points toward a limit.

---

## 9. FSM Transitions (Separate from Math)

```c
double abs_slope = fabs(d->slope);

switch (d->state) {
    case DRIFT_LEARNING:
        if (d->n >= d->cfg.n_min) {
            if (abs_slope > d->cfg.max_safe_slope) {
                d->state = (d->slope > 0) ? DRIFT_DRIFTING_UP 
                                          : DRIFT_DRIFTING_DOWN;
            } else {
                d->state = DRIFT_STABLE;
            }
        }
        break;

    case DRIFT_STABLE:
        if (d->slope > d->cfg.max_safe_slope) {
            d->state = DRIFT_DRIFTING_UP;
        } else if (d->slope < -d->cfg.max_safe_slope) {
            d->state = DRIFT_DRIFTING_DOWN;
        }
        break;

    case DRIFT_DRIFTING_UP:
        if (d->slope <= d->cfg.max_safe_slope) {
            d->state = DRIFT_STABLE;
        }
        break;

    case DRIFT_DRIFTING_DOWN:
        if (d->slope >= -d->cfg.max_safe_slope) {
            d->state = DRIFT_STABLE;
        }
        break;

    case DRIFT_FAULT:
        /* Stay in FAULT until reset */
        break;
}
```

Each case maps exactly to the transition table from Lesson 2.

---

## 10. Configuration Validation

```c
int drift_init(drift_fsm_t *d, const drift_config_t *cfg)
{
    if (d == NULL || cfg == NULL) return DRIFT_ERR_NULL;
    
    /* C1: 0 < alpha <= 1 */
    if (cfg->alpha <= 0.0 || cfg->alpha > 1.0) 
        return DRIFT_ERR_CONFIG;
    
    /* C2: max_safe_slope > 0 */
    if (cfg->max_safe_slope <= 0.0) 
        return DRIFT_ERR_CONFIG;
    
    /* C3: upper > lower */
    if (cfg->upper_limit <= cfg->lower_limit) 
        return DRIFT_ERR_CONFIG;
    
    /* C4: n_min >= 2 */
    if (cfg->n_min < 2) 
        return DRIFT_ERR_CONFIG;
    
    /* ... */
}
```

**Invalid configurations are rejected at init, not discovered at runtime.**

---

## Key Takeaway

> This file is not "C code that implements EMA."
> It is a **mathematical proof that happens to compile**.

---

## Bridge to Lesson 5

Lesson 4 answered: "How do we write code that must be correct?"

Lesson 5 answers: "How do we prove that it actually is?"

That is where the proof harness begins.
