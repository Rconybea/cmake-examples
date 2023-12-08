cmake_minimum_required(VERSION 3.25)
project(cmake-examples VERSION 1.0)
enable_language(CXX)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23 CACHE STRING "cxx standard [11/14/17/20/23]")
endif()
message("-- CMAKE_CXX_STANDARD: CXX standard is [${CMAKE_CXX_STANDARD}]")

set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT DEFINED PROJECT_CXX_FLAGS)
    set(PROJECT_CXX_FLAGS "-Werror -Wall -Wextra" CACHE STRING "project-wide cxx flags")
endif()
message("-- PROJECT_CXX_FLAGS: project CXX flags are [${PROJECT_CXX_FLAGS}]")

add_compile_options(${PROJECT_CXX_FLAGS})

set(SELF_EXE hello)
set(SELF_SRCS hello.cpp)

add_executable(${SELF_EXE} ${SELF_SRCS})
