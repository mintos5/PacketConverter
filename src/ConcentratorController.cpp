//
// Created by root on 11.3.2017.
//

#include <iostream>
#include "ConcentratorController.h"

void ConcentratorController::setConnection(const std::shared_ptr<ConnectionController> &connection) {
    ConcentratorController::connection = connection;
}

int ConcentratorController::start() {
    this->fiberReceive = std::thread(&ConcentratorController::receive,this);
    this->fiberSend = std::thread(&ConcentratorController::send,this);
    return 0;
}

void ConcentratorController::join() {
    this->fiberReceive.join();
    this->fiberSend.join();

}

void ConcentratorController::stop() {
    std::lock_guard<std::mutex> guard1(this->sendMutex);
    std::lock_guard<std::mutex> guard2(this->receiveMutex);
    this->receiveRun = false;
    this->sendRun = false;
}

void ConcentratorController::send() {
    sendMutex.lock();
    while (sendRun){
        sendMutex.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "Sending 3 sek" << std::endl;
    }

}

void ConcentratorController::receive() {
    receiveMutex.lock();
    while (receiveRun){
        receiveMutex.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "Receiving 5 sek" << std::endl;
    }
}
