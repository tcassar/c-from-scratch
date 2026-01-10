# Consensus — Triple Modular Redundancy Voter

**Module 5 of c-from-scratch**

> "A man with one clock knows what time it is.
> A man with two clocks is never sure.
> With THREE clocks, we can outvote the liar."

## What Is This?

A closed, total, deterministic state machine for achieving consensus from multiple redundant sensor inputs using Triple Modular Redundancy (TMR) and Mid-Value Selection.

**The core insight:** A single faulty sensor cannot corrupt the output when we vote with three.

## Contracts (Proven)

```
CONTRACT-1 (Single-Fault Tolerance): One faulty sensor does not corrupt output
CONTRACT-2 (Bounded Output):         Consensus always within range of healthy inputs
CONTRACT-3 (Deterministic):          Same inputs → Same consensus
CONTRACT-4 (Degradation Awareness):  Confidence decreases with fewer healthy sensors
```

## Properties

| Property | Guaranteed |
|----------|------------|
| Memory | O(1) — fixed state |
| Time per vote | O(1) — 3 comparisons |
| Fault tolerance | 1 of 3 sensors |
| Determinism | Yes |

## Quick Start

```bash
make
make test   # 18/18 tests pass
make demo   # See all voting scenarios
```

## Project Structure

```
consensus/
├── include/
│   └── consensus.h       # API and contracts
├── src/
│   ├── consensus.c       # Implementation
│   └── main.c            # Demo
├── tests/
│   └── test_consensus.c  # Contract test suite
├── lessons/
│   ├── 01-the-problem/
│   ├── 02-mathematical-model/
│   ├── 03-structs/
│   ├── 04-code/
│   ├── 05-testing/
│   └── 06-composition/
├── sim/                  # Python simulator
├── build/
├── Makefile
└── README.md
```

## Mid-Value Selection

```
sorted = sort([s0, s1, s2])
consensus = sorted[1]  // Median
```

The median is never an extreme outlier. One liar is always outvoted.

## API

```c
// Initialize
int consensus_init(consensus_fsm_t *c, const consensus_config_t *cfg);

// Vote on three inputs
int consensus_update(consensus_fsm_t *c,
                     const sensor_input_t inputs[3],
                     consensus_result_t *result);

// Reset
void consensus_reset(consensus_fsm_t *c);

// Query functions
consensus_state_t consensus_state(const consensus_fsm_t *c);
uint8_t consensus_has_quorum(const consensus_fsm_t *c);
double consensus_get_value(const consensus_fsm_t *c);
double consensus_get_confidence(const consensus_fsm_t *c);
```

## States

| State | Meaning |
|-------|---------|
| INIT | Not yet received inputs |
| AGREE | All healthy sensors agree |
| DISAGREE | Sensors disagree beyond tolerance |
| DEGRADED | Only 2 healthy sensors |
| NO_QUORUM | <2 healthy (no valid consensus) |
| FAULT | Internal error |

## Health States (from upstream)

| Health | Meaning | Maps From |
|--------|---------|-----------|
| HEALTHY | Fully operational | Pulse=ALIVE, Drift=STABLE |
| DEGRADED | Concerning but usable | Drift=DRIFTING |
| FAULTY | Not trustworthy | Pulse=DEAD, any FAULT |

## Test Results

```
╔════════════════════════════════════════════════════════════════╗
║           CONSENSUS Contract Test Suite                        ║
╚════════════════════════════════════════════════════════════════╝

Contract Tests:
  [PASS] CONTRACT-1: Single fault tolerance (outlier ignored)
  [PASS] CONTRACT-1b: Negative outlier ignored
  [PASS] CONTRACT-2: Bounded output (100 random trials)
  [PASS] CONTRACT-3: Deterministic (10 identical trials)
  [PASS] CONTRACT-4: Degradation awareness

Byzantine Fault Tests:
  [PASS] Byzantine: Slow drift resisted
  [PASS] Byzantine: Oscillating liar resisted
  [PASS] Byzantine: Two liars detected as DISAGREE

Fuzz Tests:
  [PASS] Fuzz: 100000 random inputs, invariants held

Results: 18/18 tests passed
```

## Applications

- **Flight Control**: Three accelerometers outvote a faulty one
- **Medical Devices**: Triple flow sensors ensure correct dosing
- **Nuclear Control**: Redundant position sensors prevent accidents
- **Automotive**: Brake-by-wire with sensor voting

## The Consensus Challenge

> "Sensor 3 is a 'liar' that slowly drifts. Your code must detect it and maintain consensus within 2% of ground truth."

## License

MIT License - See LICENSE in repository root.

## Author

William Murray  
Copyright (c) 2026
