//
// Created by root on 11.3.2017.
//

#include <iostream>
#include <fstream>
#include <csignal>
#include "ConcentratorController.h"

ConcentratorController::ConcentratorController(Message config) {
    nlohmann::json confSection = config.message.at("conf");
    this->localConfig = confSection;
    memset(&boardconf, 0, sizeof boardconf);
    this->boardconf.clksrc = confSection["clksrc"];
    this->boardconf.lorawan_public = confSection["lorawan_public"];
    this->ifChainCount = confSection["if_chain"];
    //TODO rezim, ci to je osobitny EMERGENCY,alebo nie, ak ano tak sa budu inak odosielat odpovede...
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

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::ifstream input("seta.json");
    std::stringstream sstr;
    while(input >> sstr.rdbuf());
    std::string seta = sstr.str();
    this->addToQueue(Message::fromStiot(seta));

    return 0;
}

int ConcentratorController::startConcentrator(Message param) {
    //SET board config
    std::cout << "Board config settings" << std::endl;
//    if (lgw_board_setconf(boardconf) != LGW_HAL_SUCCESS) {
//        std::cerr << "WARNING: Failed to configure board" << std::endl;
//        return -1;
//    }
    //SET tx gains
    std::cout << "TX gains settings" << std::endl;
//    if (lgw_txgain_setconf(&txlut) != LGW_HAL_SUCCESS) {
//        std::cerr << "WARNING: Failed to configure concentrator TX Gain LUT" << std::endl;
//        return -1;
//    }
    //GENERATE rxif and rfconf
    nlohmann::json params = param.getData().at("params");
    std::map<int32_t,lgw_conf_rxif_s> freqsMap;
    int multiSFCount = 0;
    for (auto& param1 : params) {
        if (param1.at("type").get<std::string>()=="NORMAL"){
            this->devicesTable.normalConfig = param1;
        }
        if (param1.at("type").get<std::string>()=="EMER"){
            this->devicesTable.emerConfig = param1;
        }
        if (param1.at("type").get<std::string>()=="REG"){
            this->devicesTable.regConfig = param1;
        }
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
//        if (lgw_rxrf_setconf(i, rfconf[i]) != LGW_HAL_SUCCESS) {
//            std::cerr << "WARNING: invalid configuration for radio " << i << std::endl;
//            return -1;
//        }
    }
    counter = 0;
    uint8_t index = 0;
    uint8_t i = 0;
    for(auto const &ent1 : freqsMap) {
        counter++;
        freqsMap[ent1.first].rf_chain = index;
        freqsMap[ent1.first].freq_hz -= rfconf[index].freq_hz;
        if (counter==ifChainCount){
            index++;
            counter = 0;
        }
//        if (lgw_rxif_setconf(i, freqsMap[ent1.first]) != LGW_HAL_SUCCESS) {
//            std::cerr << "WARNING: invalid configuration for Lora channel" << std::endl;
//            return -1;
//        }
        i++;
        std::cout << "frekvencia " << unsigned(i) << std::endl;
        std::cout << unsigned(ent1.second.rf_chain) << std::endl;
        std::cout << ent1.second.freq_hz << std::endl;
        std::cout << unsigned(ent1.second.bandwidth) <<std::endl;
    }
//    i = lgw_start();
//    if (i == LGW_HAL_SUCCESS) {
//        std::cout << "INFO: [main] concentrator started, packet can now be received" << std::endl;
//        this->fiberReceive = std::thread(&ConcentratorController::receive,this);
//    } else {
//        std::cout << "ERROR: [main] failed to start the concentrator" << std::endl;
//        return -1;
//    }
    this->fiberReceive = std::thread(&ConcentratorController::receive,this);
    return 0;
}

void ConcentratorController::join() {
    this->fiberReceive.join();
    this->fiberSend.join();
//    int stopStatus = lgw_stop();
//    if (stopStatus == LGW_HAL_SUCCESS) {
//        std::cout << "INFO: concentrator stopped successfully" << std::endl;
//    } else {
//        std::cerr << "WARNING: failed to stop concentrator successfully" << std::endl;
//    }
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
    int halStatus;

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
                //TODO second SETA message after starting concentrator
                halStatus = startConcentrator(msg);
                if (halStatus<0){
                    break;
                }
            }
            else {
                std::cout << "Some data" << std::endl;
                std::cout << msg.message.dump(1) << std::endl;
            }
        }
        if (halStatus<0){
            break;
        }
        sendMutex.lock();
    }
    if (halStatus<0){
        raise(SIGINT);
    }
    sendMutex.unlock();

}

void ConcentratorController::receive() {
    int halStatus;
    receiveMutex.lock();
    while (receiveRun){
        receiveMutex.unlock();
        wait_ms(3000);
        std::cout << "Receiving 3 sek" << std::endl;

//        int packetsCount;
//        struct lgw_pkt_rx_s rxpkt[NB_PKT_MAX];
//        packetsCount = lgw_receive(NB_PKT_MAX, rxpkt);
//        if (packetsCount == LGW_HAL_ERROR) {
//            std::cerr << "ERROR: failed packet fetch, exiting" << std::endl;
//            halStatus = -1;
//            break;
//        } else if (packetsCount == 0) {
//            wait_ms(fetchSleepMs);
//            continue;
//        }
//        std::cout << "NEW DATA:" << std::endl;
//        for (int i=0; i < packetsCount; ++i) {
//            std::cout << "packet " << i << "from " << packetsCount << std::endl;
//            std::cout << rxpkt[i].freq_hz << std::endl;
//            std::cout << std::fixed << std::setprecision(3) << rxpkt[i].rssi << std::endl;
//            std::cout << std::fixed << std::setprecision(3) << rxpkt[i].snr << std::endl;
//            switch(rxpkt[i].status) {
//                case STAT_CRC_OK:
//                    std::cout << "CRC OK" << std::endl;
//                    break;
//                case STAT_CRC_BAD:
//                    std::cout << "CRC BAD" << std::endl;
//                    break;
//                case STAT_NO_CRC:
//                    std::cout << "NO CRC" << std::endl;
//                    break;
//                case STAT_UNDEFINED:
//                    break;
//                default: std::cout << "DEFAULT" << std::endl;
//            }
//        }
        receiveMutex.lock();
    }
    receiveMutex.unlock();
    if (halStatus<0){
        raise(SIGINT);
    }
}

void ConcentratorController::addToQueue(Message message) {
    std::unique_lock<std::mutex> guard(this->queueMutex);
    this->serverData.push(message);
    sendConditional.notify_one();
}

