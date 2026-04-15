#ifndef OS_KEYBOARD_H
#define OS_KEYBOARD_H

int os_keyboard_init(void);
void os_keyboard_shutdown(void);
int os_key_pressed(char *out_key);
/* After reading ESC: 1 = lone Escape (quit), 0 = mouse/other CSI (discard, stay). */
int os_keyboard_esc_is_lone(void);
int os_read_line(char *buffer, int max_len);

#endif
