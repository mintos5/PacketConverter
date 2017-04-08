//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_DEVICESTABLE_H
#define PACKETCONVERTER_DEVICESTABLE_H
#define PRE_KEY_SIZE 128
#define SESSION_KEY_SIZE 128

#include <chrono>
#include <mutex>
#include "json.hpp"
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
    nlohmann::json normalConfig;
    nlohmann::json emerConfig;
    nlohmann::json regConfig;
    lgw_pkt_tx_s setPacket(std::string deviceId);

    std::map<std::string,EndDevice> map;
    static std::mutex mapMutex;
};


#endif //PACKETCONVERTER_DEVICESTABLE_H
