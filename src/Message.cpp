//
// Created by root on 11.3.2017.
//

#include <cstdint>
#include <Message.h>
#include <fstream>
#include <base64.h>
#include "DevicesTable.h"


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
                out.devId = out.getData().at("dev_id");
            }
            else if (out.message.at("message_name").get<std::string>()=="TXL"){
                out.type = TXL;
                out.devId = out.getData().at("dev_id");
            }
            else if (out.message.at("message_name").get<std::string>()=="REGA"){
                out.type = REGA;
                out.devId = out.getData().at("dev_id");
            }
            else if (out.message.at("message_name").get<std::string>()=="ERROR"){
                out.type = ERROR;
                out.devId = out.getData().at("dev_id");
            }
        }
        else {
            out.type = UNK;
        }
    }
    //std::cout << test.dump(4) << std::endl;
    return out;
}

Message Message::fromLora(LoraPacket in, uint8_t *key) {
    //todo hlavna zmena z lora na stiot a sifrovanie
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

LoraPacket Message::fromStiot(Message in,uint8_t *key) {
    //todo hlavna zmena z stiot na lora a sifrovanie
    //specialne pri registraciii sa bude diat veci
    LoraPacket out;
    if (in.type == REGA){
        //vynechat dostatocne miesto pre DH key
        //pride mi aj pre_shared key
    }
    else if (in.type == TXL){

    }
    else if (in.type == KEYA){

    }
    //vytvor LoraPacket
    //vytvor strukturu a tu zasifruj
    return out;
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
    //todo Creating lorafiit structure
    return 0;
}

bool Message::isLoraPacketCorrect(LoraPacket in) {
    //todo MIC control
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

Message Message::createREGR(std::string devId, LoraPacket in) {
    //todo write regr generator
    return Message();
}

Message Message::createRXL(std::string devId, LoraPacket in) {
    //todo write rxl generator
    return Message();
}

std::string Message::toBase64(uint8_t *data, unsigned int size) {
    int charSize = Base64::EncodedLength(size);
    std::vector<char> devIdChar(charSize);
    Base64::Encode((const char *) data, size, devIdChar.data(), charSize);
    devIdChar.push_back(0);
    std::string outData = devIdChar.data();
    return outData;
}

void Message::fromBase64(std::string data, uint8_t *outData,unsigned int outSize) {
    Base64::Decode(data.c_str(), data.size(), (char *) outData, outSize);
}
