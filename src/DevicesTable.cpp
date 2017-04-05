//
// Created by root on 11.3.2017.
//

#include "DevicesTable.h"


lgw_pkt_tx_s DevicesTable::setPacket(std::string deviceId) {
    struct lgw_pkt_tx_s txpkt;
    memset(&txpkt, 0, sizeof(txpkt));
    //TODO from table get information
    txpkt.freq_hz = 866100000;
    txpkt.tx_mode = IMMEDIATE;
    txpkt.rf_chain = 0;
    txpkt.rf_power = 14;
    txpkt.modulation = MOD_LORA;
    txpkt.bandwidth = BW_125KHZ;
    txpkt.datarate = DR_LORA_SF9;
    txpkt.coderate = CR_LORA_4_5;
    txpkt.invert_pol = true;
    txpkt.preamble = 8;
    return txpkt;
}
