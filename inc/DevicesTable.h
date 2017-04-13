//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_DEVICESTABLE_H
#define PACKETCONVERTER_DEVICESTABLE_H
#define SESSION_KEY_SIZE 128
#define TIME_INTERVAL 60
#define FLUSH_DEVICE 86400

#include <chrono>
#include <mutex>
#include <map>
#include "Message.h"

extern "C" {
#include "loragw_hal.h"
}

struct EndDevice{
    bool myDevice;
    bool sessionKeyExists;
    bool sessionKeyCheck;
    uint8_t sessionKey[SESSION_KEY_SIZE];
    uint8_t dha[SESSION_KEY_SIZE]; //todo maybe smaller DHA
    uint16_t seq;
    uint32_t frequency;
    uint8_t	rfChain;
    uint16_t bandwidth;
    uint32_t datarate;
    std::string	coderate;
    std::chrono::seconds timer;
};

class DevicesTable {

    std::map<std::string,EndDevice> map;
    static std::mutex mapMutex;
    bool isInMap(std::string deviceId);
    bool isInMap(std::string deviceId, std::map<std::string,EndDevice>::iterator &iterator);
    std::chrono::seconds time;
public:
    bool setPacket(std::string deviceId,struct LoraPacket &packet,uint16_t &seq);
    bool removeFromMap(std::string deviceId);   //will return if finded...
    bool updateMap(std::string deviceId,struct LoraPacket packet,uint16_t seq);
    bool hasSessionKey(std::string deviceId);
    bool isMine(std::string deviceId);
    uint8_t *getSessionKey(std::string deviceId);
    uint16_t getSeq(std::string deviceId);
    bool updateSessionkey(std::string deviceId,uint8_t sessionKey[SESSION_KEY_SIZE]);   //will return bool if finded
    bool setSessionKeyCheck(std::string deviceId, bool set);
    bool hasSessionKeyCheck(std::string deviceId);
    void updateByTimer(std::chrono::seconds currentTime);


    //todo add functions for dutty cyckle check and more
    uint8_t remainingDutyCycle(std::string deviceId);
    bool reduceDutyCycle(std::string deviceId,uint8_t messageSize);
    void resetDutyCycle();
    static lgw_pkt_tx_s setTestParams();
};


#endif //PACKETCONVERTER_DEVICESTABLE_H
