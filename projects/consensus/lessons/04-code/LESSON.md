# c-from-scratch — Module 5: Consensus

## Lesson 4: Code as Mathematical Transcription

---

## The Voting Algorithm

```c
/* Mid-Value Selection */
if (healthy_count == 3) {
    sort3(sorted);
    consensus = sorted[1];  /* Median */
}
else if (healthy_count == 2) {
    consensus = (healthy[0] + healthy[1]) / 2.0;
}
```

This is a literal transcription of the math from Lesson 2.

---

## The Complete Update Logic

```c
int consensus_update(consensus_fsm_t *c,
                     const sensor_input_t inputs[3],
                     consensus_result_t *result)
{
    /* 1. Reentrancy guard */
    /* 2. Filter to healthy sensors */
    /* 3. Check quorum (need >= 2) */
    /* 4. Apply MVS voting */
    /* 5. Compute spread and agreement */
    /* 6. Calculate confidence */
    /* 7. FSM transitions */
    /* 8. Store last known good */
}
```

---

## Key Implementation Details

### Filtering Healthy Sensors

```c
for (int i = 0; i < 3; i++) {
    if (!isfinite(inputs[i].value)) continue;  /* NaN check */
    if (inputs[i].health != SENSOR_FAULTY) {
        healthy_values[count++] = inputs[i].value;
    }
}
```

### Sort for Median

```c
static void sort3(double arr[3]) {
    if (arr[0] > arr[1]) swap(&arr[0], &arr[1]);
    if (arr[1] > arr[2]) swap(&arr[1], &arr[2]);
    if (arr[0] > arr[1]) swap(&arr[0], &arr[1]);
}
```

O(1) — exactly 3 comparisons, 0-3 swaps.

### Confidence Calculation

```c
if (healthy_count == 3) {
    confidence = sensors_agree ? 1.0 : 0.7;
} else {
    confidence = sensors_agree ? 0.8 : 0.5;
}
confidence -= degraded_count * 0.1;
```

---

## Totality

The function is total:
- Always returns valid error code
- Always populates result
- Never crashes on bad input

---

## Bridge to Lesson 5

Next: Prove the code is correct with tests.
