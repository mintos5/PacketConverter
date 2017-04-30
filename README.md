# PacketConverter
Packet converter between LoRa@FIIT and STIOT protocols

Avalaible also here: https://github.com/mintos5/PacketConverter

**Instalation**
1. Install lora_gateway in /opt/fiitap/lora_gateway
2. Move files to /opt/fiitap/PacketConverter
3. Run ./install.sh as root (sudo) script with or without parameters
* without parameter install to local folder.
* daemon = install systemd service.
* clear = remove all instaled files.

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
