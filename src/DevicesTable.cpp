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

bool DevicesTable::setPacket(std::string deviceId, struct LoraPacket &packet) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    if (isInMap(deviceId)){
        packet.frequency = map[deviceId].frequency;
        packet.coderate = map[deviceId].coderate;
        packet.bandwidth = map[deviceId].bandwidth;
        packet.datarate = map[deviceId].datarate;
        packet.rfChain = map[deviceId].rfChain;
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

bool DevicesTable::updateMap(std::string deviceId,struct LoraPacket packet,uint16_t seq) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::chrono::minutes currentTime = std::chrono::duration_cast< std::chrono::minutes >
            (std::chrono::system_clock::now().time_since_epoch());
    if (isInMap(deviceId)){
        map[deviceId].frequency = packet.frequency;
        map[deviceId].datarate = packet.datarate;
        map[deviceId].bandwidth = packet.bandwidth;
        map[deviceId].coderate = packet.coderate;
        map[deviceId].rfChain = packet.rfChain;
        map[deviceId].seq = seq;
        map[deviceId].timer = currentTime;
    }
    else {
        EndDevice endDevice;
        endDevice.sessionKeyExists = false;
        endDevice.sessionKeyCheck = false;
        endDevice.myDevice = true;
        endDevice.bandwidth = packet.bandwidth;
        endDevice.coderate = packet.coderate;
        endDevice.datarate = packet.datarate;
        endDevice.frequency = packet.frequency;
        endDevice.rfChain = packet.rfChain;
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

bool DevicesTable::updateSessionkey(std::string deviceId, uint8_t *sessionKey,uint16_t seq) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        std::copy(sessionKey,sessionKey+SESSION_KEY_SIZE,iterator->second.sessionKey);
        iterator->second.sessionKeyExists = true;
        iterator->second.sessionKeyCheck = false;
        iterator->second.seq = seq;
        return true;
    }
    return false;
}

bool DevicesTable::isMine(std::string deviceId) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        if (iterator->second.myDevice){
            return true;
        }
    }
    return false;
}

void DevicesTable::updateByTimer(std::chrono::seconds currentTime) {
    if (currentTime.count()-this->time.count()>TIME_INTERVAL){
        this->resetDutyCycle();
        std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
        std::map <std::string,EndDevice>::iterator it = map.begin();
        while (it != map.end()) {
            if (currentTime.count()-it->second.timer.count()>FLUSH_DEVICE){
                it = map.erase(it);
                continue;
            }
            ++it;
        }
    }
}

void DevicesTable::resetDutyCycle() {

}

uint8_t *DevicesTable::getSessionKey(std::string deviceId) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        if (iterator->second.sessionKeyExists){
            return iterator->second.sessionKey;
        }
    }
    return nullptr;
}

bool DevicesTable::setSessionKeyCheck(std::string deviceId, bool set) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        if (iterator->second.sessionKeyExists){
            iterator->second.sessionKeyCheck = set;
            return true;
        }
    }
    return false;
}

bool DevicesTable::hasSessionKeyCheck(std::string deviceId) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        if (iterator->second.sessionKeyCheck){
            return true;
        }
    }
    return false;
}

uint16_t DevicesTable::getSeq(std::string deviceId) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        return iterator->second.seq;
    }
    return 0;
}

uint8_t DevicesTable::remainingDutyCycle(std::string deviceId) {
    return 20;
}

bool DevicesTable::reduceDutyCycle(std::string deviceId, uint8_t messageSize) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        return true;
    }
    return false;
}

bool DevicesTable::setSeq(std::string deviceId, uint16_t seq) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        iterator->second.seq = seq;
        return true;
    }
    return false;
}

std::mutex DevicesTable::mapMutex;