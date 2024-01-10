#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include "compression/compression.hpp"
#include "compression/tostr.hpp"
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
        TestCase("A man, a plan, a canal - Panama!"),
        TestCase("Jabberwocky,  by Lewis Carroll"
                 ""
                 "’Twas brillig, and the slithy toves"
                 "      Did gyre and gimble in the wabe:"
                 "All mimsy were the borogoves,"
                 "      And the mome raths outgrabe."
                 ""
                 "“Beware the Jabberwock, my son!"
                 "      The jaws that bite, the claws that catch!"
                 "Beware the Jubjub bird, and shun"
                 "      The frumious Bandersnatch!”"
                 ""
                 "He took his vorpal sword in hand;"
                 "      Long time the manxome foe he sought—"
                 "So rested he by the Tumtum tree"
                 "      And stood awhile in thought."
                 ""
                 "And, as in uffish thought he stood,"
                 "      The Jabberwock, with eyes of flame,"
                 "Came whiffling through the tulgey wood,"
                 "      And burbled as it came!"
                 ""
                 "One, two! One, two! And through and through"
                 "      The vorpal blade went snicker-snack!"
                 "He left it dead, and with its head"
                 "      He went galumphing back."
                 ""
                 "“And hast thou slain the Jabberwock?"
                 "      Come to my arms, my beamish boy!"
                 "O frabjous day! Callooh! Callay!”"
                 "      He chortled in his joy."
                 ""
                 "’Twas brillig, and the slithy toves"
                 "      Did gyre and gimble in the wabe:"
                 "All mimsy were the borogoves,"
                 "      And the mome raths outgrabe."
            )
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

namespace {
    void compression_benchmark(char const * deflate_name,
                               char const * inflate_name,
                               size_t problem_size)
    {
        constexpr size_t i_tc = 2;
        size_t og_data_z = 0;
        vector<uint8_t> og_data_v;
        vector<uint8_t> z_data_v;

        BENCHMARK_ADVANCED(deflate_name)(Catch::Benchmark::Chronometer clock) {
            /* 1. setup */

            size_t text_z = s_testcase_v[i_tc].og_text.size();

            /* test string comprising consecutive copies of test pattern */
            og_data_z = problem_size;
            og_data_v.reserve(og_data_z);

            for (size_t i_copy = 0; i_copy * text_z < problem_size; ++i_copy) {
                size_t i_start = i_copy * text_z;
                size_t i_end   = std::min((i_copy + 1) * text_z, problem_size);

                std::copy(s_testcase_v[i_tc].og_text.begin(),
                          s_testcase_v[i_tc].og_text.begin() + (i_end - i_start),
                          og_data_v.begin() + i_start);
            }

            /* 2. run */

            clock.measure([&og_data_v, &z_data_v] {
                z_data_v = compression::deflate(og_data_v);

                return z_data_v.size();  /* just to make sure optimizer doesn't interfere */
            });
        };

        vector<uint8_t> og_data2_v;

        BENCHMARK(inflate_name) {
            og_data2_v = compression::inflate(z_data_v, og_data_z);

            return og_data2_v.size();
        };

        REQUIRE(og_data_v == og_data2_v);
    }
}

TEST_CASE("compression-benchmark", "[!benchmark]") {
    compression_benchmark("deflate-128k", "inflate-128k", 128*1024);
    compression_benchmark("deflate-1m", "inflate-1m", 1024*1024);
    compression_benchmark("deflate-10m", "inflate-10m", 10*1024*1024);
    compression_benchmark("deflate-128m", "inflate-128m", 128*1024*1024);
    compression_benchmark("deflate-1g", "inflate-1g", 1024*1024*1024);
}
