#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "screen.h"

int os_screen_term_cols(void) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return (int)ws.ws_col;
    }
    return 80;
}

void os_screen_clear(void) {
    printf("\033[2J");
    printf("\033[H");
}

void os_screen_begin_frame(void) {
    printf("\033[2J");
    printf("\033[H");
}

void os_screen_alt_screen_enter(void) {
    printf("\033[?1049h");
    printf("\033[2J");
    printf("\033[H");
}

void os_screen_alt_screen_leave(void) {
    printf("\033[?1049l");
}

void os_screen_move_cursor(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

static int clip_budget(int x) {
    int cols = os_screen_term_cols();
    int b = cols - x + 1;

    if (b < 1) {
        return 0;
    }
    return b;
}

void os_screen_draw_text(int x, int y, const char *text) {
    int budget;
    int i = 0;

    budget = clip_budget(x);
    if (budget == 0) {
        return;
    }

    os_screen_move_cursor(x, y);
    while (text[i] != '\0' && i < budget) {
        putchar((unsigned char)text[i]);
        i++;
    }
    printf("\033[K");
}

void os_screen_draw_typing_overlay(int x, int y, const char *target,
                                   const char *typed, int typed_len) {
    int budget;
    int i = 0;

    budget = clip_budget(x);
    if (budget == 0) {
        return;
    }

    os_screen_move_cursor(x, y);
    
    while (target[i] != '\0' && i < budget) {
        if (i < typed_len) {
            if (typed[i] == target[i]) {
                os_screen_set_color("1;32");
            } else {
                os_screen_set_color("1;31");
            }
            printf("%c", typed[i]);
            os_screen_reset_color();
        } else {
            os_screen_set_color("2;37");
            printf("%c", target[i]);
            os_screen_reset_color();
        }
        i++;
    }
    
    printf("\033[K");
    fflush(stdout);
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
