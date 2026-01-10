# c-from-scratch — Module 4: Drift

## Lesson 3: Structs & State Encoding

---

## Core Principle

> **A struct is not a bag of variables. A struct is a state vector.**

Every field exists because the math requires it. Nothing more. Nothing less.

---

## The Mathematical State (Recap)

```
Sₜ = (slope_t, x_{t-1}, t_{t-1}, n_t, q_t)
```

Your struct must represent **exactly** this state.

---

## Encoding the State Vector in C

```c
typedef struct {
    /* Configuration (immutable after init) */
    drift_config_t cfg;

    /* Mathematical state: Sₜ */
    double       slope;         /* slope_t: EMA of derivative */
    double       last_value;    /* x_{t-1}: previous observation */
    uint64_t     last_time;     /* t_{t-1}: previous timestamp */
    uint32_t     n;             /* n_t: observation count */
    drift_state_t state;        /* q_t: FSM state */

    /* Derived (cached for convenience, not for closure) */
    double       ttf;           /* Last computed TTF */

    /* Initialization flag */
    uint8_t      initialized;   /* Have we seen first observation? */

    /* Fault flags (C reality) */
    uint8_t      fault_fp;      /* NaN/Inf detected */
    uint8_t      fault_reentry; /* Atomicity violation */
    uint8_t      fault_overflow;/* Calculation overflow */

    /* Atomicity guard */
    uint8_t      in_step;
} drift_fsm_t;
```

---

## Field-by-Field Justification

### slope — The Damped Derivative

```c
double slope;
```

- Represents slope_t from the mathematical model
- The EMA accumulator — the "memory" of the filter
- Updated every step via: `slope = α·raw + (1-α)·slope`
- Required for: CONTRACT-1 (bounded detection), CONTRACT-4 (spike resistance)

### last_value — Previous Observation

```c
double last_value;
```

- Represents x_{t-1}
- Required to compute Δx = x_t - x_{t-1}
- Without this, we cannot compute raw_slope

### last_time — Previous Timestamp

```c
uint64_t last_time;
```

- Represents t_{t-1}
- Required to compute Δt = t_t - t_{t-1}
- Also used for temporal validation (monotonicity check)

### n — Observation Count

```c
uint32_t n;
```

- Represents n_t
- Required to know when learning period completes
- Invariant: (state ≠ LEARNING) → (n ≥ n_min)

### state — The FSM State

```c
drift_state_t state;
```

- The discrete state q_t
- Must be explicit for deterministic transitions
- Zero-init yields LEARNING (safe default)

---

## What Is NOT Stored (By Design)

### ❌ raw_slope

```c
double raw_slope;  // NOT STORED
```

Why not?
- It is derived: raw_slope = (x - last_value) / dt
- Not needed for future steps (EMA handles it)
- Storing it breaks minimality

### ❌ History Buffer

```c
double history[N];  // NEVER
```

Why not?
- Violates closure (state depends on unbounded history)
- Violates bounded memory
- The EMA eliminates the need for history

### ❌ Regression Coefficients

```c
double regression_a, regression_b;  // NEVER
```

Why not?
- Would require history to recompute
- Adds complexity without benefit
- EMA is mathematically sufficient

---

## The Configuration Structure

```c
typedef struct {
    double   alpha;             /* EMA smoothing ∈ (0, 1] */
    double   max_safe_slope;    /* Drift threshold */
    double   upper_limit;       /* Physical ceiling */
    double   lower_limit;       /* Physical floor */
    uint32_t n_min;             /* Learning period */
    uint64_t max_gap;           /* Time-gap limit */
    double   min_slope_for_ttf; /* TTF validity threshold */
    uint8_t  reset_on_gap;      /* Auto-reset on gap? */
} drift_config_t;
```

Configuration is embedded, not referenced, ensuring the FSM is self-contained.

---

## The Result Structure

```c
typedef struct {
    double       slope;         /* Current smoothed slope */
    double       raw_slope;     /* Instantaneous slope */
    double       ttf;           /* Time-to-failure estimate */
    double       dt;            /* Time delta */
    drift_state_t state;        /* FSM state after step */
    uint8_t      is_drifting;   /* Convenience flag */
    uint8_t      has_ttf;       /* TTF is valid? */
} drift_result_t;
```

**These are derived values for the caller's convenience.**
They exist in results, not in persistent state.

---

## Invariants as Design Constraints

| Invariant | Enforced By |
|-----------|-------------|
| INV-1: State domain | enum type |
| INV-2: (state ≠ LEARNING) → (n ≥ n_min) | Transition logic |
| INV-3: Fault → FAULT state | Fault handling code |
| INV-4: Atomicity | in_step guard |
| INV-5: initialized flag | First-observation logic |

---

## Zero-Initialisation Safety

```c
drift_fsm_t d = {0};
```

Yields:
- state = DRIFT_LEARNING (enum value 0)
- slope = 0.0
- All fault flags = 0
- initialized = 0

**Zero-initialisation is always safe** (results in LEARNING state).

---

## Bridge to Lesson 4

In the next lesson, we will:

- Translate the update equations into C
- Implement fault handling
- Show how totality shapes every line

**Structs become code. State transitions become functions.**
