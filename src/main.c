#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>

#include "keyboard.h"
#include "math.h"
#include "memory.h"
#include "screen.h"
#include "string.h"

#define TYPED_BUFFER_SIZE 512
#define KEY_END_ROUND '\t'
#define MAX_LESSONS 10
#define MAX_LESSON_LEN 256
#define HIGHSCORE_FILE ".typing_highscores.dat"
#define LESSONS_DIR "lessons"

typedef enum {
    STATE_MAIN_MENU,
    STATE_MODE_SELECT,
    STATE_DIFFICULTY_SELECT,
    STATE_PLAYING,
    STATE_ENTER_NAME,
    STATE_RESULTS,
    STATE_HIGHSCORES,
    STATE_QUIT
} GameState;

typedef enum {
    MODE_PRACTICE,
    MODE_TIMED_30,
    MODE_TIMED_60,
    MODE_SURVIVAL
} GameMode;

typedef enum {
    DIFF_EASY,
    DIFF_MEDIUM,
    DIFF_HARD
} Difficulty;

typedef struct {
    char name[32];
    int score;
    int wpm;
    int mode;
} HighScore;

typedef struct {
    GameState state;
    GameMode mode;
    Difficulty difficulty;
    int score;
    int lives;
    int wpm;
    int accuracy;
    int time_limit;
    int time_remaining;
    time_t game_start;
    int total_typed;
    int correct_typed;
    int finished;
    int lesson_index;
    char player_name[32];
} GameSession;

static const char *builtin_lessons_easy[] = {
    "the quick brown fox jumps over the lazy dog",
    "practice makes perfect typing skills better",
    "hello world this is a simple typing test"
};

static const char *builtin_lessons_medium[] = {
    "programming requires patience and dedication",
    "operating systems manage memory and processes",
    "data structures are fundamental to coding"
};

static const char *builtin_lessons_hard[] = {
    "efficient memory management prevents leaks",
    "multithreading concurrent synchronization",
    "polymorphism encapsulation abstraction"
};

static volatile sig_atomic_t g_interrupted = 0;
static volatile sig_atomic_t g_resized = 0;

static void interrupt_handler(int sig) {
    (void)sig;
    g_interrupted = 1;
}

static void resize_handler(int sig) {
    (void)sig;
    /* Terminal resize/minimize par quit nahi karna, bas next loop me redraw karna hai. */
    g_resized = 1;
}

static int take_resize_event(void) {
    /* Resize flag ko ek baar consume karo, taki har screen apna UI dobara draw kar sake. */
    if (!g_resized) {
        return 0;
    }

    g_resized = 0;
    return 1;
}

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

static int calculate_wpm(int correct_chars, int elapsed_seconds) {
    if (elapsed_seconds <= 0) {
        return 0;
    }

    if (correct_chars <= 0) {
        return 0;
    }

    /* WPM = (correct chars / 5) words per elapsed minute; 12 = 60 / 5. */
    return os_div(os_mul(correct_chars, 12), elapsed_seconds);
}

static int calculate_accuracy(int correct, int total) {
    if (total <= 0) {
        return 0;
    }
    if (correct < 0) {
        correct = 0;
    }
    if (correct > total) {
        correct = total;
    }
    return os_clamp(os_div(os_mul(correct, 100), total), 0, 100);
}

/* Score/current stats typed prefix se recalculate hote hain, backspace ke baad bhi sahi rahen. */
static void recompute_prefix_stats(GameSession *session, const char *target,
                                   const char *typed, int typed_len) {
    int i;
    int correct = 0;
    int tlen;

    if (session == NULL || target == NULL || typed == NULL || typed_len < 0) {
        return;
    }

    tlen = os_strlen(target);
    for (i = 0; i < typed_len && i < tlen; i++) {
        if (typed[i] == target[i]) {
            correct++;
        }
    }

    session->correct_typed = correct;
    session->total_typed = typed_len;
    session->score = os_mul(correct, 10);
}

static void draw_main_menu(void) {
    os_screen_begin_frame();
    os_screen_reset_color();

    os_screen_set_color("1;36");
    os_screen_draw_text(20, 2, "╔══════════════════════════════════════╗");
    os_screen_draw_text(20, 3, "║                                      ║");
    os_screen_draw_text(20, 4, "║        TYPING RUSH - PHASE 2         ║");
    os_screen_draw_text(20, 5, "║                                      ║");
    os_screen_draw_text(20, 6, "╚══════════════════════════════════════╝");
    os_screen_reset_color();

    os_screen_draw_text(25, 9, "1. Start Game");
    os_screen_draw_text(25, 11, "2. View High Scores");
    os_screen_draw_text(25, 13, "3. Quit");

    os_screen_draw_text(20, 17, "Press 1, 2, or 3 to select");
    os_screen_draw_text(20, 18, "ESC or q to quit anytime");

    os_screen_flush();
}

static void draw_mode_select(void) {
    os_screen_begin_frame();
    os_screen_reset_color();

    os_screen_set_color("1;33");
    os_screen_draw_text(25, 2, "SELECT GAME MODE");
    os_screen_reset_color();

    os_screen_draw_text(20, 5, "1. Practice Mode");
    os_screen_draw_text(23, 6, "   - No timer, learn at your pace");

    os_screen_draw_text(20, 8, "2. Timed Challenge (30s)");
    os_screen_draw_text(23, 9, "   - Complete as much as possible in 30 seconds");

    os_screen_draw_text(20, 11, "3. Timed Challenge (60s)");
    os_screen_draw_text(23, 12, "   - Complete as much as possible in 60 seconds");

    os_screen_draw_text(20, 14, "4. Survival Mode");
    os_screen_draw_text(23, 15, "   - 3 lives, lose one per mistake");

    os_screen_draw_text(20, 18, "Press 1-4 to select, ESC to go back");

    os_screen_flush();
}

static void draw_difficulty_select(void) {
    os_screen_begin_frame();
    os_screen_reset_color();

    os_screen_set_color("1;32");
    os_screen_draw_text(25, 2, "SELECT DIFFICULTY");
    os_screen_reset_color();

    os_screen_draw_text(20, 5, "1. Easy");
    os_screen_draw_text(23, 6, "   - Short, simple phrases");

    os_screen_draw_text(20, 8, "2. Medium");
    os_screen_draw_text(23, 9, "   - Moderate length sentences");

    os_screen_draw_text(20, 11, "3. Hard");
    os_screen_draw_text(23, 12, "   - Complex technical terms");

    os_screen_draw_text(20, 15, "Press 1-3 to select, ESC to go back");

    os_screen_flush();
}

static void play_win_animation(void) {
    int f;
    for (f = 0; f < 8; f++) {
        os_screen_begin_frame();
        os_screen_reset_color();
        os_screen_set_color("1;32");
        os_screen_draw_text(25, 5, "    *     *     *     *");
        if (f % 2 == 0) {
            os_screen_draw_text(28, 7, "    \\ o /");
            os_screen_draw_text(28, 8, "     |");
            os_screen_draw_text(28, 9, "    / \\");
        } else {
            os_screen_draw_text(28, 7, "    \\ ^ /");
            os_screen_draw_text(28, 8, "     |");
            os_screen_draw_text(28, 9, "    / \\");
        }
        os_screen_draw_text(20, 12, "================================");
        os_screen_draw_text(24, 14, "CONGRATULATIONS!");
        os_screen_draw_text(22, 16, "Level Complete!");
        os_screen_reset_color();
        os_screen_flush();
        usleep(120000);
    }
}

static void play_game_over_animation(void) {
    int f;
    for (f = 0; f < 8; f++) {
        os_screen_begin_frame();
        os_screen_reset_color();
        os_screen_set_color("1;31");
        if (f % 2 == 0) {
            os_screen_draw_text(28, 7, "  \\ | /");
            os_screen_draw_text(28, 8, " -- * --");
            os_screen_draw_text(28, 9, "  / | \\");
        } else {
            os_screen_draw_text(28, 7, " \\  |  /");
            os_screen_draw_text(28, 8, "--- * ---");
            os_screen_draw_text(28, 9, " /  |  \\");
        }
        os_screen_draw_text(20, 12, "================================");
        os_screen_draw_text(27, 14, "B O O M!");
        os_screen_draw_text(26, 16, "GAME OVER");
        os_screen_reset_color();
        os_screen_flush();
        usleep(120000);
    }
}

static void draw_game_ui(GameSession *session, const char *target, const char *typed, int typed_len) {
    char line[128];
    int elapsed;

    if (session == NULL || target == NULL || typed == NULL || typed_len < 0) {
        return;
    }

    elapsed = (int)(time(NULL) - session->game_start);

    os_screen_begin_frame();
    os_screen_reset_color();

    os_screen_set_color("1;36");
    os_screen_draw_text(2, 1, "=== TYPING RUSH ===");
    os_screen_reset_color();

    if (session->mode == MODE_PRACTICE) {
        os_screen_draw_text(2, 2, "Mode: Practice");
    } else if (session->mode == MODE_TIMED_30) {
        os_screen_draw_text(2, 2, "Mode: Timed 30s");
        session->time_remaining = session->time_limit - elapsed;
        if (session->time_remaining < 0) {
            session->time_remaining = 0;
        }
        safe_number_text(line, "Time: ", session->time_remaining);
        os_screen_draw_text(30, 2, line);
    } else if (session->mode == MODE_TIMED_60) {
        os_screen_draw_text(2, 2, "Mode: Timed 60s");
        session->time_remaining = session->time_limit - elapsed;
        if (session->time_remaining < 0) {
            session->time_remaining = 0;
        }
        safe_number_text(line, "Time: ", session->time_remaining);
        os_screen_draw_text(30, 2, line);
    } else if (session->mode == MODE_SURVIVAL) {
        os_screen_draw_text(2, 2, "Mode: Survival");
        safe_number_text(line, "Lives: ", session->lives);
        os_screen_draw_text(30, 2, line);
    }

    safe_number_text(line, "Score: ", session->score);
    os_screen_draw_text(2, 3, line);

    session->wpm = calculate_wpm(session->correct_typed, elapsed > 0 ? elapsed : 1);
    safe_number_text(line, "WPM: ", session->wpm);
    os_screen_draw_text(30, 3, line);

    session->accuracy = calculate_accuracy(session->correct_typed, session->total_typed);
    safe_number_text(line, "Accuracy: ", session->accuracy);
    os_screen_draw_text(2, 4, line);
    os_screen_draw_text(os_strlen(line) + 3, 4, "%");

    if (session->mode == MODE_TIMED_30 || session->mode == MODE_TIMED_60) {
        int progress = os_div(os_mul(elapsed, 20), session->time_limit);
        int i;
        char bar[32];
        
        os_screen_draw_text(50, 2, "[");
        for (i = 0; i < 20 && i < 30; i++) {
            if (i < progress) {
                bar[i] = '=';
            } else {
                bar[i] = ' ';
            }
        }
        bar[20] = '\0';
        os_screen_draw_text(51, 2, bar);
        os_screen_draw_text(71, 2, "]");
    }

    os_screen_draw_text(2, 6, "Type the text below:");
    os_screen_draw_text(2, 8, ">>");
    os_screen_draw_typing_overlay(5, 8, target, typed, typed_len);

    if (session->finished && typed_len > 0) {
        os_screen_set_color("1;32");
        os_screen_draw_text(2, 10, "*** COMPLETE! ***");
        os_screen_reset_color();
    }

    os_screen_draw_text(2, 12, "Tab=End round & results | ESC=Menu | Backspace=Delete");
    
    if (session->mode == MODE_SURVIVAL && session->lives <= 1 && !session->finished) {
        os_screen_set_color("1;31");
        os_screen_draw_text(2, 13, "WARNING: Last life remaining!");
        os_screen_reset_color();
    }

    os_screen_flush();
}

static void draw_results(GameSession *session) {
    char line[128];

    if (session == NULL) {
        return;
    }

    os_screen_begin_frame();
    os_screen_reset_color();

    os_screen_set_color("1;33");
    os_screen_draw_text(25, 2, "=== RESULTS ===");
    os_screen_reset_color();

    safe_number_text(line, "Score: ", session->score);
    os_screen_draw_text(20, 5, line);

    safe_number_text(line, "WPM: ", session->wpm);
    os_screen_draw_text(20, 7, line);

    safe_number_text(line, "Accuracy: ", session->accuracy);
    os_screen_draw_text(20, 9, line);

    safe_number_text(line, "Correct: ", session->correct_typed);
    os_screen_draw_text(20, 11, line);

    safe_number_text(line, "Total: ", session->total_typed);
    os_screen_draw_text(20, 12, line);

    if (session->finished) {
        os_screen_set_color("1;32");
        os_screen_draw_text(20, 15, "STATUS: COMPLETED!");
        os_screen_reset_color();
    } else {
        os_screen_draw_text(20, 15, "STATUS: Incomplete");
    }

    if (session->accuracy >= 95) {
        os_screen_set_color("1;32");
        os_screen_draw_text(20, 17, "Performance: EXCELLENT!");
    } else if (session->accuracy >= 80) {
        os_screen_set_color("1;33");
        os_screen_draw_text(20, 17, "Performance: GOOD");
    } else {
        os_screen_set_color("1;31");
        os_screen_draw_text(20, 17, "Performance: NEEDS PRACTICE");
    }
    os_screen_reset_color();

    os_screen_draw_text(20, 20, "Press any key to continue...");

    os_screen_flush();
}

static void load_highscores(HighScore *scores, int *count) {
    FILE *f;
    int i;
    HighScore temp;

    *count = 0;
    
    if (scores == NULL || count == NULL) {
        return;
    }

    f = fopen(HIGHSCORE_FILE, "rb");
    if (f == NULL) {
        return;
    }

    for (i = 0; i < 10; i++) {
        if (fread(&temp, sizeof(HighScore), 1, f) != 1) {
            break;
        }
        
        if (temp.score < 0 || temp.wpm < 0 || temp.wpm > 500) {
            break;
        }
        
        temp.name[31] = '\0';
        scores[i] = temp;
        (*count)++;
    }

    fclose(f);
}

static void save_highscores(HighScore *scores, int count) {
    FILE *f;
    int i;

    if (scores == NULL || count < 0 || count > 10) {
        return;
    }

    f = fopen(HIGHSCORE_FILE, "wb");
    if (f == NULL) {
        return;
    }

    for (i = 0; i < count && i < 10; i++) {
        if (fwrite(&scores[i], sizeof(HighScore), 1, f) != 1) {
            fclose(f);
            return;
        }
    }

    fclose(f);
}

static void add_highscore(GameSession *session) {
    HighScore scores[11];
    int count;
    int i, j;
    HighScore new_score;

    if (session == NULL) {
        return;
    }

    load_highscores(scores, &count);

    if (session->score < 0 || session->wpm < 0) {
        return;
    }

    new_score.score = session->score;
    new_score.wpm = session->wpm;
    new_score.mode = session->mode;

    for (i = 0; i < 31; i++) {
        new_score.name[i] = session->player_name[i];
        if (session->player_name[i] == '\0') {
            break;
        }
    }
    new_score.name[31] = '\0';

    for (i = 0; i <= count && i < 10; i++) {
        if (i == count || new_score.score > scores[i].score) {
            for (j = count; j > i; j--) {
                if (j < 10) {
                    scores[j] = scores[j - 1];
                }
            }
            if (i < 10) {
                scores[i] = new_score;
            }
            if (count < 10) {
                count++;
            }
            break;
        }
    }

    save_highscores(scores, count);
}

static void draw_highscores(void) {
    HighScore scores[10];
    int count;
    int i;
    char line[128];

    load_highscores(scores, &count);

    os_screen_begin_frame();
    os_screen_reset_color();

    os_screen_set_color("1;33");
    os_screen_draw_text(25, 2, "=== HIGH SCORES ===");
    os_screen_reset_color();

    os_screen_draw_text(15, 4, "Rank   Name        Score    WPM    Mode");
    os_screen_draw_text(15, 5, "----   ----        -----    ---    ----");

    for (i = 0; i < count && i < 10; i++) {
        char rank[8];
        os_int_to_string(i + 1, rank, 8);

        os_screen_draw_text(15, 7 + i, rank);
        os_screen_draw_text(22, 7 + i, scores[i].name);

        safe_number_text(line, "", scores[i].score);
        os_screen_draw_text(34, 7 + i, line);

        safe_number_text(line, "", scores[i].wpm);
        os_screen_draw_text(43, 7 + i, line);

        if (scores[i].mode == MODE_PRACTICE) {
            os_screen_draw_text(50, 7 + i, "Practice");
        } else if (scores[i].mode == MODE_TIMED_30) {
            os_screen_draw_text(50, 7 + i, "Timed30");
        } else if (scores[i].mode == MODE_TIMED_60) {
            os_screen_draw_text(50, 7 + i, "Timed60");
        } else {
            os_screen_draw_text(50, 7 + i, "Survival");
        }
    }

    if (count == 0) {
        os_screen_draw_text(25, 10, "No high scores yet!");
    }

    os_screen_draw_text(20, 19, "Press any key to return...");

    os_screen_flush();
}

static const char *get_lesson_for_difficulty(Difficulty diff, int index) {
    int max_easy = 3;
    int max_medium = 3;
    int max_hard = 3;
    int safe_index = index;

    if (safe_index < 0) {
        safe_index = 0;
    }

    if (diff == DIFF_EASY) {
        return builtin_lessons_easy[safe_index % max_easy];
    } else if (diff == DIFF_MEDIUM) {
        return builtin_lessons_medium[safe_index % max_medium];
    } else {
        return builtin_lessons_hard[safe_index % max_hard];
    }
}

static void run_game(GameSession *session) {
    char *target_sentence;
    char *typed_text;
    const char *lesson;
    int target_len;
    int typed_len = 0;
    int last_second = -1;

    lesson = get_lesson_for_difficulty(session->difficulty, session->lesson_index);
    target_len = os_strlen(lesson);

    target_sentence = (char *)os_alloc(target_len + 1);
    typed_text = (char *)os_alloc(TYPED_BUFFER_SIZE);

    if (target_sentence == NULL || typed_text == NULL) {
        if (target_sentence) {
            os_dealloc(target_sentence);
        }
        if (typed_text) {
            os_dealloc(typed_text);
        }
        return;
    }

    os_strcpy(target_sentence, lesson);
    typed_text[0] = '\0';

    session->game_start = time(NULL);
    session->score = 0;
    session->total_typed = 0;
    session->correct_typed = 0;
    session->finished = 0;

    if (session->mode == MODE_SURVIVAL) {
        session->lives = 3;
    }

    draw_game_ui(session, target_sentence, typed_text, 0);

    while (!g_interrupted) {
        char key;
        int need_repaint = 0;
        int elapsed;
        int current_second;

        /* Resize/minimize event aaya to game band nahi hoga, sirf screen repaint hogi. */
        if (take_resize_event()) {
            need_repaint = 1;
        }

        while (os_key_pressed(&key)) {
            need_repaint = 1;

            if (key == KEY_END_ROUND) {
                session->finished = 0;
                recompute_prefix_stats(session, target_sentence, typed_text, typed_len);
                draw_game_ui(session, target_sentence, typed_text, typed_len);
                usleep(400000);
                os_dealloc(typed_text);
                os_dealloc(target_sentence);
                session->state = STATE_ENTER_NAME;
                return;
            }

            if (key == 27) {
                if (os_keyboard_esc_is_lone()) {
                    os_dealloc(typed_text);
                    os_dealloc(target_sentence);
                    session->state = STATE_MAIN_MENU;
                    return;
                }
                continue;
            }

            if (key == '\n' || key == '\r') {
                continue;
            }

            if (key == 127 || key == 8) {
                if (typed_len > 0) {
                    typed_len--;
                    typed_text[typed_len] = '\0';
                    recompute_prefix_stats(session, target_sentence, typed_text, typed_len);
                }
                continue;
            }

            if (typed_len >= target_len) {
                continue;
            }

            /* Room for new char + trailing NUL: typed_len < TYPED_BUFFER_SIZE - 1 */
            if (os_in_bounds(typed_len, 0, TYPED_BUFFER_SIZE - 1) &&
                os_in_bounds(typed_len, 0, target_len)) {
                typed_text[typed_len] = key;
                typed_len++;
                typed_text[typed_len] = '\0';

                if (key != target_sentence[typed_len - 1]) {
                    if (session->mode == MODE_SURVIVAL) {
                        session->lives--;
                        if (session->lives <= 0) {
                            session->lives = 0;
                            recompute_prefix_stats(session, target_sentence, typed_text,
                                                   typed_len);
                            draw_game_ui(session, target_sentence, typed_text, typed_len);
                            usleep(800000);
                            play_game_over_animation();
                            os_dealloc(typed_text);
                            os_dealloc(target_sentence);
                            session->state = STATE_ENTER_NAME;
                            return;
                        }
                    }
                }

                recompute_prefix_stats(session, target_sentence, typed_text, typed_len);

                if (typed_len == target_len) {
                    session->finished = 1;
                    draw_game_ui(session, target_sentence, typed_text, typed_len);
                    usleep(1200000);
                    
                    if (session->correct_typed == target_len) {
                        play_win_animation();
                    }
                    
                    os_dealloc(typed_text);
                    os_dealloc(target_sentence);
                    session->state = STATE_ENTER_NAME;
                    return;
                }
            }
        }

        elapsed = (int)(time(NULL) - session->game_start);
        current_second = elapsed;

        if (session->mode == MODE_TIMED_30 || session->mode == MODE_TIMED_60) {
            if (elapsed >= session->time_limit) {
                session->finished = 0;
                session->time_remaining = 0;
                recompute_prefix_stats(session, target_sentence, typed_text, typed_len);
                draw_game_ui(session, target_sentence, typed_text, typed_len);
                usleep(800000);
                os_dealloc(typed_text);
                os_dealloc(target_sentence);
                session->state = STATE_ENTER_NAME;
                return;
            }
            if (current_second != last_second) {
                need_repaint = 1;
                last_second = current_second;
            }
        }

        if (need_repaint) {
            draw_game_ui(session, target_sentence, typed_text, typed_len);
        }

        usleep(16000);
    }

    os_dealloc(typed_text);
    os_dealloc(target_sentence);
}

int main(void) {
    GameSession session;
    char key;

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        printf("Please run in a real terminal.\n");
        return 1;
    }

    signal(SIGINT, interrupt_handler);
    signal(SIGTERM, interrupt_handler);
    signal(SIGWINCH, resize_handler); /* Terminal resize ka signal, game state ko change nahi karta. */

    os_memory_init();

    if (!os_keyboard_init()) {
        printf("Keyboard init failed.\n");
        return 1;
    }

    os_screen_hide_cursor();
    os_screen_alt_screen_enter();

    session.state = STATE_MAIN_MENU;
    session.mode = MODE_PRACTICE;
    session.difficulty = DIFF_EASY;
    session.lesson_index = 0;
    session.time_limit = 30;

    while (session.state != STATE_QUIT && !g_interrupted) {
        if (session.state == STATE_MAIN_MENU) {
            draw_main_menu();
            while (!g_interrupted) {
                /* Menu screen ko new terminal size ke hisaab se dobara draw karo. */
                if (take_resize_event()) {
                    draw_main_menu();
                }

                if (os_key_pressed(&key)) {
                    if (key == '1') {
                        session.state = STATE_MODE_SELECT;
                        break;
                    } else if (key == '2') {
                        session.state = STATE_HIGHSCORES;
                        break;
                    } else if (key == '3' || key == 'q' || key == 'Q') {
                        session.state = STATE_QUIT;
                        break;
                    } else if (key == 27) {
                        if (os_keyboard_esc_is_lone()) {
                            session.state = STATE_QUIT;
                            break;
                        }
                    }
                }
                usleep(16000);
            }
        } else if (session.state == STATE_MODE_SELECT) {
            draw_mode_select();
            while (!g_interrupted) {
                /* Resize ke baad selection menu ko refresh karo, game exit nahi hoga. */
                if (take_resize_event()) {
                    draw_mode_select();
                }

                if (os_key_pressed(&key)) {
                    if (key == '1') {
                        session.mode = MODE_PRACTICE;
                        session.state = STATE_DIFFICULTY_SELECT;
                        break;
                    } else if (key == '2') {
                        session.mode = MODE_TIMED_30;
                        session.time_limit = 30;
                        session.state = STATE_DIFFICULTY_SELECT;
                        break;
                    } else if (key == '3') {
                        session.mode = MODE_TIMED_60;
                        session.time_limit = 60;
                        session.state = STATE_DIFFICULTY_SELECT;
                        break;
                    } else if (key == '4') {
                        session.mode = MODE_SURVIVAL;
                        session.state = STATE_DIFFICULTY_SELECT;
                        break;
                    } else if (key == 27) {
                        if (os_keyboard_esc_is_lone()) {
                            session.state = STATE_MAIN_MENU;
                            break;
                        }
                    }
                }
                usleep(16000);
            }
        } else if (session.state == STATE_DIFFICULTY_SELECT) {
            draw_difficulty_select();
            while (!g_interrupted) {
                /* Difficulty screen resize par same state me rehkar redraw hoti hai. */
                if (take_resize_event()) {
                    draw_difficulty_select();
                }

                if (os_key_pressed(&key)) {
                    if (key == '1') {
                        session.difficulty = DIFF_EASY;
                        session.state = STATE_PLAYING;
                        break;
                    } else if (key == '2') {
                        session.difficulty = DIFF_MEDIUM;
                        session.state = STATE_PLAYING;
                        break;
                    } else if (key == '3') {
                        session.difficulty = DIFF_HARD;
                        session.state = STATE_PLAYING;
                        break;
                    } else if (key == 27) {
                        if (os_keyboard_esc_is_lone()) {
                            session.state = STATE_MODE_SELECT;
                            break;
                        }
                    }
                }
                usleep(16000);
            }
        } else if (session.state == STATE_PLAYING) {
            run_game(&session);
        } else if (session.state == STATE_ENTER_NAME) {
            char name_buf[32] = {0}; // Naam store karne ke liye buffer
            int name_len = 0;        // Naam ki length track karne ke liye
            
            while (!g_interrupted) {
                // Screen set karo aur prompt dikhao
                os_screen_begin_frame();
                os_screen_reset_color();
                os_screen_set_color("1;33");
                os_screen_draw_text(25, 5, "ENTER YOUR NAME FOR HIGH SCORE:");
                os_screen_reset_color();
                os_screen_draw_text(25, 7, ">> ");
                os_screen_draw_text(28, 7, name_buf); // Jo abhi tak likha hai wo screen par dikhao
                os_screen_draw_text(20, 15, "Press ENTER to confirm");
                os_screen_flush();
                
                if (os_key_pressed(&key)) {
                    // Agar Enter dabaya toh naam save kar lo
                    if (key == '\n' || key == '\r') {
                        // Agar koi naam nahi likha, toh default naam "Player" rakh do
                        if (name_len == 0) {
                            os_strcpy(session.player_name, "Player");
                        } else {
                            // Warna jo likha hai wo session mein daal do
                            os_strcpy(session.player_name, name_buf);
                        }
                        
                        add_highscore(&session);       // Highscore file mein save karo
                        session.state = STATE_RESULTS; // Results screen par bhejo
                        break;
                    } 
                    // Agar Backspace dabaya, toh aakhri akshar (character) mita do
                    else if ((key == 127 || key == 8) && name_len > 0) {
                        name_len--;
                        name_buf[name_len] = '\0';
                    } 
                    // Normal characters ko naam mein add karo (max 30 length tak)
                    else if (key >= 32 && key <= 126 && name_len < 30) {
                        name_buf[name_len] = key;
                        name_len++;
                        name_buf[name_len] = '\0'; // String ka end mark karo
                    }
                }
                usleep(16000); // CPU ko thoda rest dene ke liye ruk jao
            }
        } else if (session.state == STATE_RESULTS) {
            draw_results(&session);
            while (!g_interrupted) {
                /* Results screen bhi resize par refresh ho, key press ki zarurat nahi. */
                if (take_resize_event()) {
                    draw_results(&session);
                }

                if (os_key_pressed(&key)) {
                    if (key == 27 && !os_keyboard_esc_is_lone()) {
                        continue;
                    }
                    session.state = STATE_MAIN_MENU;
                    break;
                }
                usleep(16000);
            }
        } else if (session.state == STATE_HIGHSCORES) {
            draw_highscores();
            while (!g_interrupted) {
                /* Highscore list ko current terminal size ke hisaab se redraw karo. */
                if (take_resize_event()) {
                    draw_highscores();
                }

                if (os_key_pressed(&key)) {
                    if (key == 27 && !os_keyboard_esc_is_lone()) {
                        continue;
                    }
                    session.state = STATE_MAIN_MENU;
                    break;
                }
                usleep(16000);
            }
        }
    }

    os_screen_alt_screen_leave();
    os_screen_show_cursor();
    os_keyboard_shutdown();

    printf("Typing Rush closed cleanly.\n");
    return 0;
}
