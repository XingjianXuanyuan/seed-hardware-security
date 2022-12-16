#include <x86intrin.h>
#include <stdio.h>

uint8_t array[256 * 4096];
char secret = 94;                   /* Secret value */
int temp;                           /* Stores the value of the array element at the secret position */

#define CACHE_HIT_THRESHOLD (80)    /* May vary based on CPU and memory speed */
#define DELTA 1024

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

void getSecret()
{
    temp = array[secret * 4096 + DELTA];
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

int main(int argc, const char **argv)
{
    flushSideChannel();
    getSecret();
    reloadSideChannel();
    return 0;
}
