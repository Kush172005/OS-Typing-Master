#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "keyboard.h"
#include "math.h"
#include "memory.h"
#include "screen.h"
#include "string.h"

#define TYPED_BUFFER_SIZE 256
#define START_LIVES 3

// Jaise ki - test: 05
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

// Longest correct prefix hmare streak ko represent karta hain
static int prefix_run_len(const char *typed, const char *target, int typed_len) {
    int n = 0;

    while (n < typed_len && typed[n] == target[n]) {
        n++;
    }
    return n;
}

static void play_game_over_animation(void) {
    int f;

    for (f = 0; f < 8; f++) {
        os_screen_begin_frame();
        os_screen_reset_color();
        os_screen_draw_text(2, 4, "       .");
        os_screen_draw_text(2, 5, "      ***");
        os_screen_draw_text(2, 6, "     *****");
        if (f % 2 == 0) {
            os_screen_draw_text(2, 8, "    \\ | /");
            os_screen_draw_text(2, 9, "  --- * ---");
            os_screen_draw_text(2, 10, "    / | \\");
        } else {
            os_screen_draw_text(2, 8, "   \\  |  /");
            os_screen_draw_text(2, 9, " ---- * ----");
            os_screen_draw_text(2, 10, "   /  |  \\");
        }
        os_screen_draw_text(2, 13, "||||||||||||||||||||||||||||||");
        os_screen_draw_text(2, 15, "           B O O M");
        os_screen_draw_text(2, 17, "        GAME OVER");
        os_screen_draw_text(2, 19, "     No lives left - try again!");
        os_screen_flush();
        usleep(110000);
    }
}

static void play_win_animation(void) {
    int f;

    for (f = 0; f < 10; f++) {
        os_screen_begin_frame();
        os_screen_reset_color();
        os_screen_draw_text(2, 3, "       *     *     *     *");
        os_screen_draw_text(2, 4, "    *     *     *     *");
        if (f % 2 == 0) {
            os_screen_draw_text(2, 6, "         \\ o /");
            os_screen_draw_text(2, 7, "          |");
            os_screen_draw_text(2, 8, "         / \\");
        } else {
            os_screen_draw_text(2, 6, "         \\ ^ /");
            os_screen_draw_text(2, 7, "          |");
            os_screen_draw_text(2, 8, "         / \\");
        }
        os_screen_draw_text(2, 10, "   ********************************");
        os_screen_draw_text(2, 12, "      CONGRATULATIONS!");
        os_screen_draw_text(2, 14, "   You cleared the wave perfectly!");
        os_screen_draw_text(2, 16, "        Typing Rush - WIN");
        os_screen_flush();
        usleep(100000);
    }
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
    int finished,
    int score,
    int run_streak,
    int best_run,
    int lives
) {
    char line[128];
    char first_word[64];

    os_screen_begin_frame();
    os_screen_reset_color();

    os_screen_draw_text(2, 1, "=== Typing Rush  Wave 1 ===");
    os_screen_draw_text(2, 2, "ESC/q quit  |  Backspace  |  Enter (ignored)");

    safe_number_text(line, "Score: ", score);
    os_screen_draw_text(2, 3, line);
    safe_number_text(line, "Run: ", run_streak);
    os_screen_draw_text(28, 3, line);
    safe_number_text(line, "Best run: ", best_run);
    os_screen_draw_text(50, 3, line);
    safe_number_text(line, "Lives: ", lives);
    os_screen_draw_text(2, 4, line);

    os_screen_draw_text(2, 6, "Lesson: green = ok, red = typo, dim = not typed yet");
    os_screen_draw_text(2, 7, ">>");
    os_screen_draw_typing_overlay(6, 7, target, typed, typed_len);

    os_split_first_token(target, first_word, 64);
    os_screen_draw_text(2, 9, "First token from split():");
    os_screen_draw_text(28, 9, first_word);

    safe_number_text(line, "Typed chars: ", typed_len);
    os_screen_draw_text(2, 11, line);

    safe_number_text(line, "Correct chars: ", correct_count);
    os_screen_draw_text(2, 12, line);

    safe_number_text(line, "Total key checks: ", total_count);
    os_screen_draw_text(2, 13, line);

    safe_number_text(line, "Progress (%): ", progress_percent);
    os_screen_draw_text(2, 14, line);

    safe_number_text(line, "Elapsed seconds: ", elapsed_seconds);
    os_screen_draw_text(2, 15, line);

    safe_number_text(line, "Virtual free memory: ", free_bytes);
    os_screen_draw_text(2, 16, line);

    if (finished) {
        os_screen_draw_text(2, 18, "Status: Perfect line - ESC or q to exit.");
    } else {
        os_screen_draw_text(2, 18, "Status: Playing...");
    }

    os_screen_flush();
}

int main(void) {
    const char *constant_sentence = "practice daily and type with focus";
    int target_len;
    int typed_len = 0;
    int correct_count = 0;
    int total_checks = 0;
    int score = 0;
    int run_streak = 0;
    int best_run = 0;
    time_t start_time;
    int last_elapsed_shown;
    int exited_perfect = 0;
    int finished = 0;
    int lives = START_LIVES;
    int i;

    char *target_sentence;
    char *typed_text;

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        printf("Please run this program in a real terminal (interactive TTY).\n");
        printf("Example: open Terminal app, then run ./typing_tutor\n");
        return 1;
    }

    os_memory_init();

    target_len = os_strlen(constant_sentence);

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
    os_screen_alt_screen_enter();
    start_time = time(NULL);
    last_elapsed_shown = -1;

    while (1) {
        char key;
        int elapsed;
        int progress;
        int free_bytes;
        int need_repaint = 0;
        int should_exit = 0;

        while (os_key_pressed(&key)) {
            need_repaint = 1;
            if (key == 27) {
                if (os_keyboard_esc_is_lone()) {
                    should_exit = 1;
                    break;
                }
                continue;
            }
            if (key == 'q' || key == 'Q') {
                should_exit = 1;
                break;
            }

            if (key == '\n' || key == '\r') {
                continue;
            }

            if (key == 127 || key == 8) {
                if (typed_len > 0) {
                    typed_len--;
                    typed_text[typed_len] = '\0';
                    correct_count = 0;
                    for (i = 0; i < typed_len; i++) {
                        if (typed_text[i] == target_sentence[i]) {
                            correct_count++;
                        }
                    }
                    run_streak = prefix_run_len(typed_text, target_sentence, typed_len);
                }
                continue;
            }

            if (typed_len >= target_len) {
                continue;
            }

            if (os_in_bounds(typed_len, 0, TYPED_BUFFER_SIZE - 1)) {
                typed_text[typed_len] = key;
                typed_len++;
                typed_text[typed_len] = '\0';

                total_checks++;
                if (os_in_bounds(typed_len - 1, 0, target_len) &&
                    key == target_sentence[typed_len - 1]) {
                    correct_count++;
                    score += 10;
                } else {
                    lives--;
                }

                run_streak = prefix_run_len(typed_text, target_sentence, typed_len);
                if (run_streak > best_run) {
                    best_run = run_streak;
                }

                if (lives <= 0) {
                    draw_ui(
                        target_sentence,
                        typed_text,
                        typed_len,
                        correct_count,
                        total_checks,
                        target_len > 0
                            ? os_clamp(os_div(os_mul(typed_len, 100), target_len), 0, 100)
                            : 0,
                        (int)(time(NULL) - start_time),
                        os_memory_free_bytes(),
                        0,
                        score,
                        run_streak,
                        best_run,
                        0);
                    usleep(350000);
                    play_game_over_animation();
                    should_exit = 1;
                    break;
                }

                if (typed_len == target_len && correct_count == target_len) {
                    finished = 1;
                    draw_ui(
                        target_sentence,
                        typed_text,
                        typed_len,
                        correct_count,
                        total_checks,
                        100,
                        (int)(time(NULL) - start_time),
                        os_memory_free_bytes(),
                        finished,
                        score,
                        run_streak,
                        best_run,
                        lives);
                    usleep(250000);
                    exited_perfect = 1;
                    should_exit = 1;
                    break;
                }
            }
        }

        if (should_exit) {
            break;
        }

        finished = (typed_len == target_len && correct_count == target_len) ? 1 : 0;

        progress = 0;
        if (target_len > 0) {
            progress = os_div(os_mul(typed_len, 100), target_len);
            progress = os_clamp(progress, 0, 100);
        }

        elapsed = (int)(time(NULL) - start_time);
        free_bytes = os_memory_free_bytes();

        if (elapsed != last_elapsed_shown) {
            need_repaint = 1;
        }

        if (need_repaint) {
            draw_ui(
                target_sentence,
                typed_text,
                typed_len,
                correct_count,
                total_checks,
                progress,
                elapsed,
                free_bytes,
                finished,
                score,
                run_streak,
                best_run,
                lives);
            last_elapsed_shown = elapsed;
        }

        usleep(8000);
    }

    if (exited_perfect) {
        play_win_animation();
    }

    os_screen_alt_screen_leave();
    os_screen_show_cursor();
    os_keyboard_shutdown();

    os_dealloc(typed_text);
    os_dealloc(target_sentence);

    printf("Typing Tutor closed cleanly.\n");
    (void)fflush(stdout);
    return 0;
}
