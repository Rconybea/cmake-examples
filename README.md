# Progressive sequence of cmake examples.

Examples here are c++ focused.  I've tested on linux and osx.

Reflects strong preference for control + flexibility;
assumes reader wants to build/support an ecosystem of related artifacts
that build in a similar way

Intending to navigate various stumbling blocks I encountered
over a couple of years of trying things;
and to provide an opinionated (though possibly flawed) version of best practice

## Topics
1. lsp integration.
   tested with emacs
2. executables, shared libraries, interdependencies
3. header-only libraries
4. monorepo build
   limitations of monorepo build
5. separable build (+ find_package() support)
   limitations of separable build
6. github actions
7. versioning, explicit codebase dependencies, build isolation
8. pybind11 + python issues
   binary API dependence

## Progression
1.  ex1:  c++ executable X1 (`hello`)
    ex1b: c++ standard + compile-time flags
    ex1c: multiple build configurations
2.  ex2:  add LSP integration
3.  ex3:  c++ executable X1 + cmake-aware library dep O1 (`boost::program_options`), using cmake `find_package()`
4.  ex4:  c++ executable X1 + non-cmake library dep O2 (`zlib`)
5.  ex5:  refactor: move compression wrapper to 2nd translation unit
6.  ex6:  add install target
7.  ex7:  c++ executable X1 + example c++ library A1 (`compression`) with A1 -> O2, monorepo-style
8.  ex8:  refactor: move X1 to own subdir
9.  ex9:  add c++ executable X2 (`myzip`) also using A1
10. ex10: add c++ unit test + header-only library dep O3 (`catch2`)
11. ex11: add bash unit test (for `myzip`)
12. ex12: refactor: use inflate/deflate (streaming) api for non-native solution
13. ex13: example c++ header-only library A2 (`zstream`) with A2 -> A1 -> O2, monorepo-style
14. ex14: github CI example

* ex15: find_package() support
* ex16: add pybind11 library (pyzstream)
* ex17: add sphinx doc

* c++ executable X + library A, A -> O, separable-style
   provide find_package() support - can build using X-subdir's cmake if A built+installed
* project-specific macros - simplify
* project-specific macros - support (monorepo, separable) builds from same tree
* add performance benchmarks.
* add code coverage.

- monorepo-style: artifacts using dependencies supplied from same repo and build tree

## Preliminaries

Each example gets its own dedicated git branch

To get started,  clone this repo:
```
$ git clone git@github.com:Rconybea/cmake-examples.git
```

## Example 1
```
$ git checkout ex1
```

```
// hello.cpp

#include <iostream>

using namespace std;

int
main(int argc, char * argv[]) {
    cout << "Hello, world!\n" << endl;
}
```

note: here 3.25 is the version of cmake I happen to be working with
```
# CMakeLists.txt

cmake_minimum_required(VERSION 3.25)
project(ex1 VERSION 1.0)
enable_language(CXX)

set(SELF_EXE hello)
set(SELF_SRCS hello.cpp)

add_executable(${SELF_EXE} ${SELF_SRCS})
```

To build + run:
```
$ cd cmake-examples
$ git checkout ex1
$ mkdir build
$ cmake -B build       # ..configure
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build  # ..compile
[ 50%] Building CXX object CMakeFiles/hello.dir/hello.cpp.o
[100%] Linking CXX executable hello
[100%] Built target hello
$ ./build/hello        # ..run
Hello, world!

$
```

### Example 1b: persistent compiler flags

We want to be able set per-build-directory compiler flags,  and have them persist so we don't have to rehearse
them every time we invoke cmake.
We can do this using cmake cache variables:

```
$ cd cmake-examples
$ git switch ex1b
```

In top-level CMakeLists.txt:
```
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23 CACHE STRING "c++ standard level [11|14|17|20|23]")
endif()
message("-- CMAKE_CXX_STANDARD: c++ standard level is [${CMAKE_CXX_STANDARD}]")

set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT DEFINED PROJECT_CXX_FLAGS)
    set(PROJECT_CXX_FLAGS "-Werror -Wall -Wextra" CACHE STRING "project c++ compiler flags")
endif()
message("-- PROJECT_CXX_FLAGS: project c++ flags are [${PROJECT_CXX_FLAGS}]")
```

For example, to prepare a c++11 build in `build11/` with compiler's default compiler warnings:
```
$ cmake -DCMAKE_CXX_STANDARD=11 -DPROJECT_CXX_FLAGS= -B build11
-- CMAKE_CXX_STANDARD: c++ standard level is [11]
-- PROJECT_CXX_FLAGS: project c++ flags are []
-- Configuring done
-- Generating done
```

Now if we rerun cmake on `build11/` the cached settings are remembered:
```
$ cmake -B build11
-- CMAKE_CXX_STANDARD: c++ standard level is [11]
-- PROJECT_CXX_FLAGS: project c++ flags are []
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build11
```

### Example 1c: multiple build configurations (debug, release etc)

We want to support compiler flags for various build configurations:  debug, release, sanitize, coverage etc.

For compiler flags, cmake provides automatic variables `CMAKE_CXX_FLAGS_<CONFIG>`
(e.g. with `<CONFIG>` set to `debug` with `cmake -DCMAKE_BUILD_TYPE=debug`).

However,  these come with built-in non-empty default values.
for example with gcc build the default value of `CMAKE_CXX_FLAGS_RELEASE` is `-O3 -DNDEBUG`.

This creates a conflict with the following set of objectives:
1. want curated build-type-specific project-level defaults for different builds
2. want to be able to override these defaults from the command line

The problem with a built-in non-empty default, is that when writing cmake code we don't know:
was observed value provided by cmake, or from command line?

We'll work around this problem by using variable names not known to cmake.

```
# CMakeLists.txt

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
```

The fancy generator expressions like `add_compile_options("$<$<CONFIG:DEBUG>:${PROJECT_CXX_FLAGS_DEBUG}>")`
only take effect with `-DCMAKE_BUILD_TYPE=debug`.

For example:
```
$ cd cmake-examples
$ git switch ex1c
$ cmake -DCMAKE_CXX_STANDARD=20 -DCMAKE_BUILD_TYPE=debug -B build_debug
-- The C compiler identification is GNU 12.2.0
-- The CXX compiler identification is GNU 12.2.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: /usr/bin//gcc - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/g++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- CMAKE_CXX_STANDARD: c++ standard level is [20]
-- PROJECT_CXX_FLAGS: project c++ flags are [-Werror;-Wall;-Wextra;-fno-strict-aliasing]
-- PROJECT_CXX_FLAGS_DEBUG: debug c++ flags are [-Werror;-Wall;-Wextra;-fno-strict-aliasing;-ggdb]
-- PROJECT_CXX_FLAGS_RELEASE: release c++ flags are [-Werror;-Wall;-Wextra;-fno-strict-aliasing;-march=native;-O3;-DNDEBUG]
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build_debug
$ cmake --build build_debug --verbose
cmake -S/home/roland/proj/cmake-examples -B/home/roland/proj/cmake-examples/build_debug --check-build-system CMakeFiles/Makefile.cmake 0
cmake -E cmake_progress_start /home/roland/proj/cmake-examples/build_debug/CMakeFiles /home/roland/proj/cmake-examples/build_debug//CMakeFiles/progress.marks
make  -f CMakeFiles/Makefile2 all
make[1]: Entering directory '/home/roland/proj/cmake-examples/build_debug'
make  -f CMakeFiles/hello.dir/build.make CMakeFiles/hello.dir/depend
make[2]: Entering directory '/home/roland/proj/cmake-examples/build_debug'
cd /home/roland/proj/cmake-examples/build_debug && cmake -E cmake_depends "Unix Makefiles" /home/roland/proj/cmake-examples /home/roland/proj/cmake-examples /home/roland/proj/cmake-examples/build_debug /home/roland/proj/cmake-examples/build_debug /home/roland/proj/cmake-examples/build_debug/CMakeFiles/hello.dir/DependInfo.cmake --color=
make[2]: Leaving directory '/home/roland/proj/cmake-examples/build_debug'
make  -f CMakeFiles/hello.dir/build.make CMakeFiles/hello.dir/build
make[2]: Entering directory '/home/roland/proj/cmake-examples/build_debug'
[ 50%] Building CXX object CMakeFiles/hello.dir/hello.cpp.o
g++   -Werror -Wall -Wextra -fno-strict-aliasing -ggdb -std=gnu++20 -MD -MT CMakeFiles/hello.dir/hello.cpp.o -MF CMakeFiles/hello.dir/hello.cpp.o.d -o CMakeFiles/hello.dir/hello.cpp.o -c /home/roland/proj/cmake-examples/hello.cpp
[100%] Linking CXX executable hello
cmake -E cmake_link_script CMakeFiles/hello.dir/link.txt --verbose=1
g++ CMakeFiles/hello.dir/hello.cpp.o -o hello
make[2]: Leaving directory '/home/roland/proj/cmake-examples/build_debug'
[100%] Built target hello
make[1]: Leaving directory '/home/roland/proj/cmake-examples/build_debug'
cmake -E cmake_progress_start /home/roland/proj/cmake-examples/build_debug/CMakeFiles 0
```

## Example 2

LSP (language server process) integration allows compiler-driven editor interaction -- syntax highlighting,  code navigation etc.
For this to work the external LSP process needs to know exactly how we invoke the compiler.

1. By convention,  LSP will read a file `compile_commands.json` in the project's root (source) directory.
2. cmake can generate `compile_commands.json` during the configure step;   this will appear in the root of the build directory.
3. since LSP typically uses `clangd`,  we need also to tell it exactly where our preferred compiler's system headers reside;
   clangd won't always reliably locate these for itself.
4. expect user to performan last step: symlink from source directory to build
   this makes sense since if multiple build directories with different compiler switches,  only one-at-a-time can be adopted
   for LSP

```
# CMakeLists.txt
...

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "generate build/compiled_commands.json")  # 2.

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INLCUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})  # 3.
endif()
```

invoke build:
```
$ cd cmake-examples
$ git checkout ex2
$ mkdir -p build
$ ln -s build/compile_commands.json
$ cmake -B build
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
[ 50%] Building CXX object CMakeFiles/hello.dir/hello.cpp.o
[100%] Linking CXX executable hello
[100%] Built target hello
$ ./build/hello
Hello, world!

$
```

compile_commands.json will look something like:

```
# build/compile_commands.json
[
{
  "directory": "/home/roland/proj/cmake-examples/build",
  "command": "/usr/bin/g++  -isystem /usr/lib/gcc/x86_64-linux-gnu/12.2.0/include -isystem /usr/include -isystem /usr/lib/gcc/x86_64-linux-6nu/12.2.0/include-fixed -o CMakeFiles/hello.dir/hello.cpp.o -c /home/roland/proj/cmake-examples/hello.cpp",
  "file": "/home/roland/proj/cmake-examples/hello.cpp"
}
]
```

## Example 3

Use an external software package that provides cmake support.
For this example,  we'll use `boost::program_options`.

We add two lines to `CMakeLists.txt`:
1. `find_package(boost_program_options ...)`
2. `target_link_libraries(${SELF_EXE} PUBLIC Boost::program_options)`

```
# CMakeLists.xt
cmake_minimum_required(VERSION 3.25)
project(ex1 VERSION 1.0)
enable_language(CXX)

...

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "generate build/compile_commands.json")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

find_package(boost_program_options CONFIG REQUIRED)                                            # new

set(SELF_EXE hello)
set(SELF_SRCS hello.cpp)

add_executable(${SELF_EXE} ${SELF_SRCS})
target_link_libraries(${SELF_EXE} PUBLIC Boost::program_options)                               # new
```

Notes:
1. cmake `find_package()` searches directories in:
   - environment variable `CMAKE_PREFIX_PATH`
   - cmake variable `CMAKE_PREFIX_PATH`.
2. `find_package(boost_program_options ...)` searches for a directory that contains
   `boost_program_options-config.cmake` or `boost_program_optionsConfig.cmake`.
3. typical linux distribution will collect cmake `find_package()` support dirs under path like `/usr/lib/x86_64-linux-gnu/cmake`
4. the `CONFIG` argument to `find_package()` mandates that `find_package()` insist on a suitable package-specific `.cmake`
   support file, instead of falling back to `Find<packagename>.cmake` modules under `CMAKE_MODULE_PATH`.
   Disclaimer in `find_package()` docs:
   "Being externally provided, Find Moduules tend to be heuristic in nature and are susceptible to becoming out-of-date"
5. should use `target_link_libraries()` even if target library is header-only.
   cmake knows if header-only and takes responsibility for constructing suitable link line
6. need to read package docs or `boost_program_options-config.cmake` to find spelling for `Boost::program_options`

Add some program_options-using code to `hello.cpp`

```
// hello.cpp

#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;
using namespace std;

int
main(int argc, char * argv[]) {
    po::options_description po_descr{"Options"};
    po_descr.add_options()
        ("help,h",
         "this help")
        ("subject,s",
         po::value<string>()->default_value("world"),
         "say hello to this subject");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, po_descr), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cerr << po_descr << endl;
    } else {
        cout << "Hello, " << vm["subject"].as<string>() << "!\n" << endl;
    }
}
```

invoke build:
```
$ cd cmake-examples
$ git checkout ex3
$ mkdir -p build
$ ln -s build/compile_commands.json
$ cmake -B build
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
[ 50%] Building CXX object CMakeFiles/hello.dir/hello.cpp.o
[100%] Linking CXX executable hello
[100%] Built target hello
```

exercise executable
```
$ ./build/hello --help
Options:
  -h [ --help ]                 this help
  -s [ --subject ] arg (=world) say hello to this subject

$ ./build/hello --subject=Kermit
Hello, Kermit!
```

## Example 4

Use an external software package that does not provide direct cmake support,  but does support pkg-config.
For this example,  we'll use `zlib`.

We add to `CMakeLists.txt`:
1. `find_package(PkgConfig)`
   to invoke cmake pkg-config support.
2. `pkg_check_modules(zlib REQUIRED zlib)`
   to search for a `zlib.pc` configuration file associated with zlib.
   On success establishes cmake variables `zlib_CFLAGS_OTHER`, `zlib_INCLUDE_DIRS`, `zlib_LIBRARIES`.
3. `target_include_directories(${SELF_EXE} PUBLIC ${zlib_INCLUDE_DIRS})`
   to tell compiler where to find zlib include files.
4. `target_link_libraries(${SELF_EXE} PUBLIC ${zlib_LIBRARIES})`
   to tell compiler how to link zlib

```
# CMakeLists.txt
cmake_minimum_required(VERSION 3.25)
project(ex1 VERSION 1.0)
enable_language(CXX)

...

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "generate build/compile_commands.json")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

find_package(boost_program_options CONFIG REQUIRED)
find_package(PkgConfig)
pkg_check_modules(zlib REQUIRED zlib)

set(SELF_EXE hello)
set(SELF_SRCS hello.cpp)

add_executable(${SELF_EXE} ${SELF_SRCS})
target_compile_options(${SELF_EXE} PUBLIC ${zlib_CFLAGS_OTHER})
target_include_directories(${SELF_EXE} PUBLIC ${zlib_INCLUDE_DIRS})
target_link_libraries(${SELF_EXE} PUBLIC Boost::program_options)
target_link_libraries(${SELF_EXE} PUBLIC ${zlib_LIBRARIES})
```

Add some zlib-using code to `hello.cpp`
```
// hello.cpp

#include <boost/program_options.hpp>
#include <zlib.h>
#include <iostream>

namespace po = boost::program_options;
using namespace std;

int
main(int argc, char * argv[]) {
    po::options_description po_descr{"Options"};
    po_descr.add_options()
        ("help,h",
         "this help")
        ("subject,s",
         po::value<string>()->default_value("world"),
         "say hello to this subject")
        ("compress,z",
         "compress hello output using zlib")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, po_descr), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cerr << po_descr << endl;
    } else {
        stringstream ss;
        ss << "Hello, " << vm["subject"].as<string>() << "!\n" << endl;

        if (vm.count("compress")) {
            /* compress output */

            string s = ss.str();
            std::vector<uint8_t> og_data_v(s.begin(), s.end());

            /* required input space for zlib is (1.01 * input size) + 12;
             * add +1 byte to avoid thinking about rounding
             */
            uint64_t z_data_z = (1.01 * og_data_v.size()) + 12 + 1;
            uint8_t * z_data = reinterpret_cast<uint8_t *>(::malloc(z_data_z));
            int32_t zresult = ::compress(z_data,
                                         &z_data_z,
                                         og_data_v.data(),
                                         og_data_v.size());

            switch (zresult) {
            case Z_OK:
                break;
            case Z_MEM_ERROR:
                throw std::runtime_error("zlib.compress: out of memory");
            case Z_BUF_ERROR:
                throw std::runtime_error("zlib.compress: output buffer too small");
            }

            cout << "original   size:" << og_data_v.size() << endl;
            cout << "compressed size:" << z_data_z << endl;
            cout << "compressed data:";
            for (uint64_t i = 0; i < z_data_z; ++i) {
                uint8_t zi = z_data[i];
                uint8_t hi = (zi >> 4);          // hi 4 bits of zi
                uint8_t lo = (zi & 0x0f);        // lo 4 bits of zi

                char hi_ch = (hi < 10) ? '0' + hi : 'a' + hi - 10;
                char lo_ch = (lo < 10) ? '0' + lo : 'a' + lo - 10;

                cout << ' ' << hi_ch << lo_ch;   // print as hex
            }
            cout << endl;
        } else {
            cout << ss.str();
        }
    }
}
```

invoke build:
```
$ cd cmake-examples
$ git checkout ex4
$ mkdir -p build
$ ln -s build/compile_commands.json
$ cmake -B build
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
```

use executable (compression working as well as can be expected on such short input)
```
$ ./build/hello --compress --hex --subject "all the lonely people"
original   size:31
compressed size:39
compressed data: 78 9c f3 48 cd c9 c9 d7 51 48 cc c9 51 28 c9 48 55 c8 c9 cf 4b cd a9 54 28 48 cd 2f c8 49 55 e4 e2 02 00 ad 97 0a 68
```

## Example 5

This example is a preparatory refactoring step:  we refactor our inline compression code to a separate translation unit;
and while we're at it,  implement the reverse (inflation) operation.

Add to `CMakeLists.txt`:
1. new translation unit `compression.cpp`:
   `set(SELF_SRCS hello.cpp compression.cpp)`

```
# CMakeLists.txt
cmake_minimum_required(VERSION 3.25)
project(ex1 VERSION 1.0)
enable_language(CXX)

...

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "generate build/compile_commands.json")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

find_package(boost_program_options CONFIG REQUIRED)
find_package(PkgConfig)
pkg_check_modules(zlib REQUIRED zlib)

set(SELF_EXE hello)
set(SELF_SRCS hello.cpp compression.cpp)

add_executable(${SELF_EXE} ${SELF_SRCS})
target_compile_options(${SELF_EXE} PUBLIC ${zlib_CFLAGS_OTHER})
target_include_directories(${SELF_EXE} PUBLIC ${zlib_INCLUDE_DIRS})
target_link_libraries(${SELF_EXE} PUBLIC Boost::program_options)
target_link_libraries(${SELF_EXE} PUBLIC ${zlib_LIBRARIES})
```

New translation unit + header

```
// compression.hpp

#pragma once

#include <vector>
#include <cstdint>

/* thanks to Bobobobo's blog for zlib introduction
 *   [[https://bobobobo.wordpress.comp/2008/02/23/how-to-use-zlib]]
 * also
 *   [[https://zlib.net/zlib_how.html]]
 */
struct compression {
    /* compress contents of og_data_v[],  return compressed data */
    static std::vector<std::uint8_t> deflate(std::vector<std::uint8_t> const & og_data_v);
    /* uncompress contents of z_data_v[],  return uncompressed data.
     * caller expected to remember original uncompressed size + supply in og_data_z,
     * (or supply a sufficiently-large value)
     */
    static std::vector<std::uint8_t> inflate(std::vector<std::uint8_t> const & z_data_v,
                                             std::uint64_t og_data_z);
};
```

```
// compression.cpp

#include "compression.hpp"
#include <zlib.h>
#include <stdexcept>

using namespace std;

vector<uint8_t>
compression::deflate(std::vector<uint8_t> const & og_data_v)
{
    /* required input space for zlib is (1.01 * input size) + 12;
     * add +1 byte to avoid thinking about rounding
     */
    uint64_t z_data_z = (1.01 * og_data_v.size()) + 12 + 1;

    vector<uint8_t> z_data_v(z_data_z);

    int32_t zresult = ::compress(z_data_v.data(),
                                 &z_data_z,
                                 og_data_v.data(),
                                 og_data_v.size());

    switch (zresult) {
    case Z_OK:
        break;
    case Z_MEM_ERROR:
        throw std::runtime_error("compression::deflate: out of memory");
    case Z_BUF_ERROR:
        throw std::runtime_error("compression::deflate: output buffer too small");
    }

    return z_data_v;
}

vector<uint8_t>
compression::inflate(vector<uint8_t> const & z_data_v,
                     uint64_t og_data_z)
{
    vector<uint8_t> og_data_v(og_data_z);

    int32_t zresult = ::uncompress(og_data_v.data(),
                                   &og_data_z,
                                   z_data_v.data(),
                                   z_data_v.size());

    switch (zresult) {
    case Z_OK:
        break;
    case Z_MEM_ERROR:
        throw std::runtime_error("compression::inflate: out of memory");
    case Z_BUF_ERROR:
        throw std::runtime_error("compression::inflate: output buffer too small");
    }

    og_data_v.resize(og_data_z);

    return og_data_v;
} /*inflate*/
```

To invoke build:
```
$ cd cmake-examples
$ git checkout ex5
$ mkdir -p build
$ ln -s build/compile_commands.json
$ cmake -B build
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
[ 33%] Building CXX object CMakeFiles/hello.dir/hello.cpp.o
[ 66%] Linking CXX executable hello
[100%] Built target hello
```

exercise executable
```
$ ./build/hello --compress --hex --subject "all the lonely people"
original   size:31
compressed size:39
compressed data: 78 9c f3 48 cd c9 c9 d7 51 48 cc c9 51 28 c9 48 55 c8 c9 cf 4b cd a9 54 28 48 cd 2f c8 49 55 e4 e2 02 00 ad 97 0a 68
```

## Example 6

Add an install target.   This is a bit of a placeholder,  we'll need to expand on this later.

Add to `CMakeLists.txt`:
```
install(TARGETS ${SELF_EXE}
  RUNTIME DESTINATION bin COMPONENT Runtime
  BUNDLE DESTINATION bin COMPONENT Runtime)
```
My understanding is that the BUNDLE line does something useful on MacOS,  and is otherwise harmless.

Full `CMakeLists.txt` is now:
```
cmake_minimum_required(VERSION 3.25)
project(ex1 VERSION 1.0)
enable_language(CXX)

...

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "generate build/compile_commands.json")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

find_package(boost_program_options CONFIG REQUIRED)
find_package(PkgConfig)
pkg_check_modules(zlib REQUIRED zlib)

set(SELF_EXE hello)
set(SELF_SRCS hello.cpp compression.cpp)

add_executable(${SELF_EXE} ${SELF_SRCS})
target_compile_options(${SELF_EXE} PUBLIC ${zlib_CFLAGS_OTHER})
target_include_directories(${SELF_EXE} PUBLIC ${zlib_INCLUDE_DIRS})
target_link_libraries(${SELF_EXE} PUBLIC Boost::program_options)
target_link_libraries(${SELF_EXE} PUBLIC ${zlib_LIBRARIES})
install(TARGETS ${SELF_EXE}
    RUNTIME DESTINATION bin COMPONENT Runtime
    BUNDLE DESTINATION bin COMPONENT Runtime)
```

To install to `~/scratch`:
```
$ PREFIX=$HOME/scratch
$ mkdir -p $PREFIX
$ cd cmake-examples
$ mkdir -p build
$ cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -B build
...
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
[ 33%] Building CXX object CMakeFiles/hello.dir/hello.cpp.o
[ 66%] Building CXX object CMakeFiles/hello.dir/compression.cpp.o
[100%] Linking CXX executable hello
[100%] Built target hello
$ cmake --install build
-- Install configuration: ""
-- Installing: /home/roland/scratch/bin/hello
-- Set runtime path of "/home/roland/scratch/bin/hello" to ""
$ tree $PREFIX
/home/roland/scratch
`-- bin
    `-- hello

1 directory, 1 file
$ $PREFIX/bin/hello
Hello, world!
$
```

## Example 7

Refactor to move compression code to a separately-installed library.
This allows reusing our compression wrapper from some other executable.

This involves multiple steps:
1. create a dedicated subdirectory `compression/` to hold wrapper code,
   i.e. `compression.cpp` and `compression.hpp`
   (this isn't essential,  but good practice to allow for project growth)
2. tell cmake to build new library `libcompression.so`;  instructions go in `compression/CMakeLists.txt`
3. connect `compression/` subdirectory to the top-level `CMakeLists.txt` and simplify.

```
$ git checkout ex7
```

Library build:
```
#compression/CmakeLists.txt
set(SELF_LIB compression)
set(SELF_SRCS compression.cpp)
set(SELF_VERSION 2)
set(SELF_SOVERSION 2.3)

add_library(${SELF_LIB} SHARED ${SELF_SRCS})
set_target_properties(${SELF_LIB} PROPERTIES VERSION ${SELF_VERSION} SOVERSION ${SELF_SOVERSION})
target_include_directories(${SELF_LIB} PUBLIC
    $<INSTALL_INTERFACE:include/compression>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/compression>)

target_compile_options(${SELF_LIB} PRIVATE ${zlib_CFLAGS_OTHER})
target_include_directories(${SELF_LIB} PRIVATE ${zlib_INCLUDE_DIRS})
target_link_libraries(${SELF_LIB} PRIVATE ${zlib_LIBRARIES})

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/compression
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/compression)

install(
    TARGETS ${SELF_LIB}
    LIBRARY DESTINATION lib COMPONENT Runtime
    ARCHIVE DESTINATION lib COMPONENT Development
    PUBLIC_HEADER DESTINATION include COMPONENT Development)
```

Top-level build:
```
# cmake-examples/CMakeLists.txt
cmake_minimum_required(VERSION 3.25)
project(ex1 VERSION 1.0)
enable_language(CXX)

...

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "generate build/compile_commands.json")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if(NOT CMAKE_INSTALL_RPATH)
    set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib CACHE STRING
        "runpath in installed libraries/executables")
endif()

find_package(boost_program_options CONFIG REQUIRED)
find_package(PkgConfig)
pkg_check_modules(zlib REQUIRED zlib)

add_subdirectory(compression)

set(SELF_EXE hello)
set(SELF_SRCS hello.cpp)

add_executable(${SELF_EXE} ${SELF_SRCS})
target_include_directories(${SELF_EXE} PUBLIC compression/include)
target_link_libraries(${SELF_EXE} PUBLIC compression)
target_link_libraries(${SELF_EXE} PUBLIC Boost::program_options)

install(TARGETS ${SELF_EXE}
    RUNTIME DESTINATION bin COMPONENT Runtime
    BUNDLE DESTINATION bin COMPONENT Runtime)
```

Remarks:
1. we have a separate `install` instruction for `libcompression.so`;
   need it to install to `$PREFIX/lib` instead of `$PREFIX/bin`.

2. installed executables (i.e. `hello`) need to be able to use libraries (i.e. `libcompression.so`) in `$PREFIX/lib`.
   This is accomplished by setting `CMAKE_INSTALL_RPATH` in toplevel `CMakeLists.txt`

3. `compression/CMakeLists.txt` isn't self-sufficient;
   it only works as a satellite of `cmake-examples/CMakeLists.txt`.
   For example,  it relies on top-level `CMakeLists.txt` to establish zlib-specific cmake variables.

   We'd expect this to lead to maintenance problems in a project with many dependencies,
   since we're creating distance between introduction and use of these dependency-specific cmake variables.

4. If we imagine writing multiple libraries,  we're writing a dozen+ lines of boilerplate for each library;
   we'll want to work to shrink this.

5. We put compression `.hpp` header files in their own directory `compression/include`,  separate from `.cpp` files,
   because we want to install the headers along with their associated library.
   We're installing `.hpp` files to kitchen-sink `$PREFIX/include` directory;  will likely want to send to a library-specific
   subdirectory instead,  to make life easier for downstream projects that want to cherry-pick.

6. Although the compression library relies on `zlib`, `zlib.h` does not appear in `compression.hpp`;
   so we mark the compression->zlib dependency `PRIVATE` for now.

Details:
1.
in top-level `CMakeLists.txt`,  we added the line
```
target_include_directories(${SELF_EXE} PUBLIC compression/include)
```
so that in `hello.cpp` we can write
```
#include "compression.hpp"
```
instead of
```
#include "compression/include/compression.hpp"
```

2.
This line in `compression/CMakeLists.txt`:
```
target_include_directories(${SELF_LIB} PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
```
helps with mapping build-time behavior to install-time behavior.
When compiling within build tree,  compiler should receive an argument like `-Ipath/to/source/compression/include`,
referring to `.hpp` files in the source tree.
Post-install, software that uses the compression library would instead need to have flag like `-I$PREFIX/include/compression`.
When we extend build to publish cmake support files with installed compression library,
that support will have to make this distinction.  For now it's a formality.

Build + install:
```
$ PREFIX=/home/roland/scratch
$ cd cmake-examples
$ cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -B build
...
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
[ 25%] Building CXX object compression/CMakeFiles/compression.dir/compression.cpp.o
[ 50%] Linking CXX shared library libcompression.so
[ 50%] Built target compression
[ 75%] Building CXX object CMakeFiles/hello.dir/hello.cpp.o
[100%] Linking CXX executable hello
[100%] Built target hello
$ cmake --install build
-- Install configuration: ""
-- Installing: /home/roland/scratch/include/compression/compression
-- Installing: /home/roland/scratch/include/compression/compression/compression.hpp
-- Installing: /home/roland/scratch/lib/libcompression.so.2
-- Installing: /home/roland/scratch/lib/libcompression.so.2.3
-- Installing: /home/roland/scratch/lib/libcompression.so
-- Installing: /home/roland/scratch/bin/hello
-- Set runtime path of "/home/roland/scratch/bin/hello" to ""
$ tree ~/scratch
/home/roland/scratch
|-- bin
|   `-- hello
|-- include
|   `-- compression
|       `-- compression.hpp
`-- lib
    |-- libcompression.so -> libcompression.so.2.3
    |-- libcompression.so.2
    `-- libcompression.so.2.3 -> libcompression.so.2

4 directories, 5 files
```

# Example 8

Refactor to move executable `hello` to its own subdirectory,
so organization is clearer when we have more than one executable.

visit branch:
```
$ cd cmake-examples
$ git checkout ex8
```

source tree:
```
$ tree
.
|-- CMakeLists.txt
|-- LICENSE
|-- README.md
|-- app
|   `-- hello
|       |-- CMakeLists.txt
|       `-- hello.cpp
|-- compile_commands.json
`-- compression
    |-- CMakeLists.txt
    |-- compression.cpp
    `-- include
        `-- compression
            `-- compression.hpp

5 directories, 9 files
```

Top-level `CMakeLists.txt`:
```
# cmake-examples/CMakeLists.txt
cmake_minimum_required(VERSION 3.25)
project(ex8 VERSION 1.0)
enable_language(CXX)

...

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "generate build/compile_commands.json")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

if(NOT CMAKE_INSTALL_RPATH)
    set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib CACHE STRING
        "runpath in installed libraries/executables")
endif()

find_package(boost_program_options CONFIG REQUIRED)
find_package(PkgConfig)
pkg_check_modules(zlib REQUIRED zlib)

add_subdirectory(compression)
add_subdirectory(app/hello)
```

Cmake code moved from `cmake-examples/CMakeLists.txt` to new destination `cmake-examples/app/hello/CMakeLists.txt`:
```
# app/hello/CMakeLists.txt
set(SELF_EXE hello)
set(SELF_SRCS hello.cpp)

add_executable(${SELF_EXE} ${SELF_SRCS})
target_include_directories(${SELF_EXE} PUBLIC ${PROJECT_SOURCE_DIR}/compression/include)
target_link_libraries(${SELF_EXE} PUBLIC compression)
target_link_libraries(${SELF_EXE} PUBLIC Boost::program_options)

install(TARGETS ${SELF_EXE}
    RUNTIME DESTINATION bin COMPONENT Runtime
    BUNDLE DESTINATION bin COMPONENT Runtime)
```

Build + install:
```
$ PREFIX=/home/roland/scratch
$ cd cmake-examples
$ cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -B build
...
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
[ 25%] Building CXX object compression/CMakeFiles/compression.dir/compression.cpp.o
[ 50%] Linking CXX shared library libcompression.so
[ 50%] Built target compression
[ 75%] Building CXX object app/hello/CMakeFiles/hello.dir/hello.cpp.o
[100%] Linking CXX executable hello
[100%] Built target hello
$ cmake --install build
-- Install configuration: ""
-- Installing: /home/roland/scratch/include/compression
-- Installing: /home/roland/scratch/include/compression/compression.hpp
-- Installing: /home/roland/scratch/lib/libcompression.so.2
-- Installing: /home/roland/scratch/lib/libcompression.so.2.3
-- Set runtime path of "/home/roland/scratch/lib/libcompression.so.2" to "/home/roland/scratch/lib"
-- Installing: /home/roland/scratch/lib/libcompression.so
-- Installing: /home/roland/scratch/bin/hello
-- Set runtime path of "/home/roland/scratch/bin/hello" to "/home/roland/scratch/lib"
$ tree $PREFIX
/home/roland/scratch
|-- bin
|   `-- hello
|-- include
|   `-- compression
|       `-- compression.hpp
`-- lib
    |-- libcompression.so -> libcompression.so.2.3
    |-- libcompression.so.2
    `-- libcompression.so.2.3 -> libcompression.so.2

4 directories, 5 files
```

## Example 9

Add a (extremely naive) second application (`myzip`) to compress/uncompress files
(amongst other problems, won't work on files > 1M).
We put up with this to focus on the cmake build

```
$ cd cmake-examples
$ git checkout ex9
```

source tree:
```
.
|-- CMakeLists.txt
|-- LICENSE
|-- README.md
|-- app
|   |-- hello
|   |   |-- CMakeLists.txt
|   |   `-- hello.cpp
|   `-- myzip
|       |-- CMakeLists.txt
|       `-- myzip.cpp
|-- compile_commands.json
`-- compression
    |-- CMakeLists.txt
    |-- compression.cpp
    `-- include
        `-- compression
            |-- compression.hpp
            `-- tostr.hpp

6 directories, 12 files
```

Changes:
1. in top-level `CMakeLists.txt` add:
   ```
   # cmake-examples/CMakeLists.txt
   add_subdirectory(app/myzip)
   ```

2. add satellite .cmake file `app/myzip/CMakeLists.txt`, similar to `app/hello/CMakeLists.txt`:
   ```
   set(SELF_EXE myzip)
   set(SELF_SRCS myzip.cpp)

   add_executable(${SELF_EXE} ${SELF_SRCS})
   target_include_directories(${SELF_EXE} PUBLIC ${PROJECT_SOURCE_DIR}/compression/include)
   target_link_libraries(${SELF_EXE} PUBLIC compression)
   target_link_libraries(${SELF_EXE} PUBLIC Boost::program_options)

   install(TARGETS ${SELF_EXE}
       RUNTIME DESTINATION bin COMPONENT Runtime
       BUNDLE DESTINATION bin COMPONENT Runtime)
   ```

3. add utility header `compression/include/compression/tostr.hpp`:
   ```
   // tostr.hpp

   #pragma once

   #include <sstream>

   /*   tostr(x1, x2, ...)
    *
    * is shorthand for something like:
    *
    *   {
    *     stringstream s;
    *     s << x1 << x2 << ...;
    *     return s.str();
    *   }
    */


   template<class Stream>
   Stream & tos(Stream & s) { return s; }

   template <class Stream, typename T>
   Stream & tos(Stream & s, T && x) { s << x; return s; }

   template <class Stream, typename T1, typename... Tn>
   Stream & tos(Stream & s, T1 && x, Tn && ...rest) {
       s << x;
       return tos(s, rest...);
   }

   template <typename... Tn>
   std::string tostr(Tn && ...args) {
       std::stringstream ss;
       tos(ss, args...);
       return ss.str();
   }
   ```

4. in `compression.hpp` and `compression.cpp` add methods `inflate_file()` and `deflate_file()`:

   ```
   # compression.hpp
   /* compress file with path .in_file,  putting output in .out_file */
   static void inflate_file(std::string const & in_file,
                            std::string const & out_file,
                            bool keep_flag = true,
                            bool verbose_flag = false);

   /* uncompress file with path .in_file,  putting uncompressed output in .out_file */
   static void deflate_file(std::string const & in_file,
                            std::string const & out_file,
                            bool keep_flag = true,
                            bool verbose_flag = false);
   ```

   ```
   # compression.cpp
   void
   compression::inflate_file(std::string const & in_file,
                             std::string const & out_file,
                             bool keep_flag,
                             bool verbose_flag)
   {
       /* check output doesn't exist already */
       if (ifstream(out_file, ios::binary|ios::in))
           throw std::runtime_error(tostr("output file [", out_file, "] already exists"));

       if (verbose_flag)
           cerr << "compress::inflate_file: will compress [" << in_file << "] -> [" << out_file << "]" << endl;

       /* open target file (start at end) */
       ifstream fs(in_file, ios::binary|ios::ate);
       if (!fs)
           throw std::runtime_error(tostr("unable to open input file [", in_file, "]"));

       auto z = fs.tellg();

       /* read file content into memory */
       if (verbose_flag)
           cerr << "compress::inflate_file: read " << z << " bytes from [" << in_file << "] into memory" << endl;

       vector<uint8_t> fs_data_v(z);
       fs.seekg(0);
       if (!fs.read(reinterpret_cast<char *>(&fs_data_v[0]), z))
           throw std::runtime_error(tostr("unable to read contents of input file [", in_file, "]"));

       vector<uint8_t> z_data_v = compression::deflate(fs_data_v);

       /* write compresseed output */
       ofstream zfs(out_file, ios::out|ios::binary);
       zfs.write(reinterpret_cast<char *>(&(z_data_v[0])), z_data_v.size());

       if (!zfs.good())
           throw std::runtime_error(tostr("failed to write ", z_data_v.size(), " bytes to [", out_file, "]"));

       /* control here only if successfully wrote uncompressed output */
       if (!keep_flag)
           remove(in_file.c_str());
   } /*inflate_file*/

   void
   compression::deflate_file(std::string const & in_file,
                             std::string const & out_file,
                             bool keep_flag,
                             bool verbose_flag)
   {
       /* check output doesn't exist already */
       if (ifstream(out_file, ios::binary|ios::in))
           throw std::runtime_error(tostr("output file [", out_file, "] already exists"));

       if (verbose_flag)
           cerr << "compression::deflate_file will uncompress [" << in_file << "] -> [" << out_file << "]" << endl;

       /* open target file (start at end) */
       ifstream fs(in_file, ios::binary|ios::ate);
       if (!fs)
           throw std::runtime_error("unable to open input file");

       auto z = fs.tellg();

       /* read file contents into memory */
       if (verbose_flag)
           cerr << "compression::deflate_file: read " << z << " bytes from [" << in_file << "] into memory" << endl;

       vector<uint8_t> fs_data_v(z);
       fs.seekg(0);
       if (!fs.read(reinterpret_cast<char *>(&fs_data_v[0]), z))
           throw std::runtime_error(tostr("unable to read contents of input file [", in_file, "]"));

       /* uncompress */
       vector<uint8_t> og_data_v = compression::inflate(fs_data_v, 999999);

       /* write uncompressed output */
       ofstream ogfs(out_file, ios::out|ios::binary);
       ogfs.write(reinterpret_cast<char *>(&(og_data_v[0])), og_data_v.size());

       if (!ogfs.good())
           throw std::runtime_error(tostr("failed to write ", og_data_v.size(), " bytes to [", out_file, "]"));

       if (!keep_flag)
           remove(in_file.c_str());
   } /*deflate_file*/
   ```

5. add `app/myzip/myzip.cpp` application main
   ```
   // myzip.cpp

   #include "compression.hpp"

   #include <boost/program_options.hpp>
   #include <zlib.h>
   #include <iostream>
   #include <fstream>

   namespace po = boost::program_options;
   using namespace std;

   int
   main(int argc, char * argv[]) {
       po::options_description po_descr{"Options"};
       po_descr.add_options()
           ("help,h",
            "this help")
           ("keep,k",
            "keep input files instead of deleting them")
           ("verbose,v",
            "enable to report progress messages to stderr")
           ("input-file",
            po::value<vector<string>>(),
            "input file(s) to compress/uncompress")
           ;

       po::variables_map vm;

       po::positional_options_description po_pos_args;
       po_pos_args.add("input-file", -1);
       po::store(po::command_line_parser(argc, argv)
                 .options(po_descr)
                 .positional(po_pos_args)
                 .run(),
                 vm);
       po::notify(vm);

       bool keep_flag = vm.count("keep");
       bool verbose_flag = vm.count("verbose");

       try {
           if (vm.count("help")) {
               cerr << po_descr << endl;
           } else {
               vector<string> input_file_l = vm["input-file"].as<vector<string>>();

               for (string const & fname : input_file_l) {
                   if (verbose_flag)
                       cerr << "myzip: consider file [" << fname << "]" << endl;

                   constexpr int32_t sfx_z = 3;

                   if ((fname.size() > sfx_z) && (fname.substr(fname.size() - sfx_z, sfx_z) == ".mz")) {
                       /* uncompress */

                       string fname_mz = fname;
                       string fname = fname_mz.substr(0, fname_mz.size() - sfx_z);

                       compression::deflate_file(fname_mz, fname, keep_flag, verbose_flag);
                   } else {
                       /* compress */
                       string fname_mz = fname + ".mz";

                       compression::inflate_file(fname, fname_mz, keep_flag, verbose_flag);
                   }
               }
           }
       } catch(exception & ex) {
           cerr << "error: myzip: " << ex.what() << endl;
           return 1;
       }

       return 0;
   }
   ```

Build + install:
```
$ PREFIX=/home/roland/scratch
$ cd cmake-examples
$ cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -B build
...
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
[ 16%] Building CXX object compression/CMakeFiles/compression.dir/compression.cpp.o
[ 33%] Linking CXX shared library libcompression.so
[ 33%] Built target compression
[ 50%] Building CXX object app/hello/CMakeFiles/hello.dir/hello.cpp.o
[ 66%] Linking CXX executable hello
[ 66%] Built target hello
[ 83%] Building CXX object app/myzip/CMakeFiles/myzip.dir/myzip.cpp.o
[100%] Linking CXX executable myzip
[100%] Built target myzip
$ cmake --install build
-- Install configuration: ""
-- Installing: /home/roland/scratch/include/compression
-- Installing: /home/roland/scratch/include/compression/tostr.hpp
-- Installing: /home/roland/scratch/include/compression/compression.hpp
-- Installing: /home/roland/scratch/lib/libcompression.so.2
-- Installing: /home/roland/scratch/lib/libcompression.so.2.3
-- Set runtime path of "/home/roland/scratch/lib/libcompression.so.2" to "/home/roland/scratch/lib"
-- Installing: /home/roland/scratch/lib/libcompression.so
-- Installing: /home/roland/scratch/bin/hello
-- Set runtime path of "/home/roland/scratch/bin/hello" to "/home/roland/scratch/lib"
-- Installing: /home/roland/scratch/bin/myzip
-- Set runtime path of "/home/roland/scratch/bin/myzip" to "/home/roland/scratch/lib"
$ tree ~/scratch
/home/roland/scratch
|-- bin
|   |-- hello
|   `-- myzip
|-- include
|   `-- compression
|       |-- compression.hpp
|       `-- tostr.hpp
`-- lib
    |-- libcompression.so -> libcompression.so.2.3
    |-- libcompression.so.2
    `-- libcompression.so.2.3 -> libcompression.so.2

4 directories, 7 files
```

## Example 10

Add a c++ unit test,  using the `catch2` (header-only) library.

```
$ cd cmake-examples
$ git checkout ex10
```

source tree:
```
.
|-- CMakeLists.txt
|-- LICENSE
|-- README.md
|-- app
|   |-- hello
|   |   |-- CMakeLists.txt
|   |   `-- hello.cpp
|   `-- myzip
|       |-- CMakeLists.txt
|       `-- myzip.cpp
|-- compile_commands.json
`-- compression
    |-- CMakeLists.txt
    |-- compression.cpp
    |-- include
    |   `-- compression
    |       |-- compression.hpp
    |       `-- tostr.hpp
    `-- utest
        |-- CMakeLists.txt
        |-- compression.test.cpp
        `-- compression_utest_main.cpp

7 directories, 15 files
```

Changes:
1. in top-level `CMakeLists.txt`:
   First, add
   ```
   enable_testing()
   ```
   to active cmake's `ctest` feature

   Second, add
   ```
   find_package(Catch2 CONFIG REQUIRED)
   ```
   to invoke cmake support provided by the `catch2` library

   Third, add
   ```
   add_subdirectory(compression/utest)
   ```
   to use new `compression/utest/CMakeLists.txt`

2. cmake instructions for new unit test executable `compression/utest/utest.compression`):
   ```
   # compression/utest/CMakeLists.txt

   set(SELF_UTEST utest.compression)
   set(SELF_SRCS compression_utest_main.cpp compression.test.cpp)

   add_executable(${SELF_UTEST} ${SELF_SRCS})
   target_link_libraries(${SELF_UTEST} PUBLIC compression)
   target_link_libraries(${SELF_UTEST} PUBLIC Catch2::Catch2)

   add_test(
       NAME ${SELF_UTEST}
       COMMAND ${SELF_UTEST})
   ```

   As far as cmake is concerned,  the `add_test()` call introduces a native c++ unit test executable.
   Otherwise `CMakeLists.txt` look like that for a regular non-unit-test executable,
   except that we omit install instructions since we choose not to install unit tests.

3. Use catch2-provided main:
   ```
   # compression/utest/compression_utest_main.cpp

   #define CATCH_CONFIG_MAIN
   #include "catch2/catch.hpp"
   ```

4. Provide unit test implementation:
   ```
   // compression/utest/compression.test.cpp

   #include "compression.hpp"
   #include "tostr.hpp"
   #include <catch2/catch.hpp>
   #include <vector>
   #include <string>

   using namespace std;

   namespace {
       struct TestCase {
           explicit TestCase(string const & og_text_arg) : og_text{og_text_arg} {}

           string og_text;
       };

       static vector<TestCase> s_testcase_v = {
           TestCase("The quick brown fox jumps over the lazy dog"),
           TestCase("A man, a plan, a canal - Panama!")
       };
   }

   TEST_CASE("compression", "[compression]") {
       for (size_t i_tc = 0, n_tc = s_testcase_v.size(); i_tc < n_tc; ++i_tc) {
           TestCase const & tcase = s_testcase_v[i_tc];

           INFO(tostr("test case [", i_tc, "]: og_text [", tcase.og_text, "]"));

           uint32_t og_data_z = tcase.og_text.size();
           vector<uint8_t> og_data_v(tcase.og_text.data(),
                                     tcase.og_text.data() + og_data_z);
           vector<uint8_t> z_data_v = compression::deflate(og_data_v);
           vector<uint8_t> og_data2_v = compression::inflate(z_data_v, og_data_z);

           /* verify deflate->inflate recovers original text */
           REQUIRE(og_data_v == og_data2_v);
       }
   }
   ```

Build:
```
$ PREFIX=/home/roland/scratch
$ cd cmake-examples
$ cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -B build
...
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
[ 11%] Building CXX object compression/CMakeFiles/compression.dir/compression.cpp.o
[ 22%] Linking CXX shared library libcompression.so
[ 22%] Built target compression
[ 33%] Building CXX object compression/utest/CMakeFiles/utest.compression.dir/compression_utest_main.cpp.o
[ 44%] Building CXX object compression/utest/CMakeFiles/utest.compression.dir/compression.test.cpp.o
[ 55%] Linking CXX executable utest.compression
[ 55%] Built target utest.compression
[ 66%] Building CXX object app/hello/CMakeFiles/hello.dir/hello.cpp.o
[ 77%] Linking CXX executable hello
[ 77%] Built target hello
[ 88%] Building CXX object app/myzip/CMakeFiles/myzip.dir/myzip.cpp.o
[100%] Linking CXX executable myzip
[100%] Built target myzip
```

Run unit test directly:
```
$ ./build/compression/utest/utest.compression
===============================================================================
All tests passed (2 assertions in 1 test case)

```

Have `ctest` run our unit test (along with any other tests attached to cmake via `add_test()`):
```
$ (cd build && ctest)
Test project /home/roland/proj/cmake-examples/build
    Start 1: utest.compression
1/1 Test #1: utest.compression ................   Passed    0.00 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.00 sec
```

## Example 11

Add a bash unit test,  to exercise the `myzip` app.

```
$ cd cmake-examples
$ git checkout ex11
```

source tree:
```
$ tree
.
|-- CMakeLists.txt
|-- LICENSE
|-- README.md
|-- app
|   |-- hello
|   |   |-- CMakeLists.txt
|   |   `-- hello.cpp
|   `-- myzip
|       |-- CMakeLists.txt
|       |-- myzip.cpp
|       `-- utest
|           |-- CMakeLists.txt
|           |-- myzip.utest
|           `-- textfile
|-- compile_commands.json
`-- compression
    |-- CMakeLists.txt
    |-- compression.cpp
    |-- include
    |   `-- compression
    |       |-- compression.hpp
    |       `-- tostr.hpp
    `-- utest
        |-- CMakeLists.txt
        |-- compression.test.cpp
        `-- compression_utest_main.cpp

8 directories, 18 files
```

Changes:
1. in top-level `CMakeLists.txt`:
   First,  get location of `bash` executable:
   ```
   find_program(BASH_EXECUTABLE NAMES bash REQUIRED)
   ```

   Second,  add new unit test directory:
   ```
   add_subdirectory(app/myzip/utest)
   ```

   Note that `myzip/utest/CMakeLists.txt` must follow `myzip/CMakeLists.txt`,
   because it relies on the `myzip` target established in the latter file.

2. add `.cmake` file for unit test
   ```
   # app/myzip/utest/CMakeLists.txt

   set(SELF_UTEST myzip.utest)

   add_test(
       NAME ${SELF_UTEST}
       COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${SELF_UTEST} $<TARGET_FILE:myzip> ${CMAKE_CURRENT_SOURCE_DIR}/textfile)
   ```

3. add a test file `app/myzip/utest/textfile` to compress/uncompress:
   ```
   Jabberwocky,  by Lewis Carroll

   'Twas brillig, and the slithy toves
   ...
   ```

4. add bash script `app/myzip/utest/utest.myzip` implementing unit test
   ```
   #!/bin/bash

   myzip=$1
   file_path=$2
   file=$(basename ${file_path})
   file_mz=${file}.mz
   file2=${file}2
   file2_mz=${file2}.mz

   rm -f ${file}
   rm -f ${file_mz}
   rm -f ${file2}
   rm -f ${file2_mz}

   #echo "myzip=${myzip}"
   #echo "file_path=${file_path}"
   #echo "file=${file}"
   #echo "file_mz=${file_mz}"

   cp ${file_path} ${file}

   # deflate ${file} -> ${file_mz}
   ${myzip} --keep ${file}

   if [[ ! -f ${file_mz} ]]; then
       >&2 echo "expected [${file_mz}] created\n"
       exit 1
   fi

   cp ${file_mz} ${file2_mz}

   # inflate ${file2_mz} back to ${file2}
   ${myzip} ${file2_mz}

   if [[ ! -f ${file2} ]]; then
       >&2 echo "expected [${file2}] created\n"
       exit 1
   fi

   diff ${file} ${file2}
   err=$?

   if [[ $err -ne 0 ]]; then
       >&2 echo "expected [${file}] and [${file2}] to be identical"
       exit 1
   fi

   # control here: unit test successful
   ```

Build:
```
$ cd cmake-examples
$ cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -B build
...
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
[ 11%] Building CXX object compression/CMakeFiles/compression.dir/compression.cpp.o
[ 22%] Linking CXX shared library libcompression.so
[ 22%] Built target compression
[ 33%] Building CXX object compression/utest/CMakeFiles/utest.compression.dir/compression_utest_main.cpp.o
[ 44%] Building CXX object compression/utest/CMakeFiles/utest.compression.dir/compression.test.cpp.o
[ 55%] Linking CXX executable utest.compression
[ 55%] Built target utest.compression
[ 66%] Building CXX object app/hello/CMakeFiles/hello.dir/hello.cpp.o
[ 77%] Linking CXX executable hello
[ 77%] Built target hello
[ 88%] Building CXX object app/myzip/CMakeFiles/myzip.dir/myzip.cpp.o
[100%] Linking CXX executable myzip
[100%] Built target myzip
```

Run unit tests:
```
$ (cd build && ctest)
Test project /home/roland/proj/cmake-examples/build
    Start 1: utest.compression
1/2 Test #1: utest.compression ................   Passed    0.00 sec
    Start 2: myzip.utest
2/2 Test #2: myzip.utest ......................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   0.01 sec
```

Can observe temporary files in `build/app/myzip/utest`:
```
$ ls -l build/app/myzip/utest
total 32
drwxr-xr-x 2 roland roland 4096 Dec  8 15:57 CMakeFiles
-rw-r--r-- 1 roland roland  806 Dec  8 15:57 CTestTestfile.cmake
-rw-r--r-- 1 roland roland 7322 Dec  8 15:57 Makefile
-rw-r--r-- 1 roland roland 1330 Dec  8 15:57 cmake_install.cmake
-rw-r--r-- 1 roland roland 1064 Dec  8 15:59 textfile
-rw-r--r-- 1 roland roland 1087 Dec  8 15:59 textfile.mz
-rw-r--r-- 1 roland roland 1064 Dec  8 15:59 textfile2
```

# Example 12

Rework to use incremental compression api (inflate/deflate).
This example is more "c++" than "cmake"

```
$ cd cmake-examples
$ git checkout ex12
```

source tree:
```
$ tree
.
|-- CMakeLists.txt
|-- LICENSE
|-- README.md
|-- app
|   |-- hello
|   |   |-- CMakeLists.txt
|   |   `-- hello.cpp
|   `-- myzip
|       |-- CMakeLists.txt
|       |-- myzip.cpp
|       `-- utest
|           |-- CMakeLists.txt
|           |-- myzip.utest
|           `-- textfile
|-- compile_commands.json -> build/compile_commands.json
`-- compression
    |-- CMakeLists.txt
    |-- compression.cpp
    |-- deflate_zstream.cpp
    |-- include
    |   `-- compression
    |       |-- base_zstream.hpp
    |       |-- buffer.hpp
    |       |-- buffered_deflate_zstream.hpp
    |       |-- buffered_inflate_zstream.hpp
    |       |-- compression.hpp
    |       |-- deflate_zstream.hpp
    |       |-- inflate_zstream.hpp
    |       |-- span.hpp
    |       `-- tostr.hpp
    |-- inflate_zstream.cpp
    `-- utest
        |-- CMakeLists.txt
        |-- compression.test.cpp
        `-- compression_utest_main.cpp

8 directories, 27 files
```

Changes:
1. template `span`, to represent a memory range without ownership.
2. template `buffer`, to represent a memory range with possible ownership.
3. base class `base_zstream`, wrapper for `z_stream` struct from `zlib.h`.
   `z_stream` supports incremental inflation/deflation (i.e. compress/decompress)
4. class `inflate_zstream`, provides inflation using `z_stream`
5. class `deflate_zstream`, provides deflation using `z_stream`
6. add new source files to `compression/CMakeLists.txt`
7. template `buffered_inflate_zstream`, attaches input+output buffers to `inflate_zstream`
8. template `buffered_deflate_zstream`, attaches input+output buffers to `deflate_zstream`
9. re-implement `compression::inflate_file` to use `buffered_inflate_zstream` and bounded memory.
10. re-implement `compression::deflate_file` to use `buffered_deflate_zstream` and bounded memory.

Details:
1. template `span`:

```
// compression/span.hpp

#pragma once

#include <cstdint>

/* A span of un-owned memory */
template <typename CharT>
class span {
public:
    using size_type = std::uint64_t;

public:
    span(CharT * lo, CharT * hi) : lo_{lo}, hi_{hi} {}

    /* cast with different element type.  Note this may change .size */
    template <typename OtherT>
    span<OtherT>
    cast() const { return span<OtherT>(reinterpret_cast<OtherT *>(lo_),
                                       reinterpret_cast<OtherT *>(hi_)); }

    span prefix(size_type z) const { return span(lo_, lo_ + z); }

    bool empty() const { return lo_ == hi_; }
    size_type size() const { return hi_ - lo_; }

    CharT * lo() const { return lo_; }
    CharT * hi() const { return hi_; }

private:
    CharT * lo_ = nullptr;
    CharT * hi_ = nullptr;
};
```

2. template `buffer`:

```
// buffer.hpp

#pragma once

#include "span.hpp"
#include <utility>
#include <cstdint>
#include <cassert>

/*
 *  .buf
 *
 *    +------------------------------------------+
 *    |  |  ...  |  | X|  ... | X|  |    ...  |  |
 *    +------------------------------------------+
 *     ^             ^            ^               ^
 *     0             .lo          .hi             .buf_z
 *
 *
 * buffer does not support wrapped content
 */
template <typename CharT>
class buffer {
public:
    using span_type = span<CharT>;
    using size_type = std::uint64_t;

public:
    buffer(size_type buf_z)
        : is_owner_{true}, lo_pos_{0}, hi_pos_{0}, buf_{new CharT [buf_z]}, buf_z_{buf_z} {}
    ~buffer() { this->clear(); }

    CharT * buf() const { return buf_; }
    size_type buf_z() const { return buf_z_; }
    size_type lo_pos() const { return lo_pos_; }
    size_type hi_pos() const { return hi_pos_; }

    CharT const & operator[](size_type i) const { return buf_[i]; }

    span_type contents() const { return span_type(buf_ + lo_pos_, buf_ + hi_pos_); }
    span_type avail() const { return span_type(buf_ + hi_pos_, buf_ + buf_z_); }

    bool empty() const { return lo_pos_ == hi_pos_; }

    void produce(span_type const & span) {
        assert(span.lo() == buf_ + hi_pos_);

        hi_pos_ += span.size();
    }

    void consume(span_type const & span) {
        if (span.size()) {
            assert(span.lo() == buf_ + lo_pos_);

            lo_pos_ += span.size();
        } else {
            /* since .consume() that arrives at empty contents also resets .lo_pos .hi_pos,
             * we don't want to blow up when called with an empty span -- argument
             * may represent some pre-reset location in buffer
             */
        }

        if (lo_pos_ == hi_pos_) {
            lo_pos_ = 0;
            hi_pos_ = 0;
        }
    }

    void setbuf(CharT * buf, size_type buf_z) {
        /* properly reset any existing state */
        this->clear();

        is_owner_ = false;
        lo_pos_ = 0;
        hi_pos_ = 0;
        buf_ = buf;
        buf_z_ = buf_z;
    }

    void swap (buffer & x) {
        std::swap(is_owner_, x.is_owner_);
        std::swap(buf_, x.buf_);
        std::swap(buf_z_, x.buf_z_);
        std::swap(lo_pos_, x.lo_pos_);
        std::swap(hi_pos_, x.hi_pos_);
    }

    void clear() {
        if (is_owner_)
            delete [] buf_;

        is_owner_ = false;
        buf_ = nullptr;
        buf_z_ = 0;
        lo_pos_ = 0;
        hi_pos_ = 0;
    }

    /* move-assignment */
    buffer & operator= (buffer && x) {
        is_owner_ = x.is_owner_;
        buf_ = x.buf_;
        buf_z_ = x.buf_z_;
        lo_pos_ = x.lo_pos_;
        hi_pos_ = x.hi_pos_;

        x.is_owner_ = false;
        x.lo_pos_ = 0;
        x.hi_pos_ = 0;
        x.buf_ = nullptr;
        x.buf_z_ = 0;

        return *this;
    }

private:
    bool is_owner_ = false;
    CharT * buf_ = nullptr;
    size_type buf_z_ = 0;

    /* buffer locations [.lo_pos .. .hi_pos) are occupied;
     * remainder is available space
     */
    size_type lo_pos_ = 0;
    size_type hi_pos_ = 0;
};

namespace std {
    template <typename CharT>
    inline void
    swap(buffer<CharT> & lhs, buffer<CharT> & rhs) {
        lhs.swap(rhs);
    }
}
```

3. class `base_zstream`:
```
// base_zstream.hpp

#pragma once

#include "span.hpp"
#include <zlib.h>
#include <ios>
#include <utility>
#include <cstring>

class base_zstream {
public:
    using span_type = span<std::uint8_t>;

public:
    bool input_empty() const { return (zstream_.avail_in == 0); }
    bool have_input() const { return (zstream_.avail_in > 0); }
    bool output_empty() const { return (zstream_.avail_out == 0); }

    std::uint64_t n_in_total() const { return zstream_.total_in; }
    std::uint64_t n_out_total() const { return zstream_.total_out; }

    /* Require: .input_empty() */
    void provide_input(std::uint8_t * buf, std::streamsize buf_z) {
        if (! this->input_empty())
            throw std::runtime_error("base_zstream::provide_input: prior input work not complete");

        zstream_.next_in = buf;
        zstream_.avail_in = buf_z;
    }

    void provide_input(span_type const & span) {
        this->provide_input(span.lo(), span.size());
    }

    void provide_output(uint8_t * buf, std::streamsize buf_z) {
        zstream_.next_out = buf;
        zstream_.avail_out = buf_z;
    }

    void provide_output(span_type const & span) {
        this->provide_output(span.lo(), span.size());
    }

protected:
    void swap(base_zstream & x) {
        std::swap(zstream_, x.zstream_);
    }

    /* move-assignment */
    base_zstream & operator= (base_zstream && x) {
        zstream_ = x.zstream_;

        /* zero rhs to prevent ::inflateEnd() releasing memory in x dtor */
        ::memset(&x.zstream_, 0, sizeof(x.zstream_));

        return *this;
    }

protected:
    /* zlib control state.  contains heap-allocated memory */
    z_stream zstream_;
};
```

4. class `inflate_zstream`

```
// inflate_zstream.hpp

#pragma once

#include "base_zstream.hpp"
#include "buffer.hpp"
#include <ios>
#include <cstring>

class inflate_zstream : public base_zstream {
public:
    using span_type = span<std::uint8_t>;

public:
    inflate_zstream();
    ~inflate_zstream();

    /* decompress some input,  return #of uncompressed bytes obtained */
    std::streamsize inflate_chunk();

    /* .first  = span for compressed bytes consumed
     * .second = span for uncompressed bytes produced
     */
    std::pair<span_type, span_type> inflate_chunk2();

    void swap (inflate_zstream & x) {
        base_zstream::swap(x);
    }

    /* move-assignment */
    inflate_zstream & operator= (inflate_zstream && x) {
        base_zstream::operator=(std::move(x));
        return *this;
    }
};

namespace std {
    inline void
    swap(inflate_zstream & lhs, inflate_zstream & rhs) {
        lhs.swap(rhs);
    }
}
```

```
// inflate_zstream.cpp

#include "compression/inflate_zstream.hpp"
#include "compression/tostr.hpp"

using namespace std;

inflate_zstream::inflate_zstream() {
    zstream_.zalloc    = Z_NULL;
    zstream_.zfree     = Z_NULL;
    zstream_.opaque    = Z_NULL;
    zstream_.avail_in  = 0;
    zstream_.next_in   = Z_NULL;
    zstream_.avail_out = 0;
    zstream_.next_out  = Z_NULL;

    int ret = ::inflateInit(&zstream_);

    if (ret != Z_OK)
        throw std::runtime_error("inflate_zstream: failed to initialize .zstream");
}

inflate_zstream::~inflate_zstream() {
    ::inflateEnd(&zstream_);
}

std::streamsize
inflate_zstream::inflate_chunk() {
    return this->inflate_chunk2().second.size();
}

std::pair<span<std::uint8_t>, span<std::uint8_t>>
inflate_zstream::inflate_chunk2() {
    /* Z = compressed data
     * U = uncompressed   data
     *
     *                          (pre) zstream
     *         /--------------    .next_in
     *         |                  .next_out    -----------\
     *         |                                          |
     *         |                                          |
     *         v    (pre)                                 v    (pre)
     *              zstream                                    zstream
     *         <--  .avail_in ----------->                <--  .avail_out ------------------>
     *
     * input:  ZZZZZZZZZZZZZZZZZZZZZZZZZZZ       output:  UUUUUUUUUUUUU......................
     *         ^        ^                                 ^            ^
     *         uc_pre   uc_post                           z_pre        z_post
     *
     *                  <--- (post)  ---->                             <--- (post)   ------->
     *                       zstream                                        zstream
     *                  ^    .avail_in                                 ^    .avail_in
     *                  |                                              |
     *                  |       (post) zstream                         |
     *                  \------   .next_in                             |
     *                            .next_out ---------------------------/
     *
     *         < retval >                                 <  retval    >
     *         < .first >                                 <  .second   >
     *
     */

    uint8_t * z_pre = zstream_.next_in;
    uint8_t * uc_pre = zstream_.next_out;

    int err = ::inflate(&zstream_, Z_NO_FLUSH);

    switch(err) {
    case Z_NEED_DICT:
        err = Z_DATA_ERROR;
        /* fallthru */
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
        throw std::runtime_error(tostr("zstreambuf::inflate_chunk: error [", err, "] from zlib inflate"));
    }

    uint8_t * z_post = zstream_.next_in;
    uint8_t * uc_post = zstream_.next_out;

    return pair<span_type, span_type>(span_type(z_pre, z_post),
                                      span_type(uc_pre, uc_post));
} /*inflate_chunk2*/
```

5. class `deflate_zstream`

```
// deflate_zstream.hpp

#pragma once

#include "base_zstream.hpp"
#include "buffer.hpp"
#include <zlib.h>
#include <ios>
#include <cstring>

class deflate_zstream : public base_zstream {
public:
    using span_type = span<std::uint8_t>;

public:
    deflate_zstream();
    ~deflate_zstream();

    /* compress some output,  return #of compressed bytes obtained
     *
     * final_flag.  must set to true end of uncompressed input reached,
     *              so that .zstream knows to flush compressed state
     */
    std::streamsize deflate_chunk(bool final_flag);

    /* .first = span for uncompressed bytes consumed
     * .second = span for compressed bytes produced
     */
    std::pair<span_type, span_type> deflate_chunk2(bool final_flag);

    void swap(deflate_zstream & x) {
        base_zstream::swap(x);
    }

    /* move-assignment */
    deflate_zstream & operator= (deflate_zstream && x) {
        base_zstream::operator=(std::move(x));
        return *this;
    }
};

namespace std {
    inline void
    swap(deflate_zstream & lhs, deflate_zstream & rhs) {
        lhs.swap(rhs);
    }
}
```

```
// deflate_zstream.cpp

#include "compression/deflate_zstream.hpp"

using namespace std;

deflate_zstream::deflate_zstream()
{
    zstream_.zalloc    = Z_NULL;
    zstream_.zfree     = Z_NULL;
    zstream_.opaque    = Z_NULL;
    zstream_.avail_in  = 0;
    zstream_.next_in   = Z_NULL;
    zstream_.avail_out = 0;
    zstream_.next_out  = Z_NULL;

    int ret = ::deflateInit(&zstream_, Z_DEFAULT_COMPRESSION);

    if (ret != Z_OK)
        throw runtime_error("deflate_zstream: failed to initialize .zstream");
}

deflate_zstream::~deflate_zstream() {
    ::deflateEnd(&zstream_);
}

streamsize
deflate_zstream::deflate_chunk(bool final_flag) {
    return this->deflate_chunk2(final_flag).second.size();
} /*deflate_chunk*/

pair<span<uint8_t>, span<uint8_t>>
deflate_zstream::deflate_chunk2(bool final_flag) {
    /* U = uncompressed data
     * Z = compressed   data
     *
     *                          (pre) zstream
     *         /--------------    .next_in
     *         |                  .next_out    -----------\
     *         |                                          |
     *         |                                          |
     *         v    (pre)                                 v    (pre)
     *              zstream                                    zstream
     *         <--  .avail_in ----------->                <--  .avail_out ------------------>
     *
     * input:  UUUUUUUUUUUUUUUUUUUUUUUUUUU       output:  ZZZZZZZZZZZZZ......................
     *         ^        ^                                 ^            ^
     *         uc_pre   uc_post                           z_pre        z_post
     *
     *                  <--- (post)  ---->                             <--- (post)   ------->
     *                       zstream                                        zstream
     *                  ^    .avail_in                                 ^    .avail_in
     *                  |                                              |
     *                  |       (post) zstream                         |
     *                  \------   .next_in                             |
     *                            .next_out ---------------------------/
     *
     *         < retval >                                 <  retval    >
     *         < .first >                                 <  .second   >
     *
     */

    uint8_t * uc_pre = zstream_.next_in;
    uint8_t * z_pre = zstream_.next_out;

    int err = ::deflate(&zstream_,
                        (final_flag ? Z_FINISH : 0) /*flush*/);

    if (err == Z_STREAM_ERROR)
        throw runtime_error("basic_zstreambuf::sync: impossible zlib deflate returned Z_STREAM_ERROR");

    uint8_t * uc_post = zstream_.next_in;
    uint8_t * z_post = zstream_.next_out;

    return pair<span_type, span_type>(span_type(uc_pre, uc_post),
                                      span_type(z_pre, z_post));
}
```

6. in `compression/CMakeLists.txt`:
```
set(SELF_SRCS compression.cpp inflate_zstream.cpp deflate_zstream.cpp buffered_inflate_zstream.cpp buffered_deflate_zstream.cpp)
...
```

7. class `buffered_inflate_zstream`:

.hpp
```
// buffered_inflate_zstream.hpp

#include "inflate_zstream.hpp"

/* Example
 *
 *   ifstream zfs("path/to/compressedfile.z", ios::binary);
 *   buffered_inflate_zstream<char> zs;
 *   ofstream ucfs("path/to/uncompressedfile");
 *
 *   while (!zfs.eof()) {
 *       span<char> z_span = zs.z_avail();
 *       if (!zfs.read(z_span.lo(), z_span.size())) {
 *            error...
 *       }
 *       zs.z_produce(z_span.prefix(zfs.gcount()));
 *
 *       zs.inflate_chunk();
 *
 *       span<char> uc_span = zs.uc_contents();
 *       ucfs.write(uc_span.lo(), uc_span.size());
 *
 *       zs.uc_consume(uc_span);
 *   }
 */
class buffered_inflate_zstream {
public:
    using z_span_type = span<std::uint8_t>;
    using size_type = std::uint64_t;

public:
    buffered_inflate_zstream(size_type buf_z = 64UL * 1024UL)
        : z_in_buf_{buf_z},
          uc_out_buf_{buf_z}
        {
            zs_algo_.provide_output(uc_out_buf_.avail());
        }

    std::uint64_t n_in_total() const { return zs_algo_.n_in_total(); }
    std::uint64_t n_out_total() const { return zs_algo_.n_out_total(); }

    /* space available for more compressed input */
    z_span_type z_avail() const { return z_in_buf_.avail(); }
    /* space available for more uncompressed input (output of this object) */
    z_span_type uc_avail() const { return uc_out_buf_.avail(); }
    /* uncompressed content available */
    z_span_type uc_contents() const { return uc_out_buf_.contents(); }

    /* after populating some prefix of .z_avail(), make existence of that input known
     * so that it can be uncompressed
     */
    void z_produce(z_span_type const & span) {
        if (span.size()) {
            z_in_buf_.produce(span);

            /* note whenever we call .inflate,  we consume from .z_in_buf,
             * so .z_in_buf and .input_zs are kept synchronized
             */
            zs_algo_.provide_input(z_in_buf_.contents());
        }
    }

    /* consume some uncompressed input -- allows that buffer space to be reused */
    void uc_consume(z_span_type const & span) {
        if (span.size()) {
            uc_out_buf_.consume(span);
        }

        if (uc_out_buf_.empty()) {
            /* can recycle output */
            zs_algo_.provide_output(uc_out_buf_.avail());
        }
    }

    void uc_consume_all() { this->uc_consume(this->uc_contents()); }

    size_type inflate_chunk();

    void swap (buffered_inflate_zstream & x) {
        std::swap(z_in_buf_, x.z_in_buf_);
        std::swap(zs_algo_, x.zs_algo_);
        std::swap(uc_out_buf_, x.uc_out_buf_);
    }

    buffered_inflate_zstream & operator= (buffered_inflate_zstream && x) {
        z_in_buf_ = std::move(x.z_in_buf_);
        zs_algo_ = std::move(x.zs_algo_);
        uc_out_buf_ = std::move(x.uc_out_buf_);

        return *this;
    }

private:
    /* compressed input */
    buffer<std::uint8_t> z_in_buf_;

    /* inflation-state (holds zlib z_stream) */
    inflate_zstream zs_algo_;

    /* uncompressed input */
    buffer<std::uint8_t> uc_out_buf_;
};

namespace std {
    inline void
    swap(buffered_inflate_zstream & lhs,
         buffered_inflate_zstream & rhs)
    {
        lhs.swap(rhs);
    }
}
```

.cpp
```
// buffered_inflate_zstream.cpp

#include "compression/buffered_inflate_zstream.hpp"

using namespace std;

auto
buffered_inflate_zstream::inflate_chunk() -> size_type
{
    if (zs_algo_.have_input()) {
        std::pair<z_span_type, z_span_type> x = zs_algo_.inflate_chunk2();

        z_in_buf_.consume(x.first);
        uc_out_buf_.produce(x.second);

        return x.second.size();
    } else {
        return 0;
    }
}
```

8. class `buffered_deflate_zstream`:

.hpp:
```
// buffered_deflate_zstream.hpp

#include "deflate_zstream.hpp"

/* accept input (of type CharT) and compress (aka deflatee).
 * provides buffer for both uncompressed input and compressed output
 *
 * Example
 *
 *   ifstream ucfs("path/to/uncompressedfile");
 *   buffered_deflate_zstream<char> zs;
 *   ofstream zfs("path/to/compressedfile.z", ios::binary);
 *
 *   if (!ucfs)
 *       error...
 *   if (!zfs)
 *       error...
 *
 *   for (bool progress = true, final_flag = false; progress;) {
 *       streamsize nread = 0;
 *
 *       if (ucfs.eof()) {
 *           final = true;
 *       } else {
 *           span<char> uc_span = zs.uc_avail();
 *           ucfs.read(uc_span.lo(), uc_span.size());
 *           nread = ucfs.gcount();
 *           zs.uc_produce(uc_span.prefix(nread));
 *       }
 *
 *       zs.deflate_chunk(final);
 *
 *       span<uint8_t> z_span = zs.z_contents();
 *       zfs.write(z_span.lo(), z_span.size());
 *       zs.z_consume(z_span);
 *
 *       progress = (nread > 0) || (z_span.size() > 0);
 *   }
 */
class buffered_deflate_zstream {
public:
    using z_span_type = span<std::uint8_t>;
    using size_type = std::uint64_t;

public:
    buffered_deflate_zstream(size_type buf_z = 64 * 1024)
        : uc_in_buf_{buf_z},
          z_out_buf_{buf_z}
        {
            zs_algo_.provide_output(z_out_buf_.avail());
        }

    size_type n_in_total() const { return zs_algo_.n_in_total(); }
    size_type n_out_total() const { return zs_algo_.n_out_total(); }

    /* space available for more uncompressed output (input of this object) */
    z_span_type uc_avail() const { return uc_in_buf_.avail(); }
    /* spaec available for more compressed output */
    z_span_type z_avail() const { return z_out_buf_.avail(); }
    /* compressed content available */
    z_span_type z_contents() const { return z_out_buf_.contents(); }

    /* after populating some prefix of .uc_avail(),  make existence of that output
     * known to .output_zs so it can be compressed
     */
    void uc_produce(z_span_type const & span) {
        if (span.size()) {
            uc_in_buf_.produce(span);

            /* note whenever we call .deflate,  we consume from .uc_output_buf,
             * so .uc_output_buf and .output_zs are kept synchronized
             */
            zs_algo_.provide_input(uc_in_buf_.contents());
        }
    }

    /* recognize some consumed compressed output -- allows that buffer space to be reused */
    void z_consume(z_span_type const & span) {
        if (span.size()) {
            z_out_buf_.consume(span);
        }

        if (z_out_buf_.empty()) {
            /* can recycle output */
            zs_algo_.provide_output(z_out_buf_.avail());
        }
    }

    void z_consume_all() { this->z_consume(this->z_contents()); }

    /* return #of bytes compressed output available */
    size_type deflate_chunk(bool final_flag);

    void swap (buffered_deflate_zstream & x) {
        std::swap(uc_in_buf_, x.uc_in_buf_);
        std::swap(zs_algo_, x.zs_algo_);
        std::swap(z_out_buf_, x.z_out_buf_);
    }

    buffered_deflate_zstream & operator= (buffered_deflate_zstream && x) {
        uc_in_buf_ = std::move(x.uc_in_buf_);
        zs_algo_ = std::move(x.zs_algo_);
        z_out_buf_ = std::move(x.z_out_buf_);

        return *this;
    }

private:
    /* uncompressed output */
    buffer<std::uint8_t> uc_in_buf_;

    /* deflate-state (holds zlib z_stream) */
    deflate_zstream zs_algo_;

    /* compressed output */
    buffer<std::uint8_t> z_out_buf_;
}; /*buffered_deflate_zstream*/

namespace std {
    inline void
    swap(buffered_deflate_zstream & lhs,
         buffered_deflate_zstream & rhs)
    {
        lhs.swap(rhs);
    }
}
```

.cpp:
```
// buffered_deflate_zstream.cpp

#include "compression/buffered_deflate_zstream.hpp"

using namespace std;

auto
buffered_deflate_zstream::deflate_chunk(bool final_flag) -> size_type
{
    if (zs_algo_.have_input() || final_flag) {
        std::pair<z_span_type, z_span_type> x = zs_algo_.deflate_chunk2(final_flag);

        uc_in_buf_.consume(x.first);
        z_out_buf_.produce(x.second);

        return x.second.size();
    } else {
        return 0;
    }
}
```

9. Reimplement `compression::inflate_file`

```
// compression.cpp

...

void
compression::inflate_file(std::string const & in_file,
                          std::string const & out_file,
                          bool keep_flag,
                          bool verbose_flag)
{
    /* check output doesn't exist already */
    if (ifstream(out_file, ios::binary|ios::in))
        throw std::runtime_error(tostr("output file [", out_file, "] already exists"));

    if (verbose_flag)
        cerr << "compression::inflate_file will uncompress [" << in_file << "] -> [" << out_file << "]" << endl;

    /* open target file */
    ifstream fs(in_file, ios::binary);
    if (!fs)
        throw std::runtime_error("unable to open input file");

    buffered_inflate_zstream zstate;

    /* write uncompressed output */
    ofstream ucfs(out_file, ios::out|ios::binary);

    while (!fs.eof()) {
        span<uint8_t> zspan = zstate.z_avail();

        fs.read(reinterpret_cast<char *>(zspan.lo()), zspan.size());
        std::streamsize n_read = fs.gcount();

        if (n_read == 0)
            throw std::runtime_error(tostr("inflate_file: unable to read contents of input file [", in_file, "]"));

        zstate.z_produce(zspan.prefix(n_read));

        /* uncompress some text */
        zstate.inflate_chunk();

        span<uint8_t> ucspan = zstate.uc_contents();

        ucfs.write(reinterpret_cast<char *>(ucspan.lo()), ucspan.size());

        zstate.uc_consume(ucspan);
    }

    if (!ucfs.good())
        throw std::runtime_error(tostr("inflate_file: failed to write ", zstate.n_out_total(), " bytes to [", out_file, "]"));

    fs.close();
    ucfs.close();

    if (!keep_flag)
        remove(in_file.c_str());
} /*inflate_file*/
```

10. re-implement `compression::deflate_file`
```
// compression.cpp

...

void
compression::deflate_file(std::string const & in_file,
                          std::string const & out_file,
                          bool keep_flag,
                          bool verbose_flag)
{
    /* check output doesn't exist already */
    if (ifstream(out_file, ios::binary|ios::in))
        throw std::runtime_error(tostr("output file [", out_file, "] already exists"));

    if (verbose_flag || true)
        cerr << "compress::deflate_file: will compress [" << in_file << "]"
             << " -> [" << out_file << "]" << endl;

    /* open target file -- binary mode since need not be text */
    ifstream fs(in_file, ios::in|ios::binary);
    if (!fs)
        throw std::runtime_error(tostr("unable to open input file [", in_file, "]"));

    buffered_deflate_zstream zstate;

    /* write compressed output */
    ofstream zfs(out_file, ios::out|ios::binary);

    for (bool progress = true, final_flag = false; progress;) {
        streamsize nread = 0;

        if (fs.eof()) {
            final_flag = true;
        } else {
            span<uint8_t> ucspan = zstate.uc_avail();

            fs.read(reinterpret_cast<char *>(ucspan.lo()), ucspan.size());
            nread = fs.gcount();
            zstate.uc_produce(ucspan.prefix(nread));
        }

        zstate.deflate_chunk(final_flag);

        /* write compressed output */
        span<uint8_t> zspan = zstate.z_contents();

        zfs.write(reinterpret_cast<char *>(zspan.lo()), zspan.size());
        if (!zfs.good())
            throw std::runtime_error(tostr("failed to write ", zspan.size(), " bytes"
                                           , " to [", out_file, "]"));

        zstate.z_consume(zspan);

        progress = (nread > 0) || (zspan.size() > 0);
    }

    fs.close();
    zfs.close();

    /* control here only if successfully wrote uncompressed output */
    if (!keep_flag)
        remove(in_file.c_str());
} /*deflate_file*/
```

Build:
```
$ cd cmake-examples
$ cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -B build
...
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
[  7%] Building CXX object compression/CMakeFiles/compression.dir/compression.cpp.o
[ 15%] Building CXX object compression/CMakeFiles/compression.dir/inflate_zstream.cpp.o
[ 23%] Building CXX object compression/CMakeFiles/compression.dir/deflate_zstream.cpp.o
[ 30%] Building CXX object compression/CMakeFiles/compression.dir/buffered_inflate_zstream.cpp.o
[ 38%] Building CXX object compression/CMakeFiles/compression.dir/buffered_deflate_zstream.cpp.o
[ 46%] Linking CXX shared library libcompression.so
[ 46%] Built target compression
[ 53%] Building CXX object compression/utest/CMakeFiles/utest.compression.dir/compression_utest_main.cpp.o
[ 61%] Building CXX object compression/utest/CMakeFiles/utest.compression.dir/compression.test.cpp.o
[ 69%] Linking CXX executable utest.compression
[ 69%] Built target utest.compression
[ 76%] Building CXX object app/hello/CMakeFiles/hello.dir/hello.cpp.o
[ 84%] Linking CXX executable hello
[ 84%] Built target hello
[ 92%] Building CXX object app/myzip/CMakeFiles/myzip.dir/myzip.cpp.o
[100%] Linking CXX executable myzip
[100%] Built target myzip
```

Run unit tests:
```
$ (cd build && ctest)
Test project /home/roland/proj/cmake-examples/build
    Start 1: utest.compression
1/2 Test #1: utest.compression ................   Passed    0.00 sec
    Start 2: myzip.utest
2/2 Test #2: myzip.utest ......................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   0.02 sec
```

Install:
```
$ cmake --install build
-- Install configuration: ""
-- Installing: /home/roland/scratch/include/compression
-- Installing: /home/roland/scratch/include/compression/tostr.hpp
-- Installing: /home/roland/scratch/include/compression/compression.hpp
-- Installing: /home/roland/scratch/include/compression/buffered_deflate_zstream.hpp
-- Installing: /home/roland/scratch/include/compression/base_zstream.hpp
-- Installing: /home/roland/scratch/include/compression/buffered_inflate_zstream.hpp
-- Installing: /home/roland/scratch/include/compression/inflate_zstream.hpp
-- Installing: /home/roland/scratch/include/compression/buffer.hpp
-- Installing: /home/roland/scratch/include/compression/deflate_zstream.hpp
-- Installing: /home/roland/scratch/include/compression/span.hpp
-- Installing: /home/roland/scratch/lib/libcompression.so.2
-- Installing: /home/roland/scratch/lib/libcompression.so.2.3
-- Set runtime path of "/home/roland/scratch/lib/libcompression.so.2" to "/home/roland/scratch/lib"
-- Installing: /home/roland/scratch/lib/libcompression.so
-- Installing: /home/roland/scratch/bin/hello
-- Set runtime path of "/home/roland/scratch/bin/hello" to "/home/roland/scratch/lib"
-- Installing: /home/roland/scratch/bin/myzip
-- Set runtime path of "/home/roland/scratch/bin/myzip" to "/home/roland/scratch/lib"
$ tree ~/scratch
/home/roland/scratch
├── bin
│   ├── hello
│   └── myzip
├── include
│   └── compression
│       ├── base_zstream.hpp
│       ├── buffer.hpp
│       ├── buffered_deflate_zstream.hpp
│       ├── buffered_inflate_zstream.hpp
│       ├── compression.hpp
│       ├── deflate_zstream.hpp
│       ├── inflate_zstream.hpp
│       ├── span.hpp
│       └── tostr.hpp
└── lib
    ├── libcompression.so -> libcompression.so.2.3
    ├── libcompression.so.2
    └── libcompression.so.2.3 -> libcompression.so.2

4 directories, 14 files
```

# Example 13

Provide inflating/deflating specialization of `std::streambuf`.
This requires generalizing build to handle a mixture of internal-to-repo and external-to-repo library dependencies

```
$ cd cmake-examples
$ git checkout ex13
```

source tree:
```
$ tree
.
├── CMakeLists.txt
├── LICENSE
├── README.md
├── app
│   ├── hello
│   │   ├── CMakeLists.txt
│   │   └── hello.cpp
│   └── myzip
│       ├── CMakeLists.txt
│       ├── myzip.cpp
│       └── utest
│           ├── CMakeLists.txt
│           ├── myzip.utest
│           └── textfile
├── compile_commands.json -> build/compile_commands.json
├── compression
│   ├── CMakeLists.txt
│   ├── buffered_deflate_zstream.cpp
│   ├── buffered_inflate_zstream.cpp
│   ├── compression.cpp
│   ├── deflate_zstream.cpp
│   ├── include
│   │   └── compression
│   │       ├── base_zstream.hpp
│   │       ├── buffer.hpp
│   │       ├── buffered_deflate_zstream.hpp
│   │       ├── buffered_inflate_zstream.hpp
│   │       ├── compression.hpp
│   │       ├── deflate_zstream.hpp
│   │       ├── inflate_zstream.hpp
│   │       ├── span.hpp
│   │       └── tostr.hpp
│   ├── inflate_zstream.cpp
│   └── utest
│       ├── CMakeLists.txt
│       ├── compression.test.cpp
│       └── compression_utest_main.cpp
└── zstream
    ├── CMakeLists.txt
    ├── include
    │   └── zstream
    │       ├── zstream.hpp
    │       └── zstreambuf.hpp
    └── utest
        ├── CMakeLists.txt
        ├── text.cpp
        ├── text.hpp
        ├── zstream.test.cpp
        ├── zstream_utest_main.cpp
        └── zstreambuf.test.cpp

12 directories, 38 files
```

Changes:
1. new header-only library `zstream`.
2. new template `zstreambuf`,  implements `std::streambuf` api along with inflation/deflation.
3. new template `zstream`,  wraps a `zstreambuf` to provide typical iostream-style formatted i/o
4. new unit test `zstream/utest`
5. add new build files to top-level CMakeLists.txt

Remarks:
1. We have a header-only library (`zstream`) that depends on a regular library (`compression`);
   cmake allows this

Details:

1. `zstream` build

```
// zstream/CMakeLists.txt

set(SELF_LIB zstream)

add_library(${SELF_LIB} INTERFACE)
target_include_directories(${SELF_LIB} INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(${SELF_LIB} INTERFACE compression)

target_compile_options(${SELF_LIB} INTERFACE ${zlib_CFLAGS_OTHER})
target_include_directories(${SELF_LIB} INTERFACE ${zlib_INCLUDE_DIRS})
target_link_libraries(${SELF_LIB} INTERFACE ${zlib_LIBRARIES})

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/zstream
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

install(
    TARGETS ${SELF_LIB}
    PUBLIC_HEADER DESTINATION include COMPONENT Development)
```

2. `zstreambuf` header
```
// zstreambuf.hpp

#pragma once

#include "compression/buffered_inflate_zstream.hpp"
#include "compression/buffered_deflate_zstream.hpp"
#include "compression/tostr.hpp"
#include "zlib.h"

#include <iostream>
#include <string>
#include <memory>

struct hex {
    hex(std::uint8_t x, bool w_char = false) : x_{x}, with_char_{w_char} {}

    std::uint8_t x_;
    bool with_char_;
};

struct hex_view {
    hex_view(std::uint8_t const * lo, std::uint8_t const * hi, bool as_text) : lo_{lo}, hi_{hi}, as_text_{as_text} {}
    hex_view(char const * lo, char const * hi, bool as_text)
        : lo_{reinterpret_cast<std::uint8_t const *>(lo)},
          hi_{reinterpret_cast<std::uint8_t const *>(hi)},
          as_text_{as_text} {}

    std::uint8_t const * lo_;
    std::uint8_t const * hi_;
    bool as_text_;
};

inline std::ostream &
operator<< (std::ostream & os, hex const & ins) {
    std::uint8_t lo = ins.x_ & 0xf;
    std::uint8_t hi = ins.x_ >> 4;

    char lo_ch = (lo < 10) ? '0' + lo : 'a' + lo - 10;
    char hi_ch = (hi < 10) ? '0' + hi : 'a' + hi - 10;

    os << hi_ch << lo_ch;

    if (ins.with_char_) {
        os << "(";
        if (std::isprint(ins.x_))
            os << (char)ins.x_;
        else
            os << "?";
        os << ")";
    }

    return os;
}

inline std::ostream &
operator<< (std::ostream & os, hex_view const & ins) {
    os << "[";
    std::size_t i = 0;
    for (std::uint8_t const * p = ins.lo_; p < ins.hi_; ++p) {
        if (i > 0)
            os << " ";
        os << hex(*p, ins.as_text_);
        ++i;
    }
    os << "]";
    return os;
}

/* implementation of streambuf that provides output to, and input from, a compressed stream */
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_zstreambuf : public std::basic_streambuf<CharT, Traits> {
public:
    using size_type = std::uint64_t;
    using int_type = typename Traits::int_type;

public:
    basic_zstreambuf(size_type buf_z = 64 * 1024,
                     std::unique_ptr<std::streambuf> native_sbuf = std::unique_ptr<std::streambuf>())
        :
        in_zs_{aligned_upper_bound(buf_z), alignment()},
        out_zs_{aligned_upper_bound(buf_z), alignment()},
        native_sbuf_{std::move(native_sbuf)}
    {
        this->setg_span(in_zs_.uc_contents());
        this->setp_span(out_zs_.uc_avail());
    }
    ~basic_zstreambuf() {
        this->close();
    }

    std::uint64_t n_z_in_total() const { return in_zs_.n_in_total(); }
    /* note: z input side of zstreambuf = output from inflating-zstream */
    std::uint64_t n_uc_in_total() const { return in_zs_.n_out_total(); }

    /* note: uc output side of zstreambuf = input to deflating-zstream */
    std::uint64_t n_uc_out_total() const { return out_zs_.n_in_total(); }
    std::uint64_t n_z_out_total() const { return out_zs_.n_out_total(); }

    std::streambuf * native_sbuf() const { return native_sbuf_.get(); }

    void adopt_native_sbuf(std::unique_ptr<std::streambuf> x) { native_sbuf_ = std::move(x); }

    /* we have a problem writing compressed output:  compression algorithm in general
     * doesn't know how to compress byte n until it has seem byte n+1, .., n+k
     */
    void close() {
        if (!closed_flag_) {
            this->sync_impl(true /*final_flag*/);

            this->closed_flag_ = true;
        }
    }

    /* move-assignment */
    basic_zstreambuf & operator= (basic_zstreambuf && x) {
        /* assign any base-class state */
        std::basic_streambuf<CharT, Traits>::operator=(x);

        closed_flag_ = x.closed_flag_;
        in_zs_ = std::move(x.in_zs_);
        out_zs_ = std::move(x.out_zs_);

        native_sbuf_ = std::move(x.native_sbuf_);

        return *this;
    }

    void swap(basic_zstreambuf & x) {
        /* swap any base-class state */
        std::basic_streambuf<CharT, Traits>::swap(x);

        std::swap(closed_flag_, x.closed_flag_);

        std::swap(in_zs_, x.in_zs_);
        std::swap(out_zs_, x.out_zs_);

        std::swap(native_sbuf_, x.native_sbuf_);
    }

protected:
    /* estimates #of characters n available for input -- .underflow() will not be called
     * or throw exception until at least n chars are extracted.
     *
     * -1 if .showmanyc() can prove input has reached eof
     */
    //virtual std::streamsize showmanyc() override;

    /* attempt to read n chars from input,  and store in s.
     * (will call .uflow() as needed if less than n chars are immediately available)
     */
    //virtual std::streamsize xs_getn(char_type * s, std::streamsize n) override;

    /* ensure at least one character available in input area.
     * may update .gptr .egptr .eback to define input data location
     *
     * returns next input character (target of get-pointer)
     */
    virtual int_type underflow() override final {
        /* control here: .input buffer (i.e. .in_zs.uc_input_buf) has been entirely consumed */

        std::streambuf * nsbuf = native_sbuf_.get();

        /* any previous output from .in_zs has already been consumed (otherwise not in underflow state) */
        in_zs_.uc_consume_all();

        while (true) {
            /* zspan: available (unused) buffer space for compressed input */
            auto zspan = in_zs_.z_avail();

            std::streamsize n = 0;

            /* try to fill compressed-input buffer space */
            if (zspan.size()) {
                n = nsbuf->sgetn(reinterpret_cast<char *>(zspan.lo()),
                                 zspan.size());

                /* .in_zs needs to know how much we filled */
                in_zs_.z_produce(zspan.prefix(n));
            } else {
                /* it's possible previous inflate_chunk filled uncompressed output
                 * without consuming any compressed input,  in which case can have z_avail empty
                 */
            }

            /* do some decompression work */
            in_zs_.inflate_chunk();

            /* loop until uncompressed buffer filled,  or reached end of compressed input
             *
             * note this implies we always have whole-number-of-CharT in .uc_contents
             */
            if (in_zs_.uc_avail().empty() || (n < zspan.size()))
                break;
        }

        /* ucspan: uncompressed output */
        auto ucspan = in_zs_.uc_contents();

        /* streambuf pointers need to know content
         *
         * see comment on loop above -- ucspan always aligned for CharT
         */
        this->setg_span(ucspan);

        if (ucspan.size())
            return Traits::to_int_type(*ucspan.lo());
        else
            return Traits::eof();
    }

    /* write contents of .output to .native_sbuf.
     * 0 on success, -1 on failure
     *
     * NOTE: After .sync() returns may still have un-synced output in .output_zs;
     *       tradeoff is that if we insist on writing that output,  will change the contents
     *       of comppressed output + degrade compression quality.
     */
    virtual int
    sync() override final {
        return this->sync_impl(false /*!final_flag*/);
    }

    /* attempt to write n bytes starting at s[] to this streambuf.
     * returns the #of bytes actually written
     */
    virtual std::streamsize
    xsputn(CharT const * s, std::streamsize n_arg) override final {
        if (closed_flag_)
            throw std::runtime_error("basic_zstreambuf::xsputn: attempted write to closed stream");

        std::streamsize n = n_arg;

        std::size_t i_loop = 0;

        while (n > 0) {
            std::streamsize buf_avail = this->epptr() - this->pptr();

            if (buf_avail == 0) {
                /* deflate some more output + free up buffer space */
                this->sync();
            } else {
                std::streamsize n_copy = std::min(n, buf_avail);

                ::memcpy(this->pptr(), s, n_copy);
                this->pbump(n_copy);

                s += n_copy;
                n -= n_copy;
            }

            ++i_loop;
        }

        return n_arg;
    }

    virtual int_type
    overflow(int_type new_ch) override final {
        if (this->sync() != 0) {
            throw std::runtime_error("basic_zstreambuf::overflow: sync failed to create buffer space");
        };

        if (Traits::eq_int_type(new_ch, Traits::eof()) != true) {
            *(this->pptr()) = Traits::to_char_type(new_ch);
            this->pbump(1);
        }

        return new_ch;
    }

private:
    /* write contents of .output to .native_sbuf.
     * 0 on success, -1 on failure.
     *
     * final_flag = true:  compressed stream is irrevocably complete -- no further output may be written
     * final_flag = false: after .sync_impl() returns may still have un-synced output in .output_zs
     */
    int
    sync_impl(bool final_flag) {
        if (closed_flag_) {
            /* implies attempt to write more output after call to .close() promised not to */
            return -1;
        }

        std::streambuf * nsbuf = native_sbuf_.get();

        /* consume all available uncompressed output
         *
         * note: converting from CharT* -> uint8_t* ok here.
         *       we are always starting with a properly-CharT*-aligned value,
         *       and in any case destination pointer used only with deflate(),
         *       which imposes no alignment requirements
         */

        out_zs_.uc_produce(span<std::uint8_t>(reinterpret_cast<std::uint8_t *>(this->pbase()),
                                              reinterpret_cast<std::uint8_t *>(this->pptr())));

        for (bool progress = true; progress;) {
            out_zs_.deflate_chunk(final_flag);
            auto zspan = out_zs_.z_contents();

            if (nsbuf->sputn(reinterpret_cast<char *>(zspan.lo()), zspan.size()) < zspan.size())
                throw std::runtime_error("zstreambuf::sync_impl: partial write!");

            out_zs_.z_consume(zspan);

            progress = (zspan.size() > 0);
        }

        /* uncompressed output buffer is empty,  since everything was sent to deflate;
         * can recycle it
         */
        this->setp_span(out_zs_.uc_avail());

        std::streamsize buf_avail = this->epptr() - this->pptr();

        if (buf_avail > 0) {
            /* control always here */
            return 0;
        } else {
            /* something crazy - maybe .output.buf_z == 0 ? */
            return -1;
        }
    }

    void setg_span(span<std::uint8_t> const & ucspan) {
        this->setg(reinterpret_cast<CharT *>(ucspan.lo()),
                   reinterpret_cast<CharT *>(ucspan.lo()),
                   reinterpret_cast<CharT *>(ucspan.hi()));
    }


    void setp_span(span<std::uint8_t> const & ucspan) {
        this->setp(reinterpret_cast<CharT *>(ucspan.lo()),
                   reinterpret_cast<CharT *>(ucspan.hi()));
    }

    static constexpr size_type alignment() {
        /* note: we can't support alignof(CharT) > sizeof(CharT),
         *       since we assume CharT's in a stream are packed
         */
        return sizeof(CharT);
    }

    /* returns #of bytes equal to a multiple of {CharT alignment,  sizeof(CharT)},
     * whichever is larger.  Use this to round up buffer sizes
     */
    static size_type aligned_upper_bound(size_type z) {
        constexpr size_type m = alignment();

        size_type extra = z % m;

        if (extra == 0)
            return z;
        else
            return z + (m - extra);
    }

private:
    /* Input:
     *                                       .inflate_chunk();
     *                   .sgetn()            .uc_contents()
     *    .native_sbuf -----------> .in_zs -------------------> .gptr, .egptr
     *
     * Output:
     *                                       .sync();
     *                                       .deflate_chunk();
     *                   .sputn()            .z_contents()          .sputn
     *   .pbeg, .pend ------------> .out_zs -------------------------------> .native_sbuf
     */

    /* set irrevocably on .close() */
    bool closed_flag_ = false;

    /* reminder:
     * 1. .eback() <= .gptr() <= .egptr()
     * 2. input buffer pointers .eback() .gptr() .egptr() are owned by basic_streambuf,
     *    and these methods are non-virtual.
     * 3. it's required that [.eback .. .egptr] represent contiguous memory
     */

    buffered_inflate_zstream in_zs_;
    buffered_deflate_zstream out_zs_;

    /* i/o for compressed data */
    std::unique_ptr<std::streambuf> native_sbuf_;
}; /*basic_zstreambuf*/

using zstreambuf = basic_zstreambuf<char>;

namespace std {
    template <typename CharT, typename Traits>
    void swap(basic_zstreambuf<CharT, Traits> & lhs,
              basic_zstreambuf<CharT, Traits> & rhs)
    {
        lhs.swap(rhs);
    }
}
```

3. `zstream` header

```
// zstream.hpp

#pragma once

#include "zstreambuf.hpp"
#include <iostream>

template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_zstream : public std::basic_iostream<CharT, Traits> {
public:
    using char_type = CharT;
    using traits_type = Traits;
    using int_type = typename Traits::int_type;
    using pos_type = typename Traits::pos_type;
    using off_type = typename Traits::off_type;
    using zstreambuf_type = basic_zstreambuf<CharT, Traits>;

public:
    basic_zstream(std::streamsize buf_z, std::unique_ptr<std::streambuf> native_sbuf)
        : rdbuf_(buf_z, std::move(native_sbuf)),
          std::basic_iostream<CharT, Traits>(&rdbuf_)
           {}
    ~basic_zstream() = default;

    zstreambuf_type * rdbuf() { return &rdbuf_; }

    /* move-assignment */
    basic_zstream & operator=(basic_zstream && x) {
        /* assign any base-class state */
        std::basic_iostream<CharT, Traits>::operator=(x);

        this->rdbuf_ = std::move(x.rdbuf_);
        return *this;
    }

    /* exchange state with x */
    void swap(basic_zstream & x) {
        /* swap any base-class state */
        std::basic_iostream<CharT, Traits>::swap(x);
        /* swap streambuf state */
        this->rdbuf_.swap(x.rdbuf_);
    }

    /* finishes writing compressed output */
    void close() {
        this->rdbuf_.close();
    }

private:
    basic_zstreambuf<CharT, Traits> rdbuf_;
}; /*basic_zstream*/

using zstream = basic_zstream<char>;

namespace std {
    template <typename CharT, typename Traits>
    void swap(basic_zstream<CharT, Traits> & lhs,
              basic_zstream<CharT, Traits> & rhs)
    {
        lhs.swap(rhs);
    }
}
```

4. `zstream` unit test

boilerplate main
```
// zstream/utest/zstream_utest_main.cpp

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
```

some text data
```
// zstream/utest/text.hpp

#pragma once

struct Text {
    static char const * s_text;
};
```

```
// zstream/utest/text.cpp

#include "text.hpp"

char const *
Text::s_text
= ("Lorem ipsum dolor sit amet, consectetur adipiscing elit"
   ...omitted...
   );
```

unit test using `zstreambuf` api,  various read/write chunk sizes
```
// zstream/utest/zstreambuf.test.cpp

#include "text.hpp"
#include "zstream/zstreambuf.hpp"
#include "catch2/catch.hpp"

#include <string_view>
#include <sstream>
#include <array>

using namespace std;

struct text_compare {
    text_compare(string_view s1, string_view s2) : s1_{std::move(s1)}, s2_{std::move(s2)} {}

    string_view s1_;
    string_view s2_;
};

ostream &
operator<< (ostream & os, text_compare const & x) {
    size_t n1 = x.s1_.size();
    size_t n2 = x.s2_.size();
    size_t n = std::min(n1, n2);

    size_t i = 0;

    while (i < n) {
        size_t line = std::min(i + 50, n);

        os << i << ": ";

        /* print all of s1(i .. i+99) */
        size_t i1 = i;
        size_t line1 = std::min(i + 50, n1);
        for (; i1 < line1; ++i1) {
            if (isprint(x.s1_[i1]))
                os << x.s1_[i1];
            else
                os << "\\";
        }
        os << endl;

        os << i << ": ";

        /* print s2(i) only when != s1(i) */
        size_t i2 = i;
        size_t line2 = std::min(i + 50, n2);
        for (size_t i2 = i; i2 < line2; ++i2) {
            if (i2 < line1 && (x.s2_[i2] == x.s1_[i2]))
                os << " ";
            else if (isprint(x.s2_[i2]))
                os << x.s2_[i2];
            else
                os << "\\";
        }
        os << endl;

        i += 50;
    }

    return os;
}

namespace {
    struct TestCase {
        TestCase(uint32_t bufz, uint32_t wz, uint32_t rz)
            : buf_z_{bufz}, write_chunk_z_{wz}, read_chunk_z_{rz} {}

        /* buffer size for zstreambuf - applies to buffers for:
         * - uncompressed input + output
         * - compressed input + output
         */
        uint32_t buf_z_ = 0;
        /* write uncompressed text in chunks of this size */
        uint32_t write_chunk_z_ = 0;
        /* read uncompresseed text in chunks of this size */
        uint32_t read_chunk_z_ = 0;
    };

    static vector<TestCase> s_testcase_v = {
        TestCase(1, 1, 1),
        TestCase(1, 256, 256),
        TestCase(256, 15, 15),
        TestCase(256, 16, 16),
        TestCase(256, 17, 17),
        TestCase(256, 129, 129),
        TestCase(65536, 129, 129),
        TestCase(65536, 65536, 65536)
    };
}

TEST_CASE("zstreambuf", "[zstreambuf]") {
    for (size_t i_tc = 0; i_tc < s_testcase_v.size(); ++i_tc) {
        TestCase const & tc = s_testcase_v[i_tc];

        INFO(tostr("i_tc=", i_tc));

        // ----------------------------------------------------------------
        // phase 1 - compress some text
        // ----------------------------------------------------------------

        /* buffer to hold compressed output */
        using zbuf_type = array<char, 64*1024>;
        unique_ptr<zbuf_type> zbuf(new zbuf_type());

        for (size_t i=0, n=sizeof(zbuf_type); i<n; ++i)
            (*zbuf)[i] = '\0';

        /* compressed output will appear here */
        unique_ptr<streambuf> zsbuf(new stringbuf());

        zsbuf->pubsetbuf(&((*zbuf)[0]), sizeof(zbuf_type));

        /* 256: for unit test want to exercise overflow.. frequently */
        unique_ptr<zstreambuf> ogbuf(new zstreambuf(tc.buf_z_));

        ogbuf->adopt_native_sbuf(std::move(zsbuf));

        /* write from s_text in small chunk sizes */
        size_t const c_write_z = tc.write_chunk_z_;

        for (size_t i=0, n=strlen(Text::s_text); i<n;) {
            size_t nreq = std::min(c_write_z, n-i);
            REQUIRE(ogbuf->sputn(Text::s_text + i, nreq) == nreq);

            i += nreq;
        }

        ogbuf->close();

        cout << "uc out: " << ogbuf->n_uc_out_total() << endl;
        cout << "z  out: " << ogbuf->n_z_out_total() << endl;

        size_t i = 0;
        size_t n = ogbuf->n_z_out_total();
        while (i < n) {
            /* 64 hex values */
            do {
                uint8_t ch = (*zbuf)[i];
                uint8_t lo = ch & 0xf;
                uint8_t hi = ch >> 4;
                char lo_ch = (lo < 10) ? '0' + lo : 'a' + lo - 10;
                char hi_ch = (hi < 10) ? '0' + hi : 'a' + hi - 10;

                cout << " " << hi_ch << lo_ch;

                ++i;
            } while ((i < n) && (i % 64 != 0));

            cout << endl;
        }

        // ----------------------------------------------------------------
        // phase 2 - not decompress compressed output,
        //           make sure we recover original text
        // ----------------------------------------------------------------

        unique_ptr<streambuf> zsbuf2(new stringbuf());
        zsbuf2->pubsetbuf(&((*zbuf)[0]), ogbuf->n_z_out_total());

        unique_ptr<zstreambuf> ogbuf2(new zstreambuf(tc.buf_z_));
        ogbuf2->adopt_native_sbuf(std::move(zsbuf2));

        /* read from ogbuf2 in small chunk sizes */
        unique_ptr<zbuf_type> ucbuf2(new zbuf_type());

        size_t const c_read_z = tc.read_chunk_z_;
        size_t i_uc = 0;
        size_t n_uc = 0;

        do {
            n_uc = ogbuf2->sgetn(&((*ucbuf2)[i_uc]), c_read_z);
            i_uc += n_uc;
        } while (n_uc == c_read_z);

        //INFO(tostr("uc_buf2=", hex_view(&(*ucbuf2)[0], &(*ucbuf2)[ogbuf2->n_uc_in_total()], true /*as_text*/)));
        INFO(text_compare(string_view(Text::s_text),
                          string_view(&(*ucbuf2)[0], &(*ucbuf2)[i_uc])));

        CHECK(ogbuf2->n_z_in_total() == ogbuf->n_z_out_total());
        CHECK(ogbuf2->n_uc_in_total() == ogbuf->n_uc_out_total());
        CHECK(i_uc == ::strlen(Text::s_text));

        for (size_t i=0; i<i_uc; ++i) {
            INFO(tostr("i=", i, ", s_text[i]=", Text::s_text[i], ", ucbuf2[i]=", (*ucbuf2)[i]));

            REQUIRE(Text::s_text[i] == (*ucbuf2)[i]);
        }
    }
}
```

unit test using `zstream api`
```
// zstream/utest/zstream.test.cpp

#include "text.hpp"
#include "zstream/zstream.hpp"
#include "catch2/catch.hpp"

using namespace std;

TEST_CASE("zstream", "[zstream]") {
    /* make some buffer space */
    using zbuf_type = array<char, 64*1024>;
    unique_ptr<zbuf_type> zbuf(new zbuf_type());
    std::fill(zbuf->begin(), zbuf->end(), '\0');

    size_t n_z_out_total = 0;
    {
        zstream zs(64 * 1024, move(unique_ptr<streambuf>(new stringbuf())));

        zs.rdbuf()->native_sbuf()->pubsetbuf(&((*zbuf)[0]), zbuf->size());

        zs << Text::s_text << endl;

        /* reminder: have to close zstream to get complete compressed output. */
        zs.close();

        cout << "uc out: " << zs.rdbuf()->n_uc_out_total() << endl;
        cout << "z  out: " << zs.rdbuf()->n_z_out_total() << endl;

        size_t i = 0;
        size_t n = zs.rdbuf()->n_z_out_total();

        while (i < n) {
            /* 64 hex values */
            do {
                uint8_t ch = (*zbuf)[i];

                cout << " " << ::hex(ch);
                ++i;
            } while ((i < n) && (i % 64 != 0));

            cout << endl;
        }

        n_z_out_total = n;
    }

    /* now decompress.. */
    {
        zstream zs(64 * 1024, move(unique_ptr<streambuf>(new stringbuf())));

        zs.rdbuf()->native_sbuf()->pubsetbuf(&((*zbuf)[0]), n_z_out_total);

        unique_ptr<zbuf_type> zbuf2(new zbuf_type());
        std::fill(zbuf2->begin(), zbuf2->end(), '\0');

        cerr << "input" << endl;
        unique_ptr<zbuf_type> ucbuf2(new zbuf_type());
        std::fill(ucbuf2->begin(), ucbuf2->end(), '\0');

        zs.read(&((*ucbuf2)[0]), ucbuf2->size());
        streamsize n_read = zs.gcount();

        CHECK(n_read == strlen(Text::s_text) + 1);

        cerr << "uncompressed input:" << endl;
        cerr << string_view(&((*ucbuf2)[0]), &((*ucbuf2)[n_read])) << endl;

        for (size_t i=0; i<n_read-1; ++i) {
            INFO(tostr("i=", i, ", s_text[i]=", Text::s_text[i], ", ucbuf2[i]=", (*ucbuf2)[i]));

            REQUIRE(Text::s_text[i] == (*ucbuf2)[i]);
        }
    }
}

```

5. toplevel CMakeLists.txt:
```
add_subdirectory(compression/utest)
add_subdirectory(zstream)
add_subdirectory(zstream/utest)
add_subdirectory(app/hello)
```

Build:
```
$ cd cmake-examples
$ cmake -DCMAKE_INSTALL_PREFIX=$PREFIX -B build
...
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build
[  5%] Building CXX object compression/CMakeFiles/compression.dir/compression.cpp.o
[ 11%] Building CXX object compression/CMakeFiles/compression.dir/inflate_zstream.cpp.o
[ 16%] Building CXX object compression/CMakeFiles/compression.dir/deflate_zstream.cpp.o
[ 22%] Building CXX object compression/CMakeFiles/compression.dir/buffered_inflate_zstream.cpp.o
[ 27%] Building CXX object compression/CMakeFiles/compression.dir/buffered_deflate_zstream.cpp.o
[ 33%] Linking CXX shared library libcompression.so
[ 33%] Built target compression
[ 38%] Building CXX object compression/utest/CMakeFiles/utest.compression.dir/compression_utest_main.cpp.o
[ 44%] Building CXX object compression/utest/CMakeFiles/utest.compression.dir/compression.test.cpp.o
[ 50%] Linking CXX executable utest.compression
[ 50%] Built target utest.compression
[ 55%] Building CXX object zstream/utest/CMakeFiles/utest.zstream.dir/text.cpp.o
[ 61%] Building CXX object zstream/utest/CMakeFiles/utest.zstream.dir/zstream_utest_main.cpp.o
[ 66%] Building CXX object zstream/utest/CMakeFiles/utest.zstream.dir/zstream.test.cpp.o
[ 72%] Building CXX object zstream/utest/CMakeFiles/utest.zstream.dir/zstreambuf.test.cpp.o
[ 77%] Linking CXX executable utest.zstream
[ 77%] Built target utest.zstream
[ 83%] Building CXX object app/hello/CMakeFiles/hello.dir/hello.cpp.o
[ 88%] Linking CXX executable hello
[ 88%] Built target hello
[ 94%] Building CXX object app/myzip/CMakeFiles/myzip.dir/myzip.cpp.o
[100%] Linking CXX executable myzip
[100%] Built target myzip
```

Run unit tests:
```
$ (cd build && ctest)
Test project /home/roland/proj/cmake-examples/build
    Start 1: utest.compression
1/3 Test #1: utest.compression ................   Passed    0.00 sec
    Start 2: utest.zstream
2/3 Test #2: utest.zstream ....................   Passed    0.03 sec
    Start 3: myzip.utest
3/3 Test #3: myzip.utest ......................   Passed    0.01 sec

100% tests passed, 0 tests failed out of 3

Total Test time (real) =   0.05 sec
```

Install:
```
$ cmake --install build
-- Install configuration: ""
-- Installing: /home/roland/scratch/include/compression
-- Installing: /home/roland/scratch/include/compression/tostr.hpp
-- Installing: /home/roland/scratch/include/compression/compression.hpp
-- Installing: /home/roland/scratch/include/compression/buffered_deflate_zstream.hpp
-- Installing: /home/roland/scratch/include/compression/base_zstream.hpp
-- Installing: /home/roland/scratch/include/compression/buffered_inflate_zstream.hpp
-- Installing: /home/roland/scratch/include/compression/inflate_zstream.hpp
-- Installing: /home/roland/scratch/include/compression/buffer.hpp
-- Installing: /home/roland/scratch/include/compression/deflate_zstream.hpp
-- Installing: /home/roland/scratch/include/compression/span.hpp
-- Installing: /home/roland/scratch/lib/libcompression.so.2
-- Installing: /home/roland/scratch/lib/libcompression.so.2.3
-- Set runtime path of "/home/roland/scratch/lib/libcompression.so.2" to "/home/roland/scratch/lib"
-- Installing: /home/roland/scratch/lib/libcompression.so
-- Installing: /home/roland/scratch/include/zstream
-- Installing: /home/roland/scratch/include/zstream/zstream.hpp
-- Installing: /home/roland/scratch/include/zstream/zstreambuf.hpp
-- Installing: /home/roland/scratch/bin/hello
-- Set runtime path of "/home/roland/scratch/bin/hello" to "/home/roland/scratch/lib"
-- Installing: /home/roland/scratch/bin/myzip
-- Set runtime path of "/home/roland/scratch/bin/myzip" to "/home/roland/scratch/lib"
$ tree ~/scratch
/home/roland/scratch
├── bin
│   ├── hello
│   └── myzip
├── include
│   ├── compression
│   │   ├── base_zstream.hpp
│   │   ├── buffer.hpp
│   │   ├── buffered_deflate_zstream.hpp
│   │   ├── buffered_inflate_zstream.hpp
│   │   ├── compression.hpp
│   │   ├── deflate_zstream.hpp
│   │   ├── inflate_zstream.hpp
│   │   ├── span.hpp
│   │   └── tostr.hpp
│   └── zstream
│       ├── zstream.hpp
│       └── zstreambuf.hpp
└── lib
    ├── libcompression.so -> libcompression.so.2.3
    ├── libcompression.so.2
    └── libcompression.so.2.3 -> libcompression.so.2

5 directories, 16 files
```

# Example 14

Provide github action support.
This will only work out-of-the-box for a project hosted on github;
that said, you can expect the mechanics we rely on here to translate to other CI platforms.

```
$ cd cmake-examples
$ git checkout ex14
```

source tree:
```
$ tree .github
.github
└── workflows
    └── ex14.yml

1 directory, 1 file
```

(otherwise source tree unchanged from previous example)
```
$ tree
.
├── CMakeLists.txt
├── LICENSE
├── README.md
├── app
│   ├── hello
│   │   ├── CMakeLists.txt
│   │   └── hello.cpp
│   └── myzip
│       ├── CMakeLists.txt
│       ├── myzip.cpp
│       └── utest
│           ├── CMakeLists.txt
│           ├── myzip.utest
│           └── textfile
├── compile_commands.json -> build/compile_commands.json
├── compression
│   ├── CMakeLists.txt
│   ├── buffered_deflate_zstream.cpp
│   ├── buffered_inflate_zstream.cpp
│   ├── compression.cpp
│   ├── deflate_zstream.cpp
│   ├── include
│   │   └── compression
│   │       ├── base_zstream.hpp
│   │       ├── buffer.hpp
│   │       ├── buffered_deflate_zstream.hpp
│   │       ├── buffered_inflate_zstream.hpp
│   │       ├── compression.hpp
│   │       ├── deflate_zstream.hpp
│   │       ├── inflate_zstream.hpp
│   │       ├── span.hpp
│   │       └── tostr.hpp
│   ├── inflate_zstream.cpp
│   └── utest
│       ├── CMakeLists.txt
│       ├── compression.test.cpp
│       └── compression_utest_main.cpp
└── zstream
    ├── CMakeLists.txt
    ├── include
    │   └── zstream
    │       ├── zstream.hpp
    │       └── zstreambuf.hpp
    └── utest
        ├── CMakeLists.txt
        ├── text.cpp
        ├── text.hpp
        ├── zstream.test.cpp
        ├── zstream_utest_main.cpp
        └── zstreambuf.test.cpp

12 directories, 38 files
```

Changes:
1. new directory `.github/workflows`
2. new file `.github/workflows/ex14.yml`
   I believe any `.yml` file in `.github/workflows` will be included as a trigger for github actions.

ex14.yml:
```
# workflow for building cmake-examples
# using stock github runner (in practice some ubuntu release)
#

name: cmake-examples builder

on:
  # trigger github-hosted rebuild when contents of branch 'ex14' changes
  # (most project would use 'main' here;  the progressive branch structure
  # of cmake-examples makes that not viable, since the build we want to invoke
  # doesn't exist in the 'main' branch)
  #
  push:
    branches: [ "ex14" ]
  pull_request:
    branches: [ "ex14" ]

env:
  BUILD_TYPE: Release

jobs:
  ex14_build:
    name: compile ex14 artifacts + run unit tests
    runs-on: ubuntu-latest

    # ----------------------------------------------------------------
    # external dependencies

    steps:
    - name: install catch2
      run: sudo apt-get install -y catch2

    #- name: check package list
    #  run: apt-cache search boost

    - name: install boost program-options
      run: sudo apt-get install -y libboost-program-options1.74-dev

    # ----------------------------------------------------------------
    # filesystem tree on runner
    #
    #   ${{github.workspace}}
    #   +- repo
    #   |  \- cmake-examples     # source tree
    #   \- build
    #      \- cmake_examples     # build location
    #

    - name: checkout cmake-examples source
      # see https://github.com/actions/checkout for latest

      uses: actions/checkout@v3
      with:
        ref: ex14
        path: repo/cmake-examples

    - name: prepare build directory
      run: mkdir -p build/cmake-examples

    - name: configure cmake-examples
      run: cmake -B ${{github.workspace}}/build/cmake-examples -DCMAKE_INSTALL_PREFIX=${{github.workspace}}/local repo/cmake-examples

    - name: build cmake-examples
      run: cmake --build ${{github.workspace}}/build/cmake-examples --config ${{env.BUILD_TYPE}}

    - name: test cmake-examples
      run: (cd ${{github.workspace}}/build/cmake-examples && ctest)

    - name: install cmake-examples
      run: cmake --install ${{github.workspace}}/build/cmake-examples

```

Remarks:
1. You can review github actions activity at this url: https://github.com/rconybea/cmake-examples/actions
2. Our CI workflow starts with a stock linux image (`ubuntu-latest`) provided by github.
   We can and must install additional dependencies (`catch2`, `boost`)
3. Note that we don't get full control over the CI host environment here - for example we rely on
   the boost version that comes with whichever ubuntu release github provides;
   it's possible for CI to fail sometime if/when a non-backward-compatible change shows up in
   latest ubuntu release.
4. We can achieve a fully-reproducible CI pipeline by containerizing.
   See `.github/workflows/main.yml` in https://github.com/rconybea/xo-nix3 for github CI workflow using a custom docker container.
   See https://github.com/rconybea/docker-xo-builder for construction of the docker container
