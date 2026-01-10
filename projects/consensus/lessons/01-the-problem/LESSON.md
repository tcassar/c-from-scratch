# c-from-scratch — Module 5: Consensus

## Lesson 1: The Problem

---

## The Two-Clock Paradox

> "A man with one clock knows what time it is.
> A man with two clocks is never sure.
> A man with three clocks can outvote the liar."

In safety-critical systems, we never trust a single source of truth. But adding redundancy creates a new problem: **what do we do when sensors disagree?**

---

## Why Single Sensors Fail

### The Therac-25 Race Condition (1985-1987)

Software timing between components caused lethal radiation doses. Each component was "correct" in isolation. The system trusted single sources without verification.

### The Boeing 737 MAX (2018-2019)

A single angle-of-attack sensor fed the MCAS system. When it failed, the aircraft repeatedly pushed the nose down. There was no voting, no redundancy, no dissent.

### Toyota Unintended Acceleration (2009-2011)

Single-point failures in the throttle position sensor caused uncontrolled acceleration. NASA's investigation found "bit-flip" failures that a voter could have caught.

**The pattern:** Every catastrophe traces back to trusting a single sensor without independent verification.

---

## Why Two Sensors Aren't Enough

With two sensors, when they disagree, which do you trust?

| Sensor A | Sensor B | Decision |
|----------|----------|----------|
| 100°C    | 100°C    | Easy: 100°C |
| 100°C    | 200°C    | ??? |

You cannot know which sensor is lying. You need a **tiebreaker**.

---

## The TMR Solution

**Triple Modular Redundancy (TMR)** uses three independent sensors:

| S0 | S1 | S2 | Consensus |
|----|----|----|-----------|
| 100 | 100 | 100 | 100 (all agree) |
| 100 | 100 | 999 | 100 (majority wins) |
| 100 | 200 | 100 | 100 (median) |
| 50 | 999 | 50 | 50 (outlier ignored) |

With three sensors, **one liar cannot corrupt the output**.

---

## The Voting Problem

Even with TMR, we must answer:

1. **How do we select the consensus value?**
   - Simple average? (pulled by outliers)
   - Majority vote? (works for discrete values only)
   - Mid-value selection? (robust to outliers)

2. **What if a sensor is "subtly wrong"?**
   - Byzantine fault: lies but looks plausible
   - Slow drift: starts correct, gradually diverges

3. **What if two sensors fail?**
   - TMR only tolerates 1 fault
   - Must detect and report degradation

---

## Naive Approaches That Fail

### Approach 1: Simple Average

```c
double consensus = (s0 + s1 + s2) / 3.0;
```

| Problem | Consequence |
|---------|-------------|
| One outlier corrupts result | 99999 → average pulled away |
| No fault detection | Silent corruption |
| Not bounded | Output can exceed all valid inputs |

### Approach 2: Majority Vote (Discrete)

```c
if (s0 == s1) consensus = s0;
else if (s1 == s2) consensus = s1;
else consensus = s0;
```

| Problem | Consequence |
|---------|-------------|
| Only works for identical values | Sensors rarely agree exactly |
| No tolerance band | 100.0 vs 100.001 = "disagree" |
| Arbitrary tie-breaker | s0 wins by default |

---

## What We Actually Need

A voting system that is:

| Property | Meaning |
|----------|---------|
| **Fault-Tolerant** | One bad sensor doesn't corrupt output |
| **Bounded** | Output within range of healthy inputs |
| **Degradation-Aware** | Confidence decreases with fewer sensors |
| **Deterministic** | Same inputs → Same output |
| **Composable** | Integrates with Pulse/Baseline/Drift health |

---

## The Contracts We'll Prove

```
CONTRACT-1 (Single-Fault Tolerance): One faulty sensor does not corrupt output
CONTRACT-2 (Bounded Output):         Consensus always within range of healthy inputs
CONTRACT-3 (Deterministic):          Same inputs → Same consensus
CONTRACT-4 (Degradation Awareness):  Confidence decreases with fewer healthy sensors
```

---

## The Key Insight: Mid-Value Selection

Given three inputs, **sort and take the middle value**:

```
sorted = sort([s0, s1, s2])
consensus = sorted[1]  // Median
```

| Property | How MVS Delivers |
|----------|------------------|
| Fault-tolerant | Extreme outlier is never the median |
| Bounded | Median is always between min and max |
| Deterministic | Sorting is deterministic |
| O(1) | Fixed 3 elements, fixed comparisons |

**This is not an optimisation. This is what makes the system safe.**

---

## From Module 4 to Module 5

| Module | Question | Output |
|--------|----------|--------|
| Pulse | Does it exist? | ALIVE / DEAD |
| Baseline | Is it normal? | STABLE / DEVIATION |
| Timing | Is it healthy? | HEALTHY / UNHEALTHY |
| Drift | Is it moving toward failure? | STABLE / DRIFTING |
| **Consensus** | **Which sensor to trust?** | **Voted value + Confidence** |

---

## Deliverables

By the end of Lesson 1, you understand:

- ☐ Why single sensors are dangerous
- ☐ Why two sensors create the two-clock paradox
- ☐ Why simple averaging fails
- ☐ The TMR solution with three sensors
- ☐ Mid-value selection as the voting algorithm

---

## Key Takeaway

> **Module 4 proved velocity toward failure.**
> **Module 5 proves truth from many liars.**

In the next lesson, we formalise the voting algorithms and prove their fault tolerance properties.
