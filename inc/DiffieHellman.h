//
// Created by root on 9.4.2017.
//

#ifndef PACKETCONVERTER_DIFFIEHELLMAN_H
#define PACKETCONVERTER_DIFFIEHELLMAN_H


#include <cstdint>

class DiffieHellman {
public:
    static void getDHB(uint8_t *preSharedKey,uint8_t *publicKey,uint8_t *privateKey);
    static void getSessionKey(uint8_t *preSharedKey,uint8_t *dha, uint8_t *privateKey,uint8_t *sessionKey);
    static uint64_t powerMod(uint64_t a,uint64_t b,uint64_t prime);
    static uint64_t multiplyMod(uint64_t a, uint64_t b, uint64_t prime);
};


#endif //PACKETCONVERTER_DIFFIEHELLMAN_H
