//
// Created by root on 11.3.2017.
//

#include <cstdint>
#include <fstream>
#include <base64.h>
#include <Encryption.h>
#include <MessageConverter.h>
#include "DevicesTable.h"


std::string Message::toStiot() {
    return this->message.dump();
}

nlohmann::json Message::getData() {
    return this->message.at("message_body");
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

Message Message::fromFile(std::string file) {
    std::ifstream input(file.c_str());
    std::stringstream sstr;
    while(input >> sstr.rdbuf());
    std::string seta = sstr.str();
    return Message::fromJsonString(seta);
}

Message Message::fromJsonString(std::string message) {
    Message out;
    if (message.empty()){
        out.micOk = false;
        return out;
    }
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
            }
        }
        else {
            out.type = UNK;
            out.micOk = false;
        }
        if (out.message.find("message_body")== out.message.end()){
            out.micOk = false;
        }
    }
    //std::cout << test.dump(4) << std::endl;
    return out;
}

LoraPacket Message::fromStiot(Message in,uint8_t *key, uint16_t &seq) {
    LoraPacket out;
    //random number generator
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, 65535); // define the range

    //create from base64 string dev_id data
    int devIdSize= Base64::DecodedLength(in.devId);
    Message::fromBase64(in.devId,out.devId,devIdSize);

    if (in.type == REGA){
        out.rfPower = in.getData().at("power");
        out.ack = MANDATORY_ACK;
        out.type = REGISTER_DOWN;
        //get random seq number
        seq = distr(gen);

        //get pointer to network data size with space for DH key
        uint8_t *networkLength = out.payload + DH_SESSION_KEY_SIZE;
        //calculate networkData size and create network data
        uint8_t networkDataSize = Message::createNetworkData(in.getData().at("net_data"),networkLength+1, true);
        //write networkData size to pointer
        *networkLength = networkDataSize;

        //get pointer to app data
        uint8_t *appLength = networkLength + 1 + networkDataSize;
        //get string from json
        std::string appDataB64 = in.getData().at("app_data");
        //get appData size and write it to pointer
        uint8_t appDataSize = Base64::DecodedLength(appDataB64);
        *appLength = appDataSize;
        //convert data from Base64 string to regular data
        Message::fromBase64(appDataB64,appLength+1,appDataSize);

        //set seq number
        uint8_t *seqPointer = appLength + 1 + appDataSize;
        uint16_t *sequnce = (uint16_t *) seqPointer;
        *sequnce = seq;
        //set mic data
        uint8_t *micPointer = seqPointer + sizeof(uint16_t);
        u_int32_t *mic = (u_int32_t *) micPointer;
        *mic = Message::createCheck(networkLength,1 + networkDataSize + 1 +appDataSize + 2);
        //add random data to fill block size
        uint8_t *padding = micPointer + sizeof(uint32_t);
        //totalSize calculation
        int totalSize = 1 + networkDataSize + 1 +appDataSize + 2 + 4;
        int missingBytes;
        if (totalSize < MIN_BLOCK_SIZE){
            missingBytes = MIN_BLOCK_SIZE - totalSize;
        }
        else {
            missingBytes = totalSize%BLOCK_SIZE;
            if (missingBytes != 0){
                missingBytes = BLOCK_SIZE - missingBytes;
            }
        }
        for (int i =0;i<missingBytes;i++){
            *padding = distr(gen);
            ++padding;
        }
        //set correct size of packaet
        out.size = totalSize + missingBytes + DH_SESSION_KEY_SIZE;
        //debuging data
        //encrypt data
        Encryption::encrypt(networkLength,out.size - DH_SESSION_KEY_SIZE,key);
    }
    else if (in.type == TXL){
        out.rfPower = in.getData().at("power");
        seq +=1;
        out.ack = NO_ACK;
        out.type = DATA_DOWN;

        //get pointer to network data size
        uint8_t *networkLength = out.payload;
        //calculate networkData size and create network data
        uint8_t networkDataSize = Message::createNetworkData(in.getData().at("net_data"),networkLength+1, false);
        //write networkData size to pointer
        *networkLength = networkDataSize;

        //get pointer to app data
        uint8_t *appLength = networkLength + 1 + networkDataSize;
        //get string from json
        std::string appDataB64 = in.getData().at("app_data");
        //get appData size and write it to pointer
        uint8_t appDataSize = Base64::DecodedLength(appDataB64);
        *appLength = appDataSize;
        //convert data from Base64 string to regular data
        Message::fromBase64(appDataB64,appLength+1,appDataSize);

        //set seq number
        uint8_t *seqPointer = appLength + 1 + appDataSize;
        uint16_t *sequnce = (uint16_t *) seqPointer;
        *sequnce = seq;
        //set mic data
        uint8_t *micPointer = seqPointer + sizeof(uint16_t);
        u_int32_t *mic = (u_int32_t *) micPointer;
        *mic = Message::createCheck(networkLength,1 + networkDataSize + 1 +appDataSize + 2);
        //add random data to fill block size
        uint8_t *padding = micPointer + sizeof(uint32_t);
        //totalSize calculation
        int totalSize = 1 + networkDataSize + 1 +appDataSize + 2 + 4;
        int missingBytes;
        if (totalSize < MIN_BLOCK_SIZE){
            missingBytes = MIN_BLOCK_SIZE - totalSize;
        }
        else {
            missingBytes = totalSize%BLOCK_SIZE;
            if (missingBytes != 0){
                missingBytes = BLOCK_SIZE - missingBytes;
            }
        }
        for (int i =0;i<missingBytes;i++){
            *padding = distr(gen);
            ++padding;
        }
        //set correct size of packaet
        out.size = totalSize + missingBytes;
        //debuging
//        if (APP_DEBUG){
//            std::cout << "unecrypted data:" << std::endl;
//            uint8_t *readPointer = out.payload;
//            for (int i=0;i<out.size;i++){
//                printf ("%02x ", *readPointer);
//                if( (i+1)%10 == 0){
//                    printf("\n");
//                }
//                ++readPointer;
//            }
//        }
        //encrypt data
        Encryption::encrypt(networkLength,out.size,key);
    }
    return out;
}

Message Message::createRXL(std::string devId, LoraPacket in, uint8_t *key, uint16_t &seq, unsigned int dutyC) {
    Message out;
    out.type = RXL;
    out.message["message_name"] = "RXL";
    out.message["message_body"] = nlohmann::json::object();
    nlohmann::json &data = out.message.at("message_body");
    data["time"] = in.time.count();
    data["dev_id"] = devId;
    data["sf"] = in.datarate;
    data["cr"] = in.coderate;
    data["band"] = in.bandwidth;
    data["rssi"] = in.rssi;
    data["snr"] = in.snr;
    data["duty_c"] = dutyC;
    if (in.type == DATA_UP || in.type == HELLO_UP){
        data["conf_need"] = false;
    }
    else if (in.type == EMERGENCY_UP){
        data["conf_need"] = true;
    }
    std::string ackString;
    if (in.ack==NO_ACK){
        ackString = "UNSUPPORTED";
    }
    else if (in.ack==OPTIONAL_ACK){
        ackString = "VOLATILE";
    }
    else if (in.ack==MANDATORY_ACK){
        ackString = "MANDATORY";
    }
    data["ack"] = ackString;


    Encryption::decrypt(in.payload,in.size,key);
    uint8_t *dataPointer = in.payload;
    uint8_t dataSize = *dataPointer;
    ++dataPointer;
    std::string dataB64 = Message::toBase64(dataPointer,dataSize);
    data["data"] = dataB64;
    dataPointer += dataSize;
    uint8_t *seqByte = dataPointer;
    uint16_t *seqNum = (uint16_t *) seqByte;
    data["seq"] = *seqNum;
    seq = *seqNum;
    uint8_t *micByte = seqByte + sizeof (uint16_t);
    uint32_t *mic = (uint32_t *) micByte;
    //check mic, size = data_len + data + seq size
    if (isLoraPacketCorrect(in.payload,1+in.payload[0]+2,*mic)){
        out.micOk = true;
        if (APP_DEBUG){
            std::cout << "debug out:" << std::endl;
            std::cout << out.toStiot() << std::endl;
            //create from base64 data
            int messageSize= Base64::DecodedLength(dataB64);
            std::vector<uint8_t > rawData(messageSize);
            Message::fromBase64(dataB64,rawData.data(),messageSize);
            std::cout << "app_data: ";
            for (int i=0;i<messageSize;i++){
                std::cout << rawData[i];
            }
            std::cout << std::endl;
        }
    }
    else {
        out.micOk = false;
    }
    return out;
}

Message Message::createREGR(std::string devId, LoraPacket in, unsigned int dutyC) {
    Message out;
    out.type = REGR;
    out.message["message_name"] = "REGR";
    out.message["message_body"] = nlohmann::json::object();
    nlohmann::json &data = out.message.at("message_body");
    data["time"] = in.time.count();
    data["dev_id"] = devId;
    data["sf"] = in.datarate;
    data["cr"] = in.coderate;
    data["band"] = in.bandwidth;
    data["rssi"] = in.rssi;
    data["snr"] = in.snr;
    data["duty_c"] = dutyC;
    if (APP_DEBUG){
        std::cout << "debug out:" << std::endl;
        std::cout << out.toStiot() << std::endl;
    }
    return out;
}

Message Message::createSETR(std::string setrFile) {
    Message out = Message::fromFile(setrFile);
    out.type = SETR;
    std::cout << "debug out:" << std::endl;
    std::cout << out.toStiot() << std::endl;
    return out;
}

Message Message::createKEYS(std::string devId,uint16_t seq,std::string key) {
    Message out;
    out.type = KEYS;
    out.message["message_name"] = "KEYS";
    out.message["message_body"] = nlohmann::json::object();
    nlohmann::json &data = out.message.at("message_body");
    data["dev_id"] = devId;
    data["seq"] = seq-1;
    data["key"] = key;
    if (APP_DEBUG){
        std::cout << "debug out:" << std::endl;
        std::cout << out.toStiot() << std::endl;
    }
    return out;
}

Message Message::createKEYR(std::string devId) {
    Message out;
    out.type = KEYR;
    out.message["message_name"] = "KEYR";
    out.message["message_body"] = nlohmann::json::object();
    nlohmann::json &data = out.message.at("message_body");
    data["dev_id"] = devId;
    if (APP_DEBUG){
        std::cout << "debug out:" << std::endl;
        std::cout << out.toStiot() << std::endl;
    }
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
    if (APP_DEBUG){
        std::cout << "debug out:" << std::endl;
        std::cout << out.toStiot() << std::endl;
    }
    return out;
}

uint8_t Message::createNetworkData(nlohmann::json paramArray, uint8_t *data,bool full) {
    uint8_t size = 0;
    uint8_t *pointer = data;
    //full config with reduced overhead
    if (full){
        //initial NULL byte
        *pointer = 0;
        ++pointer;
        ++size;
        for (auto& param1 : paramArray) {
            uint8_t typeMultiply = 0;
            if (param1.find("type")!= param1.end()){
                std::string typeString = param1.at("type");
                if (typeString=="NORMAL"){
                    typeMultiply = 0;
                }
                else if(typeString=="REG"){
                    typeMultiply = 1;
                }
                else if (typeString=="EMER"){
                    typeMultiply = 2;
                }
            }
            else {
                return 0;
            }
            if (param1.find("freqs")!= param1.end()){
                uint8_t *backupPointer = pointer;
                *pointer = NET_FR + (typeMultiply*NET_TYPE_DIFF);
                uint8_t count = 0;
                ++pointer;
                ++size;
                for (auto& freq1 : param1.at("freqs")) {
                    unsigned int freqNum = freq1;
                    unsigned int diff = (freqNum - NET_MIN_FREQ)/100000;
                    *pointer = (uint8_t) diff;
                    ++count;
                    ++pointer;
                    ++size;
                }
                *backupPointer +=count;
            }
            else {
                return 0;
            }
            if (param1.find("band")!= param1.end()){
                int bandwith = param1.at("band");
                uint8_t reducedBand = 0;
                switch (bandwith){
                    case 500000:
                        reducedBand = 0;
                        break;
                    case 250000:
                        reducedBand = 1;
                        break;
                    case 125000:
                        reducedBand = 2;
                        break;
                    default:
                        reducedBand = 3;
                }
                reducedBand = reducedBand << 4;
                *pointer = reducedBand;
            }
            else {
                return 0;
            }
            if (param1.find("cr")!= param1.end()){
                std::string codeRade = param1.at("cr");
                uint8_t reducedCR =0;
                if (codeRade=="4/5"){
                    reducedCR = 0;
                }
                else if (codeRade=="4/6"){
                    reducedCR = 1;
                }
                else if (codeRade=="4/7"){
                    reducedCR = 2;
                }
                else if (codeRade=="4/8"){
                    reducedCR = 3;
                }
                else {
                    reducedCR = 4;
                }
                *pointer += reducedCR;
                ++pointer;
                ++size;
            }
            else {
                return 0;
            }
            if (param1.find("power")!= param1.end()){
                int power = param1.at("power");
                uint8_t reducedPower = (uint8_t) power;
                reducedPower = reducedPower & 0x0F;
                reducedPower = reducedPower << 4;
                *pointer = reducedPower;
            }
            else {
                return 0;
            }
            if (param1.find("sf")!= param1.end()){
                int spreadingFactor = param1.at("sf");
                uint8_t reducedSF = 0;
                switch (spreadingFactor){
                    case 7:
                        reducedSF = 0;
                        break;
                    case 8:
                        reducedSF = 1;
                        break;
                    case 9:
                        reducedSF = 2;
                        break;
                    case 10:
                        reducedSF = 3;
                        break;
                    case 11:
                        reducedSF = 4;
                        break;
                    case 12:
                        reducedSF = 5;
                        break;
                    default:
                        reducedSF = 6;
                }
                *pointer += reducedSF;
                ++pointer;
                ++size;
            }
            else {
                return 0;
            }
        }
    }
    else {
        for (auto& param1 : paramArray) {
            uint8_t typeMultiply = 0;
            if (param1.find("type")!= param1.end()){
                std::string typeString = param1.at("type");
                if (typeString=="NORMAL"){
                    typeMultiply = 0;
                }
                else if(typeString=="REG"){
                    typeMultiply = 1;
                }
                else if (typeString=="EMER"){
                    typeMultiply = 2;
                }
            }
            if (param1.find("freqs")!= param1.end()){
                uint8_t *backupPointer = pointer;
                uint8_t count = 0;
                bool leastOne = false;
                for (auto& freq1 : param1.at("freqs")) {
                    if(!leastOne){
                        *pointer = NET_FR + (typeMultiply*NET_TYPE_DIFF);
                        ++pointer;
                        ++size;
                        leastOne = true;
                    }
                    unsigned int freqNum = freq1;
                    unsigned int diff = (freqNum - NET_MIN_FREQ)/100000;
                    *pointer = (uint8_t) diff;
                    ++count;
                    ++pointer;
                    ++size;
                }
                *backupPointer +=count;
            }
            if (param1.find("band")!= param1.end()){
                *pointer = NET_BAND + (typeMultiply*NET_TYPE_DIFF);
                int bandwith = param1.at("band");
                uint8_t reducedBand = 0;
                switch (bandwith){
                    case 500000:
                        reducedBand = 0;
                        break;
                    case 250000:
                        reducedBand = 1;
                        break;
                    case 125000:
                        reducedBand = 2;
                        break;
                    default:
                        reducedBand = 3;
                }
                *pointer += reducedBand;
                ++pointer;
                ++size;
            }
            if (param1.find("cr")!= param1.end()){
                *pointer = NET_CR + (typeMultiply*NET_TYPE_DIFF);
                std::string codeRade = param1.at("cr");
                uint8_t reducedCR =0;
                if (codeRade=="4/5"){
                    reducedCR = 0;
                }
                else if (codeRade=="4/6"){
                    reducedCR = 1;
                }
                else if (codeRade=="4/7"){
                    reducedCR = 2;
                }
                else if (codeRade=="4/8"){
                    reducedCR = 3;
                }
                else {
                    reducedCR = 4;
                }
                *pointer += reducedCR;
                ++pointer;
                ++size;

            }
            if (param1.find("power")!= param1.end()){
                *pointer = NET_PW + (typeMultiply*NET_TYPE_DIFF);
                int power = param1.at("power");
                uint8_t reducedPower = (uint8_t) power;
                reducedPower = reducedPower & 0x0F;
                *pointer += reducedPower;
                ++pointer;
                ++size;
            }
            if (param1.find("sf")!= param1.end()){
                *pointer = NET_SF + (typeMultiply*NET_TYPE_DIFF);
                int spreadingFactor = param1.at("sf");
                uint8_t reducedSF = 0;
                switch (spreadingFactor){
                    case 7:
                        reducedSF = 0;
                        break;
                    case 8:
                        reducedSF = 1;
                        break;
                    case 9:
                        reducedSF = 2;
                        break;
                    case 10:
                        reducedSF = 3;
                        break;
                    case 11:
                        reducedSF = 4;
                        break;
                    case 12:
                        reducedSF = 5;
                        break;
                    default:
                        reducedSF = 6;
                }
                *pointer += reducedSF;
                ++pointer;
                ++size;
            }
        }
    }
    return size;
}

uint32_t Message::createCheck(uint8_t *data,int size) {
    //One-at-a-Time hash
    uint32_t hash = 0;
    for (int i = 0; i < size; i++)
    {
        hash += data[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

bool Message::isLoraPacketCorrect(uint8_t *in,int size,uint32_t compare) {
    uint32_t result = Message::createCheck(in,size);
    if (result==compare){
        return true;
    }
    return false;
}
