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
#include "DevicesTable.h"
#include <condition_variable>
extern "C" {
#include "loragw_hal.h"
#include "loragw_aux.h"
}
#define NB_PKT_MAX		8 /* max number of packets per fetch/send cycle */


class ConnectionController;

class ConcentratorController {
    int ifChainCount;
    nlohmann::json localConfig;
    struct lgw_conf_board_s boardconf;
    struct lgw_tx_gain_lut_s txlut;
    const int fetchSleepMs = 10;

    std::shared_ptr<ConnectionController> connection;

    bool sendRun = true;
    std::mutex sendMutex;
    bool receiveRun = false;
    std::mutex receiveMutex;

    std::thread fiberReceive;
    std::thread fiberSend;

    std::condition_variable sendConditional;
    std::queue<Message> serverData;
    std::mutex queueMutex;

    DevicesTable devicesTable;

    void processStiot();
    void receiveHal();
    int startConcentrator(Message param);
    int sendHal(Message msg);

public:

    int start();
    void join();
    void stop();
    void addToQueue(Message message);

    ConcentratorController(Message config);
    void setConnection(const std::shared_ptr<ConnectionController> &connection);


};


#endif //PACKETCONVERTER_CONCENTRATORCONTROLLER_H
