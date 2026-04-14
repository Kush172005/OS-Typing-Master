#include "math.h"
#include "string.h"

int os_strlen(const char *text) {
    int length = 0;

    while (text[length] != '\0') {
        length++;
    }

    return length;
}

void os_strcpy(char *dest, const char *src) {
    int i = 0;

    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }

    dest[i] = '\0';
}

int os_strcmp(const char *a, const char *b) {
    int i = 0;

    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return (int)((unsigned char)a[i] - (unsigned char)b[i]);
        }
        i++;
    }

    return (int)((unsigned char)a[i] - (unsigned char)b[i]);
}

int os_split_first_token(const char *input, char *token_out, int token_max) {
    int i = 0;
    int token_len = 0;

    if (token_max <= 1) {
        return 0;
    }

    while (input[i] == ' ' || input[i] == '\t' || input[i] == '\n') {
        i++;
    }

    while (input[i] != '\0' && input[i] != ' ' && input[i] != '\t' && input[i] != '\n') {
        if (token_len < token_max - 1) {
            token_out[token_len] = input[i];
            token_len++;
        }
        i++;
    }

    token_out[token_len] = '\0';
    return token_len;
}

void os_int_to_string(int value, char *buffer, int buffer_size) {
    int temp[16];
    int digits = 0;
    int is_negative = 0;
    int i;
    int out = 0;

    if (buffer_size <= 1) {
        return;
    }

    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    while (value > 0 && digits < 16) {
        temp[digits] = os_mod(value, 10);
        value = os_div(value, 10);
        digits++;
    }

    if (is_negative && out < buffer_size - 1) {
        buffer[out] = '-';
        out++;
    }

    i = digits - 1;
    while (i >= 0 && out < buffer_size - 1) {
        buffer[out] = (char)('0' + temp[i]);
        out++;
        i--;
    }

    buffer[out] = '\0';
}
