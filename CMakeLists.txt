cmake_minimum_required(VERSION 3.15)
project(TransferFiles)

include_directories(${CMAKE_SOURCE_DIR}/common)

set(COMMON_SOURCE
    common/message.cpp
    common/log.cpp
    common/crc.cpp
)

add_executable(client
    src/client/main.cpp
    src/client/client.cpp
    ${COMMON_SOURCE}
)
target_link_libraries(client PRIVATE Ws2_32)

add_executable(server
    src/server/main.cpp
    src/server/server.cpp
    ${COMMON_SOURCE}
)
target_link_libraries(server PRIVATE Ws2_32)