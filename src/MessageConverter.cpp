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
            else if (in.type==REGA){
                if (devicesTable.isMine(in.devId)){
                    //todo generate session key
                    uint64_t dhb = DiffieHellman::getDHB(7,2,128);
                    //uint64_t ses1 = DiffieHellman::getSessionKey()
                    devicesTable.updateSessionkey(in.devId, (uint8_t *) &dhb);
                    //message is ready
                    LoraPacket out = Message::fromStiot(in,devicesTable.getSessionKey(in.devId));
                    concentrator->addToQueue(out);
                }
            }
            else if (in.type==TXL){
                if (devicesTable.isMine(in.devId)){
                    if (devicesTable.hasSessionKey(in.devId)){
                        LoraPacket out = Message::fromStiot(in,devicesTable.getSessionKey(in.devId));
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
            if (element.type==REGISTER){
                this->devicesTable.updateMap(devId,element,0);
                connection->addToQueue(Message::createREGR(devId,element));
            }
            else if (element.type==NORMAL || element.type==EMERGENCY){
                if (devicesTable.hasSessionKey(devId) && devicesTable.isMine(devId)){
                    out = Message::fromLora(element,devicesTable.getSessionKey(devId));
                    if (out.micOk){
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
            std::vector<uint8_t > key(128);
            key[0] = 5;
            key[1] = 5;
            std::vector<uint8_t > plain(64);
            plain[0] = 22;
            plain[1] = 93;
            std::vector<uint8_t > cypher(64);
            Encryption::encrypt(plain.data(),plain.size(),key.data(),cypher.data());
            std::vector<uint8_t > plain2(64);
            Encryption::decrypt(cypher.data(),cypher.size(),key.data(),plain2.data());
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
