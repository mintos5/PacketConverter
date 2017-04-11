//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_MESSAGE_H
#define PACKETCONVERTER_MESSAGE_H
#include <string>
#include "json.hpp"

enum MessageType{
    SETR,SETA,REGR,REGA,KEYS,KEYR,KEYA,RXL,TXL,CONFIG,UNK
};

enum LoraType{
    NORMAL,EMERGENCY,REGISTER
};

enum LoraAck{
    NO_ACK,OPTIONAL_ACK,MANDATORY_ACK
};

struct LorafiitHeader{

};
struct LorafiitFooter{

};

struct LoraPacket{
    uint32_t frequency;
    uint8_t	rfChain;
    uint16_t bandwidth;
    uint32_t datarate;
    std::string	coderate;
    float rssi;
    float snr;
    uint16_t size;
    uint8_t payload[256];
    uint8_t devId[24];
    uint8_t dh[128];
    int8_t rfPower;
    LoraType type;
    LoraAck ack;
};

class Message {
    //todo Base64 function
    std::string to64String(uint8_t *data);
    void from64String(std::string,uint8_t *data);

public:
    MessageType type;
    nlohmann::json message;

    static Message fromJsonString(std::string message);
    static Message fromLora(LoraPacket in);
    static LoraPacket fromStiot(Message in);
    static uint8_t createNetworkData(nlohmann::json paramObject, uint8_t *data);
    //todo generators for jsons + networkData + citanie
    static Message createSETR();
    static Message createKEYS();
    static Message createKEYR();
    static Message createERR();
    static Message fromFile(std::string file);
    std::string toStiot();
    nlohmann::json getData();
};


#endif //PACKETCONVERTER_MESSAGE_H
