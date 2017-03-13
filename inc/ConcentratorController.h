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
#include <loragw_hal.h>
#include <condition_variable>


class ConnectionController;

class ConcentratorController {
    nlohmann::json localConfig;
    struct lgw_conf_board_s boardconf;
    struct lgw_conf_rxrf_s rfconf;
    struct lgw_conf_rxif_s ifconf;
    struct lgw_tx_gain_lut_s txlut;

    std::shared_ptr<ConnectionController> connection;

    bool sendRun = true;
    std::mutex sendMutex;
    bool receiveRun = true;
    std::mutex receiveMutex;

    void send();
    void receive();
    int startConcentrator(Message param);

public:
    std::thread fiberReceive;
    std::thread fiberSend;
    std::condition_variable sendConditional;
    std::queue<Message> serverData;
    std::mutex queueMutex;

    int start();
    void join();
    void stop();
    void call();
    void addToQueue(Message message);

    ConcentratorController(Message config);
    void setConnection(const std::shared_ptr<ConnectionController> &connection);


};


#endif //PACKETCONVERTER_CONCENTRATORCONTROLLER_H
