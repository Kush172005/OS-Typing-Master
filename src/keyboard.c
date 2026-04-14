#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include "keyboard.h"

static struct termios g_original_termios;
static int g_keyboard_ready = 0;

int os_keyboard_init(void) {
    struct termios raw;
    int flags;

    if (tcgetattr(STDIN_FILENO, &g_original_termios) == -1) {
        return 0;
    }

    raw = g_original_termios;
    raw.c_lflag &= (unsigned int)(~(ICANON | ECHO));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        return 0;
    }

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        return 0;
    }

    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        return 0;
    }

    g_keyboard_ready = 1;
    return 1;
}

void os_keyboard_shutdown(void) {
    if (!g_keyboard_ready) {
        return;
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_termios);
    g_keyboard_ready = 0;
}

int os_key_pressed(char *out_key) {
    char c;
    int read_count;

    if (!g_keyboard_ready) {
        return 0;
    }

    read_count = (int)read(STDIN_FILENO, &c, 1);
    if (read_count == 1) {
        *out_key = c;
        return 1;
    }

    return 0;
}

int os_read_line(char *buffer, int max_len) {
    int index = 0;

    if (max_len <= 1) {
        return 0;
    }

    while (1) {
        char c;
        int got = os_key_pressed(&c);

        if (!got) {
            usleep(10000);
            continue;
        }

        if (c == '\n' || c == '\r') {
            break;
        }

        if ((c == 127 || c == 8) && index > 0) {
            index--;
            continue;
        }

        if (index < max_len - 1) {
            buffer[index] = c;
            index++;
        }
    }

    buffer[index] = '\0';
    return index;
}
