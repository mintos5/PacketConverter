cmake_minimum_required(VERSION 3.1)
project(PacketConverter)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})
SET(GCC_COVERAGE_COMPILE_FLAGS "-pthread")
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )


#LibLoRa library
set ( LIBLORAGW_LIB libloragw.a )
link_directories( ../lora_gateway/libloragw)
include_directories(../lora_gateway/libloragw/inc)
include_directories(inc)

#OpenSSL library (in Ubuntu you need libssl-dev)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})


set(SOURCES src/MainApp.cpp src/ConcentratorController.cpp inc/ConcentratorController.h src/ConnectionController.cpp
        inc/ConnectionController.h src/Message.cpp inc/Message.h src/DevicesTable.cpp inc/DevicesTable.h
        src/Encryption.cpp inc/Encryption.h src/MessageConverter.cpp inc/MessageConverter.h
        src/DiffieHellman.cpp inc/DiffieHellman.h inc/config.h)

add_executable(packet_converter ${SOURCES})
target_link_libraries(packet_converter ${LIBLORAGW_LIB} ${OPENSSL_LIBRARIES})
install(TARGETS packet_converter DESTINATION .)