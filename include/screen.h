#ifndef OS_SCREEN_H
#define OS_SCREEN_H

void os_screen_clear(void);
void os_screen_begin_frame(void);
void os_screen_alt_screen_enter(void);
void os_screen_alt_screen_leave(void);
int os_screen_term_cols(void);
/* Terminal ki current height nikalne ke liye, resize ke baad layout safe rahe. */
int os_screen_term_rows(void);
void os_screen_move_cursor(int x, int y);
void os_screen_draw_text(int x, int y, const char *text);
void os_screen_draw_typing_overlay(int x, int y, const char *target,
                                   const char *typed, int typed_len);
void os_screen_set_color(const char *color_code);
void os_screen_reset_color(void);
void os_screen_hide_cursor(void);
void os_screen_show_cursor(void);
void os_screen_flush(void);

#endif
