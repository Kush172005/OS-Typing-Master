#include "math.h"

int os_mul(int a, int b) {
    int negative = 0;
    long long aa = a;
    long long bb = b;
    long long res = 0;

    if (aa < 0) {
        aa = -aa;
        negative = !negative;
    }
    if (bb < 0) {
        bb = -bb;
        negative = !negative;
    }

    while (bb > 0) {
        if (bb & 1) {
            res += aa;
        }
        aa <<= 1;
        bb >>= 1;
    }

    if (negative) {
        res = -res;
    }
    return (int)res;
}


int os_div(int numerator, int denominator) {
    int negative = 0;
    int i;
    long long x;
    long long y;
    unsigned long long ux;
    unsigned long long uy;
    unsigned long long res = 0;

    if (denominator == 0) {
        return 0;
    }

    x = numerator;
    y = denominator;

    if (x < 0) {
        x = -x;
        negative = !negative;
    }
    if (y < 0) {
        y = -y;
        negative = !negative;
    }

    ux = (unsigned long long)x;
    uy = (unsigned long long)y;

    if (uy == 0) {
        return 0;
    }

    for (i = 63; i >= 0; i--) {
        if ((ux >> i) >= uy) {
            res += 1ULL << i;
            ux -= uy << i;
        }
    }

    if (negative) {
        return (int)(-(long long)res);
    }
    return (int)res;
}


int os_mod(int numerator, int denominator) {
    long long q;
    long long prod;

    if (denominator == 0) {
        return 0;
    }

    q = os_div(numerator, denominator);
    prod = q * (long long)denominator;
    return (int)((long long)numerator - prod);
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
