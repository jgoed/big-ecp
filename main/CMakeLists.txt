add_executable(main main.cpp)

target_link_libraries(main PUBLIC ecp_lib)

include_directories(main PUBLIC ../include/)
