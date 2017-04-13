//
// Created by root on 5.4.2017.
//

#ifndef PACKETCONVERTER_ENCRYPTION_H
#define PACKETCONVERTER_ENCRYPTION_H


#include <cstdint>

class Encryption {
public:
    static void encrypt(uint8_t *indata,unsigned int n,uint8_t *key,uint8_t *outdata);
    static void decrypt(uint8_t *indata,unsigned int size,uint8_t *key,uint8_t *outdata);
};


#endif //PACKETCONVERTER_ENCRYPTION_H
