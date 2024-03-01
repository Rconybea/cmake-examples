set(SELF_UTEST utest.compression)
set(SELF_SRCS compression_utest_main.cpp compression.test.cpp hex.test.cpp)

add_executable(${SELF_UTEST} ${SELF_SRCS})
target_link_libraries(${SELF_UTEST} PUBLIC compression)
target_link_libraries(${SELF_UTEST} PUBLIC Catch2::Catch2)

add_test(
    NAME ${SELF_UTEST}
    COMMAND ${SELF_UTEST})
