//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_MESSAGE_H
#define PACKETCONVERTER_MESSAGE_H
#include <string>
#include <chrono>
#include "json.hpp"

enum MessageType{
    SETR,SETA,REGR,REGA,KEYS,KEYR,KEYA,RXL,TXL,CONFIG,ERROR,UNK
};

enum LoraType{
    NORMAL,EMERGENCY,REGISTER
};

enum LoraAck{
    NO_ACK,OPTIONAL_ACK,MANDATORY_ACK
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
    std::chrono::milliseconds time;
};

class Message {
    //pomocne funkcie
    static uint8_t createNetworkData(nlohmann::json paramObject, uint8_t *data);
    static bool isLoraPacketCorrect(LoraPacket in);
public:
    MessageType type;
    nlohmann::json message;
    std::string devId;
    bool micOk;
    std::string toStiot();
    nlohmann::json getData();

    static std::string toBase64(uint8_t *data, unsigned int size);
    static void fromBase64(std::string data,uint8_t *outData,unsigned int outSize);

    static Message fromJsonString(std::string message);
    static Message fromLora(LoraPacket in, uint8_t *key);
    static LoraPacket fromStiot(Message in,uint8_t *key);
    //todo generators for jsons + networkData + fromLoraPacket
    static Message createSETR(std::string setrFile);
    static Message createKEYS(std::string devId,uint16_t seq,std::string key);
    static Message createKEYR(std::string devId);
    static Message createERR(uint32_t error,std::string description);
    static Message createREGR(std::string devId,LoraPacket in);
    static Message createRXL(std::string devId,LoraPacket in);
    static Message fromFile(std::string file);
};


#endif //PACKETCONVERTER_MESSAGE_H
