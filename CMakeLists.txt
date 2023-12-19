cmake_minimum_required(VERSION 3.25)
project(cmake-examples VERSION 1.0)
enable_language(CXX)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23 CACHE STRING "c++ standard level [11|14|17|20|23]")
endif()
message("-- CMAKE_CXX_STANDARD: c++ standard level is [${CMAKE_CXX_STANDARD}]")

set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT DEFINED PROJECT_CXX_FLAGS)
    set(PROJECT_CXX_FLAGS -Werror -Wall -Wextra -fno-strict-aliasing CACHE STRING "project c++ compiler flags")
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

set(SELF_EXE hello)
set(SELF_SRCS hello.cpp)

add_executable(${SELF_EXE} ${SELF_SRCS})
