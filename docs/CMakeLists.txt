set(DOXYGEN_INDEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/html/index.html)
set(DOXYGEN_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

set(DOX_DEPS ${PROJECT_SOURCE_DIR}/mainpage.dox)
set(DOX_INPUT_DIR ${PROJECT_SOURCE_DIR})
set(DOX_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/dox)

configure_file(
    Doxyfile.in ${DOXYGEN_CONFIG_FILE}
    FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    @ONLY)

# depending on {library, utest} targets here:
# - as means to getting (indirect) dependency on .hpp files
# - so that command rebuilds docs when some source file changed
#
set(ALL_LIBRARY_TARGETS compression zstream pyzstream)
set(ALL_UTEST_TARGETS utest.compression utest.zstream)

file(MAKE_DIRECTORY ${DOX_OUTPUT_DIR})
add_custom_command(
    OUTPUT ${DOXYGEN_INDEX_FILE}
    DEPENDS ${DOX_DEPS} ${ALL_LIBRARY_TARGETS} ${ALL_UTEST_TARGETS}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    MAIN_DEPENDENCY ${DOXYGEN_CONFIG_FILE}
    COMMENT "Generating docs (doxygen)")

# To build this target
#   $ cmake --build .build -j -- doxygen
# or
#   $ cd .build
#   $ make doxygen
#
add_custom_target(
    doxygen
    DEPENDS ${DOXYGEN_INDEX_FILE}
)

# root of sphinx doc tree
set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)

# TODO: add_custom_command as afor doxygen;  output CMAKE_CURRENT_BINARY_DIR/sphinx/index.html

# make sphinx --> generate sphinx documentation
add_custom_target(
    sphinx
    ALL
    COMMAND ${SPHINX_EXECUTABLE} -b html
    -Dbreathe_projects.cmkx=${CMAKE_CURRENT_BINARY_DIR}/dox/xml
    ${SPHINX_SOURCE} ${SPHINX_BUILD}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating docs (sphinx)")

# - html docs generated in build/docs/html
# - copy the doc tree to share/doc/cmake-examples/html
#
#  OPTIONAL: install directory tree if it exists,
#            but don't complain if it's missing
install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/${PROJECT_NAME}
    COMPONENT Documentation
    OPTIONAL)
