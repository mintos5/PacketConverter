//
// Created by root on 5.4.2017.
//
#include "Encryption.h"
#define DELTA 0x9e3779b9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)))

void Encryption::encrypt(uint8_t *indata, unsigned int size, uint8_t *keyByte) {
    uint32_t *v = (uint32_t *) indata;
    int n = size/ sizeof(uint32_t);
    uint32_t *key = (uint32_t *) keyByte;

    uint32_t y, z, sum;
    unsigned p, rounds, e;
        rounds = 6 + 52/n;
        sum = 0;
        z = v[n-1];
        do {
            sum += DELTA;
            e = (sum >> 2) & 3;
            for (p=0; p<n-1; p++) {
                y = v[p+1];
                z = v[p] += MX;
            }
            y = v[0];
            z = v[n-1] += MX;
        } while (--rounds);
}

void Encryption::decrypt(uint8_t *indata,unsigned int size, uint8_t *keyByte) {
    uint32_t *v = (uint32_t *) indata;
    int n = size/ sizeof(uint32_t);
    uint32_t *key = (uint32_t *) keyByte;

    uint32_t y, z, sum;
    unsigned p, rounds, e;
    rounds = 6 + 52/n;
    sum = rounds*DELTA;
    y = v[0];
    do {
        e = (sum >> 2) & 3;
        for (p=n-1; p>0; p--) {
            z = v[p-1];
            y = v[p] -= MX;
        }
        z = v[n-1];
        y = v[0] -= MX;
        sum -= DELTA;
    } while (--rounds);
}
