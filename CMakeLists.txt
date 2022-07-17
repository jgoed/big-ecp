cmake_minimum_required(VERSION 3.18)

project(ecp VERSION 1.0 DESCRIPTION "ecp" LANGUAGES CXX)

set(CMAKE_VERBOSE_MAKEFILE on)

if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
  message("GNU compiler found.")

  list(APPEND generalflags "-Wall")
  message("Appending general flags: " ${generalflags})
  add_compile_options(${generalflags})

  if (${CMAKE_BUILD_TYPE} MATCHES Release)
    list(APPEND relflags "-O3" "-fPIC" )
    message("Build type is: " ${CMAKE_BUILD_TYPE})
    message("Appending further flags: " ${relflags})
    add_compile_options(${relflags})

  elseif (${CMAKE_BUILD_TYPE} MATCHES RelWithDebInfo)
    list(APPEND reldebflags "-g" "-O3" "-fPIC" )
    message("Build type is: " ${CMAKE_BUILD_TYPE})
    message("Appending further flags: " ${reldebflags})
    add_compile_options(${reldebflags})

  elseif (${CMAKE_BUILD_TYPE} MATCHES Debug)
    list(APPEND debugflags "-g" "-Wextra" "-Wpedantic")
    message("Build type is: " ${CMAKE_BUILD_TYPE})
    message("Appending further flags: " ${debugflags})
    add_compile_options(${debugflags})
  endif()

else()
  message(FATAL_ERROR "Current compiler not supported. Only GNU/GCC. Compile options/flags will not be set.")
endif()

find_package(SWIG COMPONENTS python)

add_subdirectory(src)
add_subdirectory(main)
add_subdirectory(swig)