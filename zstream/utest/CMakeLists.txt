set(SELF_UTEST utest.zstream)
set(SELF_SRCS text.cpp zstream_utest_main.cpp zstream.test.cpp zstreambuf.test.cpp)

add_executable(${SELF_UTEST} ${SELF_SRCS})
target_link_libraries(${SELF_UTEST} PUBLIC zstream)
target_link_libraries(${SELF_UTEST} PUBLIC Catch2::Catch2)

add_test(
    NAME ${SELF_UTEST}
    COMMAND ${SELF_UTEST})
