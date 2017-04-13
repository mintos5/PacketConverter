//
// Created by root on 11.3.2017.
//

#include <cstring>
#include "DevicesTable.h"



lgw_pkt_tx_s DevicesTable::setTestParams() {
    struct lgw_pkt_tx_s txpkt;
    memset(&txpkt, 0, sizeof(txpkt));
    txpkt.freq_hz = 866300000;
    txpkt.tx_mode = IMMEDIATE;
    txpkt.rf_chain = 0;
    txpkt.rf_power = 14;
    txpkt.modulation = MOD_LORA;
    txpkt.bandwidth = BW_125KHZ;
    txpkt.datarate = DR_LORA_SF11;
    txpkt.coderate = CR_LORA_4_5;
    txpkt.invert_pol = true;
    txpkt.preamble = 8;
    return txpkt;
}

bool DevicesTable::isInMap(std::string deviceId) {
    //need to lock mutex before
    if (map.find(deviceId) != map.end()){
        return true;
    }
    return false;
}

bool DevicesTable::isInMap(std::string deviceId, std::map<std::string, EndDevice>::iterator &iterator) {
    //need to lock with mutex
    iterator = map.find(deviceId);
    if (iterator != map.end()){
        return true;
    }
    return false;
}

bool DevicesTable::setPacket(std::string deviceId, LoraPacket &packet, uint16_t &seq) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    if (isInMap(deviceId)){
        packet.frequency = map[deviceId].frequency;
        packet.coderate = map[deviceId].coderate;
        packet.bandwidth = map[deviceId].bandwidth;
        packet.datarate = map[deviceId].datarate;
        seq = map[deviceId].seq;
        seq++;
        return true;
    }
    return false;
}

bool DevicesTable::removeFromMap(std::string deviceId) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        map.erase(iterator);
        return true;
    }
    return false;
}

bool DevicesTable::updateMap(std::string deviceId,LoraPacket packet,uint16_t seq) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::chrono::minutes currentTime = std::chrono::duration_cast< std::chrono::minutes >
            (std::chrono::system_clock::now().time_since_epoch());
    if (isInMap(deviceId)){
        map[deviceId].frequency = packet.frequency;
        map[deviceId].datarate = packet.datarate;
        map[deviceId].bandwidth = packet.bandwidth;
        map[deviceId].coderate = packet.coderate;
        map[deviceId].seq = seq;
        map[deviceId].timer = currentTime;
    }
    else {
        EndDevice endDevice;
        endDevice.sessionKeyExists = false;
        endDevice.bandwidth = packet.bandwidth;
        endDevice.coderate = packet.coderate;
        endDevice.datarate = packet.datarate;
        endDevice.frequency = packet.frequency;
        endDevice.seq = seq;
        endDevice.timer = currentTime;
        std::copy(packet.dh,packet.dh+SESSION_KEY_SIZE,endDevice.dha);
        map[deviceId] = endDevice;
    }
}

bool DevicesTable::hasSessionKey(std::string deviceId) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        if (iterator->second.sessionKeyExists){
            return true;
        }
    }
    return false;
}

bool DevicesTable::updateSessionkey(std::string deviceId, uint8_t *sessionKey) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        std::copy(sessionKey,sessionKey+SESSION_KEY_SIZE,iterator->second.sessionKey);
        return true;
    }
    return false;
}

std::mutex DevicesTable::mapMutex;