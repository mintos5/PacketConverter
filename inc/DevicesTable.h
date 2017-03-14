//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_DEVICESTABLE_H
#define PACKETCONVERTER_DEVICESTABLE_H


#include "json.hpp"

class DevicesTable {
public:
    nlohmann::json normalConfig;
    nlohmann::json emerConfig;
    nlohmann::json regConfig;

};


#endif //PACKETCONVERTER_DEVICESTABLE_H
