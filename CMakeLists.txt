cmake_minimum_required(VERSION 3.25)
project(ex8 VERSION 1.0)
enable_language(CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "generate build/compile_commands.json")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

find_package(boost_program_options CONFIG REQUIRED)
find_package(PkgConfig)
pkg_check_modules(zlib REQUIRED zlib)

#message("zlib_CFLAGS_OTHER=${zlib_CFLAGS_OTHER}")
#message("zlib_INCLUDE_DIRS=${zlib_INCLUDE_DIRS}")
#message("zlib_LIBRARIES=${zlib_LIBRARIES}")

#message("boost_program_options_DIR=${boost_program_options_DIR}")

add_subdirectory(compression)
add_subdirectory(app/hello)
