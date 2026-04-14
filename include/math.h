#ifndef OS_MATH_H
#define OS_MATH_H

int os_mul(int a, int b);
int os_div(int numerator, int denominator);
int os_mod(int numerator, int denominator);
int os_abs(int value);
int os_clamp(int value, int min_value, int max_value);
int os_in_bounds(int value, int min_value, int max_value_exclusive);

#endif
