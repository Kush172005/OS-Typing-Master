#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "screen.h"

int os_screen_term_cols(void) {
    struct winsize ws;

    /* Terminal ki width runtime par padhte hain, taaki resize ke baad text clip ho sake. */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return (int)ws.ws_col;
    }
    return 80;
}

int os_screen_term_rows(void) {
    struct winsize ws;

    /* Terminal minimize/resize ho to available rows ka updated value yahan se milta hai. */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) {
        return (int)ws.ws_row;
    }
    return 24;
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
    /* Cursor ko valid terminal coordinate me rakho; 0 ya negative values unsafe hoti hain. */
    if (x < 1) {
        x = 1;
    }
    if (y < 1) {
        y = 1;
    }
    printf("\033[%d;%dH", y, x);
}

static int clip_budget(int x) {
    int cols = os_screen_term_cols();
    int b = cols - x + 1;

    /* Jitni jagah screen par bachi hai utna hi print karna hai, overflow nahi. */
    if (b < 1) {
        return 0;
    }
    return b;
}

void os_screen_draw_text(int x, int y, const char *text) {
    int budget;
    int i = 0;

    /* Agar terminal chhota ho gaya hai aur row visible nahi hai, draw skip karo. */
    if (y > os_screen_term_rows()) {
        return;
    }

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

    /* Resize/minimize ke baad hidden row par likhne ki koshish mat karo. */
    if (y > os_screen_term_rows()) {
        return;
    }

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
