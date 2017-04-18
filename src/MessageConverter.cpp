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
#define P 0xffffffffffffffc5ull

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
            std::cout << "new STIOT data:";
            if (in.type==SETA){
                std::cout << "SETA" << std::endl;
                //todo uncomment starting
                if (oneTime){
                    //need to turn off and turn on
                    concentrator->stop();
                    concentrator->join();
                    concentrator->start();
                    //concentrator->startConcentrator(in);
                }
                else {
                    //concentrator->startConcentrator(in);
                    oneTime = true;
                }
            }
            else if (in.type==KEYA){
                std::cout << "KEYA" << std::endl;
                if (devicesTable.isMine(in.devId)){
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
                std::cout << "REGA" << std::endl;
                if (devicesTable.isMine(in.devId)){
                    nlohmann::json data = in.getData();
                    if (data.find("sh_key") == data.end()){
                        //bad REGA STIOT message
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
                    //testing key vector
                    std::vector<uint8_t > key(16);
                    key[0] = 75;
                    key[1] = 75;
                    key[2] = 75;
                    key[3] = 75;
                    key[4] = 75;
                    key[5] = 75;
                    key[6] = 75;
                    key[7] = 75;
                    key[8] = 75;
                    key[9] = 75;
                    key[10] = 75;
                    key[11] = 75;
                    key[12] = 75;
                    key[13] = 75;
                    key[14] = 75;
                    key[15] = 75;
                    //set correct new session key and default (0) sequence Number
                    //todo start using session key
                    devicesTable.setSessionkey(in.devId, key.data(), 0);
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
                        //todo send error messages
                        std::cerr << "no enough time to air" << std::endl;
                    }
                }
            }
            else if (in.type==TXL){
                std::cout << "TXL" << std::endl;
                if (devicesTable.isMine(in.devId)){
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
                            //todo send error messages
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
                //todo handle errors, like KEYR(need to set isMine to false)
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
                std::cout << "NEW LORA data (IF)" << std::endl;
            }
        }
        while (!this->fromLoraData.empty()){
            std::cout << "NEW LORA data (while)" << std::endl;
            if (this->getFromLoraData(in,guardData)){
                inVector.push_back(in);
            }
        }

        std::cout << "END OF while in fromLora"<< std::endl;
        //process new LoRaFIIT msg
        for (auto& element: inVector){
            std::string devId = Message::toBase64(element.devId,DEV_ID_SIZE);
            if (element.type==REGISTER_UP){
                this->devicesTable.addDevice(devId, element, 0);
                connection->addToQueue(Message::createREGR(devId,element,this->devicesTable.remainingDutyCycle(devId)));
            }
            else if (element.type==DATA_UP || element.type==HELLO_UP || element.type == EMERGENCY_UP){
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
                }
                else {
                    if (devicesTable.isMine(devId)){
                        //message must be from mine end device
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
            std::string devId = Message::toBase64(historyIterator->devId,DEV_ID_SIZE);
            if (devicesTable.hasSessionKey(devId) && devicesTable.isMine(devId)){
                uint16_t seqNum;
                out = Message::createRXL(devId, *historyIterator, devicesTable.getSessionKey(devId), seqNum,
                                         this->devicesTable.remainingDutyCycle(devId));
                if (out.micOk){
                    devicesTable.setSeq(devId,seqNum);
                    devicesTable.setSessionKeyCheck(devId,true);
                    outVector.push_back(out);
                }
                historyIterator = oldData.erase(historyIterator);
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
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::chrono::seconds currentTime = std::chrono::duration_cast< std::chrono::seconds >
                (std::chrono::system_clock::now().time_since_epoch());
        std::cout << "STATUS TIMER" << std::endl;
        if (oneTime){
            //todo tests area

            std::vector<uint8_t > preKeyVector(DH_SESSION_KEY_SIZE+8);
            for (int i=0;i<3;i++){
                uint8_t *pointerPreKey = preKeyVector.data() + i*sizeof(uint64_t);
                uint64_t *pointer = (uint64_t *) pointerPreKey;
                *pointer = P;
                if (i==2){
                    *pointer = 2;
                }
            }

            std::vector<uint8_t > privateKeyA(DH_SESSION_KEY_SIZE);
            std::vector<uint8_t > publicKeyA(DH_SESSION_KEY_SIZE);
            std::vector<uint8_t > sessionKeyA(DH_SESSION_KEY_SIZE);
            DiffieHellman::getDHB(preKeyVector.data(),publicKeyA.data(),privateKeyA.data());

            std::vector<uint8_t > privateKeyB(DH_SESSION_KEY_SIZE);
            std::vector<uint8_t > publicKeyB(DH_SESSION_KEY_SIZE);
            std::vector<uint8_t > sessionKeyB(DH_SESSION_KEY_SIZE);

            DiffieHellman::getDHB(preKeyVector.data(),publicKeyB.data(),privateKeyB.data());
            //generate session on B side
            DiffieHellman::getSessionKey(preKeyVector.data(),publicKeyA.data(),privateKeyB.data(),sessionKeyB.data());
            //generate session on A side
            DiffieHellman::getSessionKey(preKeyVector.data(),publicKeyB.data(),privateKeyA.data(),sessionKeyA.data());

            //testing addition to table
            LoraPacket test1;
            test1.frequency = 868000000;
            test1.coderate = "4/5";
            test1.bandwidth = 500000;
            test1.datarate = 8;
            test1.rfChain = 1;
            test1.devId[0] = 65;
            test1.devId[1] = 65;
            test1.devId[2] = 65;
            test1.type = REGISTER_UP;
            test1.ack = MANDATORY_ACK;
            std::copy(publicKeyA.data(),publicKeyA.data()+DH_SESSION_KEY_SIZE,test1.payload);
            test1.size = 16;
            this->addToQueue(test1);



            this->addToQueue(Message::fromFile("tests/rega.json"));
            std::this_thread::sleep_for(std::chrono::seconds(10));



            std::vector<uint8_t > key(16);
            key[0] = 75;
            key[1] = 75;
            key[2] = 75;
            key[3] = 75;
            key[4] = 75;
            key[5] = 75;
            key[6] = 75;
            key[7] = 75;
            key[8] = 75;
            key[9] = 75;
            key[10] = 75;
            key[11] = 75;
            key[12] = 75;
            key[13] = 75;
            key[14] = 75;
            key[15] = 75;

            LoraPacket test2;
            test2.frequency = 868000000;
            test2.coderate = "4/5";
            test2.bandwidth = 500000;
            test2.datarate = 8;
            test2.rfChain = 1;
            test2.devId[0] = 65;
            test2.devId[1] = 65;
            test2.devId[2] = 65;
            test2.type = DATA_UP;
            test2.ack = MANDATORY_ACK;
            char dataTest2[] = "ALoRaWan12CD1234";
            dataTest2[0] = 9;
            uint32_t *mic = (uint32_t * ) & dataTest2[12];
            *mic = Message::createCheck((uint8_t *) dataTest2, 12);
            Encryption::encrypt((uint8_t *) dataTest2, 16, key.data());
            std::copy(dataTest2,dataTest2+16,test2.payload);
            test2.size = 16;
            this->addToQueue(test2);



//            std::this_thread::sleep_for(std::chrono::seconds(6));
//            this->addToQueue(Message::fromFile("tests/keya.json"));
//            concentrator->testFunc();
            this->addToQueue(Message::fromFile("tests/txl.json"));
            std::this_thread::sleep_for(std::chrono::seconds(2));
            concentrator->testFunc();


            oneTime = false;

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
