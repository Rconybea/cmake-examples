#include "text.hpp"
#include "zstream/zstreambuf.hpp"
#include "catch2/catch.hpp"

#include <string_view>
#include <sstream>
#include <array>

using namespace std;

struct text_compare {
    text_compare(string_view s1, string_view s2) : s1_{std::move(s1)}, s2_{std::move(s2)} {}

    string_view s1_;
    string_view s2_;
};

ostream &
operator<< (ostream & os, text_compare const & x) {
    size_t n1 = x.s1_.size();
    size_t n2 = x.s2_.size();
    size_t n = std::min(n1, n2);

    size_t i = 0;

    while (i < n) {
        os << i << ": ";

        /* print all of s1(i .. i+99) */
        size_t line1 = std::min(i + 50, n1);
        for (size_t i1 = i; i1 < line1; ++i1) {
            if (isprint(x.s1_[i1]))
                os << x.s1_[i1];
            else
                os << "\\";
        }
        os << endl;

        os << i << ": ";

        /* print s2(i) only when != s1(i) */
        size_t line2 = std::min(i + 50, n2);
        for (size_t i2 = i; i2 < line2; ++i2) {
            if (i2 < line1 && (x.s2_[i2] == x.s1_[i2]))
                os << " ";
            else if (isprint(x.s2_[i2]))
                os << x.s2_[i2];
            else
                os << "\\";
        }
        os << endl;

        i += 50;
    }

    return os;
}

namespace {
    struct TestCase {
        TestCase(uint32_t bufz, uint32_t wz, uint32_t rz)
            : buf_z_{bufz}, write_chunk_z_{wz}, read_chunk_z_{rz} {}

        /* buffer size for zstreambuf - applies to buffers for:
         * - uncompressed input + output
         * - compressed input + output
         */
        uint32_t buf_z_ = 0;
        /* write uncompressed text in chunks of this size */
        uint32_t write_chunk_z_ = 0;
        /* read uncompresseed text in chunks of this size */
        uint32_t read_chunk_z_ = 0;
    };

    static vector<TestCase> s_testcase_v = {
        TestCase(1, 1, 1),
        TestCase(1, 256, 256),
        TestCase(256, 15, 15),
        TestCase(256, 16, 16),
        TestCase(256, 17, 17),
        TestCase(256, 129, 129),
        TestCase(65536, 129, 129),
        TestCase(65536, 65536, 65536)
    };
}

TEST_CASE("zstreambuf", "[zstreambuf]") {
    /* true to enable some logging,  useful if this unit test should fail */
    constexpr bool c_debug_flag = false;

    for (size_t i_tc = 0; i_tc < s_testcase_v.size(); ++i_tc) {
        TestCase const & tc = s_testcase_v[i_tc];

        INFO(tostr("i_tc=", i_tc));

        // ----------------------------------------------------------------
        // phase 1 - compress some text
        // ----------------------------------------------------------------

        /* buffer to hold compressed output */
        using zbuf_type = array<char, 64*1024>;
        unique_ptr<zbuf_type> zbuf(new zbuf_type());

        for (size_t i=0, n=sizeof(zbuf_type); i<n; ++i)
            (*zbuf)[i] = '\0';

        /* compressed output will appear here */
        unique_ptr<streambuf> zsbuf(new stringbuf());

        zsbuf->pubsetbuf(&((*zbuf)[0]), sizeof(zbuf_type));

        /* 256: for unit test want to exercise overflow.. frequently */
        unique_ptr<zstreambuf> ogbuf(new zstreambuf(tc.buf_z_));

        ogbuf->adopt_native_sbuf(std::move(zsbuf));

        /* write from s_text in small chunk sizes */
        size_t const c_write_z = tc.write_chunk_z_;

        for (size_t i=0, n=strlen(Text::s_text); i<n;) {
            size_t nreq = std::min(c_write_z, n-i);
            REQUIRE(ogbuf->sputn(Text::s_text + i, nreq) == static_cast<streamsize>(nreq));

            i += nreq;
        }

        ogbuf->close();

        if (c_debug_flag) {
            cout << "uc out: " << ogbuf->n_uc_out_total() << endl;
            cout << "z  out: " << ogbuf->n_z_out_total() << endl;

            size_t i = 0;
            size_t n = ogbuf->n_z_out_total();
            while (i < n) {
                /* 64 hex values */
                do {
                    uint8_t ch = (*zbuf)[i];
                    uint8_t lo = ch & 0xf;
                    uint8_t hi = ch >> 4;
                    char lo_ch = (lo < 10) ? '0' + lo : 'a' + lo - 10;
                    char hi_ch = (hi < 10) ? '0' + hi : 'a' + hi - 10;

                    cout << " " << hi_ch << lo_ch;

                    ++i;
                } while ((i < n) && (i % 64 != 0));

                cout << endl;
            }
        }

        // ----------------------------------------------------------------
        // phase 2 - now decompress compressed output,
        //           make sure we recover original text
        // ----------------------------------------------------------------

        unique_ptr<streambuf> zsbuf2(new stringbuf());
        zsbuf2->pubsetbuf(&((*zbuf)[0]), ogbuf->n_z_out_total());

        unique_ptr<zstreambuf> ogbuf2(new zstreambuf(tc.buf_z_));
        ogbuf2->adopt_native_sbuf(std::move(zsbuf2));

        /* read from ogbuf2 in small chunk sizes */
        unique_ptr<zbuf_type> ucbuf2(new zbuf_type());

        size_t const c_read_z = tc.read_chunk_z_;
        size_t i_uc = 0;
        size_t n_uc = 0;

        do {
            n_uc = ogbuf2->sgetn(&((*ucbuf2)[i_uc]), c_read_z);
            i_uc += n_uc;
        } while (n_uc == c_read_z);

        //INFO(tostr("uc_buf2=", hex_view(&(*ucbuf2)[0], &(*ucbuf2)[ogbuf2->n_uc_in_total()], true /*as_text*/)));
        INFO(text_compare(string_view(Text::s_text),
                          string_view(&(*ucbuf2)[0], &(*ucbuf2)[i_uc])));

        CHECK(ogbuf2->n_z_in_total() == ogbuf->n_z_out_total());
        CHECK(ogbuf2->n_uc_in_total() == ogbuf->n_uc_out_total());
        CHECK(i_uc == ::strlen(Text::s_text));

        for (size_t i=0; i<i_uc; ++i) {
            INFO(tostr("i=", i, ", s_text[i]=", Text::s_text[i], ", ucbuf2[i]=", (*ucbuf2)[i]));

            REQUIRE(Text::s_text[i] == (*ucbuf2)[i]);
        }
    }
}
