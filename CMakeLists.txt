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
    set(PROJECT_CXX_FLAGS -Werror -Wall -Wextra -Wold-style-cast -Wstrict-aliasing -fno-strict-aliasing CACHE STRING "project c++ compiler flags")
endif()
message("-- PROJECT_CXX_FLAGS: project c++ flags are [${PROJECT_CXX_FLAGS}]")

# ----------------------------------------------------------------
# cmake -DCMAKE_BUILD_TYPE=debug

# clear out hardwired default.
# we want override project-level defaults, so need to prevent interference from hardwired defaults
# (the problem with non-empty hardwired defaults is that we can't tell if they've been set on the
# command line)
#
set(CMAKE_CXX_FLAGS_DEBUG "")

# CMAKE_CXX_FLAGS_DEBUG is built-in to cmake and has non-empty default.
#  -> we cannot tell whether it was set on the command line
#  -> use PROJECT_CXX_FLAGS_DEBUG instead
#
# built-in default value is -g; can hardwire different project policy here
#
if (NOT DEFINED PROJECT_CXX_FLAGS_DEBUG)
    set(PROJECT_CXX_FLAGS_DEBUG ${PROJECT_CXX_FLAGS} -ggdb
        CACHE STRING "debug c++ compiler flags")
endif()
message("-- PROJECT_CXX_FLAGS_DEBUG: debug c++ flags are [${PROJECT_CXX_FLAGS_DEBUG}]")

add_compile_options("$<$<CONFIG:DEBUG>:${PROJECT_CXX_FLAGS_DEBUG}>")

# ----------------------------------------------------------------
# cmake -DCMAKE_BUILD_TYPE=release

# clear out hardwired default.
# we want override project-level defaults, so need to prevent interference from hardwired defaults
# (the problem with non-empty hardwired defaults is that we can't tell if they've been set on the
# command line)
#
set(CMAKE_CXX_FLAGS_RELEASE "")

# CMAKE_CXX_FLAGS_Release is built-in to cmake
#  -> automatically added to all c++ compilation targets
#     when CMAKE_BUILD_TYPE=Release
#
# built-in default value is -O3 -DNDEBUG;  can hardwire different project policy here
#
if (NOT DEFINED PROJECT_CXX_FLAGS_RELEASE)
    set(PROJECT_CXX_FLAGS_RELEASE ${PROJECT_CXX_FLAGS} -march=native -O3 -DNDEBUG
        CACHE STRING "release c++ compiler flags")
endif()
message("-- PROJECT_CXX_FLAGS_RELEASE: release c++ flags are [${PROJECT_CXX_FLAGS_RELEASE}]")

add_compile_options("$<$<CONFIG:RELEASE>:${PROJECT_CXX_FLAGS_RELEASE}>")

# ----------------------------------------------------------------
# cmake -DCMAKE_BUILD_TYPE=coverage

if (NOT DEFINED PROJECT_CXX_FLAGS_COVERAGE)
    # note: for clang would use -fprofile-instr-generate -fcoverage-mapping here instead and also at link time
    set(PROJECT_CXX_FLAGS_COVERAGE ${PROJECT_CXX_FLAGS} -ggdb -Og -fprofile-arcs -ftest-coverage
        CACHE STRING "coverage c++ compiler flags")
endif()
message("-- PROJECT_CXX_FLAGS_COVERAGE: coverage c++ flags are [${PROJECT_CXX_FLAGS_COVERAGE}]")

add_compile_options("$<$<CONFIG:COVERAGE>:${PROJECT_CXX_FLAGS_COVERAGE}>")
# when -DCMAKE_BUILD_TYPE=coverage, link executables with gcov
link_libraries("$<$<CONFIG:COVERAGE>:gcov>")

find_program(LCOV_EXECUTABLE NAMES lcov)
find_program(GENHTML_EXECUTABLE NAMES genhtml)

# with coverage build:
# 1. invoke instrumented executables for which you want coverage:
#     (cd path/to/build && ctest)
# 2. post-process low-level coverage data
#     (path/to/build/gen-ccov)
# 3. point browser to generated html data
#     file:///path/to/build/ccov/html/index.html
#
configure_file(
    ${PROJECT_SOURCE_DIR}/cmake/gen-ccov.in
    ${PROJECT_BINARY_DIR}/gen-ccov)

file(CHMOD ${PROJECT_BINARY_DIR}/gen-ccov PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

# ----------------------------------------------------------------

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "generate build/compile_commands.json")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if(NOT CMAKE_INSTALL_RPATH)
    set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib CACHE STRING
        "runpath in installed libraries/executables")
endif()

# ----------------------------------------------------------------

find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11)
message("-- Python_VERSION=${Python_VERSION}")
#message("Python_VERSION_MAJOR=${Python_VERSION_MAJOR}")
#message("Python_VERSION_MINOR=${Python_VERSION_MINOR}")
set(PYTHON_LIBRARY_PREFIX ${Python_VERSION_MAJOR}.${Python_VERSION_MINOR})

find_package(Catch2 CONFIG REQUIRED)
find_package(boost_program_options CONFIG REQUIRED)
find_package(PkgConfig)
pkg_check_modules(zlib REQUIRED zlib)

find_program(BASH_EXECUTABLE NAMES bash REQUIRED)

#message("zlib_CFLAGS_OTHER=${zlib_CFLAGS_OTHER}")
#message("zlib_INCLUDE_DIRS=${zlib_INCLUDE_DIRS}")
#message("zlib_LIBRARIES=${zlib_LIBRARIES}")

#message("boost_program_options_DIR=${boost_program_options_DIR}")

add_subdirectory(compression)
add_subdirectory(compression/utest)
add_subdirectory(zstream)
add_subdirectory(zstream/utest)
add_subdirectory(pyzstream)
add_subdirectory(pyzstream/utest)
add_subdirectory(app/hello)
add_subdirectory(app/myzip)
add_subdirectory(app/myzip/utest)
