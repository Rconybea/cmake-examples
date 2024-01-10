set(SELF_UTEST utest.compression)
set(SELF_BENCHMARK benchmark.compression)
set(SELF_SRCS compression_utest_main.cpp compression.test.cpp)

add_executable(${SELF_UTEST} ${SELF_SRCS})
target_link_libraries(${SELF_UTEST} PUBLIC compression)
target_link_libraries(${SELF_UTEST} PUBLIC Catch2::Catch2)

add_test(
    NAME ${SELF_UTEST}
    COMMAND ${SELF_UTEST})

add_test(
    NAME ${SELF_BENCHMARK}
    COMMAND ${SELF_UTEST} ~[benchmark] --benchmark-no-analysis --benchmark-samples 1)
set_tests_properties(${SELF_BENCHMARK} PROPERTIES LABELS "benchmark")
