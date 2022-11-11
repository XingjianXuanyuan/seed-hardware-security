/*
 * https://www.cs.umd.edu/~jkatz/security/example.c
 * An example of using the AES block cipher,
 * with key (in hex) 01000000000000000000000000000000
 * and input (in hex) 01000000000000000000000000000000.
 * The result should be the output (in hex) FAEB01888D2E92AEE70ECC1C638BF6D6
 * (AES_encrypt corresponds to computing AES in the forward direction.)
 * We then compute the inverse, and check that we recovered the original input.
 * (AES_decrypt corresponds to computing the inverse of AES.)
 * (The terminology "AES_encrypt" and "AES_decrypt" is unfortunate since,
 * as discussed in class, AES is a block cipher, not an encryption scheme.
 */

/*
 * To compile (on unix):
 * gcc -c aes_core.c
 * gcc -c example.c
 * (make sure to #include "aes.h" in your program)
 * gcc -o example example.o aes_core.o
 */

#include "openssl/aes.h"
#include <stdio.h>

void main()
{
    AES_KEY AESkey;
    unsigned char MBlock[16];
    unsigned char MBlock2[16];
    unsigned char CBlock[16];
    unsigned char Key[16];
    int i;

    /*
     * Key contains the actual 128-bit AES key. AESkey is a data structure
     * holding a transformed version of the key, for efficiency.
     */
    Key[0] = 1;

    for (i = 1; i <= 15; i++)
    {
        Key[i] = 0;
    }

    AES_set_encrypt_key((const unsigned char *)Key, 128, &AESkey);

    MBlock[0] = 1;

    for (i = 1; i < 16; i++)
        MBlock[i] = 0;

    AES_encrypt((const unsigned char *)MBlock, CBlock, (const AES_KEY *)&AESkey);

    for (i = 0; i < 16; i++)
    {
        printf("%X", CBlock[i] / 16);
        printf("%X", CBlock[i] % 16);
    }
    printf("\n");

    /*
     * We need to set AESkey appropriately before inverting AES.
     * Note that the underlying key Key is the same; just the data structure
     * AESkey is changing (for reasons of efficiency).
     */
    AES_set_decrypt_key((const unsigned char *)Key, 128, &AESkey);

    AES_decrypt((const unsigned char *)CBlock, MBlock2, (const AES_KEY *)&AESkey);

    for (i = 0; i < 16; i++)
    {
        printf("%X", MBlock2[i] / 16), printf("%X", MBlock2[i] % 16);
    }
    printf("\n");
}