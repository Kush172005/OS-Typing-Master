#ifndef OS_STRING_H
#define OS_STRING_H

/* String helpers built without using <string.h>. */
int os_strlen(const char *text);
void os_strcpy(char *dest, const char *src);
int os_strcmp(const char *a, const char *b);
int os_split_first_token(const char *input, char *token_out, int token_max);
void os_int_to_string(int value, char *buffer, int buffer_size);

#endif
