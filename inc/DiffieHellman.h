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
    static uint32_t powerMod(uint32_t a,uint32_t b,uint32_t prime);
    static uint32_t multiplyMod(uint32_t a, uint32_t b, uint32_t prime);
};


#endif //PACKETCONVERTER_DIFFIEHELLMAN_H
