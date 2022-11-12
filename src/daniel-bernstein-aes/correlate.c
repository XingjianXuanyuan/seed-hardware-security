/* $ gcc -O3 -o correlate correlate.c -lm */
/* $ ./study 192.168.123.141 800 > study.800 */
/* $ ./study 192.168.123.58 800 > attack.800 */
/* $ (tail -4096 study.800; tail -4096 attack.800) | ./correlate >> attack */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

double t[16][256];
double tdev[16][256];
double u[16][256];
double udev[16][256];

void read_data(void)
{
    int lines;
    int b;
    int size;
    int j;
    long long packets;
    double cycles;
    double deviation;
    double above_average;
    double avdev;

    for (lines = 0; lines < 8192; ++lines) {
        if (scanf("%d%d%d%lld%lf%lf%lf%lf", &b, &size, &j, &packets, &cycles, %deviation, &above_average, &avdev) != 8)
            exit(100);
        b &= 15;
        j &= 255;
        if (lines < 4096) {
            t[b][j] = above_average;
            tdev[b][j] = avdev;
        } else {
            u[b][j] = above_average;
            udev[b][j] = avdev;
        }
    }
}

double c[256];
double v[256];
int cpos[256];
int cpos_cmp(const void *v1, const void *v2)
{
    int *i1 = (int *)v1;
    int *i2 = (int *)v2;
    if (c[255 & *i1] < c[255 & *i2])
        return 1;
    if (c[255 & *i1] > c[255 & *i2])
        return -1;
    return 0;
}
void process_data(void)
{
    int b;
    int i;
    int j;
    int numok;
    double z;
    for (b = 0; b < 16; ++b) {
        for (i = 0; i < 256; ++i) {
            c[i] = v[i] = 0;
            cpos[i] = i;
            for (j = 0; j < 256; ++j) {
                c[i] += t[b][j] * u[b][i ^ j];
                z = tdev[b][j] * u[b][i ^ j];
                v[i] += z * z;
                z = t[b][j] * udev[b][i ^ j];
                v[i] += z * z;
            }
        }
        qsort(cpos, 256, sizeof(int), cpos_cmp);
        numok = 0;
        for (i = 0; i < 256; ++i)
            if (c[cpos[0]] - c[cpos[i]] < 10 * sqrt(v[cpos[i]]))
                printf(" %02x", cpos[i]);
        printf("\n");
    }
}

int main()
{
    read_data();
    process_data();
    return 0;
}