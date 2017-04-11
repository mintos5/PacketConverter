//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_CONCENTRATORCONTROLLER_H
#define PACKETCONVERTER_CONCENTRATORCONTROLLER_H


#include <cstdint>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "MessageConverter.h"
extern "C" {
#include "loragw_hal.h"
#include "loragw_aux.h"
}
#define NB_PKT_MAX		8 /* max number of packets per fetch/send cycle */


class ConcentratorController {
    int ifChainCount;
    nlohmann::json localConfig;
    struct lgw_conf_board_s boardconf;
    struct lgw_tx_gain_lut_s txlut;
    const int fetchSleepMs = 10;

    std::shared_ptr<MessageConverter> converter;

    bool sendRun = false;
    std::mutex sendMutex;
    std::thread fiberSend;

    bool receiveRun = false;
    std::mutex receiveMutex;
    std::thread fiberReceive;

    std::condition_variable sendConditional;
    std::queue<LoraPacket> serverData;
    std::mutex queueMutex;

    void processStiot();
    void receiveHal();
    int sendHal(LoraPacket msg);
    struct lgw_pkt_tx_s toHal(LoraPacket msg);
    LoraPacket fromHal(struct lgw_pkt_rx_s msg);

public:

    int start();
    int startOffline();
    int sendRawTest(std::string);
    void join();
    void stop();
    int startConcentrator(Message param);
    void addToQueue(LoraPacket message);

    ConcentratorController(const std::shared_ptr<MessageConverter> &converter,Message config);


};


#endif //PACKETCONVERTER_CONCENTRATORCONTROLLER_H
