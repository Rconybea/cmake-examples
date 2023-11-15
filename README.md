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
1. c++ executable X (branch ex1)
2. add LSP integration (branch ex2)
3. c++ executable X + outside library O, using find_package()
3. c++ executable X + library A, A -> O, monorepo-style.
   X,A in same repo + build together.
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
1. `find_package()`
2. `target_link_libraries()`

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
