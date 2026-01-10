# c-from-scratch — Module 5: Consensus

## Lesson 5: Testing & Byzantine Faults

---

## Contract Tests

### CONTRACT-1: Single Fault Tolerance

```c
/* One sensor is 99999, others are 100 */
inputs[2].value = 99999.0;
consensus_update(&c, inputs, &r);
assert(fabs(r.value - 100.0) < 1.0);  /* Outlier ignored */
```

### CONTRACT-2: Bounded Output

```c
/* Output always within range of healthy inputs */
assert(r.value >= min_healthy);
assert(r.value <= max_healthy);
```

### CONTRACT-3: Deterministic

```c
/* Same inputs → Same output, every time */
for (int i = 0; i < 10; i++) {
    consensus_update(&c, inputs, &r);
    assert(r.value == first_result);
}
```

### CONTRACT-4: Degradation Awareness

```c
/* Confidence drops with fewer sensors */
assert(conf_3_healthy > conf_2_healthy);
```

---

## Byzantine Fault Tests

### Slow Drift Liar

```c
for (int step = 0; step < 50; step++) {
    inputs[2].value = ground_truth + step * 0.5;  /* Drifts */
    consensus_update(&c, inputs, &r);
    assert(fabs(r.value - ground_truth) < 1.0);  /* Still accurate */
}
```

### Oscillating Liar

```c
inputs[2].value = (step % 2) ? +1000 : -1000;
/* MVS always selects median, ignoring wild swings */
```

### Coordinated Two Liars

```c
/* TMR only tolerates 1 fault */
/* Two liars → DISAGREE state (detected, not hidden) */
assert(r.state == CONSENSUS_DISAGREE);
```

---

## Edge Cases

- NaN input → Excluded from voting
- All identical → Exact value, zero spread
- Config validation → Invalid params rejected

---

## Fuzz Testing

```c
for (int i = 0; i < 100000; i++) {
    /* Random values, random health states */
    consensus_update(&c, random_inputs, &r);
    assert(state_is_valid(c.state));
}
```

---

## Bridge to Lesson 6

Next: Applications and integration with other modules.
