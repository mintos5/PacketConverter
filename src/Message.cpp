//
// Created by root on 11.3.2017.
//

#include <cstdint>
#include <Message.h>
#include <json.hpp>


Message Message::fromStiot(std::string message) {
    Message out;
    out.message = nlohmann::json::parse(message);
    if (out.message.find("conf")!= out.message.end()){
        out.type = CONFIG;
    }
    else {
        if (out.message.find("message_name")!= out.message.end()){
            if (out.message.at("message_name").get<std::string>()=="SETA"){
                out.type = SETA;
            }
            else {
                out.type = SETR;
            }
        }
        else {
            out.type = UNK;
        }
    }
    //std::cout << test.dump(4) << std::endl;
    return out;
}

Message Message::fromLora(uint8_t *payload, uint16_t size) {
    return Message();
}

std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char>> Message::toStiot() {
    return this->message.dump();
}

unsigned short Message::writePayload(uint8_t *payload) {
    return 0;
}

nlohmann::json Message::getData() {
    return this->message.at("message_body");
}
