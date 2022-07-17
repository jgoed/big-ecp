find_package(Threads REQUIRED)

add_executable(main main.cpp)

target_link_libraries(main PUBLIC ecp_lib ${CMAKE_THREAD_LIBS_INIT})

include_directories(main PUBLIC ../include/)
