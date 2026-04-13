# Phase 1 Evaluation Guide (Simple English)

This file is for viva explanation. It explains exactly what is implemented and why.

## A) What examiner expects in Phase 1

1. All 5 libraries are built and usable.
2. Libraries are integrated in one working app.
3. App has real-time non-blocking loop.
4. At least one dynamic allocation and matching deallocation.
5. At least one boundary check using math library.
6. Program should run without normal crashes.

All 6 points are implemented.

## B) File by file explanation

## `src/math.c`

- `os_mul(a,b)`: custom multiplication using repeated addition.
- `os_div(n,d)`: custom integer division using repeated subtraction.
- `os_mod(n,d)`: custom modulo using repeated subtraction.
- `os_abs(v)`: absolute value.
- `os_clamp(v,min,max)`: keeps value inside a range.
- `os_in_bounds(v,min,max_exclusive)`: returns 1 if inside range else 0.

Why needed in typing tutor:
- Prevent typed index going out of buffer.
- Compute and clamp progress percentage.

## `src/string.c`

- `os_strlen()`: count characters.
- `os_strcpy()`: copy source string.
- `os_strcmp()`: compare two strings.
- `os_split_first_token()`: get first word token.
- `os_int_to_string()`: convert integer to text.

Why needed in typing tutor:
- Get target sentence length.
- Copy target sentence into dynamic memory.
- Show first token on UI as split() demo.
- Convert stats numbers to strings for drawing.

## `src/memory.c`

- Defines one global virtual RAM region.
- `os_memory_init()` creates first free block.
- `os_alloc(size)` finds free block, splits when needed.
- `os_dealloc(ptr)` marks block free and merges free neighbors.
- `os_memory_used_bytes()` and `os_memory_free_bytes()` for stats.

Why needed in typing tutor:
- Target sentence buffer allocated by `os_alloc()`.
- Typed text buffer allocated by `os_alloc()`.
- Both are released by `os_dealloc()`.

## `src/screen.c`

- `os_screen_clear()` clears terminal.
- `os_screen_move_cursor(x,y)` positions cursor.
- `os_screen_draw_text(x,y,text)` prints text at location.
- `os_screen_hide_cursor()` and `os_screen_show_cursor()`.
- `os_screen_flush()` forces draw immediately.

Why needed in typing tutor:
- Redraw UI every frame in real time.

## `src/keyboard.c`

- `os_keyboard_init()` sets terminal raw + non-blocking input.
- `os_key_pressed()` returns key only when available (no blocking).
- `os_read_line()` reads full line (helper API also ready).
- `os_keyboard_shutdown()` restores terminal settings.

Why needed in typing tutor:
- Real-time input loop without stopping screen updates.

## `src/main.c`

Main integration logic:
1. Initialize memory.
2. Allocate buffers via custom allocator.
3. Initialize keyboard and screen.
4. Run loop:
   - read key non-blocking
   - update typed buffer with boundary checks
   - compute progress with custom math
   - render UI with custom screen
5. On exit:
   - restore terminal
   - free allocated memory

This is exactly Phase 1 integration.

## C) Possible viva questions with short answers

Q1. Why non-blocking keyboard is needed?
A: So screen can keep updating even when user is not pressing key.

Q2. Where is dynamic memory used?
A: In `main.c`, `target_sentence` and `typed_text` are allocated with `os_alloc()` and released with `os_dealloc()`.

Q3. Show boundary check usage.
A: `os_in_bounds(typed_len, 0, TYPED_BUFFER_SIZE - 1)` prevents writing outside buffer.

Q4. How do you ensure no memory leak in normal exit?
A: Every successful `os_alloc()` has matching `os_dealloc()` before return.

Q5. Which standard restricted functions did you avoid?
A: `strlen/strcpy/...` from `<string.h>`, `<math.h>` functions, and `malloc/free`.

## D) Fast demo script for evaluation

1. Run:
```bash
make clean && make
./typing_tutor
```
2. Type a few characters.
3. Show backspace.
4. Show progress updates.
5. Press ESC and show clean exit message.

