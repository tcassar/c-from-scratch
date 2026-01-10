# c-from-scratch — Module 5: Consensus

## Lesson 2: The Mathematical Model

---

## Purpose

In Lesson 1, we identified the problem: trusting sensors that may lie.

In Lesson 2, we formalise:

1. The voting algorithms mathematically
2. Why mid-value selection is provably fault-tolerant
3. The minimal state required for consensus
4. Confidence calculation

---

## 1. The Voting Problem (Formal)

Given n sensors with values $x_1, x_2, ..., x_n$ and health states $h_1, h_2, ..., h_n$:

Find a consensus value $C$ such that:

1. $C$ is within the range of healthy inputs
2. A single faulty sensor cannot significantly affect $C$
3. The computation is deterministic and O(1)

For TMR (n=3), we prove that mid-value selection satisfies all conditions.

---

## 2. Mid-Value Selection (MVS)

### Definition

Given three values, sort and select the middle:

```
MVS(x₁, x₂, x₃) = sorted([x₁, x₂, x₃])[1]
```

### Theorem: Single Fault Tolerance

**Claim:** If one sensor $x_i$ is faulty (arbitrarily bad), MVS returns a value within the range of the two healthy sensors.

**Proof:**

Let $x_a, x_b$ be healthy with $x_a ≤ x_b$, and $x_f$ be faulty.

Case 1: $x_f < x_a$
  - Sorted: $[x_f, x_a, x_b]$
  - MVS = $x_a$ ∈ $[x_a, x_b]$ ✓

Case 2: $x_f > x_b$
  - Sorted: $[x_a, x_b, x_f]$
  - MVS = $x_b$ ∈ $[x_a, x_b]$ ✓

Case 3: $x_a ≤ x_f ≤ x_b$
  - Sorted: $[x_a, x_f, x_b]$
  - MVS = $x_f$, but this is bounded by healthy values ✓

**Conclusion:** The faulty value is either excluded or bounded. QED.

---

## 3. Why Not Simple Average?

### Average is NOT fault-tolerant

```
AVG(x₁, x₂, x₃) = (x₁ + x₂ + x₃) / 3
```

**Counterexample:**

| x₁ | x₂ | x₃ (faulty) | AVG | MVS |
|----|----|----|-----|-----|
| 100 | 100 | 100000 | 33400 | 100 |

The average is **pulled toward the outlier** by 33,300 units!

MVS is **immune** — the outlier is never selected.

---

## 4. Fault-Tolerant Averaging (FTA)

For applications requiring smooth output, we can use:

```
FTA(x₁, x₂, x₃) = (x_min + x_mid + x_max) / 3
               but excluding outliers beyond threshold
```

However, FTA has edge cases. MVS is simpler and provably correct.

---

## 5. Minimal State Definition

For consensus, we need:

```
Sₜ = (last_consensus, confidence, n, state)
```

| Symbol | Meaning | Type |
|--------|---------|------|
| last_consensus | Previous voted value (for continuity) | double |
| confidence | Trust level 0.0-1.0 | double |
| n | Update count | uint32 |
| state | FSM state | enum |

**We do NOT store history.** Each vote is independent.

---

## 6. Health State Integration

Inputs come with health states from upstream modules:

```
typedef enum {
    SENSOR_HEALTHY  = 0,  // Pulse=ALIVE, Baseline=STABLE, Drift=STABLE
    SENSOR_DEGRADED = 1,  // Drift=DRIFTING (concerning but usable)
    SENSOR_FAULTY   = 2   // Pulse=DEAD or Baseline/Drift FAULT
} sensor_health_t;
```

**Voting Logic:**

1. Filter: Include only HEALTHY and DEGRADED
2. Exclude: FAULTY sensors don't vote
3. Count: Need ≥2 healthy for valid consensus

---

## 7. FSM States

| State | Meaning |
|-------|---------|
| INIT | Not yet received inputs |
| AGREE | All healthy sensors agree (spread ≤ tolerance) |
| DISAGREE | Sensors disagree beyond tolerance |
| DEGRADED | Only 2 healthy sensors (reduced confidence) |
| NO_QUORUM | <2 healthy sensors (no valid consensus) |
| FAULT | Internal error detected |

### Transitions

```
INIT → AGREE/DISAGREE/DEGRADED   on first valid update with quorum
INIT → NO_QUORUM                 if <2 healthy sensors

AGREE ↔ DISAGREE                 based on spread vs tolerance
AGREE/DISAGREE → DEGRADED        when sensor count drops to 2
DEGRADED → AGREE/DISAGREE        when third sensor recovers

ANY → NO_QUORUM                  when <2 healthy remain
NO_QUORUM → AGREE/etc            when sensors recover

ANY → FAULT                      on internal error
FAULT → INIT                     only via reset()
```

---

## 8. Confidence Calculation

Confidence reflects trust in the consensus:

| Condition | Confidence |
|-----------|------------|
| 3 sensors agree | 1.0 |
| 3 sensors disagree | 0.7 |
| 2 sensors agree | 0.8 |
| 2 sensors disagree | 0.5 |
| <2 sensors | 0.0-0.1 (using stale value) |

**Degraded sensors reduce confidence:**
- Each DEGRADED sensor: -0.1 confidence
- Minimum confidence: 0.1

---

## 9. Spread and Agreement

**Spread** = max(healthy) - min(healthy)

**Agreement** = (spread ≤ max_deviation)

```c
double spread = max_val - min_val;
bool agree = (spread <= cfg.max_deviation);
```

---

## 10. Configuration Constraints

| Constraint | Meaning |
|------------|---------|
| C1: max_deviation > 0 | Valid agreement tolerance |
| C2: tie_breaker ∈ {0,1,2} | Valid sensor index |
| C3: n_min ≥ 1 | Minimum updates before stable |

---

## 11. Complexity Guarantees

| Property | Guaranteed |
|----------|------------|
| Memory | O(1) — fixed state, no buffers |
| Time per vote | O(1) — fixed 3 sensors, 3 comparisons |
| Determinism | Yes |
| Fault tolerance | 1 of 3 sensors |

---

## Lesson 2 Checklist

- ☐ Prove mid-value selection is fault-tolerant
- ☐ Explain why averaging fails
- ☐ Define the minimal state
- ☐ Map health states to voting inclusion
- ☐ Calculate confidence from sensor count

---

## Bridge to Lesson 3

Next: Encode the mathematical model in C structs.

**Theorems become type definitions. Proofs become invariants.**
