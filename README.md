# Typing Master

> A terminal typing game built entirely in C — no libraries, no frameworks, no shortcuts.

We started with a simple question: **how much of what we take for granted in everyday software can we actually build ourselves, from nothing?** Not as an academic exercise, but for real — a working application that someone would sit down and use.

The answer: all of it. Memory allocation, string manipulation, math utilities, screen rendering, keyboard input — every layer is hand-written. The only things we used are a C compiler, the POSIX API, and whatever the OS hands you at startup.

---

## At a glance

| | |
|---|---|
| Language | C (c11) |
| Lines of code | ~1,830 |
| Libraries built from scratch | 5 |
| External dependencies | 0 |
| Game modes | 4 |
| Difficulty levels | 3 |
| Built-in lessons | 9 |
| Virtual heap | 128 KB |
| High score slots | 10 |

---

## Getting started

```bash
make
./typing_tutor
```

That's it. There's nothing to install.

```bash
make clean && make   # rebuild from scratch
make run             # build and run in one step
```

Requires a POSIX-compatible terminal. Tested on macOS (arm64) and Linux.

---

## How it plays

You pick a mode, pick a difficulty, and type. Characters light up green as you hit them right and red when you miss. WPM, accuracy, and score update live on every keystroke. When the session ends, you enter your name and your result is saved to a persistent leaderboard.

**Modes**

| Mode | What it is |
|------|------------|
| Practice | No pressure — no timer, no lives. Just you and the text. |
| Timed 30s | Thirty seconds. Type as much as you can. |
| Timed 60s | Same deal, but longer sentences and more room to breathe. |
| Survival | Three lives. Every mistake costs one. Don't lose them all. |

**Difficulty**

| Level | What changes |
|-------|--------------|
| Easy | Short, common phrases — good for warming up |
| Medium | Longer sentences with mixed case |
| Hard | Technical vocabulary, symbols, and words designed to trip your fingers |

**Controls**

| Key | Action |
|-----|--------|
| Any letter / space | Type into the active lesson |
| Backspace | Delete the last character |
| Tab | End the round and go to results |
| Enter | Confirm selections and name entry |
| ESC or `q` | Quit cleanly from anywhere |

**Live stats tracked every frame**

- Words per minute — calculated as `(correct_chars × 12) / elapsed_seconds`
- Accuracy percentage
- Score — 10 points per correct character
- Remaining lives (Survival mode)
- Countdown with a live progress bar (Timed modes)
- Free heap bytes left in the custom allocator

High scores (top 10) are written to `.typing_highscores.dat` and loaded back on the next run.

---

## Architecture

Six source files, five independent libraries, one entry point that wires them together.

```
.
├── include/
│   ├── math.h
│   ├── string.h
│   ├── memory.h
│   ├── screen.h
│   └── keyboard.h
├── src/
│   ├── main.c       — state machine, game loop, rendering, file I/O
│   ├── math.c       — arithmetic without <math.h>
│   ├── string.c     — string ops without <string.h>
│   ├── memory.c     — heap allocator without malloc/free
│   ├── screen.c     — terminal rendering via ANSI escape sequences
│   └── keyboard.c   — raw mode input, non-blocking reads
└── Makefile
```

**State machine**

The game is driven by an enum-based state machine. Each state owns a full-screen render and routes input to a narrow set of valid transitions — no state bleeds into another.

```
MAIN_MENU → MODE_SELECT → DIFFICULTY_SELECT → PLAYING → ENTER_NAME → RESULTS → HIGHSCORES
```

Any state can return to the main menu. ESC or `q` exits cleanly from anywhere and always restores the terminal before the process ends.

---

## The libraries

This is the part we're most proud of. Every primitive the application depends on was written by hand.

### Math — `src/math.c`

No `<math.h>`. Multiplication is a shift-and-add algorithm operating on raw bits. Division uses repeated subtraction with bit shifts. Modulo derives from the division result. The library also exposes clamp and bounds-check functions that the game loop uses before every buffer write.

```c
int os_mul(int a, int b);           // shift-add, handles negatives
int os_div(int a, int b);           // long division via bit ops
int os_mod(int a, int b);           // a - (a/b)*b
int os_clamp(int v, int lo, int hi);
int os_in_bounds(int i, int len);   // exclusive upper bound check
```

### Strings — `src/string.c`

No `<string.h>`. Length, copy, compare, integer-to-string, and token splitting are all character-by-character loops with explicit null-terminator handling. Used for building stats overlays, comparing typed input against the target sentence, and formatting the WPM/accuracy display every frame.

```c
int  os_strlen(const char *s);
void os_strcpy(char *dst, const char *src, int max);
int  os_strcmp(const char *a, const char *b);
void os_int_to_str(int n, char *buf, int max);
int  os_split_first_token(const char *src, char *tok, char *rest, int max);
```

### Memory — `src/memory.c`

No `malloc`, no `free`. The allocator manages a 128 KB static byte array in the data segment. A linked list of block headers tracks free and allocated regions inside it. Allocation is first-fit with block splitting when the remainder is large enough to reuse. Deallocation coalesces adjacent free blocks to prevent fragmentation over time.

```c
void  os_memory_init(void);
void *os_alloc(int size);
void  os_dealloc(void *ptr);
int   os_free_bytes(void);   // shown live in the UI
```

The game allocates two buffers at session start — one for the target sentence, one for typed text — and frees both when the session ends.

### Screen — `src/screen.c`

No ncurses. The renderer writes ANSI escape sequences directly to stdout. Each frame saves the cursor position, overwrites only the lines that changed, and restores it — the terminal never flashes. Colors, bold, dim, and cursor visibility are all escape-code driven. The screen enters the alternate buffer (`\033[?1049h`) on startup and leaves it cleanly on exit, so the user's scrollback history is never touched.

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

### Keyboard — `src/keyboard.c`

By default, the terminal buffers input until Enter and echoes every character back to the screen. Both behaviors break a typing game. The keyboard library calls `tcgetattr()` to read the current terminal attributes, clears `ICANON` and `ECHO`, and writes them back with `tcsetattr()`. It also sets `O_NONBLOCK` on stdin via `fcntl()` so reads return immediately whether or not a key was pressed.

```c
void os_keyboard_init(void);
void os_keyboard_restore(void);
int  os_key_pressed(char *out);   // 1 if key available, 0 otherwise
```

ESC sequences (arrow keys, function keys) are detected by reading the full three-byte sequence and classifying it before the game loop sees it.

---

## Signal handling

Three signal handlers are registered at startup:

- **`SIGINT` / `SIGTERM`** — set a volatile flag the game loop checks, triggering a clean exit with terminal restoration
- **`SIGWINCH`** — fired by the OS when the terminal is resized; sets a flag that causes the current screen to fully redraw on the next frame

The resize handler means the UI stays correct whether the user resizes mid-game or between menus.

## System calls used

| Call | Purpose |
|------|---------|
| `tcgetattr` / `tcsetattr` | Switch terminal between cooked and raw mode |
| `fcntl(F_SETFL, O_NONBLOCK)` | Make stdin reads non-blocking |
| `read()` | Pull keyboard bytes directly from the file descriptor |
| `ioctl(TIOCGWINSZ)` | Query live terminal dimensions |
| `signal()` | Register `SIGINT`, `SIGTERM`, `SIGWINCH` handlers |
| `time()` | Unix timestamp for elapsed time and WPM calculation |
| `usleep()` | Microsecond sleep to cap frame rate without busy-waiting |
| `fopen` / `fread` / `fwrite` / `fclose` | Binary high-score persistence |
| `isatty()` | Validate stdin/stdout are real TTYs before entering raw mode |

---

## Why build this?

Every character you type in most applications travels through a deep stack before reaching the screen — language runtimes, libc, ncurses, OS abstraction layers. Most of the time that's fine. But understanding what the stack is actually doing requires going below it at least once.

This project is that exercise. The constraints are real: no `<string.h>`, no `<math.h>`, no heap allocator from the OS. And the result has to be something you'd actually want to use — not a toy stub, but a finished application. We think that bar matters. It's easy to claim you "implemented malloc" when you write twenty lines and call it done. It's harder when the malloc has to run a real program without leaking or fragmenting.

Building it this way taught us things that reading about systems never quite does. Watching the allocator's free bytes counter tick down in the live stats display — knowing exactly where that number comes from because you wrote every line between the game loop and the byte array — is a different kind of understanding.

---

Developed by **Kush Agarwal** and **Himanshu Rawat**
