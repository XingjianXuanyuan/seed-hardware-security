/* $ gcc -O3 -o server server.c -lcrypto */
/* $ printenv | wc -c */
/* $ ./server 192.168.123.141 < /dev/zero */
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* Apple has deprecated the use of OpenSSL
 * in favor of its own TLS and crypto libraries:
 * #include <openssl/aes.h>
 */
#include "openssl/aes.h"

unsigned int timestamp(void)
{
    unsigned int top, bottom;
    asm volatile(
        ".byte 15; .byte 49" : "=a"(bottom), "=d"(top)
    );
    return bottom;
}

AES_KEY expanded;
unsigned char key[16];
unsigned char zero[16];
unsigned char scrambled_zero[16];

void handle(char out[40], const unsigned char in[], int len)
{
    unsigned char workarea[len * 3];
    int i;

    for (i = 0; i < 40; ++i) {
        out[i] = 0;
    }
    *(unsigned int *)(out + 32) = timestamp();

    if (len < 16) {
        return;
    }
    for (i = 0; i < 16; ++i) {
        out[i] = in[i];
    }

    for (i = 16; i < len; ++i) {
        workarea[i] = in[i];
    }
    AES_encrypt(in, workarea, &expanded);
    /* A real server would now check AES-based authenticator,
     * process legitimate packets, and generate useful output. */

    for (i = 0; i < 16; ++i) {
        out[16 + i] = scrambled_zero[i];
    }
    *(unsigned int *)(out + 36) = timestamp();
}

struct sockaddr_in server;
struct sockaddr_in client;
socklen_t clientlen;
int s;
char in[1537];
int r;
char out[40];

int main(int argc, char **argv)
{
    if (read(0, key, sizeof key) < sizeof key) {
        return 111;
    }
    AES_set_encrypt_key(key, 128, &expanded);
    AES_encrypt(zero, scrambled_zero, &expanded);

    if (!argv[1]) {
        return 100;
    }
    if (!inet_aton(argv[1], &server.sin_addr)) {
        return 100;
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(10000);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1) {
        return 111;
    }
    if (bind(s, (struct sockaddr *)&server, sizeof server) == -1) {
        return 111;
    }

    for (;;) {
        clientlen = sizeof client;
        r = recvfrom(s, in, sizeof in, 0, (struct sockaddr *)&client, &clientlen);
        if (r < 16) {
            continue;
        }
        if (r >= sizeof in) {
            continue;
        }
        handle(out, in, r);
        sendto(s, out, 40, 0, (struct sockaddr *)&client, clientlen);
    }

    return 0;
}