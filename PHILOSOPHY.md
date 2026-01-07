# Philosophy: Math → Structs → Code

## The Core Idea

Most programming tutorials teach you to:
1. Write code
2. Test code
3. Debug code
4. Hope it works

We do the opposite:
1. **Define the problem** precisely
2. **Prove the solution** mathematically
3. **Design the data** with explicit constraints
4. **Transcribe to code** (it writes itself)
5. **Verify the code** matches the math

## Why This Works

### Traditional Approach
```
Vague idea → Code → Bugs → Patches → More bugs → "Ship it"
```

### Our Approach
```
Problem → Math Model → Proof → Data Design → Code → Verification
    │         │          │          │          │          │
    │         │          │          │          │          └── Does code match math?
    │         │          │          │          └── Direct transcription
    │         │          │          └── Every field justified
    │         │          └── Contracts proven
    │         └── State machine defined
    └── Failure modes analysed
```

## The Three Pillars

### 1. Mathematical Closure

A system is **closed** if every possible input leads to exactly one defined output.

- **Closed**: Traffic light with defined transitions
- **Not closed**: `if (x > 0) return 1;` — what about x ≤ 0?

Our state machines are closed:
- Every state × every input = defined transition
- No undefined behaviour possible
- No "what if?" scenarios unconsidered

**Example from Baseline:**
```
Sₜ = f(Sₜ₋₁, xₜ)
```
The entire state depends only on the previous state and current input. No hidden history. No unbounded buffers.

### 2. Contracts as Specifications

Instead of vague requirements like "should work well", we have formal contracts:

**Pulse contracts:**
- **SOUNDNESS**: Never report ALIVE if actually dead
- **LIVENESS**: Eventually report DEAD if heartbeats stop
- **STABILITY**: No spurious transitions

**Baseline contracts:**
- **CONVERGENCE**: μₜ → E[X] for stationary input
- **SENSITIVITY**: Deviations detected in O(1/α) steps
- **STABILITY**: False positive rate bounded by P(|Z|>k)
- **SPIKE RESISTANCE**: |Δμ| ≤ α·M for single outlier M

Each contract is:
- Mathematically defined
- Proven to hold
- Tested in implementation

### 3. Data-Driven Design

Every struct field traces to a mathematical requirement.

**Pulse:**
| Field | Why It Exists |
|-------|---------------|
| `st` | Represents state space S |
| `t_init` | Required for init window W |
| `last_hb` | Required for timeout T |
| `have_hb` | Distinguishes UNKNOWN from DEAD |
| `fault_*` | Fail-safe contract enforcement |

**Baseline:**
| Field | Why It Exists |
|-------|---------------|
| `mu` | EMA mean (μₜ) — CONTRACT-1 |
| `variance` | EMA variance (σₜ²) — CONTRACT-3 |
| `n` | Observation count — learning phase |
| `state` | FSM state (qₜ) — transition logic |
| `fault_*` | Fail-safe contract enforcement |

No field exists "just in case" or "might be useful".

**What is NOT stored (equally important):**
- z-score (derived per-step, not needed for future state)
- History buffer (violates closure)
- Configuration per-call (embedded at init)

## The Composability Principle

Modules are not demos. They are systems. And systems compose.

```
Pulse proves existence in time:    ∃ eventₜ
Baseline proves normality in value: xₜ ~ N(μ, σ²)

Composed:
  event_t → Pulse → Δt → Baseline → deviation?
```

This works because both modules are:
- Closed (no hidden state leakage)
- Total (always return valid results)
- Deterministic (same inputs → same outputs)

## What You'll Learn

After this course, you'll be able to:

1. **Analyse failure modes** before writing code
2. **Define state machines** that cover all cases
3. **Prove properties** about your design
4. **Design data structures** where every field is justified
5. **Write code** that's obviously correct
6. **Test systematically** against contracts
7. **Compose systems** safely

## The Payoff

Code written this way:
- Has fewer bugs (proven design)
- Is easier to modify (clear contracts)
- Is easier to review (math matches code)
- Survives corner cases (all cases considered)
- Documents itself (contracts as comments)
- Composes safely (closed systems interact predictably)

## A Warning

This approach takes more time upfront. You'll spend hours thinking before writing a line of code.

But you'll spend near-zero time debugging.

The total time is less. The quality is higher. The stress is lower.

---

*"Give me six hours to chop down a tree and I will spend the first four sharpening the axe."* — Abraham Lincoln

---

*"Don't learn to code. Learn to prove, then transcribe."*
