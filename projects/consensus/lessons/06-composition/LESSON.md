# c-from-scratch — Module 5: Consensus

## Lesson 6: Composition & Applications

---

## What We Built

| Module | Question | Output |
|--------|----------|--------|
| Pulse | Does it exist? | ALIVE / DEAD |
| Baseline | Is it normal? | STABLE / DEVIATION |
| Timing | Is it healthy? | HEALTHY / UNHEALTHY |
| Drift | Is it moving toward failure? | STABLE / DRIFTING |
| **Consensus** | **Which sensor to trust?** | **Voted value + Confidence** |

---

## The Complete Safety Stack

```
┌─────────────────────────────────────────────────────────────┐
│                     APPLICATION LAYER                        │
│   Uses consensus value with confidence-based decisions       │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     CONSENSUS LAYER (Module 5)               │
│   [S0, S1, S2] → MVS Voter → Consensus Value + Confidence   │
└─────────────────────────────────────────────────────────────┘
                              │
          ┌───────────────────┼───────────────────┐
          ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│   CHANNEL 0     │ │   CHANNEL 1     │ │   CHANNEL 2     │
│  Sensor → Pulse │ │  Sensor → Pulse │ │  Sensor → Pulse │
│        → Base   │ │        → Base   │ │        → Base   │
│        → Drift  │ │        → Drift  │ │        → Drift  │
│        → Health │ │        → Health │ │        → Health │
└─────────────────┘ └─────────────────┘ └─────────────────┘
```

---

## Integration Example

```c
/* Each channel has its own monitoring pipeline */
sensor_input_t inputs[3];

for (int ch = 0; ch < 3; ch++) {
    double value = read_sensor(ch);
    uint64_t now = get_time_ms();
    
    /* Run through monitoring pipeline */
    hb_step(&pulse[ch], now, 1, T, W);
    drift_update(&drift[ch], value, now, &drift_result);
    
    /* Determine health from pipeline */
    if (hb_state(&pulse[ch]) == STATE_DEAD) {
        inputs[ch].health = SENSOR_FAULTY;
    } else if (drift_is_drifting(&drift[ch])) {
        inputs[ch].health = SENSOR_DEGRADED;
    } else {
        inputs[ch].health = SENSOR_HEALTHY;
    }
    inputs[ch].value = value;
}

/* Vote on consensus */
consensus_result_t result;
consensus_update(&voter, inputs, &result);

/* Use consensus value with confidence */
if (result.confidence > 0.8) {
    actuate(result.value);
} else if (result.confidence > 0.5) {
    actuate_with_caution(result.value);
} else {
    safe_shutdown();
}
```

---

## Applications

### Flight Control Surfaces

Three accelerometers feed the autopilot. If one drifts, the others outvote it. Aircraft remains stable.

### Medical Dosing Pumps

Three flow sensors monitor drug delivery. A stuck sensor is detected and excluded. Patient receives correct dose.

### Nuclear Reactor Control Rods

Triple redundant position sensors. Mid-value selection prevents a single faulty sensor from causing rod withdrawal.

### Automotive Brake-by-Wire

Three wheel speed sensors per wheel. Consensus prevents ABS malfunction from single sensor failure.

---

## The Consensus Challenge

Post to your community:

> **THE CHALLENGE: The Truth from Many Liars** 🕵️‍♂️
>
> I've uploaded `byzantine_noise.csv`. Sensor 3 is a 'liar'—it starts perfect but slowly drifts.
>
> **Your Task:** Implement Module 5 (Consensus) in C.
> 1. Use the Init-Update-Status pattern
> 2. Your code must detect that Sensor 3 is deviating
> 3. Pass the sim.py test where 'Consensus' stays within 2% of Ground Truth

---

## Framework Status

```
Phase 1: "The William Show" (Modules 1-3)     ✓ Complete
Phase 2: "The Framework" (Modules 4-6 + SPEC) ← Module 5 done
Phase 3: "The Ecosystem" (Contributors build)
Phase 4: "The Standard" (Industry adoption)
```

### Next: Module 6 — Pressure

> **"How do we handle overflow safely?"**

When messages arrive faster than we can process them, what do we do?
- Drop oldest?
- Block producer?
- Ring buffers and backpressure

---

## Final Takeaway

> **Good systems don't trust. They verify.**
> **Better systems don't verify once. They vote.**

Module 5 proves truth from many liars using mid-value selection.

**End of Module 5: Consensus — Triple Modular Redundancy**
