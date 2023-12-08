cmake_minimum_required(VERSION 3.25)
project(cmake-examples VERSION 1.0)
enable_testing()
enable_language(CXX)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23 CACHE STRING "c++ standard level [11|14|17|20|23]")
endif()
message("-- CMAKE_CXX_STANDARD: c++ standard level is [${CMAKE_CXX_STANDARD}]")

set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT DEFINED PROJECT_CXX_FLAGS)
    set(PROJECT_CXX_FLAGS "-Werror -Wall -Wextra" CACHE STRING "project c++ compiler flags")
endif()
message("-- PROJECT_CXX_FLAGS: project c++ flags are [${PROJECT_CXX_FLAGS}]")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "generate build/compile_commands.json")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if(NOT CMAKE_INSTALL_RPATH)
    set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib CACHE STRING
        "runpath in installed libraries/executables")
endif()

find_package(Catch2 CONFIG REQUIRED)
find_package(boost_program_options CONFIG REQUIRED)
find_package(PkgConfig)
pkg_check_modules(zlib REQUIRED zlib)

#message("zlib_CFLAGS_OTHER=${zlib_CFLAGS_OTHER}")
#message("zlib_INCLUDE_DIRS=${zlib_INCLUDE_DIRS}")
#message("zlib_LIBRARIES=${zlib_LIBRARIES}")

#message("boost_program_options_DIR=${boost_program_options_DIR}")

add_subdirectory(compression)
add_subdirectory(compression/utest)
add_subdirectory(app/hello)
add_subdirectory(app/myzip)
