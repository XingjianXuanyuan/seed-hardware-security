#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h> /* for memset */
#include <sched.h>  /* for CPU_SET, CPU_ZERO, sched_setaffinity */
#include <unistd.h> /* for sysconf*/
#include <sys/time.h>

#define CACHE_LINE_SIZE 64
#define DEFAULT_ALLOC_SIZE_KB 4096
#define DEFAULT_ITER 100

enum access_type { READ, WRITE };

int g_mem_size = DEFAULT_ALLOC_SIZE_KB * 1024;
int *g_mem_ptr = 0;
volatile uint64_t g_nread = 0;
volatile unsigned int g_start;

/* user-defined parameters */
int acc_type = READ;
int cpuid = 0;

void set_cpu_affinity(int cpuid)
{
    cpu_set_t cmask;
    int nprocessors = sysconf(_SC_NPROCESSORS_CONF);
    CPU_ZERO(&cmask);
    CPU_SET(cpuid % nprocessors, &cmask);
    if (sched_setaffinity(0, nprocessors, &cmask) < 0) {
        perror("sched_setaffinity() error");
    } else {
        fprintf(stderr, "Assigned to CPU%d\n", cpuid);
    }
}

unsigned int get_usecs()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return (time.tv_sec * 1000000 + time.tv_usec);
}

void exit_fn(int param)
{
    float duration_in_sec;
    float bandwidth;
    float duration = get_usecs() - g_start;
    duration_in_sec = (float)duration / 1000000;
    printf("g_nread(bytes read) = %lld\n", (long long)g_nread);
    printf("Elapsed = %.2f sec (%.0f usec)\n", duration_in_sec, duration);
    bandwidth = (float)g_nread / duration_in_sec / 1024 / 1024;
    printf("CPU%d: Bandwidth = %.3f MB/s | ", cpuid, bandwidth);
    printf("CPU%d: Average = %.2f ns\n", cpuid, (duration * 1000) / (g_nread / CACHE_LINE_SIZE));
    exit(0);
}

int64_t bench_read()
{
    int i;
    int64_t sum = 0;
    for (i = 0; i < g_mem_size / 4; i += (CACHE_LINE_SIZE / 4)) {
        sum += g_mem_ptr[i];
    }
    g_nread += g_mem_size;
    return sum;
}

int bench_write()
{
    register int i;
    for (i = 0; i < g_mem_size / 4; i += (CACHE_LINE_SIZE / 4)) {
        g_mem_ptr[i] = i;
    }
    g_nread += g_mem_size;
    return 1;
}

int main(int argc, char **argv)
{
    int64_t sum = 0;
    int i;

    if (argc > 0) {
        cpuid = argv[1];
    }
    set_cpu_affinity(cpuid);

    g_mem_ptr = (int *)malloc(g_mem_size);

    memset((char *)g_mem_ptr, 1, g_mem_size);

    for (i = 0; i < g_mem_size / sizeof(int); i++) {
        g_mem_ptr[i] = i;
    }

    /* actual memory access */
    g_start = get_usecs();
    for (i = 0; ; i++) {
        switch(acc_type) {
        case READ:
            sum += bench_read();
            break;
        case WRITE:
            sum += bench_write();
            break;
        }

        if (i >= DEFAULT_ITER) {
            break;
        }
    }
    printf("Total sum = %ld\n", (long)sum);
    exit_fn(0);

    return 0;
}