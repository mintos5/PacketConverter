//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_CONNECTIONCONTROLLER_H
#define PACKETCONVERTER_CONNECTIONCONTROLLER_H

#include <time.h>
#include <openssl/ssl.h>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include "Message.h"



class ConcentratorController;

class ConnectionController {
    int socket;
    BIO *bio;
    SSL *ssl;
    SSL_CTX *ctx;
    const std::string caCert = "/etc/ssl/certs/ca-certificates.crt";
    const std::string hostname = "localhost:12345";
    const int buffSize = 2048;

    std::shared_ptr<ConcentratorController> concentrator;

    int sendNum = 0;

    void process();
    void send();

    bool processRun = true;
    std::mutex processMutex;

public:
    std::thread fiberProcess;
    std::queue<Message> endDeviceData;
    std::mutex queueMutex;

    int start();
    void join();
    void stop();
    void call();
    void addToQueue();

    void setConcentrator(const std::shared_ptr<ConcentratorController> &concentrator);

};


#endif //PACKETCONVERTER_CONNECTIONCONTROLLER_H
