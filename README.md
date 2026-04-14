# Os Typing Master — Phase 1

Terminal typing tutor written in C for the capstone Phase 1 milestone: five custom libraries and one interactive application wired together.

## What evaluators should see

- **Libraries:** `math`, `string`, `memory`, `screen`, and `keyboard` each have a `.c` in `src/` and a matching header in `include/`. Core logic does not use `<string.h>`, `<math.h>`, or `malloc`/`free`.
- **App:** `main.c` runs a loop that polls the keyboard without blocking, redraws the UI, and uses the libraries end-to-end (strings for the target line and stats, math for bounds/progress, heap for two buffers, screen for ANSI output, keyboard for raw mode).
- **Behaviour:** The user types over a fixed practice sentence. Correct keys show in green, wrong keys in red, and the rest of the line stays dim until typed. Stats include typed length, correctness, progress, elapsed time, and free heap bytes. `os_split_first_token` is used once on the target string to show the first word on screen (string API demo).

## Build and run

Requires a normal interactive terminal (not piped input/output).

```bash
make clean && make
./typing_tutor
```

`make run` runs the binary the same way. Use `make clean` before a fresh build if object files are stale.

## Controls

| Key | Action |
|-----|--------|
| Letters / space | Type into the lesson line |
| Backspace | Remove last character |
| Enter | Mark session finished |
| ESC or `q` | Quit and restore the terminal |

## Repository layout

| Path | Role |
|------|------|
| `include/*.h` | Public declarations for the five modules |
| `src/*.c` | Implementations + `main.c` |
| `Makefile` | Builds the `typing_tutor` binary |
| `docs/` | Optional notes (if present) |

## Phase 1 checklist (mapping)

| Requirement | Where it shows up |
|-------------|-------------------|
| Five custom libraries | `src/math.c` … `keyboard.c` |
| Integrated application | `src/main.c` |
| Non-blocking input + live redraw | Main loop + `os_key_pressed` |
| At least one `alloc` / matching free | Buffers for target + typed text |
| Boundary check via `math` | `os_in_bounds` before writing typed buffer |
| Stable normal run | Raw mode restored on exit; allocations freed |

## Scope note

Phase 1 is integration and basic mechanics: one built-in sentence, no file loading or Phase 2 game-length features. That is intentional for this submission.
