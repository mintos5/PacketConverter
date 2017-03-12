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
    concentrator->stop();
    connection->stop();
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
    std::ifstream input("json.txt");
    std::stringstream sstr;
    while(input >> sstr.rdbuf());
    std::string testSPolu = sstr.str();
        json test = json::parse(sstr.str());
        std::cout << test.dump(4) << std::endl;
        if (test.find("SETR") != test.end()) {
            std::cout << "Nasiel som ta" << std::endl;
        }
    concentrator = std::make_shared<ConcentratorController>();
    connection = std::make_shared<ConnectionController>();
    concentrator->setConnection(connection);
    connection->setConcentrator(concentrator);

    //concentrator->start();
    connection->start();
    //concentrator->join();
    connection->join();
}

bool MainApp::running;

