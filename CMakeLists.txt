cmake_minimum_required(VERSION 3.1)
project(PacketConverter)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})

#LibLoRa library
set ( LIBLORAGW_LIB libloragw.a )
link_directories( ../lora_gateway/libloragw)
include_directories(inc)
include_directories(../lora_gateway/libloragw/inc)

#OpenSSL library (in Ubuntu you need libssl-dev)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})


set(SOURCES src/MainApp.cpp)

add_executable(packet_converter ${SOURCES})
target_link_libraries(packet_converter ${OPENSSL_LIBRARIES} ${LIBLORAGW_LIB})
install(TARGETS packet_converter DESTINATION .)