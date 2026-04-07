#include <stdio.h>

#include "screen.h"

/* Clear full terminal and move cursor to top-left. */
void os_screen_clear(void) {
    printf("\033[2J");
    printf("\033[H");
}

/* Move cursor to x,y coordinate. */
void os_screen_move_cursor(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

/* Draw text at a fixed position. */
void os_screen_draw_text(int x, int y, const char *text) {
    os_screen_move_cursor(x, y);
    printf("%s", text);
}

void os_screen_hide_cursor(void) {
    printf("\033[?25l");
}

void os_screen_show_cursor(void) {
    printf("\033[?25h");
}

/* Flush output so the frame appears immediately. */
void os_screen_flush(void) {
    fflush(stdout);
}
