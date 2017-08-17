cmake_minimum_required(VERSION 3.8)

project(libxdrpps)

include(configure_xdr.cmake)
link_directories(xdrc)

add_subdirectory(xdrc)
add_subdirectory(xdrpp)
