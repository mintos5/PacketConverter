//
// Created by root on 11.3.2017.
//

#include <cstring>
#include <algorithm>
#include <MessageConverter.h>
#include "DevicesTable.h"


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

bool DevicesTable::setSeq(std::string deviceId, uint16_t seq) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        iterator->second.seq = seq;
        return true;
    }
    return false;
}

bool DevicesTable::setSessionkey(std::string deviceId, uint8_t *sessionKey, uint16_t seq) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        std::copy(sessionKey,sessionKey+DH_SESSION_KEY_SIZE,iterator->second.sessionKey);
        iterator->second.sessionKeyExists = true;
        iterator->second.sessionKeyCheck = false;
        iterator->second.seq = seq;
        return true;
    }
    return false;
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

uint16_t DevicesTable::getSeq(std::string deviceId) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        return iterator->second.seq;
    }
    return 0;
}

uint8_t *DevicesTable::getDh(std::string deviceId) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        return iterator->second.dha;
    }
    return nullptr;
}

bool DevicesTable::addDevice(std::string deviceId, struct LoraPacket packet, uint16_t seq) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::chrono::minutes currentTime = std::chrono::duration_cast< std::chrono::minutes >
            (std::chrono::system_clock::now().time_since_epoch());
    if (isInMap(deviceId)){
        map.erase(deviceId);
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
        std::copy(packet.payload,packet.payload+DH_SESSION_KEY_SIZE,endDevice.dha);
        map[deviceId] = endDevice;
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
        std::copy(packet.payload,packet.payload+DH_SESSION_KEY_SIZE,endDevice.dha);
        map[deviceId] = endDevice;
    }
}

bool DevicesTable::updateDevice(std::string deviceId, struct LoraPacket packet, uint16_t seq) {
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
    //60*60*1000/1000 for 0.1%
    this->onAir0 = 60*60;
    this->onAir1 = 60*60*10;
    this->onAir10 = 60*60*100;
}

long DevicesTable::remainingDutyCycle(std::string deviceId) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        long currentOnAirCounter;
        if (iterator->second.frequency >= 863000000 && iterator->second.frequency < 868000000){
            currentOnAirCounter = this->onAir0;
        }
        else if (iterator->second.frequency >= 868000000 && iterator->second.frequency <= 868600000){
            currentOnAirCounter = this->onAir1;
        }
        else if (iterator->second.frequency >= 868700000 && iterator->second.frequency <= 869200000){
            currentOnAirCounter = this->onAir0;
        }
        else if (iterator->second.frequency >= 869400000 && iterator->second.frequency <= 869650000){
            currentOnAirCounter = this->onAir10;
        }
        else {
            currentOnAirCounter = this->onAir0;
        }
        return currentOnAirCounter;
    }
    return 0;
}

long DevicesTable::calculateDutyCycle(std::map<std::string, EndDevice>::iterator &iterator, uint8_t payloadSize) {
    double tSym = pow(2,iterator->second.datarate);
    tSym = (tSym/iterator->second.bandwidth) * 1000;
    double tPreamble = (8+4.25)*tSym;
    int coderate = 5;
    if (iterator->second.coderate=="4/5"){
        coderate = 5;
    }
    else if (iterator->second.coderate=="4/6"){
        coderate = 6;
    }
    else if (iterator->second.coderate=="4/7"){
        coderate = 7;
    }
    else if (iterator->second.coderate=="4/8"){
        coderate = 8;
    }
    else {
        coderate = 5;
    }
    int dataRateOptimization = 0;
    if (iterator->second.datarate == 11 || iterator->second.datarate == 12){
        dataRateOptimization = 1;
    }
    double ceil1 = 8*payloadSize - 4*iterator->second.datarate + 28 +16 - 20;
    double ceil2 = 4*(iterator->second.datarate-2*dataRateOptimization);
    ceil1 = ceil(ceil1/ceil2);
    double zero = 0;
    double numSymbols = 8 + std::max(ceil1*coderate,zero);
    double tPayload = numSymbols*tSym;
    return tPreamble+tPayload;
}

bool DevicesTable::reduceDutyCycle(std::string deviceId, uint8_t messageSize) {
    std::lock_guard<std::mutex> guard(DevicesTable::mapMutex);
    std::map<std::string, EndDevice>::iterator iterator;
    if (isInMap(deviceId,iterator)){
        long *currentOnAirCounter;
        if (iterator->second.frequency >= 863000000 && iterator->second.frequency < 868000000){
            currentOnAirCounter = &onAir0;
        }
        else if (iterator->second.frequency >= 868000000 && iterator->second.frequency <= 868600000){
            currentOnAirCounter = &onAir1;
        }
        else if (iterator->second.frequency >= 868700000 && iterator->second.frequency <= 869200000){
            currentOnAirCounter = &onAir0;
        }
        else if (iterator->second.frequency >= 869400000 && iterator->second.frequency <= 869650000){
            currentOnAirCounter = &onAir10;
        }
        else {
            currentOnAirCounter = &onAir0;
        }
        //calculate time on air
        long messageTime = calculateDutyCycle(iterator,messageSize);
        //if bigger return false
        if (messageTime > *currentOnAirCounter){
            return false;
        }
        //reduce on all counters
        //*currentOnAirCounter = *currentOnAirCounter - messageTime;
        onAir0 -= messageTime;
        onAir1 -= messageTime;
        onAir10 -= messageTime;
        if (APP_DEBUG){
            std::cout << "debug out:" << std::endl;
            std::cout << "reduced time:" << messageTime << std::endl;
            std::cout << "remaining time:" << *currentOnAirCounter << std::endl;
        }
        return true;
    }
    return false;
}

std::mutex DevicesTable::mapMutex;