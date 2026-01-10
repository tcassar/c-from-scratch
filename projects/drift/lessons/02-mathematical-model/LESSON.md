# c-from-scratch — Module 4: Drift

## Lesson 2: The Mathematical Model

---

## Purpose of This Lesson

In Lesson 1, we answered *why* naive slope calculations fail and identified the properties we need.

In Lesson 2, we answer:

**What is the smallest mathematical system that satisfies all those properties?**

By the end of this lesson, you will:

- Understand the exact mathematical state of the drift monitor
- Derive the damped derivative from first principles
- See why the system is closed and O(1)
- Be ready to map math → structs in Lesson 3

**No code yet. Only math that earns its keep.**

---

## 1. From Observations to State

We observe a scalar signal with timestamps:

```
(x₁, t₁), (x₂, t₂), (x₃, t₃), ..., (xₜ, tₜ)
```

A naive system stores history. But this violates closure, boundedness, and determinism.

### The Core Insight

**We do not need history. We need state.**

We define a state vector Sₜ such that:

```
Sₜ = f(Sₜ₋₁, xₜ, tₜ)
```

If this is true, the system is closed, bounded, and deterministic.

---

## 2. Minimal State Definition

```
Sₜ = (slope_t, x_{t-1}, t_{t-1}, n_t, q_t)
```

| Symbol | Meaning | Type |
|--------|---------|------|
| slope_t | Exponentially-weighted slope estimate | double |
| x_{t-1} | Previous observation value | double |
| t_{t-1} | Previous observation timestamp | uint64 |
| n_t | Observation count | uint32 |
| q_t | FSM state | enum |

**Anything less is insufficient. Anything more violates boundedness.**

---

## 3. The Damped Derivative

### Raw Slope (Noisy)

```
raw_slope_t = (x_t - x_{t-1}) / Δt
```

### Damped Slope (Smoothed via EMA)

```
slope_t = α · raw_slope_t + (1 - α) · slope_{t-1}
```

Where α ∈ (0, 1] is the smoothing factor.

The EMA is a first-order IIR low-pass filter:
- DC (constant drift) passes through unchanged
- High frequency (noise) is strongly attenuated
- Effective time constant: τ ≈ 1/α samples

---

## 4. The Complete Update Sequence

```
1. Validate input x_t is finite
2. Validate timestamp t_t > t_{t-1}
3. Check time gap: (t_t - t_{t-1}) ≤ max_gap
4. Compute Δt = t_t - t_{t-1}
5. Compute Δx = x_t - x_{t-1}
6. Compute raw_slope = Δx / Δt
7. Apply EMA: slope_t = α · raw_slope + (1-α) · slope_{t-1}
8. Compute TTF if |slope| is significant
9. Apply FSM transitions based on |slope|
10. Store x_{t-1} = x_t, t_{t-1} = t_t, increment n
```

**This ordering is not optional.**

---

## 5. FSM States and Transitions

| State | Meaning |
|-------|---------|
| LEARNING | n < n_min, slope estimate not stable |
| STABLE | |slope| ≤ max_safe_slope |
| DRIFTING_UP | slope > max_safe_slope |
| DRIFTING_DOWN | slope < -max_safe_slope |
| FAULT | NaN/Inf or corruption detected |

### Transitions

```
LEARNING → STABLE/DRIFTING  when n ≥ n_min
STABLE ↔ DRIFTING_UP/DOWN   based on |slope| vs threshold
ANY → FAULT                 on error
FAULT → LEARNING            only via reset()
```

---

## 6. Time-To-Failure (TTF)

```
If slope > 0:  TTF = (upper_limit - x_t) / slope
If slope < 0:  TTF = (x_t - lower_limit) / |slope|
```

TTF is meaningful only when |slope| > min_slope_for_ttf.

---

## 7. Configuration Constraints

| Constraint | Meaning |
|------------|---------|
| C1: 0 < α ≤ 1 | Valid smoothing factor |
| C2: max_safe_slope > 0 | Positive threshold |
| C3: upper > lower | Valid limit range |
| C4: n_min ≥ 2 | Need 2+ points for slope |
| C5: max_gap > 0 | Time-gap protection |
| C6: min_slope_for_ttf > 0 | TTF validity |

---

## 8. Proven Contracts

### CONTRACT-1 — Bounded Slope Detection
When |slope| > max_safe_slope and n ≥ n_min, state ∈ {DRIFTING_UP, DRIFTING_DOWN}.

### CONTRACT-2 — Noise Immunity
For jitter with E[ε] = 0, E[slope] → 0 (noise cancels).

### CONTRACT-3 — TTF Accuracy
For linear drift x = x₀ + vt, slope → v and TTF is exact.

### CONTRACT-4 — Spike Resistance
Single outlier shifts slope by at most α·(outlier_slope).

---

## 9. Complexity Guarantees

| Property | Guaranteed |
|----------|------------|
| Memory | O(1) |
| Time per step | O(1) |
| Determinism | Yes |
| Closure | Yes |

---

## Bridge to Lesson 3

Next: Map symbols to C struct fields, encode FSM explicitly.

**Math becomes memory layout. Equations become state transitions.**
