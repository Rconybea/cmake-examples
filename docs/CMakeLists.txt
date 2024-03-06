set(DOXYGEN_INDEX_FILE ${CMAKE_CURRENT_BINARY_DIR}/html/index.html)
set(DOXYGEN_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

configure_file(
    Doxyfile.in ${DOXYGEN_CONFIG_FILE}
    FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    @ONLY)

set(DOX_DEPS ${PROJECT_SOURCE_DIR}/mainpage.dox)

# depending on {library, utest} targets here, as means to getting (indirect)
# dependency on .hpp files.
#
set(ALL_LIBRARY_TARGETS compression zstream pyzstream)
set(ALL_UTEST_TARGETS utest.compression utest.zstream)

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
    DEPENDS ${DOXYGEN_INDEX_FILE})

# - html docs generated in build/docs/html
# - copy the doc tree to share/doc/cmake-examples/html
#
install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
    FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/${PROJECT_NAME})
