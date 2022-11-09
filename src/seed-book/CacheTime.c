/* CacheTime.c */
/* Compile with gcc -march=native option */
#include <x86intrin.h>
#include <stdio.h>

#define N 10

uint8_t array[N * 4096];

int main(int argc, const char **argv)
{
    unsigned int junk = 0;
    register uint64_t time1, time2;
    volatile uint8_t *addr;
    int i;

    for (i = 0; i < N; i++) {
        array[i * 4096] = 1;
    }

    for (i = 0; i < N; i++) {
        _mm_clflush(&array[i * 4096]);
    }

    array[3 * 4096] = 100;
    array[7 * 4096] = 200;

    for (i = 0; i < N; i++) {
        addr = &array[i * 4096];
        time1 = __rdtscp(&junk);
        junk = *addr;
        time2 = __rdtscp(&junk) - time1;
        printf("Access time for array[%d * 4096]: %5u CPU cycles.\n", i, (unsigned int)time2);
    }

    return 0;
}
