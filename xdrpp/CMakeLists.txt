cmake_minimum_required(VERSION 3.2.2)

project(xdrpp)

include(gen_xdrpp.cmake)

include_directories(../)

set(XDRPP_SOURCE_FILES
        marshal.cc
        printer.cc
        )

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    include_directories(../msvc_xdrpp/include)
endif()

if((${CMAKE_COMPILER_IS_GNUCXX}) OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    add_definitions(-DHAVE_CONFIG_H)
    set(CPP_COMMAND ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -std=c++11 -Wall -pthread")
    set(XDRPP_SOURCE_FILES
            ${XDRPP_SOURCE_FILES}
            msgsock.cc
            rpcbind.cc
            rpc_msg.cc
            server.cc
            socket_unix.cc
            srpc.cc
            arpc.cc)
endif()

if(${CMAKE_COMPILER_IS_GNUCXX})
    set(CPP_COMMAND_STRING "gcc -E -xc")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CPP_COMMAND_STRING "clang -E -xc")
endif()



add_library(xdrpp STATIC ${XDRPP_SOURCE_FILES} ${OUTPUT_XDRPP_FILES})