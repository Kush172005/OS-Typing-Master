#ifndef OS_KEYBOARD_H
#define OS_KEYBOARD_H

/* Keyboard helpers for real-time and line input. */
int os_keyboard_init(void);
void os_keyboard_shutdown(void);
int os_key_pressed(char *out_key);
int os_read_line(char *buffer, int max_len);

#endif
