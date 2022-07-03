cmake_minimum_required(VERSION 3.18)

project(
    ecp
    VERSION 1.0
    DESCRIPTION "ecp"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

find_package(SWIG COMPONENTS python)

add_subdirectory(src)
add_subdirectory(main)
add_subdirectory(swig)