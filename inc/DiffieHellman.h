//
// Created by root on 9.4.2017.
//

#ifndef PACKETCONVERTER_DIFFIEHELLMAN_H
#define PACKETCONVERTER_DIFFIEHELLMAN_H


#include <cstdint>

class DiffieHellman {
public:
    //todo add funckie asymetrickeho posielania klucov
    static uint64_t getDHB(uint64_t prime,uint64_t base,uint64_t randomNum);
    static uint64_t getSessionKey(uint64_t dha, uint64_t dhb,uint64_t prime,uint64_t base,uint64_t randomNum);
};


#endif //PACKETCONVERTER_DIFFIEHELLMAN_H
