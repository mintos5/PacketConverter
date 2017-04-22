//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_DEVICESTABLE_H
#define PACKETCONVERTER_DEVICESTABLE_H
#define DH_SESSION_KEY_SIZE 16
#define TIME_INTERVAL 3600
#define FLUSH_DEVICE 86400

#include <chrono>
#include <mutex>
#include <map>
#include "Message.h"

struct EndDevice{
    bool myDevice;
    bool sessionKeyExists;
    bool sessionKeyCheck;
    uint8_t sessionKey[DH_SESSION_KEY_SIZE];
    uint8_t dha[DH_SESSION_KEY_SIZE];
    uint16_t seq;
    uint32_t frequency;
    uint8_t	rfChain;
    uint32_t bandwidth;
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
    //counters
    long onAir0 = 3600;
    long onAir1 = 36000;
    long onAir10 = 360000;

public:
    bool isMine(std::string deviceId);
    bool hasSessionKey(std::string deviceId);
    bool hasSessionKeyCheck(std::string deviceId);

    bool setSeq(std::string deviceId,uint16_t seq);
    bool setSessionkey(std::string deviceId, uint8_t *sessionKey, uint16_t seq);   //will return bool if finded
    bool setSessionKeyCheck(std::string deviceId, bool set);

    uint8_t *getSessionKey(std::string deviceId);
    uint16_t getSeq(std::string deviceId);
    uint8_t *getDh(std::string deviceId);

    bool addDevice(std::string deviceId, struct LoraPacket packet, uint16_t seq);
    bool updateDevice(std::string deviceId, struct LoraPacket packet, uint16_t seq);
    bool setPacket(std::string deviceId,struct LoraPacket &packet);
    bool removeFromMap(std::string deviceId);

    void updateByTimer(std::chrono::seconds currentTime);

    void resetDutyCycle();
    long remainingDutyCycle(std::string deviceId);
    long calculateDutyCycle(std::map<std::string, EndDevice>::iterator &iterator, uint8_t payloadSize);
    bool reduceDutyCycle(std::string deviceId,uint8_t messageSize);
};


#endif //PACKETCONVERTER_DEVICESTABLE_H
