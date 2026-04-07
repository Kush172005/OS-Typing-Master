#include "math.h"

/* Multiply using repeated addition. */
int os_mul(int a, int b) {
    int result = 0;
    int negative = 0;
    int count;

    if (a < 0) {
        a = -a;
        negative = 1 - negative;
    }
    if (b < 0) {
        b = -b;
        negative = 1 - negative;
    }

    count = 0;
    while (count < b) {
        result += a;
        count++;
    }

    if (negative) {
        return -result;
    }
    return result;
}

/* Divide using repeated subtraction (integer division). */
int os_div(int numerator, int denominator) {
    int quotient = 0;
    int negative = 0;

    if (denominator == 0) {
        return 0;
    }

    if (numerator < 0) {
        numerator = -numerator;
        negative = 1 - negative;
    }
    if (denominator < 0) {
        denominator = -denominator;
        negative = 1 - negative;
    }

    while (numerator >= denominator) {
        numerator -= denominator;
        quotient++;
    }

    if (negative) {
        return -quotient;
    }
    return quotient;
}

/* Modulo using repeated subtraction. */
int os_mod(int numerator, int denominator) {
    int negative = 0;

    if (denominator == 0) {
        return 0;
    }

    if (numerator < 0) {
        numerator = -numerator;
        negative = 1;
    }
    if (denominator < 0) {
        denominator = -denominator;
    }

    while (numerator >= denominator) {
        numerator -= denominator;
    }

    if (negative) {
        return -numerator;
    }
    return numerator;
}

int os_abs(int value) {
    if (value < 0) {
        return -value;
    }
    return value;
}

int os_clamp(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

int os_in_bounds(int value, int min_value, int max_value_exclusive) {
    if (value < min_value) {
        return 0;
    }
    if (value >= max_value_exclusive) {
        return 0;
    }
    return 1;
}
