//
// Created by root on 5.4.2017.
//

#ifndef PACKETCONVERTER_ENCRYPTION_H
#define PACKETCONVERTER_ENCRYPTION_H


#include <cstdint>

class Encryption {
public:
    static void encrypt(uint8_t *indata,unsigned int size,uint8_t *keyByte);
    static void decrypt(uint8_t *indata,unsigned int size,uint8_t *keyByte);
};


#endif //PACKETCONVERTER_ENCRYPTION_H
