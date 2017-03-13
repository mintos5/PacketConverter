#include <getopt.h>
#include "../inc/MainApp.h"
#include <csignal>
#include <fstream>
#include <ConcentratorController.h>
#include <ConnectionController.h>
//#include <iostream>


std::shared_ptr<ConnectionController> connection;
std::shared_ptr<ConcentratorController> concentrator;

void usage(){
    using namespace std;
    cout << "LoRa@FIIT and STIOT packet converter" << endl;
    cout << "____________________________________" << endl;
    cout << endl;
    cout << "Aplication for receiving LoRa@FIIT packets from LoRa concentrator"
         << "to STIOT server" << endl;
}

void signalHandler(int signum) {
    std::cout << std::endl;
    std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
    std::cout << "Closing application" << std::endl;
    MainApp::running = false;
    connection->stop();
    concentrator->stop();
    //concentrator->join();   //wait to end...
}

int main(int argc, char *argv[]){
    MainApp::running = true;
    signal(SIGINT,signalHandler);
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
    Message localConfigMessage = Message::fromStiot(config);
    std::cout << localConfigMessage.message.dump(2) << std::endl;
    concentrator = std::make_shared<ConcentratorController>(localConfigMessage);
    connection = std::make_shared<ConnectionController>(localConfigMessage);
    concentrator->setConnection(connection);
    connection->setConcentrator(concentrator);

    if (concentrator->start()<0){
        std::cerr << "Problem starting concentrator" << std::endl;
        return 0;
    }
    if (connection->start()<0){
        std::cerr << "Problem starting network communication" << std::endl;
        //signalHandler(2);
        concentrator->join();
    }
    else {
        connection->join();
    }
}

bool MainApp::running;

