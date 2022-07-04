project(ecp_lib
    DESCRIPTION "Lib of ecp"
    VERSION 1.0
)

add_library(${PROJECT_NAME}
    SHARED
        assignment.cpp
        datastructure.cpp
        distance.cpp
        ecp.cpp
        index.hpp
        query.hpp
        assignment.hpp
        datastructure.hpp
        distance.hpp
        index.cpp
        query.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        # where the library itself will look for its internal headers
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC
        # where top-level project will look for the library's public headers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)