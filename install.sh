#!/bin/bash
command="default"
if [ "$#" -ge 1 ]; then
	if [ "$1" == "daemon" ] || [ "$1" == "clear" ]; then
    command=$1
	fi
fi


if [ "$command" == "default" ]; then
	echo "Locally installing to this folder:"
	mkdir -p debug
	cd ./debug
	#cmake -DCMAKE_BUILD_TYPE=Debug .. build with debug symbols
	cmake ..
	make
	make install
	cd ..
fi
if [ "$command" == "daemon" ]; then
	echo "Installing daemon service:"
	if [ $UID != 0 ]; then
		echo "ERROR: Not root user?"
		exit 1
	fi
	cp ./packet_converter.service /lib/systemd/system/
	systemctl enable packet_converter.service
fi
if [ "$command" == "clear" ]; then
	echo "Clearing all install files:"
	if [ $UID != 0 ]; then
    echo "ERROR: Not root user?"
    exit 1
	fi
	rm ./packet_converter
	rm -rf ./debug/
fi
