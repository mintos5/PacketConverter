# PacketConverter
Packet converter between LoRa@FIIT and STIOT protocols

Avalaible also here: https://github.com/mintos5/PacketConverter

**Instalation**

Run ./install.sh script with or without parameters
* daemon = install systemd service (run as root)
* clear = remove all instal files (run as root)
* without parameter install to local folder (root permission not needed)

**How to use**

1. Reset LoRa concentrator before starting (use script board_reset.sh)
2. Run application ./packet_converter
3. Close application by pressing CTRL+C


**Config files**

* config.json is file to set program
* setr.json is file containing SETR message for server 
* seta.json is testing file for running application without server (response to SETR message)
* rega.json is testing file for running application without server (response to registration of end device)

* /inc/config.h is for setting offline and debug mode
