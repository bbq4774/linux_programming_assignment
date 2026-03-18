#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <time.h>

#define N 1000000

int main()
{
    struct timespec start, end;

    volatile long long a = 123456789;
    volatile long long b;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (long long i = 0; i < N; i++)
    {
        b = a * 1000000000LL + 123456789;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    long long time_mul_add =
        (end.tv_sec - start.tv_sec) * 1000000000LL +
        (end.tv_nsec - start.tv_nsec);

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (long long i = 0; i < N; i++)
    {
        b = a / 1000000000LL;
        b = a % 1000000000LL;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    long long time_div_mod =
        (end.tv_sec - start.tv_sec) * 1000000000LL +
        (end.tv_nsec - start.tv_nsec);

    printf("Mul + Add: %lld ns\n", time_mul_add);
    printf("Div + Mod: %lld ns\n", time_div_mod);
    printf("Ratio time_div_mod/time_mul_add: %.2f\n",
           (double)time_div_mod / time_mul_add);

    return 0;
}