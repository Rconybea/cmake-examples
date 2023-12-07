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
2. github actions
3. executables, shared libraries, interdependencies
4. header-only libraries
5. monorepo build
   limitations of monorepo build
6. separable build (+ find_package() support)
   limitations of separable build
7. versioning, explicit codebase dependencies, build isolation
8. pybind11 + python issues
   binary API dependence

## Progression
1. ex1: c++ executable X
2. ex2: add LSP integration
3. ex3: c++ executable X + cmake-aware outside library O1 (boost::program_options), using find_package())
4. ex4: c++ executable X + non-cmake outside library O2 (zlib)
5. ex5: refactor: move compression wrapper to 2nd translation unit
6. ex6: add install target

7. ex7: c++ executable X + c++ library A1, monorepo-style

5. c++ executable X + header-only library (catch2) + unit test
5. c++ executable X + c++ library A1, A1 -> O2, monorepo-style.
   X,A1 in same repo + build together.

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
$ cmake -B build       # configure
-- Configuring done
-- Generating done
-- Build files have been written to: /home/roland/proj/cmake-examples/build
$ cmake --build build  # compile
[ 50%] Building CXX object CMakeFiles/hello.dir/hello.cpp.o
[100%] Linking CXX executable hello
[100%] Built target hello
$ ./build/hello        # run
Hello, world!

$
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
   On success establishes cmake variables `zlib_CFLAGS_OTHER`, `zlib_INCLUDE_DIRS`, `zlib_LIBRARIES`
3. `target_include_directories(${SELF_EXE} PUBLIC ${zlib_INCLUDE_DIRS})`
   to tell compiler where to find zlib include files.
4. `target_link_libraries(${SELF_EXE} PUBLIC ${zlib_LIBRARIES})`
   to tell compiler how to link zlib

```
# CMakeLists.txt
cmake_minimum_required(VERSION 3.25)
project(ex1 VERSION 1.0)
enable_language(CXX)

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
