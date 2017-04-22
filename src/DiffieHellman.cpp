//
// Created by root on 9.4.2017.
//

#include <random>
#include <iostream>
#include "DiffieHellman.h"

void DiffieHellman::getDHB(uint8_t *preSharedKey, uint8_t *publicKey, uint8_t *privateKey) {
    //random number generator
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<uint32_t > distr;

    uint8_t *generatorByte = preSharedKey + 4 * sizeof(uint32_t);
    uint32_t *generator = (uint32_t *) generatorByte;

    for (int i=0;i<4;i++){
        //get prime number for 64bit calculation
        uint8_t *primeByte = preSharedKey + i * sizeof(uint32_t);
        uint32_t *prime = (uint32_t *) primeByte;

        //get pointer to write private key
        uint8_t *keyByte = privateKey + i * sizeof(uint32_t);
        uint32_t *keyOut = (uint32_t *) keyByte;
        *keyOut = distr(gen);

        //get pointer for dataOut
        uint8_t *dhbByte = publicKey + i * sizeof(uint32_t);
        uint32_t *dhbOut = (uint32_t *) dhbByte;

        *dhbOut = DiffieHellman::powerMod(*generator,*keyOut,*prime);
    }
}

void DiffieHellman::getSessionKey(uint8_t *preSharedKey,uint8_t *dha, uint8_t *privateKey,uint8_t *sessionKey) {
    for (int i=0;i<4;i++){
        //get prime number for 64bit calculation
        uint8_t *primeByte = preSharedKey + i * sizeof(uint32_t);
        uint32_t *prime = (uint32_t *) primeByte;

        //get pointer for received DHA
        uint8_t *dhaByte = dha + i * sizeof(uint32_t);
        uint32_t *dhaIn = (uint32_t *) dhaByte;

        //get pointer to write private key
        uint8_t *keyByte = privateKey + i * sizeof(uint32_t);
        uint32_t *key = (uint32_t *) keyByte;

        //get pointer for session key out
        uint8_t *sessionByte = sessionKey + i * sizeof(uint32_t);
        uint32_t *session = (uint32_t *) sessionByte;

        *session = DiffieHellman::powerMod(*dhaIn,*key,*prime);
    }
}

uint32_t DiffieHellman::powerMod(uint32_t a, uint32_t b, uint32_t prime) {
    if (a > prime){
        a = a%prime;
    }
    if (b==1){
        return a;
    }
    uint32_t t = powerMod(a,b/2,prime);
    t = multiplyMod(t,t,prime);
    if (b%2){
        t = multiplyMod(t,a,prime);
    }
    return t;
}

uint32_t DiffieHellman::multiplyMod(uint32_t a, uint32_t b, uint32_t prime) {
    uint32_t m = 0;
    while(b) {
        if(b&1) {
            uint32_t t = prime-a;
            if ( m >= t) {
                m -= t;
            } else {
                m += a;
            }
        }
        if (a >= prime - a) {
            a = a * 2 - prime;
        } else {
            a = a * 2;
        }
        b>>=1;
    }
    return m;
}
