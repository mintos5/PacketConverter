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
#define LORAFIIT_TYPE_MASK 0xe0
#define LORAFIIT_ACK_MASK 0x07

#define LORAFIIT_REG_UP 0x00
#define LORAFIIT_DATA_UP 0x20
#define LORAFIIT_HELLO_UP 0x40
#define LORAFIIT_EMER_UP 0x60
#define LORAFIIT_REG_DOWN 0x80
#define LORAFIIT_DATA_DOWN 0xA0

#define LORAFIIT_NO_ACK 0x00
#define LORAFIIT_OPT_ACK 0x02
#define LORAFIIT_MAN_ACK 0x06


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
    ConcentratorController(const std::shared_ptr<MessageConverter> &converter,Message config);
    int start();
    int startOffline();
    void join();
    void stop();
    int startConcentrator(Message param);
    void addToQueue(LoraPacket message);
};


#endif //PACKETCONVERTER_CONCENTRATORCONTROLLER_H
