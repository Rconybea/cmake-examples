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
        TestCase("A man, a plan, a canal - Panama!")
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
