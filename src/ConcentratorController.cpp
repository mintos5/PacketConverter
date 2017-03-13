//
// Created by root on 11.3.2017.
//

#include <iostream>
#include <fstream>
#include "ConcentratorController.h"

ConcentratorController::ConcentratorController(Message config) {
    nlohmann::json confSection = config.message.at("conf");
    this->localConfig = confSection;
    memset(&boardconf, 0, sizeof boardconf);
    this->boardconf.clksrc = confSection["clksrc"];
    this->boardconf.lorawan_public = confSection["lorawan_public"];

    nlohmann::json txArray = config.message.at("tx_luts");
    memset(&txlut, 0, sizeof txlut);
    int i = 0;
    for (auto& element : txArray) {
        //TODO checking if exists...
        txlut.lut[i].pa_gain = element.at("pa_gain");
        txlut.lut[i].dac_gain = 3;
        txlut.lut[i].dig_gain = element.at("dig_gain");
        txlut.lut[i].mix_gain = element.at("mix_gain");
        txlut.lut[i].rf_power = element.at("rf_power");
        i++;
    }
    txlut.size = i;
}

void ConcentratorController::setConnection(const std::shared_ptr<ConnectionController> &connection) {
    ConcentratorController::connection = connection;
}

int ConcentratorController::start() {
    //this->fiberReceive = std::thread(&ConcentratorController::receive,this);
    this->fiberSend = std::thread(&ConcentratorController::send,this);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::ifstream input("seta.json");
    std::stringstream sstr;
    while(input >> sstr.rdbuf());
    std::string seta = sstr.str();
    this->addToQueue(Message::fromStiot(seta));

    return 0;
}

void ConcentratorController::join() {
    //this->fiberReceive.join();
    this->fiberSend.join();
}

void ConcentratorController::stop() {
    std::lock_guard<std::mutex> guard1(this->sendMutex);
    std::lock_guard<std::mutex> guard2(this->receiveMutex);
    this->receiveRun = false;
    this->sendRun = false;
    std::unique_lock<std::mutex> guardQueue(this->queueMutex);
    sendConditional.notify_one();
}

void ConcentratorController::send() {
    sendMutex.lock();
    while (sendRun){
        sendMutex.unlock();

        std::unique_lock<std::mutex> guard(this->queueMutex);
        sendConditional.wait(guard);
        std::cout << "conditionVariable.unlocked" << std::endl;
        while(!serverData.empty()){
            Message msg = serverData.front();
            serverData.pop();
            if (msg.type == SETA){
                startConcentrator(msg);
            }
            else {
                std::cout << "Some data" << std::endl;
                std::cout << msg.message.dump(1) << std::endl;
            }
        }

        sendMutex.lock();
    }
    sendMutex.unlock();

}

int ConcentratorController::startConcentrator(Message param) {
    nlohmann::json params = param.getData().at("params");
    int i;
    for (auto& element : params) {
        std::cout << element.dump(2) << std::endl;
        i++;
    }
//    for (nlohmann::json::iterator it = params.begin(); it != params.end(); ++it) {
//        std::cout << it.key() << " : " << it.value() << "\n";
//    }
    return 0;
}

void ConcentratorController::receive() {

    receiveMutex.lock();
    while (receiveRun){
        receiveMutex.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "Receiving 5 sek" << std::endl;


        receiveMutex.lock();
    }
    receiveMutex.unlock();
}

void ConcentratorController::addToQueue(Message message) {
    std::unique_lock<std::mutex> guard(this->queueMutex);
    this->serverData.push(message);
    sendConditional.notify_one();
}

