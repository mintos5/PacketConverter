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
#define P 0xfffffffb

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
    std::cout << "Converter thread stopped" << std::endl;
}

void MessageConverter::join() {
    this->fromLoraFiber.join();
    this->fromStiotFiber.join();
    this->timerFiber.join();
}

void MessageConverter::addToQueue(Message message) {
    std::unique_lock<std::mutex> guard(this->stiotDataMutex);
    if (message.micOk){
        this->fromStiotData.push(message);
    }
    else {
        if (APP_DEBUG){
            std::cout << "BAD STIOT data in addToQueue";
        }
    }
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

bool MessageConverter::getFromStiotData(Message &data,std::unique_lock<std::mutex> &guard) {

    //while until some data are in queue or thread end
    while (this->fromStiotData.empty()){
        std::unique_lock<std::mutex> guardRun(this->fromStiotMutex);
        if (!this->fromStiotRun){
            return false;
        }
        guardRun.unlock();
        this->fromStiotCond.wait(guard);
        guardRun.lock();
        if (!this->fromStiotRun){
            return false;
        }
    }
    data = this->fromStiotData.front();
    this->fromStiotData.pop();
    return true;
}

bool MessageConverter::getFromLoraData(LoraPacket &data,std::unique_lock<std::mutex> &guard) {
    //while until some data are in queue or thread end
    while (this->fromLoraData.empty()){
        std::unique_lock<std::mutex> guardRun(this->fromLoraMutex);
        if (!this->fromLoraRun){
            return false;
        }
        guardRun.unlock();
        this->fromLoraCond.wait(guard);
        guardRun.lock();
        if (!this->fromLoraRun || !this->oldData.empty()){
            return false;
        }
    }
    data = this->fromLoraData.front();
    this->fromLoraData.pop();
    return true;
}

void MessageConverter::fromStiot() {
    std::unique_lock<std::mutex> guard(this->fromStiotMutex);
    std::unique_lock<std::mutex> guardData(this->stiotDataMutex);
    bool oneTime = false;
    while (this->fromStiotRun){
        guard.unlock();
        Message in;
        //just gen one message and send
        if (this->getFromStiotData(in,guardData)){
            if (APP_DEBUG){
                std::cout << "new STIOT data:";
            }
            if (in.type==SETA){
                if (APP_DEBUG){
                    std::cout << "SETA" << std::endl;
                }
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
                if (APP_DEBUG){
                    std::cout << "KEYA" << std::endl;
                }
                if (devicesTable.isMine(in.devId)){
                    nlohmann::json data = in.getData();
                    if (data.find("seq") == data.end() || data.find("key") == data.end()){
                        if (APP_DEBUG){
                            std::cerr << "BAD STIOT DATA" << std::endl;
                        }
                        guard.lock();
                        continue;
                    }
                    std::string stringTest = in.getData().at("key");
                    if (stringTest.empty()){
                        if (APP_DEBUG){
                            std::cerr << "BAD STIOT DATA" << std::endl;
                        }
                        guard.lock();
                        continue;
                    }
                    //get SEQ number from json
                    uint16_t seq = in.getData().at("seq");
                    //get base64 string with session key
                    std::string keyB64 = in.getData().at("key");
                    int dataSize = Base64::DecodedLength(keyB64);
                    std::vector<uint8_t > keyVector(dataSize);
                    Message::fromBase64(keyB64,keyVector.data(),dataSize);
                    //update data
                    devicesTable.setSessionkey(in.devId, keyVector.data(), seq);
                    //notify waiting thread
                    std::unique_lock<std::mutex> loraGuardData(this->loraDataMutex);
                    loraGuardData.unlock();
                    this->fromLoraCond.notify_one();
                }
            }
            else if (in.type==REGA){
                if (APP_DEBUG){
                    std::cout << "REGA" << std::endl;
                }
                if (devicesTable.isMine(in.devId)){
                    nlohmann::json data = in.getData();
                    if (data.find("sh_key") == data.end() || data.find("power") == data.end()
                        || data.find("net_data") == data.end() || data.find("app_data") == data.end()){
                        if (APP_DEBUG){
                            std::cerr << "BAD STIOT DATA" << std::endl;
                        }
                        guard.lock();
                        continue;
                    }
                    std::string preSharedKey = data.at("sh_key");
                    int preKeySize = Base64::DecodedLength(preSharedKey);
                    std::vector<uint8_t > preKeyVector(preKeySize);
                    Message::fromBase64(preSharedKey,preKeyVector.data(),preKeySize);

                    std::vector<uint8_t > privateKey(DH_SESSION_KEY_SIZE);
                    std::vector<uint8_t > publicKey(DH_SESSION_KEY_SIZE);
                    std::vector<uint8_t > sessionKey(DH_SESSION_KEY_SIZE);

                    DiffieHellman::getDHB(preKeyVector.data(),publicKey.data(),privateKey.data());
                    DiffieHellman::getSessionKey(preKeyVector.data(),devicesTable.getDh(in.devId)
                            ,privateKey.data(),sessionKey.data());
                    //set correct new session key and default (0) sequence Number
                    if (APP_DEBUG){
                        std::cout << "my public KEY" << std::endl;
                        for (int i=0;i<16;i++){
                            //std::cout << std::hex << sessionKey[i] << std::endl;
                            printf ("%02x ", publicKey[i]);
                        }
                        std::cout << std::endl;

                        std::cout << "end device public KEY" << std::endl;
                        uint8_t *simonKey = devicesTable.getDh(in.devId);
                        for (int i=0;i<16;i++){
                            //std::cout << std::hex << sessionKey[i] << std::endl;
                            printf ("%02x ", simonKey[i]);
                        }
                        std::cout << std::endl;

                        std::cout << "SESSION KEY" << std::endl;
                        for (int i=0;i<16;i++){
                            //std::cout << std::hex << sessionKey[i] << std::endl;
                            printf ("%02x ", sessionKey[i]);
                        }
                        std::cout << std::endl;
                    }
                    devicesTable.setSessionkey(in.devId, sessionKey.data(), 0);
                    //message is ready
                    uint16_t seqNum;
                    LoraPacket out = Message::fromStiot(in,devicesTable.getSessionKey(in.devId),seqNum);
                    //set new seq number to table
                    devicesTable.setSeq(in.devId,seqNum);
                    //set packet to correct freq, etc.
                    devicesTable.setPacket(in.devId,out);
                    //set DHB to packet
                    std::copy(publicKey.data(),publicKey.data() + DH_SESSION_KEY_SIZE,out.payload);
                    if (devicesTable.reduceDutyCycle(in.devId,out.size+4)){
                        concentrator->addToQueue(out);
                    }
                    else {
                        std::cerr << "no enough time to air" << std::endl;
                    }
                }
            }
            else if (in.type==TXL){
                if (APP_DEBUG){
                    std::cout << "TXL" << std::endl;
                }
                if (devicesTable.isMine(in.devId)){
                    nlohmann::json data = in.getData();
                    if (data.find("power") == data.end() ||data.find("net_data") == data.end()
                        || data.find("app_data") == data.end()){
                        if (APP_DEBUG){
                            std::cerr << "BAD STIOT DATA" << std::endl;
                        }
                        guard.lock();
                        continue;
                    }
                    if (devicesTable.hasSessionKey(in.devId)){
                        uint16_t seqNum;
                        seqNum = devicesTable.getSeq(in.devId);
                        LoraPacket out = Message::fromStiot(in,devicesTable.getSessionKey(in.devId),seqNum);
                        devicesTable.setSeq(in.devId,seqNum);
                        devicesTable.setPacket(in.devId,out);
                        if (devicesTable.reduceDutyCycle(in.devId,out.size+4)){
                            concentrator->addToQueue(out);
                        }
                        else {
                            std::cerr << "no enough time to air" << std::endl;
                        }
                    }
                    else {
                        //send KEYR to server and discard message
                        connection->addToQueue(Message::createKEYR(in.devId));
                    }
                }
            }
            else if (in.type==ERROR){
            }
        }
        guard.lock();
    }
    guard.unlock();

}

void MessageConverter::fromLora() {
    std::unique_lock<std::mutex> guard(this->fromLoraMutex);
    std::unique_lock<std::mutex> guardData(this->loraDataMutex);

    while (this->fromLoraRun){
        guard.unlock();

        LoraPacket in;
        std::vector<LoraPacket> inVector;
        Message out;
        std::vector<Message> outVector;
        if (this->fromLoraData.empty()){
            if (this->getFromLoraData(in,guardData)){
                inVector.push_back(in);
                if (APP_DEBUG){
                    std::cout << "NEW LORA data (IF)" << std::endl;
                }
            }
        }
        while (!this->fromLoraData.empty()){
            if (APP_DEBUG){
                std::cout << "NEW LORA data (while)" << std::endl;
            }
            if (this->getFromLoraData(in,guardData)){
                inVector.push_back(in);
            }
        }
        if (APP_DEBUG){
            std::cout << "END OF while in fromLora"<< std::endl;
        }
        //process new LoRaFIIT msg
        for (auto& element: inVector){
            std::string devId = Message::toBase64(element.devId,DEV_ID_SIZE);
            if (element.type==REGISTER_UP){
                std::cout << "LORA REGISTER"<< std::endl;
                this->devicesTable.addDevice(devId, element, 0);
                connection->addToQueue(Message::createREGR(devId,element,this->devicesTable.remainingDutyCycle(devId)));
                this->timerResponseMutex.lock();
                timerRegResponse++;
                this->timerResponseMutex.unlock();
            }
            else if (element.type==DATA_UP || element.type==HELLO_UP || element.type == EMERGENCY_UP){
                std::cout << "LORA DATA_UP + ETC"<< std::endl;
                if (devicesTable.hasSessionKey(devId) && devicesTable.isMine(devId)){
                    uint16_t seqNum;
                    out = Message::createRXL(devId, element, devicesTable.getSessionKey(devId), seqNum,
                                             this->devicesTable.remainingDutyCycle(devId));
                    if (out.micOk){
                        this->devicesTable.updateDevice(devId, element, seqNum);
                        outVector.push_back(out);
                        if (!devicesTable.hasSessionKeyCheck(devId)){
                            devicesTable.setSessionKeyCheck(devId,true);
                            //send KEYS message
                            std::string keyString = Message::toBase64(devicesTable.getSessionKey(devId),DH_SESSION_KEY_SIZE);
                            connection->addToQueue(Message::createKEYS(devId,devicesTable.getSeq(devId),keyString));
                        }
                    }
                    else {
                        if (APP_DEBUG){
                            std::cout << "BAD MIC checksum" << std::endl;
                        }
                    }
                }
                else {
                    //device registration
                    if (!devicesTable.isInTable(devId)){
                        this->devicesTable.addDevice(devId, element, 0);
                    }
                    if (devicesTable.isMine(devId)){
                        //message must be from mine end device and need wait
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                        connection->addToQueue(Message::createKEYR(devId));
                        //send the message to oldData and wait for server KEYA
                        oldData.push_back(element);
                    }
                }
            }
        }
        //process old msg without session key
        std::vector<LoraPacket>::iterator historyIterator;
        historyIterator = oldData.begin();
        while (historyIterator != oldData.end()){
            if (APP_DEBUG){
                std::cout << "processing chached message" << std::endl;
            }
            std::string devId = Message::toBase64(historyIterator->devId,DEV_ID_SIZE);
            if (devicesTable.hasSessionKey(devId) && devicesTable.isMine(devId)){
                uint16_t seqNum;
                out = Message::createRXL(devId, *historyIterator, devicesTable.getSessionKey(devId), seqNum,
                                         this->devicesTable.remainingDutyCycle(devId));
                if (out.micOk){
                    devicesTable.setSeq(devId,seqNum);
                    devicesTable.setSessionKeyCheck(devId,true);
                    outVector.push_back(out);
                    historyIterator = oldData.erase(historyIterator);
                }
                else {
                    if (APP_DEBUG){
                        std::cout << "BAD MIC checksum" << std::endl;
                    }
                    ++historyIterator;
                }
            }
            else {
                ++historyIterator;
            }
        }
        //send all messages
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
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::chrono::seconds currentTime = std::chrono::duration_cast< std::chrono::seconds >
                (std::chrono::system_clock::now().time_since_epoch());

        if (GATEWAY_OFFLINE){
            this->timerResponseMutex.lock();
            if (timerRegResponse > 0){
                timerRegResponse--;
                this->timerResponseMutex.unlock();
                std::cout << "Waiting before sending" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                this->addToQueue(Message::fromFile("rega.json"));
            }
            else {
                this->timerResponseMutex.unlock();
            }
        }

        if (oneTime){
            oneTime = false;
        }
        if (!GATEWAY_OFFLINE){
            if (!this->connection->connected && currentTime.count()-startTime.count()>CONNECTION_TIMEOUT){
                std::cerr << "Connection timeout" << std::endl;
                raise(SIGINT);
            }
        }
        devicesTable.updateByTimer(currentTime);
        guard.lock();
    }
    guard.unlock();
}
