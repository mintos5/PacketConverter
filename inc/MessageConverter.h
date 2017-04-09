//
// Created by root on 8.4.2017.
//

#ifndef PACKETCONVERTER_MESSAGECONVERTOR_H
#define PACKETCONVERTER_MESSAGECONVERTOR_H
#define MAX_RXL 5
#define CONNECTION_TIMEOUT 10000

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "Message.h"
#include "DevicesTable.h"

class ConcentratorController;
class ConnectionController;

class MessageConverter {
    bool fromStiotRun = true;
    std::mutex fromStiotMutex;
    std::thread fromStiotFiber;
    std::condition_variable fromStiotCond;
    std::queue<Message> fromStiotData;
    std::mutex stiotDataMutex;

    bool fromLoraRun = true;
    std::mutex fromLoraMutex;
    std::thread fromLoraFiber;
    std::condition_variable fromLoraCond;
    std::queue<LoraPacket> fromLoraData;
    std::mutex loraDataMutex;

    bool timerRun = true;
    std::mutex timerMutex;
    std::thread timerFiber;

    bool getFromStiotData(Message &data);
    bool getFromLoraData(LoraPacket &data);

    void fromStiot();
    void fromLora();
    void timerFunction();

    std::shared_ptr<ConnectionController> connection;
    std::shared_ptr<ConcentratorController> concentrator;
    DevicesTable devicesTable;


public:
    int start();
    int startOffline();
    void stop();
    void join();
    void addToQueue(Message message);
    void addToQueue(LoraPacket message);
    void addBulk(std::vector<LoraPacket> vector);

    void setConnection(const std::shared_ptr<ConnectionController> &connection);
    void setConcentrator(const std::shared_ptr<ConcentratorController> &concentrator);

};


#endif //PACKETCONVERTER_MESSAGECONVERTOR_H
