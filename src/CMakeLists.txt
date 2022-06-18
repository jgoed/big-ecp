cmake_minimum_required(VERSION 3.10)

project(big-ecp)

include_directories(${PROJECT_SOURCE_DIR})

add_executable(big-ecp main.cpp datastructure.cpp preprocessing.cpp queryprocessing.cpp)