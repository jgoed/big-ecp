find_package(Threads REQUIRED)

add_executable(main main.cpp)

target_link_libraries(main PUBLIC ecp_lib pthread)

include_directories(main PUBLIC ../include/)
