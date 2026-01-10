# c-from-scratch — Module 4: Drift

## Lesson 1: The Problem

---

## Why Rate Detection Matters

Every sensor, every system, every process has a failure mode that thresholds miss: **the silent drift**.

Consider a battery pack in an electric vehicle. At 40°C, it's within spec. At 45°C, still fine. At 50°C, warning territory. At 60°C, thermal runaway imminent. But here's the problem:

> **"The temperature is 45°C and normal."**

This tells you nothing about whether you'll be at 60°C in 30 seconds or 30 minutes. The *absolute value* is fine. The *velocity* is lethal.

### Real-World Failures

**Boeing 787 Battery Fire (2013)**

Battery temperature was within spec. But the rate of temperature rise indicated internal short circuit. By the time the threshold alarm triggered, the lithium-ion cells were already in thermal runaway. The aircraft was grounded worldwide.

**Therac-25 (1985-1987)**

Software timing drift between components led to race conditions that delivered lethal radiation doses. Each individual component was "within spec." The *rate* at which their states diverged was the failure mode.

**Deepwater Horizon (2010)**

Pressure readings were monitored against thresholds. The rate of pressure change — which would have indicated the blowout preventer failure — was not tracked as a first-class signal. By the time absolute thresholds were breached, 11 people were dead.

### The Pattern

Every time:

1. Individual measurements are within acceptable limits
2. The *rate of change* indicates imminent failure
3. Threshold-based monitoring sees "all green"
4. Catastrophic failure occurs

---

## Why Naive Approaches Fail

### Approach 1: Raw Slope Calculation

```c
/* Naive: compute instantaneous slope */
double slope = (x_now - x_prev) / (t_now - t_prev);
if (fabs(slope) > THRESHOLD) {
    alarm();
}
```

| Problem | Consequence |
|---------|-------------|
| Sensor noise amplified | A 0.1°C jitter at 1Hz looks like 0.1°C/s slope |
| No memory | Each calculation is independent |
| Threshold hunting | Signal oscillates around threshold |
| Division by small Δt | Near-zero intervals cause infinite slopes |

This is worse than useless — it generates false alarms that train operators to ignore the system.

### Approach 2: Moving Average of Slope

```c
/* Less naive: average slope over window */
double sum = 0;
for (int i = 0; i < WINDOW_SIZE; i++) {
    sum += (samples[i] - samples[i-1]) / dt;
}
double avg_slope = sum / WINDOW_SIZE;
```

| Problem | Consequence |
|---------|-------------|
| O(n) memory | Buffer grows with window size |
| Unbounded | Memory allocation in safety-critical code |
| Lag | Large window means slow response |
| Still noisy | Window averaging doesn't reject impulse noise well |

### Approach 3: Linear Regression

```c
/* Sophisticated but wrong: fit line to history */
double slope = linear_regression(history, WINDOW_SIZE);
```

| Problem | Consequence |
|---------|-------------|
| O(n) memory | Must store history buffer |
| O(n) computation | Recalculate every sample |
| Numerical instability | Matrix operations can overflow |
| Not composable | Can't chain with other monitors |

---

## What We Actually Need

A system that is:

| Property | Meaning |
|----------|---------|
| **Closed** | State depends only on previous state + new input |
| **Bounded** | O(1) memory, O(1) update |
| **Noise-Immune** | Small jitter does not trigger false alarms |
| **Responsive** | True drift detected quickly |
| **Deterministic** | Same inputs → same outputs |

### The Contracts We'll Prove

```
CONTRACT-1 (Bounded Slope):    |slope| bounded by physics, not runaway
CONTRACT-2 (Noise Immunity):   Jitter < ε does not trigger DRIFTING
CONTRACT-3 (TTF Accuracy):     Time-to-failure estimate within bounded error
CONTRACT-4 (Spike Resistance): Single outlier shifts slope by at most α·(outlier_slope)
```

---

## The Key Insight: Damped Derivative

**Exponential Moving Average (EMA) of the slope** gives us everything:

```
raw_slope_t = (x_t - x_{t-1}) / Δt
slope_t = α · raw_slope_t + (1 - α) · slope_{t-1}
```

This is not a moving average. This is a **first-order IIR low-pass filter** applied to the derivative.

| Property | How It Delivers |
|----------|-----------------|
| Closed | slope_t depends only on slope_{t-1} and x_t |
| Bounded | O(1) memory: store only slope, last_value, last_time |
| Noise-Immune | High-frequency jitter attenuated by (1-α) |
| Responsive | Tunable via α (higher = faster response) |
| Deterministic | Pure recurrence relation |

**This is not an optimisation. This is what makes the system closed.**

The EMA acts as a low-pass filter. Noise (high frequency) is attenuated. True drift (low frequency) passes through. The cutoff frequency is determined by α.

---

## From Module 3 to Module 4

| Module | Question | Output |
|--------|----------|--------|
| Pulse | Does it exist? | ALIVE / DEAD |
| Baseline | Is it normal? | STABLE / DEVIATION + z-score |
| Timing | Is it healthy? | HEALTHY / UNHEALTHY |
| **Drift** | **Is it moving toward failure?** | **STABLE / DRIFTING + slope + TTF** |

Module 3 (Timing) composed Pulse and Baseline to detect timing anomalies. Module 4 asks a different question: even if the signal is currently "normal," is it *heading* toward failure?

### The Composition Vision

```
value → Drift → (slope, TTF) → Action
```

In a complete system:
```
sensor → Baseline (is value normal?) 
      → Drift (is value trending toward limit?)
      → Combined health assessment
```

---

## Deliverables

By the end of Lesson 1, you understand:

- ☐ Why absolute thresholds miss "silent drift" failures
- ☐ Why raw slope calculation amplifies noise
- ☐ Why moving averages violate closure and boundedness
- ☐ The damped derivative insight (EMA of slope)
- ☐ The four contracts we'll prove

---

## Key Takeaway

> **Module 3 proved health over time by composing existence and normality.**
> **Module 4 proves velocity toward failure using damped derivatives.**

In the next lesson, we derive the mathematical model formally and prove why each component of the state is necessary and sufficient.
