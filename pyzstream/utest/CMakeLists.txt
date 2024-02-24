set(SELF_UTEST pyzstream.utest)
set(SELF_UTEST_EXE ${CMAKE_CURRENT_BINARY_DIR}/${SELF_UTEST})

# generate runner script so that it's convenient to invoke from command line
# (instead of only via ctest)
#
configure_file(
    ${SELF_UTEST}.in ${SELF_UTEST_EXE}
    FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    @ONLY
)

# unit test will run in the build directory
add_test(
    NAME ${SELF_UTEST}
    COMMAND ${BASH_EXECUTABLE} ${SELF_UTEST_EXE}
    )
