//
// Created by root on 8.4.2017.
//

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
    this->fromLoraCond.notify_one();

    std::unique_lock<std::mutex> guard2(this->fromStiotMutex);
    this->fromStiotRun = false;
    guard2.unlock();
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

void MessageConverter::setConnection(const std::shared_ptr<ConnectionController> &connection) {
    MessageConverter::connection = connection;
}

void MessageConverter::setConcentrator(const std::shared_ptr<ConcentratorController> &concentrator) {
    MessageConverter::concentrator = concentrator;
}

Message MessageConverter::getFromStiotData() {
    std::unique_lock<std::mutex> guard(this->stiotDataMutex);
    while (this->fromStiotData.empty()){
        this->fromStiotCond.wait(guard);
    }
    Message out = this->fromStiotData.front();
    this->fromStiotData.pop();
    return out;
}

LoraPacket MessageConverter::getFromLoraData() {
    std::unique_lock<std::mutex> guard(this->loraDataMutex);
    while (this->fromLoraData.empty()){
        this->fromLoraCond.wait(guard);
    }
    LoraPacket out = this->fromLoraData.front();
    this->fromLoraData.pop();
    return out;
}

void MessageConverter::fromStiot() {
    std::unique_lock<std::mutex> guard(this->fromStiotMutex);
    while (this->fromStiotRun){
        guard.unlock();
        Message in = this->getFromStiotData();
        //todo tu bude viac checkou
        LoraPacket out = Message::fromStiot(in);
        concentrator->addToQueue(out);
        guard.lock();
    }
    guard.unlock();

}

void MessageConverter::fromLora() {
    std::unique_lock<std::mutex> guard(this->fromLoraMutex);
    while (this->fromLoraRun){
        guard.unlock();
        LoraPacket in = this->getFromLoraData();
        Message out = Message::fromLora(in);
        //todo a toto poslat dalej
        connection->addToQueue(out);
        guard.lock();
    }
    guard.unlock();
}

void MessageConverter::timerFunction() {
    std::unique_lock<std::mutex> guard(this->timerMutex);
    while (this->timerRun){
        guard.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::cout << "STATUS TIMER" << std::endl;

        guard.lock();
    }
    guard.unlock();

}
