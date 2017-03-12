//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_CONCENTRATORCONTROLLER_H
#define PACKETCONVERTER_CONCENTRATORCONTROLLER_H


#include <cstdint>
#include <queue>
#include <thread>
#include <mutex>
#include "Message.h"


class ConnectionController;

class ConcentratorController {
    uint64_t gatewayId;

    std::shared_ptr<ConnectionController> connection;

    bool sendRun = true;
    std::mutex sendMutex;
    bool receiveRun = true;
    std::mutex receiveMutex;

    void send();
    void receive();

public:
    std::thread fiberReceive;
    std::thread fiberSend;
    std::queue<Message> serverData;
    std::mutex queueMutex;

    int start();
    void join();
    void stop();

    void call();
    void addToQueue();

    void setConnection(const std::shared_ptr<ConnectionController> &connection);


};


#endif //PACKETCONVERTER_CONCENTRATORCONTROLLER_H
