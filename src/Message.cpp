//
// Created by root on 11.3.2017.
//

#include <cstdint>
#include <Message.h>
#include <json.hpp>
#include <fstream>


Message Message::fromJsonString(std::string message) {
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
            else if (out.message.at("message_name").get<std::string>()=="KEYA"){
                out.type = KEYA;
            }
            else if (out.message.at("message_name").get<std::string>()=="TXL"){
                out.type = TXL;
            }
            else if (out.message.at("message_name").get<std::string>()=="REGA"){
                out.type = REGA;
            }
        }
        else {
            out.type = UNK;
        }
    }
    //std::cout << test.dump(4) << std::endl;
    return out;
}

Message Message::fromLora(LoraPacket in) {
    //todo hlavna zmena z lora na stiot
    //vytvor hlavicku a tu zparsuj
    //desifruj data a premen na Base64
    //ak sa jedna o registraciu, budes pouzivat tabulku
    //zabal vsetko do Message
    //uloz info do tabulky, keby bolo nutne posielat odpoved
    std::cout << "NIECO mi doslo" << std::endl;
    std::cout << "frekvencia " << in.frequency << std::endl;
    std::cout << "SF: " << in.coderate << std::endl;
    std::cout << "rssi: " << std::fixed << std::setprecision(3) << in.rssi << std::endl;
    std::cout << "SNR: " << std::fixed << std::setprecision(3) << in.snr << std::endl;
    for (int j = 0; j < in.size; ++j) {
        printf("%02X", in.payload[j]);
    }
    std::cout << std::endl;
    return Message();
}

LoraPacket Message::fromStiot(Message in) {
    //todo hlavna zmena z stiot na lora
    //cekni tabulku, ci viem o koho sa jedna a ci nevyprsal timer
    //specialne pri registraciii sa bude diat veci
    //vytvor LoraPacket
    //vytvor strukturu a tu zasifruj
    return LoraPacket();
}

std::string Message::toStiot() {
    return this->message.dump();
}

nlohmann::json Message::getData() {
    return this->message.at("message_body");
}

Message Message::fromFile(std::string file) {
    std::ifstream input(file.c_str());
    std::stringstream sstr;
    while(input >> sstr.rdbuf());
    std::string seta = sstr.str();
    return Message::fromJsonString(seta);
}

uint8_t Message::createNetworkData(nlohmann::json paramObject, uint8_t *data) {
    return 0;
}

bool Message::isLoraPacketCorrect(LoraPacket in) {
    return false;
}

Message Message::createSETR(std::string setrFile) {
    Message out = Message::fromFile(setrFile);
    out.type = SETR;
    return out;
}

Message Message::createKEYS(std::string devId,uint16_t seq,std::string key) {
    Message out;
    out.type = KEYS;
    out.message["message_name"] = "KEYS";
    out.message["message_body"] = nlohmann::json::object();
    nlohmann::json &data = out.message.at("message_body");
    data["dev_id"] = devId;
    data["seq"] = seq;
    data["key"] = key;
    std::cout << out.toStiot() << std::endl;
    return out;
}

Message Message::createKEYR(std::string devId) {
    Message out;
    out.type = KEYR;
    out.message["message_name"] = "KEYR";
    out.message["message_body"] = nlohmann::json::object();
    nlohmann::json &data = out.message.at("message_body");
    data["dev_id"] = devId;
    std::cout << out.toStiot() << std::endl;
    return out;
}

Message Message::createERR(uint32_t error,std::string description) {
    Message out;
    out.type = ERROR;
    out.message["message_name"] = "ERROR";
    out.message["message_body"] = nlohmann::json::object();
    nlohmann::json &data = out.message.at("message_body");
    data["error"] = error;
    data["error_desc"] = description;
    std::cout << out.toStiot() << std::endl;
    return out;
}
