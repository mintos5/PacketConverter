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

struct LoraPacket{
    uint8_t payload[256];
    uint16_t	size;
    LoraType type;
    //todo dalsie parametre... aj na posielanie a prijem aj rf_chain
};

class Message {

public:
    MessageType type;
    nlohmann::json message;

    static Message fromJsonString(std::string message);
    static Message fromLora(LoraPacket in);
    static LoraPacket fromStiot(Message in);
    static Message createSETR();
    static Message createKEYS();
    static Message createKEYR();
    static Message createERR();
    static Message fromFile(std::string file);
    std::string toStiot();
    uint16_t writePayload(uint8_t payload[]);
    nlohmann::json getData();
};


#endif //PACKETCONVERTER_MESSAGE_H
