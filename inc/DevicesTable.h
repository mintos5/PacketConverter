//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_DEVICESTABLE_H
#define PACKETCONVERTER_DEVICESTABLE_H
#define PRE_KEY_SIZE 128
#define SESSION_KEY_SIZE 128

#include <chrono>
#include <mutex>
#include <map>

extern "C" {
#include "loragw_hal.h"
}

struct EndDevice{
    uint8_t preSharedKey[PRE_KEY_SIZE];
    uint8_t preSharedGenerator;
    uint8_t sessionKey[SESSION_KEY_SIZE];
    std::chrono::milliseconds timer;
};

class DevicesTable {
public:
    lgw_pkt_tx_s setPacket(std::string deviceId);
    static lgw_pkt_tx_s setTestParams();

    std::map<std::string,EndDevice> map;
    static std::mutex mapMutex;
};


#endif //PACKETCONVERTER_DEVICESTABLE_H
