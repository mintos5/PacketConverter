//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_MESSAGE_H
#define PACKETCONVERTER_MESSAGE_H
#include <string>
#include <chrono>
#include "json.hpp"
#define MIN_BLOCK_SIZE 8
#define BLOCK_SIZE 4
#define NET_BAND 0x10
#define NET_CR 0x20
#define NET_FR 0x30
#define NET_PW 0x40
#define NET_SF 0x50
#define NET_TYPE_DIFF 0x50
#define NET_MIN_FREQ 863000000

#define DEV_ID_SIZE 3

enum MessageType{
    SETR,SETA,REGR,REGA,KEYS,KEYR,KEYA,RXL,TXL,CONFIG,ERROR,UNK
};

enum LoraType{
    REGISTER_UP,DATA_UP,HELLO_UP,EMERGENCY_UP,REGISTER_DOWN,DATA_DOWN
};

enum LoraAck{
    NO_ACK,OPTIONAL_ACK,MANDATORY_ACK
};

struct LorafiitFooter{

};

struct LoraPacket{
    uint32_t frequency;
    uint8_t	rfChain;
    uint32_t bandwidth;
    uint32_t datarate;
    std::string	coderate;
    float rssi;
    float snr;
    uint16_t size;          // size without devId and type
    uint8_t payload[251];
    uint8_t devId[DEV_ID_SIZE];
    int8_t rfPower;
    LoraType type;
    LoraAck ack;
    std::chrono::milliseconds time;
};

class Message {

public:
    //pomocne funkcie
    static uint8_t createNetworkData(nlohmann::json paramArray, uint8_t *data,bool full);
    static uint32_t createCheck(uint8_t *data, int size);
    static bool isLoraPacketCorrect(uint8_t *in,int size,uint32_t compare);

    MessageType type;
    nlohmann::json message;
    std::string devId;
    bool micOk = true;
    std::string toStiot();
    nlohmann::json getData();

    static std::string toBase64(uint8_t *data, unsigned int size);
    static void fromBase64(std::string data,uint8_t *outData,unsigned int outSize);

    static Message fromFile(std::string file);
    static Message fromJsonString(std::string message);

    static LoraPacket fromStiot(Message in,uint8_t *key, uint16_t &seq);

    static Message createRXL(std::string devId, LoraPacket in, uint8_t *key, uint16_t &seq, unsigned int dutyC);
    static Message createREGR(std::string devId,LoraPacket in, unsigned int dutyC);
    static Message createSETR(std::string setrFile);
    static Message createKEYS(std::string devId,uint16_t seq,std::string key);
    static Message createKEYR(std::string devId);
    static Message createERR(uint32_t error,std::string description);
};


#endif //PACKETCONVERTER_MESSAGE_H
