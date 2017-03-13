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

class Message {

public:
    MessageType type;
    nlohmann::json message;

    static Message fromStiot(std::string message);
    static Message fromLora(uint8_t payload[],uint16_t	size);
    static Message createSETR();
    static Message createKEYS();
    static Message createKEYR();
    static Message createERR();
    std::string toStiot();
    uint16_t writePayload(uint8_t payload[]);
    nlohmann::json getData();
};


#endif //PACKETCONVERTER_MESSAGE_H
