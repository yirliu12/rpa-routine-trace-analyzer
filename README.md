# Boolean Trace Analyzer

A C program for simulating Boolean-variable actions and finding routine-like patterns in an observed action trace.

Each action has preconditions and effects. The program applies actions to a Boolean state, checks whether the trace can execute, and compares candidate routines with contiguous sections of the trace.

## Stages

- **Stage 0 — Trace simulation:** Validates each action against the current state, applies its effects, and reports the state after each valid action.
- **Stage 1 — Exact-effect matching:** Finds non-overlapping trace sections whose cumulative Boolean effect matches a candidate routine.
- **Stage 2 — Restoration-aware matching:** Also finds sections that temporarily change other variables, as long as those variables return to their starting values by the end of the section.

Together, the stages move from validating an observed trace to identifying routines whose effects appear within it, including routines obscured by temporary state changes.


## Assignment context

Coursework for **COMP10002: Foundations of Algorithms**, University of Melbourne.  

## Repository contents

- `src/main.c` — the revised C implementation.
- `tests/case0`–`tests/case5` — sample inputs and expected outputs from the project archive.
- `scripts/test_reference.py` — builds `src/main.c` and checks it against the included sample outputs.


## Build

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 src/main.c -o boolean-trace-analyzer
```

## Test

Requires Python 3 and GCC:

```bash
python3 scripts/test_reference.py
```

The test script checks all six included sample cases.

## Topics

`c` · `algorithms` · `simulation` · `pattern-matching` · `coursework`
