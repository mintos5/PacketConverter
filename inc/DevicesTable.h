//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_DEVICESTABLE_H
#define PACKETCONVERTER_DEVICESTABLE_H
#define PRE_KEY_SIZE 192
#define SESSION_KEY_SIZE 128

#include <chrono>
#include <mutex>
#include <map>
#include "Message.h"

extern "C" {
#include "loragw_hal.h"
}

struct EndDevice{
    uint8_t preSharedKey[PRE_KEY_SIZE];
    uint64_t preSharedGenerator;
    bool sessionKeyExists;
    uint8_t sessionKey[SESSION_KEY_SIZE];
    std::chrono::milliseconds timer;
    uint8_t dhb[SESSION_KEY_SIZE];
    uint32_t frequency;
    uint8_t	rfChain;
    uint16_t bandwidth;
    uint32_t datarate;
    std::string	coderate;
};

class DevicesTable {

    std::map<std::string,EndDevice> map;
    static std::mutex mapMutex;

public:
    //todo add funckie tabulky
    void setPacket(std::string deviceId,LoraPacket &packet);
    bool isInMap(std::string deviceId);
    void addToMap(std::string deviceId);
    bool removeFromMap(std::string deviceId);   //will return if finded...
    void updateMap(std::string deviceId);
    bool hasSessionKey(std::string deviceId);
    bool updateSessionkey(std::string deviceId,uint8_t sessionKey[SESSION_KEY_SIZE]);   //will return bool if finded
    static lgw_pkt_tx_s setTestParams();
};


#endif //PACKETCONVERTER_DEVICESTABLE_H
