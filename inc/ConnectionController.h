//
// Created by root on 11.3.2017.
//

#ifndef PACKETCONVERTER_CONNECTIONCONTROLLER_H
#define PACKETCONVERTER_CONNECTIONCONTROLLER_H

#include <openssl/ssl.h>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include "MessageConverter.h"


class ConnectionController {
    int socket;
    BIO *bio;
    SSL *ssl;
    SSL_CTX *ctx;
    const std::string caCert = "/etc/ssl/certs/ca-certificates.crt";
    std::string hostname;
    const int buffSize = 65535;

    std::shared_ptr<MessageConverter> converter;

    bool processRun = true;
    std::mutex processMutex;
    std::thread fiberProcess;
    std::queue<Message> endDeviceData;
    std::mutex queueMutex;

    void send();
    void process();

public:
    ConnectionController(const std::shared_ptr<MessageConverter> &converter,Message config);
    int start();
    void join();
    void stop();
    void addToQueue(Message message);
    void addBulk(std::vector<Message> vector);

    bool connected = false;
};


#endif //PACKETCONVERTER_CONNECTIONCONTROLLER_H
