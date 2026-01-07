# Contributing to C-From-Scratch

Thank you for your interest in contributing!

## Philosophy

This is an educational repository. Contributions should:

1. **Maintain rigour** — No hand-waving. Proofs matter.
2. **Preserve clarity** — Beginner-friendly explanations
3. **Follow the method** — Math → Structs → Code

## Current Modules

| Module | Status | Tests |
|--------|--------|-------|
| Pulse | Complete | All passing |
| Baseline | Complete | 18/18 passing |
| Composition | Planned | — |

## What We Need

### High Priority
- Typo fixes and clarifications
- Additional exercises
- Test cases for edge conditions
- Translations (if you're fluent)

### Medium Priority
- Additional projects following the same methodology
- Improved diagrams and visualisations
- Platform-specific build instructions

### Discussions Welcome
- Alternative approaches to proofs
- Additional failure mode analysis
- Real-world war stories
- Ideas for Module 3 (Composition)

## How to Contribute

### Simple Fixes
1. Fork the repository
2. Make your change
3. Submit a pull request with clear description

### Larger Changes
1. Open an issue first to discuss
2. Wait for feedback before investing time
3. Follow the existing style and structure

### New Modules
New modules should follow the established pattern:

```
projects/your-module/
├── include/
│   └── module.h          # API + contracts in comments
├── src/
│   ├── module.c          # Implementation
│   └── main.c            # Demo
├── tests/
│   └── test_module.c     # Contract + invariant tests
├── lessons/
│   ├── 01-the-problem/   # Why this matters
│   ├── 02-math-model/    # Formal specification
│   ├── 03-structs/       # Data encoding
│   ├── 04-code/          # Implementation walkthrough
│   └── 05-testing/       # Proof harness
├── Makefile
└── README.md
```

## Style Guidelines

### Markdown
- One sentence per line (easier diffs)
- Use ATX headers (`#`, `##`, etc.)
- Code blocks with language specifier

### C Code
- C11 standard (previously C99)
- `snake_case` for functions and variables
- `UPPER_CASE` for constants and enums
- Comments explain *why*, not *what*
- Every function must be **total** (defined for all inputs)

### Naming Conventions
Follow the module pattern:
- Pulse: `hb_init`, `hb_step`, `hb_state`, `hb_faulted`
- Baseline: `base_init`, `base_step`, `base_state`, `base_faulted`

### Struct Design
- Every field must trace to a mathematical requirement
- Derived values belong in result structs, not state
- Configuration is immutable after init
- Faults are sticky until explicit reset

### Commit Messages
- Present tense ("Add feature" not "Added feature")
- First line under 50 characters
- Reference issues when applicable

Example:
```
Add Module 2: Baseline statistical normality monitor

- Closed, total, deterministic FSM for anomaly detection
- Four proven contracts: Convergence, Sensitivity, Stability, Spike Resistance
- 18/18 contract and invariant tests passing
- Six lessons from problem statement to composition
```

## Testing Standards

Every module needs:

1. **Contract tests** — One test per proven contract
2. **Invariant tests** — Verify structural guarantees hold
3. **Fuzz tests** — Random input, check invariants
4. **Edge case tests** — Zero, overflow, NaN, reset

Tests should print clear pass/fail with contract names:
```
[PASS] CONTRACT-1: Convergence (error=0.0263)
[PASS] CONTRACT-4: Spike Resistance (Shift=90.00, Max=90.00)
```

## Code of Conduct

Be kind. Be helpful. Assume good intent.

This is a learning environment. Questions are welcome, no matter how basic.

## Questions?

Open an issue with the "question" label.
