# c-from-scratch — Module 5: Consensus

## Lesson 3: Structs & State Encoding

---

## Core Principle

> **A struct encodes the mathematical state. An enum encodes the FSM.**

---

## The Input Structure

```c
typedef struct {
    double          value;   /* Sensor reading */
    sensor_health_t health;  /* Health from upstream module */
} sensor_input_t;
```

Bundles value with health so they stay paired.

---

## The Result Structure

```c
typedef struct {
    double           value;           /* Consensus value */
    double           confidence;      /* 0.0 to 1.0 */
    consensus_state_t state;          /* FSM state */
    uint8_t          active_sensors;  /* Count of healthy */
    uint8_t          sensors_agree;   /* Within tolerance? */
    double           spread;          /* Max - Min */
    uint8_t          used[3];         /* Which sensors voted */
    uint8_t          valid;           /* Is consensus valid? */
} consensus_result_t;
```

---

## The FSM Structure

```c
typedef struct {
    consensus_config_t cfg;           /* Immutable config */
    consensus_state_t state;          /* Current state */
    uint32_t          n;              /* Update count */
    double            last_value;     /* For continuity */
    double            last_confidence;
    uint8_t           has_last;
    double            last_values[3]; /* Per-sensor tracking */
    sensor_health_t   last_health[3];
    uint8_t           fault_fp;       /* Fault flags */
    uint8_t           fault_reentry;
    uint8_t           in_step;        /* Atomicity guard */
} consensus_fsm_t;
```

---

## What Is NOT Stored

- ❌ History buffer — each vote is independent
- ❌ Sensor trust weights — comes from upstream modules

---

## Invariants

| Invariant | Enforced By |
|-----------|-------------|
| State domain | enum type |
| AGREE → spread ≤ tolerance | Transition logic |
| NO_QUORUM → active < 2 | Voting logic |

---

## Bridge to Lesson 4

Next: Implement the voting logic in C.
