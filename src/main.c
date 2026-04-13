#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "keyboard.h"
#include "math.h"
#include "memory.h"
#include "screen.h"
#include "string.h"

#define TYPED_BUFFER_SIZE 256

/* Build one UI line by joining a label and number text. */
static void safe_number_text(char *dest, const char *label, int value) {
    char num[32];
    int write_index = 0;
    int i = 0;

    while (label[i] != '\0') {
        dest[write_index] = label[i];
        write_index++;
        i++;
    }

    os_int_to_string(value, num, 32);

    i = 0;
    while (num[i] != '\0') {
        dest[write_index] = num[i];
        write_index++;
        i++;
    }

    dest[write_index] = '\0';
}

static void draw_ui(
    const char *target,
    const char *typed,
    int typed_len,
    int correct_count,
    int total_count,
    int progress_percent,
    int elapsed_seconds,
    int free_bytes,
    int finished
) {
    char line[128];
    char first_word[64];

    os_screen_begin_frame();
    os_screen_draw_text(2, 1, "==================== Typing Tutor - Phase 1 ====================");
    os_screen_draw_text(2, 2, "Controls: ESC/q = quit | Backspace = delete | Enter = finish");

    os_screen_draw_text(2, 4, "Type over the line (green=ok, red=wrong, dim=still to type):");
    os_screen_draw_text(2, 5, ">>");
    os_screen_draw_typing_overlay(6, 5, target, typed, typed_len);

    os_split_first_token(target, first_word, 64);
    os_screen_draw_text(2, 7, "First token from split():");
    os_screen_draw_text(28, 7, first_word);

    safe_number_text(line, "Typed chars: ", typed_len);
    os_screen_draw_text(2, 9, line);

    safe_number_text(line, "Correct chars: ", correct_count);
    os_screen_draw_text(2, 10, line);

    safe_number_text(line, "Total key checks: ", total_count);
    os_screen_draw_text(2, 11, line);

    safe_number_text(line, "Progress (%): ", progress_percent);
    os_screen_draw_text(2, 12, line);

    safe_number_text(line, "Elapsed seconds: ", elapsed_seconds);
    os_screen_draw_text(2, 13, line);

    safe_number_text(line, "Virtual free memory: ", free_bytes);
    os_screen_draw_text(2, 14, line);

    if (finished) {
        os_screen_draw_text(2, 16, "Status: Completed. Press ESC or q to exit.");
    } else {
        os_screen_draw_text(2, 16, "Status: Running...");
    }

    os_screen_flush();
}

int main(void) {
    const char *constant_sentence = "practice daily and type with focus";
    int target_len;
    int typed_len = 0;
    int correct_count = 0;
    int total_checks = 0;
    int done = 0;
    time_t start_time;

    char *target_sentence;
    char *typed_text;

    /* This app needs an interactive terminal for live screen control. */
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        printf("Please run this program in a real terminal (interactive TTY).\n");
        printf("Example: open Terminal app, then run ./typing_tutor\n");
        return 1;
    }

    os_memory_init();

    target_len = os_strlen(constant_sentence);

    /* Dynamic allocation from custom memory.c */
    target_sentence = (char *)os_alloc(target_len + 1);
    typed_text = (char *)os_alloc(TYPED_BUFFER_SIZE);

    if (target_sentence == NULL || typed_text == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    os_strcpy(target_sentence, constant_sentence);
    typed_text[0] = '\0';

    if (!os_keyboard_init()) {
        printf("Keyboard init failed.\n");
        os_dealloc(typed_text);
        os_dealloc(target_sentence);
        return 1;
    }

    os_screen_hide_cursor();
    os_screen_clear();
    start_time = time(NULL);

    /* Main real-time typing loop. */
    while (1) {
        char key;
        int elapsed;
        int progress;
        int free_bytes;

        /* Non-blocking input read. */
        if (os_key_pressed(&key)) {
            if (key == 27 || key == 'q' || key == 'Q') {
                break;
            }

            if (!done && (key == '\n' || key == '\r')) {
                done = 1;
            } else if (!done && (key == 127 || key == 8)) {
                if (typed_len > 0) {
                    typed_len--;
                    typed_text[typed_len] = '\0';
                }
            } else if (!done) {
                /* Boundary check using custom math.c helper. */
                if (os_in_bounds(typed_len, 0, TYPED_BUFFER_SIZE - 1)) {
                    typed_text[typed_len] = key;
                    typed_len++;
                    typed_text[typed_len] = '\0';

                    total_checks++;
                    if (os_in_bounds(typed_len - 1, 0, target_len) &&
                        key == target_sentence[typed_len - 1]) {
                        correct_count++;
                    }

                    if (typed_len >= target_len) {
                        done = 1;
                    }
                }
            }
        }

        /* Clamp keeps percentage inside 0 to 100. */
        progress = 0;
        if (target_len > 0) {
            progress = os_div(os_mul(typed_len, 100), target_len);
            progress = os_clamp(progress, 0, 100);
        }

        elapsed = (int)(time(NULL) - start_time);
        free_bytes = os_memory_free_bytes();

        draw_ui(
            target_sentence,
            typed_text,
            typed_len,
            correct_count,
            total_checks,
            progress,
            elapsed,
            free_bytes,
            done
        );

        usleep(16000);
    }

    os_screen_show_cursor();
    os_screen_clear();
    os_keyboard_shutdown();

    /* Matching deallocation for every allocation. */
    os_dealloc(typed_text);
    os_dealloc(target_sentence);

    printf("Typing Tutor closed cleanly.\n");
    return 0;
}
