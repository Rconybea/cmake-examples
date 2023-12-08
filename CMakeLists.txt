cmake_minimum_required(VERSION 3.25)
project(cmake-examples VERSION 1.0)
enable_language(CXX)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23 CACHE STRING "cxx standard [11/14/17/20/23]")
endif()
message("-- CXX standard is ${CMAKE_CXX_STANDARD}")

set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Werror -Wall -Wextra)

set(SELF_EXE hello)
set(SELF_SRCS hello.cpp)

add_executable(${SELF_EXE} ${SELF_SRCS})
