# Typing Tutor in C (Phase 1 Ready)

This project is a **Typing Tutor** built in C from scratch for the capstone rules.

It includes all 5 custom libraries:
- `math.c/.h`
- `string.c/.h`
- `memory.c/.h`
- `screen.c/.h`
- `keyboard.c/.h`

The application runs a real-time typing loop in terminal and integrates all libraries in one pipeline.

## 1) Project Goal

Build a working terminal Typing Tutor using only our own core logic (no `string.h`, no `math.h`, no `malloc/free` for project logic).

## 2) Phase 1 Requirements Coverage

### Requirement: 5 core libraries implemented
Status: Done

- `math.c`: custom multiply, divide, modulo, abs, clamp, bounds check.
- `string.c`: custom length, copy, compare, split/tokenize, int-to-string.
- `memory.c`: custom virtual RAM + `os_alloc()` + `os_dealloc()`.
- `screen.c`: terminal clear, cursor move, draw text, hide/show cursor.
- `keyboard.c`: non-blocking key read + line read.

### Requirement: library integration into one app
Status: Done

Pipeline in `main.c`:
- keyboard input with `os_key_pressed()`
- string handling with `os_strlen()`, `os_strcpy()`, `os_split_first_token()`, `os_int_to_string()`
- dynamic memory with `os_alloc()` and `os_dealloc()`
- boundary/progress math with `os_in_bounds()`, `os_mul()`, `os_div()`, `os_clamp()`
- rendering with `os_screen_draw_text()` and screen controls

### Requirement: basic real-time interactive loop
Status: Done

- Continuous loop with non-blocking keyboard input.
- User types sentence in live screen.
- ESC exits, Backspace removes characters.

### Requirement: dynamic memory use with matching free
Status: Done

- Allocated: target sentence copy, typed buffer.
- Deallocated: both before exit.

### Requirement: boundary math usage
Status: Done

- Input buffer limits checked using `os_in_bounds()`.
- Progress percentage clamped by `os_clamp()`.

### Requirement: stability
Status: Done

- Null checks after allocation.
- Terminal state restored on exit.
- No crash in normal run flow.

## 3) Build and Run

### Build
```bash
make
```

### Run
```bash
make run
```

or
```bash
./typing_tutor
```

### Clean
```bash
make clean
```

## 4) Controls

- Type normal keys to enter text.
- `Backspace` to remove last character.
- `Enter` to mark complete.
- `ESC` to exit.

## 5) Folder Structure

- `include/` -> all header files
- `src/` -> all source files
- `docs/` -> evaluation support notes
- `Makefile` -> build commands

## 6) Known Issues / Honest Notes

- This Phase 1 version focuses on one fixed sentence.
- Arrow keys are not separately parsed as special keys.
- Color highlighting is not added to keep code simple for evaluation clarity.

## 7) Demo Evidence (for final submission)

Add at least one of below before final ZIP:
- 3 screenshots of running app
- or 1-3 minute screen recording

## 8) Important Rule Compliance

- No `<string.h>` functions used.
- No `<math.h>` functions used.
- No `malloc()` / `free()` for app memory logic.
- Core logic uses custom libraries.

## 9) Recommended Commands for Evaluation Day

```bash
make clean && make
./typing_tutor
```

Then explain files in order:
1. `memory.c`
2. `string.c`
3. `math.c`
4. `keyboard.c`
5. `screen.c`
6. `main.c`
