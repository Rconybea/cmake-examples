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
2. c++ executable X + outside library O, using find_package()
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

## Example 1a

LSP integration allows compiler-driven editor interaction -- syntax highlighting,  code navigation etc.
