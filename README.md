# Typing Master

A terminal-based typing game written entirely in C — no game engines, no frameworks, no high-level abstractions. Just a C compiler, POSIX APIs, and the terminal.

The project started as a question: how much of what we take for granted in application development (memory allocation, string handling, math utilities, screen rendering, keyboard input) can we build ourselves, from scratch, using only what the OS gives us? The answer turned out to be: all of it.

---

## What it does

Typing Master is a fully interactive typing practice application that runs directly in your terminal. You pick a mode, pick a difficulty, and type. Characters light up green as you get them right and red when you miss. The game tracks your words per minute, accuracy, and score in real time. When the session ends, your result is saved to a leaderboard.

**Game modes**

| Mode | Description |
|------|-------------|
| Practice | No time limit — focus on accuracy and building muscle memory |
| Timed 30s | Race the clock with a 30-second countdown |
| Timed 60s | Same, but with 60 seconds for longer sentences |
| Survival | Three lives — every mistake costs one |

**Difficulty levels**

| Level | What changes |
|-------|-------------|
| Easy | Short, everyday phrases |
| Medium | Longer mixed-case sentences |
| Hard | Technical terms, symbols, and deliberate finger-jumpers |

**Statistics tracked every frame**

- Words per minute (calculated as `(correct_chars × 12) / elapsed_seconds`)
- Accuracy percentage
- Score (each correct character = 10 points)
- Remaining lives (Survival mode)
- Countdown timer with a live progress bar (Timed modes)
- Free heap bytes remaining from the custom allocator

High scores (top 10) are written to `.typing_highscores.dat` and loaded on the next run, so your records persist between sessions.

---

## Architecture

The codebase is split into five independent libraries and one application entry point. Every library has its own header in `include/` and its own implementation in `src/`. The application in `main.c` wires them together.

```
.
├── include/
│   ├── math.h
│   ├── string.h
│   ├── memory.h
│   ├── screen.h
│   └── keyboard.h
├── src/
│   ├── main.c          — state machine, game loop, rendering, file I/O
│   ├── math.c          — arithmetic without <math.h>
│   ├── string.c        — string ops without <string.h>
│   ├── memory.c        — heap allocator without malloc/free
│   ├── screen.c        — terminal rendering via ANSI escape sequences
│   └── keyboard.c      — raw mode input, non-blocking reads
└── Makefile
```

**State machine**

The game is driven by an enum-based state machine. Every state owns a full-screen render and routes input to a narrow set of transitions — nothing leaks across states.

```
MAIN_MENU → MODE_SELECT → DIFFICULTY_SELECT → PLAYING → ENTER_NAME → RESULTS → HIGHSCORES
```

Any state can return to the main menu. ESC or `q` exits cleanly from anywhere, restoring the terminal before the process ends.

---

## Low-level systems work

This is the part of the project we're most proud of. Every primitive the application depends on was written by hand.

### Custom math library (`src/math.c`)

No `<math.h>`. Multiplication is implemented as a shift-and-add algorithm operating on raw bits. Division uses repeated subtraction with bit shifts. Modulo derives from the division result. The library also exposes clamp and bounds-check functions that the game loop uses before every buffer write.

```c
int os_mul(int a, int b);          // shift-add, handles negatives
int os_div(int a, int b);          // long division via bit ops
int os_mod(int a, int b);          // a - (a/b)*b
int os_clamp(int v, int lo, int hi);
int os_in_bounds(int i, int len);  // exclusive upper bound check
```

### Custom string library (`src/string.c`)

No `<string.h>`. Length, copy, compare, integer-to-string, and token splitting are all implemented as character-by-character loops with explicit null-terminator handling. The game uses these for building stats strings, comparing typed input against the target sentence, and formatting the WPM/accuracy display on every frame.

```c
int os_strlen(const char *s);
void os_strcpy(char *dst, const char *src, int max);
int os_strcmp(const char *a, const char *b);
void os_int_to_str(int n, char *buf, int max);
int os_split_first_token(const char *src, char *tok, char *rest, int max);
```

### Custom memory allocator (`src/memory.c`)

No `malloc`, no `free`. The allocator manages a 128 KB static byte array declared in the data segment. Inside that array, a linked list of block headers tracks free and allocated regions. Allocation uses a first-fit strategy and splits blocks when the remainder is large enough to be useful. Deallocation coalesces adjacent free blocks to prevent fragmentation.

```c
void  os_memory_init(void);
void *os_alloc(int size);
void  os_dealloc(void *ptr);
int   os_free_bytes(void);
```

The game allocates two heap buffers at session start — one for the target sentence and one for the typed text — and frees both when the session ends.

### Terminal screen rendering (`src/screen.c`)

The renderer writes ANSI escape sequences directly to stdout. There is no ncurses dependency. Every frame starts by saving the cursor position, overwriting only the changed lines, and restoring it — the terminal does not flash. Colors, bold, dim, and cursor visibility are all escape-sequence driven.

```c
void os_screen_clear(void);
void os_screen_move(int row, int col);
void os_screen_set_color(int fg, int bg, int bold);
void os_screen_reset_color(void);
void os_screen_hide_cursor(void);
void os_screen_show_cursor(void);
void os_screen_begin_frame(void);
void os_screen_end_frame(void);
```

The screen enters the alternate buffer (`\033[?1049h`) on startup and exits it on quit, so the user's terminal scrollback is never polluted.

### Raw keyboard input (`src/keyboard.c`)

By default, the terminal buffers input until the user presses Enter and echoes every character back to the screen. Both behaviors break a typing game. The keyboard library calls `tcgetattr()` to read the current terminal attributes, clears `ICANON` and `ECHO`, and writes them back with `tcsetattr()`. It also sets `O_NONBLOCK` on stdin via `fcntl()`, so the read call returns immediately whether or not a key was pressed.

```c
void os_keyboard_init(void);
void os_keyboard_restore(void);
int  os_key_pressed(char *out);     // returns 1 if key available, 0 otherwise
```

ESC sequences (arrow keys, function keys) are detected by reading the full three-byte sequence and classifying it before the caller sees it.

### Signal handling

Three signal handlers are registered at startup:

- `SIGINT` / `SIGTERM` — set a volatile flag that the game loop checks, triggering a clean exit with terminal restoration
- `SIGWINCH` — fired by the OS when the terminal is resized; sets a flag that causes the current screen to be fully redrawn on the next frame

The resize handler means the UI stays correct whether the user resizes mid-game or between menus.

### System calls used

| Call | Purpose |
|------|---------|
| `tcgetattr` / `tcsetattr` | Switch terminal between cooked and raw mode |
| `fcntl(F_SETFL, O_NONBLOCK)` | Make stdin reads non-blocking |
| `read()` | Pull keyboard bytes directly from the file descriptor |
| `ioctl(TIOCGWINSZ)` | Query live terminal dimensions (rows × columns) |
| `signal()` | Register `SIGINT`, `SIGTERM`, `SIGWINCH` handlers |
| `time()` | Unix timestamp for elapsed time and WPM calculation |
| `usleep()` | Microsecond sleep to cap frame rate without busy-waiting |
| `fopen` / `fread` / `fwrite` / `fclose` | Binary high-score file persistence |
| `isatty()` | Validate that stdin/stdout are real TTYs before entering raw mode |

---

## Build and run

Requires a POSIX-compatible terminal. Tested on macOS (arm64) and Linux.

```bash
make
./typing_tutor
```

```bash
make clean && make    # rebuild from scratch
make run              # build and run in one step
```

The Makefile compiles all six source files with `-Wall -Wextra -Werror -std=c11`. There are no external dependencies beyond libc.

**Controls**

| Key | Action |
|-----|--------|
| Any letter / space | Type into the active lesson |
| Backspace | Delete the last character |
| Tab | End the current round and go to results |
| Enter | Confirm name entry and menu selections |
| ESC or `q` | Quit from anywhere, terminal is always restored |

---

## Numbers

| | |
|---|---|
| Total lines of code | ~1,830 |
| Libraries built from scratch | 5 |
| Game modes | 4 |
| Difficulty levels | 3 |
| Built-in lessons | 9 |
| Virtual heap size | 128 KB |
| High score slots | 10 |
| External dependencies | 0 |

---

## Why

Most typed characters travel through a stack of libraries before reaching your screen — language runtimes, libc, ncurses, OS abstraction layers. Understanding what that stack is actually doing requires going below it at least once. This project is that exercise: implement the stack yourself, keep the constraints real (no `<string.h>`, no `<math.h>`, no dynamic allocator from the OS), and build something you'd actually want to use with the pieces you made.

---

Developed by **Kush Agarwal** and **Himanshu Rawat**
