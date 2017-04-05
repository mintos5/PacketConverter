//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_DEVICESTABLE_H
#define PACKETCONVERTER_DEVICESTABLE_H


#include "json.hpp"
extern "C" {
#include "loragw_hal.h"
}

class DevicesTable {
public:
    nlohmann::json normalConfig;
    nlohmann::json emerConfig;
    nlohmann::json regConfig;
    lgw_pkt_tx_s setPacket(std::string deviceId);

};


#endif //PACKETCONVERTER_DEVICESTABLE_H
