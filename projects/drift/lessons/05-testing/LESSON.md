# c-from-scratch — Module 4: Drift

## Lesson 5: Testing & Hardening

---

## Philosophy: Tests as Proofs

Traditional unit tests ask: "Does this input produce this output?"

We ask: "Does this system **always** obey its guarantees?"

| Unit Test | Proof Harness |
|-----------|---------------|
| Example-based | Property-based |
| Tests behaviour | Tests contracts |
| Can pass accidentally | Fails if invariant violated |

**A passing test here corresponds to a theorem holding.**

---

## What We Are Proving

### Contracts

```
CONTRACT-1: Bounded slope → DRIFTING state
CONTRACT-2: Noise immunity (jitter doesn't trigger drift)
CONTRACT-3: TTF accuracy for linear drift
CONTRACT-4: Spike resistance (bounded by α)
```

### Invariants

```
INV-1: State always in valid domain
INV-2: (state ≠ LEARNING) → (n ≥ n_min)
INV-3: Fault flag → FAULT state
INV-4: Atomicity guard
INV-5: n increments monotonically
```

---

## Contract Tests

### CONTRACT-1: Bounded Slope Detection

```c
/* Ramp that exceeds threshold */
for (int i = 0; i < 10; i++) {
    drift_update(&d, value, ts, &r);
    value += 10.0;  /* 10 units per 100ms = 0.1 units/ms */
    ts += 100;
}

/* With max_safe_slope = 0.05, should be DRIFTING */
assert(d.state == DRIFT_DRIFTING_UP);
```

### CONTRACT-2: Noise Immunity

```c
/* Random jitter around constant 50 */
for (int i = 0; i < 100; i++) {
    double jitter = ((rand() % 200) / 100.0) - 1.0;  /* ±1 */
    drift_update(&d, 50.0 + jitter, ts, &r);
    ts += 100;
}

/* Should remain STABLE (noise cancels) */
assert(d.state == DRIFT_STABLE);
assert(fabs(d.slope) < max_safe_slope);
```

### CONTRACT-4: Spike Resistance

```c
/* Establish baseline */
for (int i = 0; i < 20; i++) {
    drift_update(&d, 50.0, ts, &r);
    ts += 100;
}
double slope_before = d.slope;

/* Inject massive spike */
drift_update(&d, 1050.0, ts, &r);  /* +1000 spike */

/* Change should be bounded by α */
double slope_change = fabs(d.slope - slope_before);
double max_change = alpha * (1000.0 / 100.0);  /* α × spike_slope */
assert(slope_change <= max_change * 1.1);  /* 10% tolerance */
```

---

## Invariant Tests

### INV-3: Fault Implies FAULT State

```c
drift_update(&d, 50.0, ts, &r);
ts += 100;

/* Inject NaN */
drift_update(&d, 0.0 / 0.0, ts, &r);

assert(drift_faulted(&d));
assert(d.state == DRIFT_FAULT);
```

### INV-5: Monotonic n

```c
for (int i = 0; i < 50; i++) {
    uint32_t n_before = d.n;
    drift_update(&d, 50.0, ts, &r);
    assert(d.n == n_before + 1);
    ts += 100;
}
```

---

## Fuzz Tests

### Random Streams (100k observations)

```c
for (int i = 0; i < 100000; i++) {
    double value = (rand() % 10000) / 100.0;
    drift_update(&d, value, ts, &r);
    
    /* All invariants must hold */
    assert(d.state >= DRIFT_LEARNING && d.state <= DRIFT_FAULT);
    assert(!drift_faulted(&d) || d.state == DRIFT_FAULT);
    
    ts += 100;
}
```

### Fault Injection (NaN/Inf)

```c
double special[] = { 0.0/0.0, 1.0/0.0, -1.0/0.0 };

for (int i = 0; i < 3; i++) {
    drift_reset(&d);
    drift_update(&d, 50.0, ts, &r);
    ts += 100;
    
    int err = drift_update(&d, special[i], ts, &r);
    assert(err == DRIFT_ERR_DOMAIN || err == DRIFT_ERR_FAULT);
    assert(d.state == DRIFT_FAULT);
}
```

---

## Edge Case Tests

- **Config validation**: Invalid α, limits, n_min
- **Reset clears faults**: FAULT → LEARNING after reset
- **Temporal monotonicity**: Same/earlier timestamp rejected
- **Time-gap handling**: Large gap triggers auto-reset

---

## Hardening Checklist

```
Build Quality:
☐ Compiles with -Wall -Wextra -Werror -pedantic
☐ No dynamic memory allocation
☐ All inputs validated
☐ NaN/Inf handled
☐ Division by zero prevented (via min_slope check)
☐ Overflow checked
☐ Reentrancy detected
☐ Faults sticky until reset

Contract Tests:
☐ CONTRACT-1: Bounded slope detection
☐ CONTRACT-2: Noise immunity
☐ CONTRACT-3: TTF accuracy
☐ CONTRACT-4: Spike resistance

Invariant Tests:
☐ INV-1: State domain
☐ INV-2: Learning threshold
☐ INV-3: Fault implies FAULT
☐ INV-5: Monotonic n

Fuzz Tests:
☐ 100K random observations
☐ Fault injection
☐ Edge cases
```

---

## Key Insight

> **Tests don't prove correctness. Contracts define correctness. Tests verify contracts.**

---

## Bridge to Lesson 6

Lesson 5 answered: "How do we prove the code is correct?"

Lesson 6 answers: "What have we built, and where does it go from here?"
