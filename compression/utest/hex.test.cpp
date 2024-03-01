#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include "compression/hex.hpp"
#include <catch2/catch.hpp>
#include <sstream>

using namespace std;

TEST_CASE("hex", "[hex]") {
    stringstream ss;

    ss << ::hex(15, false);

    REQUIRE(ss.str() == "0f");
}

TEST_CASE("hex_view", "[hex]") {
    stringstream ss;

    array<uint8_t, 3> v = {{ 10, 20, 30 }};

    ss << ::hex_view(&v[0], &v[v.size()], false);

    REQUIRE(ss.str() == "[0a 14 1e]");
}

TEST_CASE("hex_view_2", "[hex]") {
    stringstream ss;

    array<uint8_t, 3> v = {{ 'a', 'm', 'z' }};

    ss << ::hex_view(&v[0], &v[v.size()], true);

    REQUIRE(ss.str() == "[61(a) 6d(m) 7a(z)]");
}
