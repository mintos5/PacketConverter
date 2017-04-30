#include <getopt.h>
#include "../inc/MainApp.h"
#include <csignal>
#include <fstream>
#include <ConcentratorController.h>
#include <ConnectionController.h>
//#include <iostream>


std::shared_ptr<ConnectionController> connection;
std::shared_ptr<ConcentratorController> concentrator;
std::shared_ptr<MessageConverter> converter;

void usage(){
    using namespace std;
    cout << "LoRa@FIIT and STIOT packet converter" << endl;
    cout << "____________________________________" << endl;
    cout << endl;
    cout << "Aplication for receiving LoRa@FIIT packets from LoRa concentrator"
         << "and sending it to STIOT server" << endl;
    cout << std::endl;
    cout << "to configure edit setr.json or config.json" << std::endl;
}

void MainApp::shutDown() {
    MainApp::running = false;
    converter->stop();
    connection->stop();
    concentrator->stop();
}

void signalHandler(int signum) {
    std::cout << std::endl;
    std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
    std::cout << "Closing application" << std::endl;
    MainApp::shutDown();
}

int main(int argc, char *argv[]){
    MainApp::running = true;
    signal(SIGINT,signalHandler);
    signal(SIGTERM,signalHandler);
    int opt;
    while ((opt = getopt(argc,argv,"h")) != -1){
        switch (opt){
            case 'h':
                usage();
                return EXIT_FAILURE;
                break;

            default:
                std::cout << "Use -h for printing help." << std::endl;
                return EXIT_FAILURE;
                break;
        }
    }
    using json = nlohmann::json;
    std::ifstream input("config.json");
    std::stringstream sstr;
    while(input >> sstr.rdbuf());
    std::string config = sstr.str();
    Message localConfigMessage = Message::fromJsonString(config);
    //std::cout << localConfigMessage.message.dump(2) << std::endl;

    converter = std::make_shared<MessageConverter>();

    concentrator = std::make_shared<ConcentratorController>(converter,localConfigMessage);
    connection = std::make_shared<ConnectionController>(converter,localConfigMessage);

    converter->setConcentrator(concentrator);
    converter->setConnection(connection);

    if (converter->start()<0){
        std::cerr << "Problem starting converter thread" << std::endl;
        return -1;
    }

    if (GATEWAY_OFFLINE){
        std::cout << "Starting gateway in offline mode" << std::endl;
        if (concentrator->startOffline()<0){
            std::cerr << "Problem starting concentrator" << std::endl;
            return -1;
        }
        concentrator->join();
        converter->join();
    }
    else {
        if (concentrator->start()<0){
            std::cerr << "Problem starting concentrator" << std::endl;
            return -1;
        }
        if (connection->start()<0){
            std::cerr << "Problem starting network communication" << std::endl;
            signalHandler(SIGINT);
            concentrator->join();
            converter->join();
        }
        else {
            connection->addToQueue(Message::createSETR("setr.json"));
            connection->join();
            concentrator->join();
            converter->join();
        }
    }
}

bool MainApp::running;
