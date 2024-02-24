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
        TestCase("Jabberwocky,  by Lewis Carroll\n"
                 "\n"
                 "’Twas brillig, and the slithy toves\n"
                 "      Did gyre and gimble in the wabe:\n"
                 "All mimsy were the borogoves,\n"
                 "      And the mome raths outgrabe.\n"
                 "\n"
                 "“Beware the Jabberwock, my son!\n"
                 "      The jaws that bite, the claws that catch!\n"
                 "Beware the Jubjub bird, and shun\n"
                 "      The frumious Bandersnatch!”\n"
                 "\n"
                 "He took his vorpal sword in hand;\n"
                 "      Long time the manxome foe he sought—\n"
                 "So rested he by the Tumtum tree\n"
                 "      And stood awhile in thought.\n"
                 "\n"
                 "And, as in uffish thought he stood,\n"
                 "      The Jabberwock, with eyes of flame,\n"
                 "Came whiffling through the tulgey wood,\n"
                 "      And burbled as it came!\n"
                 "\n"
                 "One, two! One, two! And through and through\n"
                 "      The vorpal blade went snicker-snack!\n"
                 "He left it dead, and with its head\n"
                 "      He went galumphing back.\n"
                 "\n"
                 "“And hast thou slain the Jabberwock?\n"
                 "      Come to my arms, my beamish boy!\n"
                 "O frabjous day! Callooh! Callay!”\n"
                 "      He chortled in his joy.\n"
                 "\n"
                 "’Twas brillig, and the slithy toves\n"
                 "      Did gyre and gimble in the wabe:\n"
                 "All mimsy were the borogoves,\n"
                 "      And the mome raths outgrabe.\n"
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
