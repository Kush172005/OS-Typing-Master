#include <stdio.h>

#include "screen.h"

void os_screen_clear(void) {
    printf("\033[2J");
    printf("\033[H");
}

void os_screen_begin_frame(void) {
    printf("\033[H");
}

void os_screen_move_cursor(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

void os_screen_draw_text(int x, int y, const char *text) {
    os_screen_move_cursor(x, y);
    printf("%s", text);
    printf("\033[K");
}

void os_screen_draw_typing_overlay(int x, int y, const char *target,
                                   const char *typed, int typed_len) {
    int i = 0;

    while (target[i] != '\0') {
        os_screen_move_cursor(x + i, y);
        if (i < typed_len) {
            if (typed[i] == target[i]) {
                os_screen_set_color("1;32");
            } else {
                os_screen_set_color("1;31");
            }
            printf("%c", typed[i]);
        } else {
            os_screen_set_color("2;37");
            printf("%c", target[i]);
        }
        i++;
    }
    os_screen_reset_color();
    os_screen_move_cursor(x + i, y);
    printf("\033[K");
}

void os_screen_set_color(const char *color_code) {
    printf("\033[%sm", color_code);
}

void os_screen_reset_color(void) {
    printf("\033[0m");
}

void os_screen_hide_cursor(void) {
    printf("\033[?25l");
}

void os_screen_show_cursor(void) {
    printf("\033[?25h");
}

void os_screen_flush(void) {
    fflush(stdout);
}
