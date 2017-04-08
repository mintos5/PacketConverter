//
// Created by root on 11.3.2017.
//
#include <iostream>
#include <sstream>
#include <csignal>
#include "ConnectionController.h"
#include "ConcentratorController.h"

ConnectionController::ConnectionController(Message config) {
    //std::cout << config.message["conf"]["server_address"].dump() << std::endl;
    nlohmann::json confSection = config.message.at("conf");
    this->hostname = confSection["server_address"].get<std::string>();
    this->gatewayId = confSection["gateway_ID"];
}

void ConnectionController::setConcentrator(const std::shared_ptr<ConcentratorController> &concentrator) {
    ConnectionController::concentrator = concentrator;
}

int ConnectionController::start() {
    //SSL library set up
    SSL_library_init();
    ERR_load_BIO_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    //SSL context set up
    ctx = SSL_CTX_new(SSLv23_client_method());
    //Trust store load up
    if(! SSL_CTX_load_verify_locations(ctx, caCert.c_str(), NULL))
    {
        std::cerr << "Error loading Trust store" << std::endl;
        SSL_CTX_free(ctx);
        return -1;
    }
    //Connection set up
    //SSL_CTX_set_timeout(ctx,5);
    bio = BIO_new_ssl_connect(ctx);
    BIO_get_ssl(bio, & ssl);
    BIO_set_conn_hostname(bio,hostname.c_str());
    BIO_set_nbio(bio,1);
    //Try to connect
    int connectReturn;
    int retry = 0;
    while(connectReturn = BIO_do_connect(bio),connectReturn <= 0){
        if(!BIO_should_retry(bio) || retry > 6){
            std::cerr << "Error connecting  to server" << std::endl;
            BIO_free_all(bio);
            SSL_CTX_free(ctx);
            return -1;
        }
        ++retry;
    }
    if (BIO_get_fd(bio, &socket) < 0) {
        std::cerr << "Error getting connection fd" << std::endl;
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return -1;
    }
    //Certificate verification
    if(SSL_get_verify_result(ssl) != X509_V_OK)
    {
        std::cerr << "Error checking certificate" << std::endl;
        BIO_free_all(bio);
        SSL_CTX_free(ctx);
        return -1;
    }
    this->fiberProcess = std::thread(&ConnectionController::process,this);
    return 0;
}

void ConnectionController:: join() {
    this->fiberProcess.join();
}

void ConnectionController::send() {
    std::lock_guard<std::mutex> guard1(this->queueMutex);
    this->sendNum++;
    std::stringstream ss;
    ss << "GET" << this->sendNum;
    Message keys = Message::fromFile("keyr.json");
    std::string request = keys.toStiot();
    //TODO check for messages in queue
    if (this->sendNum<2){
        while(BIO_write(bio, request.c_str(), request.size()) <= 0) {
            if(! BIO_should_retry(bio))
            {
                std::cerr << "Error writing to SSL socket" << std::endl;
            }
            std::cout << "Repeating writing to SSL socket" << std::endl;
        }
    }
}


void ConnectionController::process() {
    std::stringstream stream;
    std::string out;
    fd_set fds;
    timeval readTimeout;
    bool connectionDown = false;

    processMutex.lock();
    while(processRun){
        processMutex.unlock();

        send();

        //std::this_thread::sleep_for(std::chrono::seconds(1));
        //std::cout << "Sended message, waiting 1 sec" << std::endl;


        char buffData[buffSize];
        int readReturn;
        bool someData = false;
        do{
            readReturn = BIO_read(bio, buffData, buffSize-1);
            if (readReturn<=0){
                break;
            }
            //Received good data
            buffData[readReturn] = 0;
            stream << std::string(buffData);
            std::cout << "readReturn is:" << readReturn << std::endl;
            someData = true;
        } while(readReturn==buffSize-1);
        if(readReturn == 0){
            std::cerr << "Error reading from SSL socket" << std::endl;
            connectionDown = true;
            break;
        }
        else if (readReturn<0 && !someData){
            if(! BIO_should_retry(bio)) {
                /* Handle failed read here */
                std::cerr << "Error reading from SSL socket,ignoring" << std::endl;
            }
            //waiting for communication
            FD_ZERO(&fds);
            FD_SET(socket, &fds);
            readTimeout.tv_sec = 0;
            readTimeout.tv_usec = 500000;
            int selectWait = select(socket+1, &fds, NULL, NULL, &readTimeout);
            printf("Wau %d\n",selectWait);
        }
        else {
            //GOOD DATA
            out = stream.str();
            std::cout << out << std::endl;
            concentrator->addToQueue(Message::fromStiot(out));
            //clear stream for other messages
            stream.str(std::string());
        }
        processMutex.lock();
    }
    processMutex.unlock();
    if (connectionDown){
        raise(SIGINT);
    }
}

void ConnectionController::stop() {
    std::lock_guard<std::mutex> guard1(this->processMutex);
    this->processRun = false;
}

void ConnectionController::addToQueue(Message message) {
    std::lock_guard<std::mutex> guard1(this->queueMutex);
    this->endDeviceData.push(message);
}
