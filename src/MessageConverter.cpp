//
// Created by root on 8.4.2017.
//

#include <csignal>
#include <base64.h>
#include <DiffieHellman.h>
#include <Encryption.h>
#include "MessageConverter.h"
#include "ConcentratorController.h"
#include "ConnectionController.h"

int MessageConverter::start() {
    this->fromLoraFiber = std::thread(&MessageConverter::fromLora,this);
    this->fromStiotFiber = std::thread(&MessageConverter::fromStiot,this);
    this->timerFiber = std::thread(&MessageConverter::timerFunction,this);
    return 0;
}

void MessageConverter::stop() {
    std::unique_lock<std::mutex> guard1(this->fromLoraMutex);
    this->fromLoraRun = false;
    guard1.unlock();
    std::unique_lock<std::mutex> guardData1(this->loraDataMutex);
    guardData1.unlock();
    this->fromLoraCond.notify_one();

    std::unique_lock<std::mutex> guard2(this->fromStiotMutex);
    this->fromStiotRun = false;
    guard2.unlock();
    std::unique_lock<std::mutex> guardData2(this->stiotDataMutex);
    guardData2.unlock();
    this->fromStiotCond.notify_one();

    std::unique_lock<std::mutex> guard3(this->timerMutex);
    this->timerRun = false;
    guard3.unlock();
}

void MessageConverter::join() {
    this->fromLoraFiber.join();
    this->fromStiotFiber.join();
    this->timerFiber.join();
}

void MessageConverter::addToQueue(Message message) {
    std::unique_lock<std::mutex> guard(this->stiotDataMutex);
    this->fromStiotData.push(message);
    guard.unlock();
    fromStiotCond.notify_one();
}

void MessageConverter::addToQueue(LoraPacket message) {
    std::unique_lock<std::mutex> guard(this->loraDataMutex);
    this->fromLoraData.push(message);
    guard.unlock();
    fromLoraCond.notify_one();
}

void MessageConverter::addBulk(std::vector<LoraPacket> vector) {
    std::unique_lock<std::mutex> guard(this->loraDataMutex);
    for (auto const& ent: vector){
        this->fromLoraData.push(ent);
    }
    guard.unlock();
    fromLoraCond.notify_one();
}

void MessageConverter::setConnection(const std::shared_ptr<ConnectionController> &connection) {
    MessageConverter::connection = connection;
}

void MessageConverter::setConcentrator(const std::shared_ptr<ConcentratorController> &concentrator) {
    MessageConverter::concentrator = concentrator;
}

bool MessageConverter::getFromStiotData(Message &data) {
    std::unique_lock<std::mutex> guard(this->stiotDataMutex);
    //while until some data are in queue or thread end
    while (this->fromStiotData.empty()){
        this->fromStiotCond.wait(guard);
        std::unique_lock<std::mutex> guardRun(this->fromStiotMutex);
        if (!this->fromStiotRun){
            return false;
        }
    }
    data = this->fromStiotData.front();
    this->fromStiotData.pop();
    return true;
}

bool MessageConverter::getFromLoraData(LoraPacket &data) {
    std::unique_lock<std::mutex> guard(this->loraDataMutex);
    //while until some data are in queue or thread end
    while (this->fromLoraData.empty()){
        this->fromLoraCond.wait(guard);
        std::unique_lock<std::mutex> guardRun(this->fromLoraMutex);
        if (!this->fromLoraRun){
            return false;
        }
    }
    data = this->fromLoraData.front();
    this->fromLoraData.pop();
    return true;
}

void MessageConverter::fromStiot() {
    std::unique_lock<std::mutex> guard(this->fromStiotMutex);
    bool oneTime = false;
    while (this->fromStiotRun){
        guard.unlock();
        Message in;
        //just gen one message and send
        if (this->getFromStiotData(in)){
            if (in.type==SETA){
                if (oneTime){
                    //need to turn off and turn on
                    concentrator->stop();
                    concentrator->join();
                    concentrator->start();
                    concentrator->startConcentrator(in);
                }
                else {
                    concentrator->startConcentrator(in);
                    oneTime = true;
                }
            }
            else if (in.type==KEYA){
                if (devicesTable.isMine(in.devId)){
                    //get SEQ number from json
                    uint16_t seq = in.getData().at("seq");
                    //get base64 string with session key
                    std::string keyB64 = in.getData().at("key");
                    int dataSize = Base64::DecodedLength(keyB64);
                    std::vector<uint8_t > keyVector(dataSize);
                    Message::fromBase64(keyB64,keyVector.data(),dataSize);
                    devicesTable.updateSessionkey(in.devId,keyVector.data(),seq);
                }
            }
            else if (in.type==REGA){
                if (devicesTable.isMine(in.devId)){
                    //todo generate session key + random number from pre_shared key extract data
                    std::string preSharedKey = in.getData().at("sh_key");

                    uint64_t dhb = DiffieHellman::getDHB(7,2,128);
                    //uint64_t ses1 = DiffieHellman::getSessionKey()

                    devicesTable.updateSessionkey(in.devId, (uint8_t *) &dhb,0);
                    //message is ready
                    uint16_t seqNum;
                    LoraPacket out = Message::fromStiot(in,devicesTable.getSessionKey(in.devId),seqNum);
                    devicesTable.setSeq(in.devId,seqNum);
                    devicesTable.setPacket(in.devId,out);
                    //todo set dhb to packet
                    concentrator->addToQueue(out);
                }
            }
            else if (in.type==TXL){
                if (devicesTable.isMine(in.devId)){
                    if (devicesTable.hasSessionKey(in.devId)){
                        uint16_t seqNum;
                        seqNum = devicesTable.getSeq(in.devId);
                        LoraPacket out = Message::fromStiot(in,devicesTable.getSessionKey(in.devId),seqNum);
                        devicesTable.setSeq(in.devId,seqNum);
                        devicesTable.setPacket(in.devId,out);
                        concentrator->addToQueue(out);
                    }
                    else {
                        //send KEYR to server and discard message
                        connection->addToQueue(Message::createKEYR(in.devId));
                    }
                }
            }
            else if (in.type==ERROR){
                //todo handle errors, like KEYR(need to set isMine to false)
            }
        }
        guard.lock();
    }
    guard.unlock();

}

void MessageConverter::fromLora() {
    std::unique_lock<std::mutex> guard(this->fromLoraMutex);
    while (this->fromLoraRun){
        guard.unlock();
        std::unique_lock<std::mutex> guardData(this->loraDataMutex);
        LoraPacket in;
        std::vector<LoraPacket> inVector;
        Message out;
        std::vector<Message> outVector;
        if (this->fromLoraData.empty()){
            guardData.unlock();
            if (this->getFromLoraData(in)){
                inVector.push_back(in);
                std::cout << "jeden ";
            }
            else {
                return;
            }
            guardData.lock();
        }
        while (!this->fromLoraData.empty()){
            guardData.unlock();
            std::cout << "mnoho ";
            if (this->getFromLoraData(in)){
                inVector.push_back(in);
                guardData.lock();
            }
        }
        guardData.unlock();
        std::cout << "koniec whilu"<< std::endl;

        for (auto const& element: inVector){
            std::string devId = Message::toBase64(in.devId,24);
            if (element.type==REGISTER_UP){
                this->devicesTable.updateMap(devId,element,0);
                connection->addToQueue(Message::createREGR(devId,element,this->devicesTable.remainingDutyCycle(devId)));
            }
            else if (element.type==DATA_UP || element.type==HELLO_UP || element.type == EMERGENCY_UP){
                if (devicesTable.hasSessionKey(devId) && devicesTable.isMine(devId)){
                    uint16_t seqNum;
                    out = Message::fromLora(devId,element,devicesTable.getSessionKey(devId),seqNum,
                                            this->devicesTable.remainingDutyCycle(devId) );
                    if (out.micOk){
                        devicesTable.setSeq(devId,seqNum);
                        outVector.push_back(out);
                        if (!devicesTable.hasSessionKeyCheck(devId)){
                            devicesTable.setSessionKeyCheck(devId,true);
                            //send KEYS message
                            std::string keyString = Message::toBase64(devicesTable.getSessionKey(devId),SESSION_KEY_SIZE);
                            connection->addToQueue(Message::createKEYS(devId,devicesTable.getSeq(devId),keyString));
                        }
                    }
                }
                else {
                    if (devicesTable.isMine(devId)){
                        //message must be from mine end device
                        connection->addToQueue(Message::createKEYR(devId));
                        //send the message again to queue and wait for server KEYA
                        this->addToQueue(element);
                    }
                }
            }
        }
        connection->addBulk(outVector);
        guard.lock();
    }
    guard.unlock();
}

void MessageConverter::timerFunction() {
    std::chrono::seconds startTime = std::chrono::duration_cast< std::chrono::seconds >
            (std::chrono::system_clock::now().time_since_epoch());
    std::unique_lock<std::mutex> guard(this->timerMutex);
    bool oneTime = true;
    while (this->timerRun){
        guard.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::chrono::seconds currentTime = std::chrono::duration_cast< std::chrono::seconds >
                (std::chrono::system_clock::now().time_since_epoch());
        std::cout << "STATUS TIMER" << std::endl;
        if (oneTime){
            //todo dolezite
//            std::string textik = "ahojsvetasjndasdas1234567890";
//            std::copy(textik.c_str(),textik.c_str()+(test.size),test.payload);
//            std::string vstup = "WAUU";
//            uint8_t vonku[256];
//            char extra[256];
//            Base64::Encode(vstup.c_str(), 3, (char *) vonku, 256);
//            std::cout << vonku[0] << std::endl;
//            Base64::Decode((const char *) vonku, 4, extra, 256);
//            std::string extra2 = extra;
//            std::cout << extra2 << std::endl;
            //this->concentrator->sendRawTest("");
            //Message test;
            //test = Message::fromFile("keys.json");
            //test = Message::fromFile("keyr.json");
            //connection->addToQueue(test);
            //Message::createKEYR("asud");
            //Message::createKEYS("wauw",78,"asygdaisudjgaisdasdyabsdjhabsdasd");
            std::vector<uint8_t > key(16);
            key[0] = 5;
            key[1] = 5;
            std::vector<uint8_t > plain(8);
            plain[0] = 22;
            plain[1] = 93;
            std::vector<uint8_t > cypher(8);
            std::copy(plain.data(),plain.data()+8,cypher.data());
            Encryption::encrypt(cypher.data(),plain.size(),key.data());
            std::vector<uint8_t > plain2(8);
            std::copy(cypher.data(),cypher.data()+8,plain2.data());
            Encryption::decrypt(plain2.data(),plain2.size(),key.data());
            std::cout << "asdas";

        }
        if (!this->connection->connected && currentTime.count()-startTime.count()>CONNECTION_TIMEOUT){
            //std::cerr << "Connection timeout" << std::endl;
            //raise(SIGINT);
        }
        devicesTable.updateByTimer(currentTime);
//        std::vector<LoraPacket> test;
//        LoraPacket tt;
//        test.push_back(tt);
//        test.push_back(tt);
//        test.push_back(tt);
//        this->addBulk(test);
        guard.lock();
    }
    guard.unlock();
}
