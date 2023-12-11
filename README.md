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

4. c++ executable X + library A, A -> O, separable-style
   provide find_package() support - can build using X-subdir's cmake if A built+installed
5. project-specific macros - simplify
6. project-specific macros - support (monorepo, separable) builds from same tree
7. c++ executable X + library A + library B.
8. c++ executable X + library A + library B + library C, A -> B -> C, C header-only
9. add unit tests + performance benchmarks.
10. add code coverage.

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
