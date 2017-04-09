//
// Created by root on 8.4.2017.
//

#include <csignal>
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
        if (this->getFromStiotData(in)){
            //todo tu bude viac checkou
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
            else {
                //std::cout <<in.toStiot() << std::endl;
            }
        }
//        LoraPacket out = Message::fromStiot(in);
//        concentrator->addToQueue(out);
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
        Message out;
        std::vector<Message> outVector;
        if (this->fromLoraData.empty()){
            guardData.unlock();
            if (this->getFromLoraData(in)){
                std::cout << "jeden ";
                out = Message::fromLora(in);
                outVector.push_back(out);
            }
            else {
                return;
            }
            guardData.lock();
        }
        //buffer MAX 5 messages
        while (!this->fromLoraData.empty() && outVector.size() < MAX_RXL){
            guardData.unlock();
            std::cout << "mnoho ";
            if (this->getFromLoraData(in)){
                out = Message::fromLora(in);
                outVector.push_back(out);
                guardData.lock();
            }
        }
        std::cout << "koniec whilu"<< std::endl;
        guardData.unlock();
        //todo a toto poslat dalej
        connection->addBulk(outVector);
        guard.lock();
    }
    guard.unlock();
}

void MessageConverter::timerFunction() {
    std::chrono::milliseconds startTime = std::chrono::duration_cast< std::chrono::milliseconds >
            (std::chrono::system_clock::now().time_since_epoch());
    std::unique_lock<std::mutex> guard(this->timerMutex);
    while (this->timerRun){
        guard.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::chrono::milliseconds currentTime = std::chrono::duration_cast< std::chrono::milliseconds >
                (std::chrono::system_clock::now().time_since_epoch());
        std::cout << "STATUS TIMER" << std::endl;





        if (!this->connection->connected && currentTime.count()-startTime.count()>CONNECTION_TIMEOUT){
            std::cerr << "Connection timeout" << std::endl;
            //raise(SIGINT);
        }
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
