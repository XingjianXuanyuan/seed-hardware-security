#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sched.h>  /* for CPU_SET, CPU_ZERO, sched_setaffinity */
#include <unistd.h> /* for sysconf*/
#include <time.h>
#include "list.h"

#define CACHE_LINE_SIZE 64
#define DEFAULT_ALLOC_SIZE_KB 4096
#define DEFAULT_ITER 100

struct item {
    int data;
    int in_use;
    struct list_head list;
} __attribute__((aligned(CACHE_LINE_SIZE)));;

int g_mem_size = DEFAULT_ALLOC_SIZE_KB * 1024;

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

uint64_t get_elapsed(struct timespec * start, struct timespec *end)
{
    uint64_t result;
    result = ((uint64_t)end->tv_sec * 1000000000L + end->tv_nsec) - ((uint64_t)start->tv_sec * 1000000000L + start->tv_nsec);
    return result;
}

int main(int argc, char **argv)
{
    struct item *list;
    struct item *itmp;
    struct list_head head;
    struct list_head *pos;
    struct timespec start, end;
    int workingset_size = 1024;
    int i, j;
    int serial = 0;
    int tmp, next;
    int *perm;
    uint64_t readsum = 0;
    uint64_t nsdiff;
    double avglat;

    if (argc > 0) {
        cpuid = argv[1];
    }
    set_cpu_affinity(cpuid);

    workingset_size = g_mem_size / CACHE_LINE_SIZE;
    srand(0);
    INIT_LIST_HEAD(&head);

    /* allocate */
    list = (struct item *)malloc(sizeof(struct item) * workingset_size + CACHE_LINE_SIZE);
    for (i = 0; i < workingset_size; i++) {
        list[i].data = i;
        list[i].in_use = 0;
        INIT_LIST_HEAD(&list[i].list);
    }

    /* initialize */
    perm = (int *)malloc(workingset_size * sizeof(int));
    for (i = 0; i < workingset_size; i++) {
        perm[i] = i;
    }
    if (!serial) {
        for (i = 0; i < workingset_size; i++) {
            tmp = perm[i];
            next = rand() % workingset_size;
            perm[i] = perm[next];
            perm[next] = tmp;
        }
    }
    for (i = 0; i < workingset_size; i++) {
        list_add(&list[perm[i]].list, &head);
    }

    /* actual access */
    clock_gettime(CLOCK_REALTIME, &start);
    for (j = 0; j < DEFAULT_ITER; j++) {
        pos = (&head)->next;
        for (i = 0; i < workingset_size; i++) {
            itmp = list_entry(pos, struct item, list);
            readsum += itmp->data; /* read attack*/
            pos = pos->next;
        }
    }
    clock_gettime(CLOCK_REALTIME, &end);

    nsdiff = get_elapsed(&start, &end);
    avglat = (double)nsdiff / workingset_size/ DEFAULT_ITER;
    printf("Duration %.0f us\nAverage %.2f ns | ", (double)nsdiff / 1000, avglat);
    printf("Bandwidth %.2f MB (%.2f MiB)/s\n", (double)64 * 1000 / avglat, (double)64 * 1000000000 / avglat / 1024 / 1024);
    printf("readsum %lld\n", (unsigned long long)readsum);
    return 0;
}