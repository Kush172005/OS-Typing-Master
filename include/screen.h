#ifndef OS_SCREEN_H
#define OS_SCREEN_H

/* Terminal drawing helpers. */
void os_screen_clear(void);
void os_screen_move_cursor(int x, int y);
void os_screen_draw_text(int x, int y, const char *text);
void os_screen_hide_cursor(void);
void os_screen_show_cursor(void);
void os_screen_flush(void);

#endif
