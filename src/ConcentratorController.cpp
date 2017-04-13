//
// Created by root on 11.3.2017.
//

#include <iostream>
#include <fstream>
#include <csignal>
#include "ConcentratorController.h"

ConcentratorController::ConcentratorController(const std::shared_ptr<MessageConverter> &converter,Message config)
        : converter(converter) {
    nlohmann::json confSection = config.message.at("conf");
    this->localConfig = confSection;
    memset(&boardconf, 0, sizeof boardconf);
    this->boardconf.clksrc = confSection["clksrc"];
    this->boardconf.lorawan_public = confSection["lorawan_public"];
    this->ifChainCount = confSection["if_chain"];
    nlohmann::json txArray = config.message.at("tx_luts");
    memset(&txlut, 0, sizeof txlut);
    int i = 0;
    for (auto& element : txArray) {
        txlut.lut[i].pa_gain = element.at("pa_gain");
        txlut.lut[i].dac_gain = 3;
        txlut.lut[i].dig_gain = element.at("dig_gain");
        txlut.lut[i].mix_gain = element.at("mix_gain");
        txlut.lut[i].rf_power = element.at("rf_power");
        i++;
    }
    txlut.size = i;
}

int ConcentratorController::start() {
    return 0;
}

int ConcentratorController::startOffline() {
    std::ifstream input("seta.json");
    std::stringstream sstr;
    while(input >> sstr.rdbuf());
    std::string seta = sstr.str();
    return this->startConcentrator(Message::fromJsonString(seta));
}

int ConcentratorController::startConcentrator(Message param) {
    //SET board config
    std::cout << "Board config settings" << std::endl;
    if (lgw_board_setconf(boardconf) != LGW_HAL_SUCCESS) {
        std::cerr << "WARNING: Failed to configure board" << std::endl;
        return -1;
    }
    //SET tx gains
    std::cout << "TX gains settings" << std::endl;
    if (lgw_txgain_setconf(&txlut) != LGW_HAL_SUCCESS) {
        std::cerr << "WARNING: Failed to configure concentrator TX Gain LUT" << std::endl;
        return -1;
    }
    //GENERATE rxif and rfconf
    nlohmann::json params = param.getData().at("params");
    std::map<int32_t,lgw_conf_rxif_s> freqsMap;
    int multiSFCount = 0;
    for (auto& param1 : params) {
        for (auto& freq : param1.at("freqs")) {
            struct lgw_conf_rxif_s temp1;
            memset(&temp1, 0, sizeof temp1);
            temp1.freq_hz = freq;
            if (param1.at("sf")==0){
                if (multiSFCount < LGW_MULTI_NB){
                    temp1.enable = true;
                    freqsMap[freq] = temp1;
                    multiSFCount++;
                }
                else {
                    std::cerr << "Maximum of multi SF ..." << std::endl;
                }
            }
            else {
                uint32_t bw =  param1.at("band");
                switch(bw) {
                    case 500000: temp1.bandwidth = BW_500KHZ; break;
                    case 250000: temp1.bandwidth = BW_250KHZ; break;
                    case 125000: temp1.bandwidth = BW_125KHZ; break;
                    default: temp1.bandwidth = BW_UNDEFINED;
                }
                uint32_t sf = param1.at("sf");
                switch(sf) {
                    case  7: temp1.datarate = DR_LORA_SF7;  break;
                    case  8: temp1.datarate = DR_LORA_SF8;  break;
                    case  9: temp1.datarate = DR_LORA_SF9;  break;
                    case 10: temp1.datarate = DR_LORA_SF10; break;
                    case 11: temp1.datarate = DR_LORA_SF11; break;
                    case 12: temp1.datarate = DR_LORA_SF12; break;
                    default: temp1.datarate = DR_UNDEFINED;
                }
                temp1.enable = true;
                freqsMap[freq] = temp1;
            }
        }
    }

    //find stredne frekvencie
    int counter = 0;
    int rfCounter = 0;
    std::vector<int32_t> allFreq;
    std::vector<int> position;
    for(auto const &ent1 : freqsMap) {
        counter++;
        allFreq.push_back(ent1.second.freq_hz);
        //std::cout << ent1.second.freq_hz << std::endl;
        if (counter==ifChainCount){
            //position starting from 1
            position.push_back(ceil(ifChainCount/2.0)+(rfCounter*ifChainCount));
            counter = 0;
            rfCounter++;
        }
    }
    if (counter>1){
        //position starting from 1
        position.push_back(ceil(counter/2.0)+(rfCounter*ifChainCount));
        rfCounter++;
    }

    //generate rfconf and set it
    struct lgw_conf_rxrf_s rfconf[LGW_RF_CHAIN_NB];
    for (int i= 0;i<LGW_RF_CHAIN_NB;i++){
        memset(&(rfconf[i]), 0, sizeof rfconf[i]);
        if (i<rfCounter){
            rfconf[i].enable = true;
            std::stringstream ss;
            ss << "radio_" << i;
            rfconf[i].tx_enable = localConfig.at(ss.str()).at("tx_enable");
            rfconf[i].rssi_offset = localConfig.at("rssi_offset");
            if(localConfig.at(ss.str()).at("type")=="SX1257"){
                rfconf[i].type = LGW_RADIO_TYPE_SX1257;
            }
            else if(localConfig.at(ss.str()).at("type")=="SX1255"){
                rfconf[i].type = LGW_RADIO_TYPE_SX1255;
            }
            else {
                std::cerr << "Bad radio type" << std::endl;
                return -1;
            }
            rfconf[i].freq_hz = allFreq[position[i]-1];
        }
        else {
            rfconf[i].enable = false;
        }
        if (lgw_rxrf_setconf(i, rfconf[i]) != LGW_HAL_SUCCESS) {
            std::cerr << "WARNING: invalid configuration for radio " << i << std::endl;
            //return -1;
        }
    }
    //set IFs
    counter = 0;
    uint8_t index = 0;
    uint8_t i = 0;
    bool oneTime = true;
    for(auto const &ent1 : freqsMap) {
        counter++;
        //set correct RF index and frequency difference
        freqsMap[ent1.first].rf_chain = index;
        freqsMap[ent1.first].freq_hz -= rfconf[index].freq_hz;
        if (counter==ifChainCount){
            index++;
            counter = 0;
        }
        //LORA STD channel
        if (freqsMap[ent1.first].bandwidth != BW_UNDEFINED && oneTime){
            if (lgw_rxif_setconf(8, freqsMap[ent1.first]) != LGW_HAL_SUCCESS) {
                std::cerr << "WARNING: invalid configuration for Lora channel" << std::endl;
                return -1;
            }
            oneTime = false;
        }
        else if (lgw_rxif_setconf(i, freqsMap[ent1.first]) != LGW_HAL_SUCCESS) {
            std::cerr << "WARNING: invalid configuration for Lora channel" << std::endl;
            return -1;
        } else{
            i++;
        }
    }

    i = lgw_start();
    if (i == LGW_HAL_SUCCESS) {
        std::cout << "INFO: [main] concentrator started, packet can now be received" << std::endl;
        this->receiveRun = true;
        this->fiberReceive = std::thread(&ConcentratorController::receiveHal,this);
        this->sendRun = true;
        this->fiberSend = std::thread(&ConcentratorController::processStiot,this);
    } else {
        std::cout << "ERROR: [main] failed to start the concentrator" << std::endl;
        return -1;
    }
    return 0;
}

void ConcentratorController::join() {
    this->receiveMutex.lock();
    if (this->receiveRun){
        this->receiveMutex.unlock();
        this->fiberReceive.join();
    }else {
        this->receiveMutex.unlock();
    }

    this->sendMutex.lock();
    if (this->sendRun){
        this->sendMutex.unlock();
        this->fiberSend.join();
    }
    else {
        this->sendMutex.unlock();
    }

    int stopStatus = lgw_stop();
    if (stopStatus == LGW_HAL_SUCCESS) {
        std::cout << "INFO: concentrator stopped successfully" << std::endl;
    } else {
        std::cerr << "WARNING: failed to stop concentrator successfully" << std::endl;
    }
}

void ConcentratorController::stop() {
    std::lock_guard<std::mutex> guard1(this->sendMutex);
    std::lock_guard<std::mutex> guard2(this->receiveMutex);
    this->receiveRun = false;
    this->sendRun = false;
    std::unique_lock<std::mutex> guardQueue(this->queueMutex);
    guardQueue.unlock();
    sendConditional.notify_one();
}

void ConcentratorController::receiveHal() {
    std::cerr << "Starting receiveHal thread" << std::endl;
    int halStatus;
    receiveMutex.lock();
    while (receiveRun){
        receiveMutex.unlock();

        int packetsCount;
        struct lgw_pkt_rx_s rxpkt[NB_PKT_MAX];
        packetsCount = lgw_receive(NB_PKT_MAX, rxpkt);
        if (packetsCount == LGW_HAL_ERROR) {
            std::cerr << "ERROR: failed packet fetch, exiting" << std::endl;
            halStatus = -1;
            break;
        } else if (packetsCount == 0) {
            wait_ms(fetchSleepMs);
            continue;
        }
        std::cout << "NEW DATA:" << std::endl;
        std::vector<LoraPacket> vector;
        for (int i=0; i < packetsCount; ++i) {
            if (rxpkt[i].status == STAT_CRC_OK){
                vector.push_back(this->fromHal(rxpkt[i]));
            }
        }
        converter->addBulk(vector);
        receiveMutex.lock();
    }
    receiveMutex.unlock();
    if (halStatus<0){
        raise(SIGINT);
    }
}

void ConcentratorController::processStiot() {
    std::unique_lock<std::mutex> guard(this->queueMutex);
    sendMutex.lock();
    while (sendRun){
        sendMutex.unlock();
        //here I can send testings
        if (serverData.empty()){
            sendConditional.wait(guard);
            std::cout << "conditionVariable.unlocked" << std::endl;
        }
        while(!serverData.empty()){
            LoraPacket msg = serverData.front();
            serverData.pop();
            this->sendHal(msg);
        }
        sendMutex.lock();
    }
    sendMutex.unlock();
}

int ConcentratorController::sendHal(LoraPacket msg) {
    uint8_t status_var;
    do {
        lgw_status(TX_STATUS, &status_var); /* get TX status */
        if (status_var != TX_FREE){
            wait_ms(5);
        }
    } while (status_var != TX_FREE);
    std::cout << "Ok" << std::endl;
    lgw_pkt_tx_s out = toHal(msg);
    if (out.size==0){
        std::cerr << "empty message or error" << std::endl;
        return -1;
    }
    int i = lgw_send(out); /* non-blocking scheduling of TX packet */
    if (i != LGW_HAL_SUCCESS) {
        std::cerr << "error" << std::endl;
        return -1;
    }
    return 0;
}

void ConcentratorController::addToQueue(LoraPacket message) {
    std::unique_lock<std::mutex> guard(this->queueMutex);
    this->serverData.push(message);
    guard.unlock();
    sendConditional.notify_one();
}

int ConcentratorController::sendRawTest(std::string) {
    //GETTING SENDING PARAMETERS
    struct lgw_pkt_tx_s txpkt = DevicesTable::setTestParams();
    //GETTING DATA
    char message[] = "ping pong";
    std::copy(message,message+8,txpkt.payload);
    txpkt.size = 8;
    uint8_t status_var;
    do {
        lgw_status(TX_STATUS, &status_var); /* get TX status */
        if (status_var != TX_FREE){
            wait_ms(5);
        }
    } while (status_var != TX_FREE);
    printf("OK\n");


    int i = lgw_send(txpkt); /* non-blocking scheduling of TX packet */
    if (i != LGW_HAL_SUCCESS) {
        printf("ERROR\n");
        return -1;
    }
    return 0;
}

LoraPacket ConcentratorController::fromHal(struct lgw_pkt_rx_s msg) {
    LoraPacket out;
    //todo set type and devId
    std::copy(msg.payload,msg.payload + (msg.size),out.payload);
    out.size = msg.size;
    out.frequency = msg.freq_hz;
    out.rssi = msg.rssi;
    out.snr = msg.snr;
    out.rfChain = msg.rf_chain;
    out.time = std::chrono::duration_cast< std::chrono::milliseconds >
            (std::chrono::system_clock::now().time_since_epoch());
    switch(msg.bandwidth) {
        case BW_500KHZ:
            out.bandwidth = 500;
            break;
        case BW_250KHZ:
            out.bandwidth = 250;
            break;
        case BW_125KHZ:
            out.bandwidth = 125;
            break;
        case BW_62K5HZ:
            out.bandwidth = 625;
            break;
        case BW_31K2HZ:
            out.bandwidth = 312;
            break;
        case BW_15K6HZ:
            out.bandwidth = 156;
            break;
        case BW_7K8HZ:
            out.bandwidth = 78;
            break;
        case BW_UNDEFINED:
            out.bandwidth = 000;
            break;
        default:
            out.bandwidth = 000;
    }

    switch (msg.datarate) {
        case DR_LORA_SF7:
            out.datarate = 7;
            break;
        case DR_LORA_SF8:
            out.datarate = 8;
            break;
        case DR_LORA_SF9:
            out.datarate = 9;
            break;
        case DR_LORA_SF10:
            out.datarate = 10;
            break;
        case DR_LORA_SF11:
            out.datarate = 11;
            break;
        case DR_LORA_SF12:
            out.datarate = 12;
            break;
        default:
            out.datarate = 0;
    }
    switch (msg.coderate) {
        case CR_LORA_4_5:
            out.coderate = "4/5";
            break;
        case CR_LORA_4_6:
            out.coderate = "4/6";
            break;
        case CR_LORA_4_7:
            out.coderate = "4/7";
            break;
        case CR_LORA_4_8:
            out.coderate = "4/8";
            break;
        case CR_UNDEFINED:
            out.coderate = "UND";
            break;
        default: out.coderate = "UND";
    }
    return out;
}

struct lgw_pkt_tx_s ConcentratorController::toHal(LoraPacket msg) {
    lgw_pkt_tx_s out;
    memset(&out, 0, sizeof(out));
    //special settings
    switch (msg.bandwidth) {
        case 125: out.bandwidth = BW_125KHZ; break;
        case 250: out.bandwidth = BW_250KHZ; break;
        case 500: out.bandwidth = BW_500KHZ; break;
        default:
            std::cerr << "BAD BW" << std::endl;
            return out;
    }
    switch (msg.datarate) {
        case  7: out.datarate = DR_LORA_SF7;  break;
        case  8: out.datarate = DR_LORA_SF8;  break;
        case  9: out.datarate = DR_LORA_SF9;  break;
        case 10: out.datarate = DR_LORA_SF10; break;
        case 11: out.datarate = DR_LORA_SF11; break;
        case 12: out.datarate = DR_LORA_SF12; break;
        default:
            std::cerr << "BAD SF" << std::endl;
            return out;
    }

    if (msg.coderate=="4/5"){
        out.coderate = CR_LORA_4_5;
    }
    else if (msg.coderate=="4/6"){
        out.coderate = CR_LORA_4_6;
    }
    else if (msg.coderate=="4/7"){
        out.coderate = CR_LORA_4_7;
    }
    else if (msg.coderate=="4/8"){
        out.coderate = CR_LORA_4_8;
    }
    else {
        std::cerr << "BAD CR" << std::endl;
        return out;
    }

    //solid settings
    out.tx_mode = IMMEDIATE;
    out.modulation = MOD_LORA;
    out.invert_pol = false;
    out.preamble = 8;
    //easy settings
    out.size = msg.size;
    out.freq_hz = msg.frequency;
    out.rf_power = msg.rfPower;
    //todo write first bytes devID and type
    std::copy(msg.payload,msg.payload + (msg.size),out.payload);
    return out;
}

