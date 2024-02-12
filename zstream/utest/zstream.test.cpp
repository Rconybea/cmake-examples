#include "text.hpp"
#include "zstream/zstream.hpp"
#include "catch2/catch.hpp"

using namespace std;

TEST_CASE("zstream", "[zstream]") {
    /* true to enable some logging,  useful if this unit test should fail */
    constexpr bool c_debug_flag = false;

    /* make some buffer space */
    using zbuf_type = array<char, 64*1024>;
    unique_ptr<zbuf_type> zbuf(new zbuf_type());
    std::fill(zbuf->begin(), zbuf->end(), '\0');

    size_t n_z_out_total = 0;

    /* compress.. */
    {
        zstream zs(64 * 1024, std::move(unique_ptr<streambuf>(new stringbuf())), ios::out);

        zs.rdbuf()->native_sbuf()->pubsetbuf(&((*zbuf)[0]), zbuf->size());

        zs << Text::s_text << endl;

        /* reminder: have to close zstream to get complete compressed output. */
        zs.close();

        if (c_debug_flag) {
            cout << "uc out: " << zs.rdbuf()->n_uc_out_total() << endl;
            cout << "z  out: " << zs.rdbuf()->n_z_out_total() << endl;
        }

        size_t n = zs.rdbuf()->n_z_out_total();

        if (c_debug_flag) {
            size_t i = 0;
            while (i < n) {
                /* 64 hex values */
                do {
                    uint8_t ch = (*zbuf)[i];

                    cout << " " << ::hex(ch);
                    ++i;
                } while ((i < n) && (i % 64 != 0));

                cout << endl;
            }
        }

        n_z_out_total = n;
    }

    /* now decompress.. */
    {
        zstream zs(64 * 1024,
                   std::move(unique_ptr<streambuf>(new stringbuf())),
                   ios::in);

        zs.rdbuf()->native_sbuf()->pubsetbuf(&((*zbuf)[0]), n_z_out_total);

        unique_ptr<zbuf_type> zbuf2(new zbuf_type());
        std::fill(zbuf2->begin(), zbuf2->end(), '\0');

        unique_ptr<zbuf_type> ucbuf2(new zbuf_type());
        std::fill(ucbuf2->begin(), ucbuf2->end(), '\0');

        zs.read(&((*ucbuf2)[0]), ucbuf2->size());
        streamsize n_read = zs.gcount();

        CHECK(n_read == static_cast<streamsize>(strlen(Text::s_text) + 1));

        INFO("uncompressed input:");
        INFO(string_view(&((*ucbuf2)[0]), &((*ucbuf2)[n_read])));

        for (streamsize i=0; i<n_read-1; ++i) {
            INFO(tostr("i=", i, ", s_text[i]=", Text::s_text[i], ", ucbuf2[i]=", (*ucbuf2)[i]));

            REQUIRE(Text::s_text[i] == (*ucbuf2)[i]);
        }
    }
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

/* use zstream + write to file on disk.
 */
TEST_CASE("zstream-filebuf", "[zstream]") {
    for (size_t i_tc = 0; i_tc < s_testcase_v.size(); ++i_tc) {
        TestCase const & tc = s_testcase_v[i_tc];

        INFO(tostr("i_tc=", i_tc));

        // ----------------------------------------------------------------
        // 1 - compress some text
        // ----------------------------------------------------------------

        std::string fname = tostr("test", i_tc, ".gz");

        {
            INFO(tostr("writing to fname=", fname));

            zstream zs(fname.c_str(), ios::out);

            /* could just do
             *   zs.write(Text::s_text, strlen(Text::s_text))
             * here.
             *
             * Instead write from s_text in small chunk sizes
             */
            size_t const c_write_z = tc.write_chunk_z_;

            for (size_t i=0, n=strlen(Text::s_text); i<n;) {
                size_t nreq = std::min(c_write_z, n-i);

                zs.write(Text::s_text + i, nreq);
                i += nreq;
            }

            zs.close();
        }

        // ----------------------------------------------------------------
        // 2 - uncompress + verify
        // ----------------------------------------------------------------

        /* NOTE:
         * Can also demonstrate successful compression step with for example
         *   $ gunzip -c test0.gz
         */

        {
            INFO(tostr("reading from fname=", fname));

            zstream zs(fname.c_str(), ios::in);

            std::string input;
            input.resize(strlen(Text::s_text));

            size_t const c_read_z = tc.read_chunk_z_;
            size_t n_uc = 0;
            size_t i_uc = 0;
            do {
                zs.read(input.data() + n_uc, c_read_z);
                i_uc = zs.gcount();
                n_uc += i_uc;
            } while (i_uc == c_read_z);

            REQUIRE(n_uc == input.size());

            CHECK(n_uc == ::strlen(Text::s_text));

            for (size_t i=0; i<n_uc; ++i) {
                INFO(tostr("i=", i, ", s_text[i]=", Text::s_text[i], ", input[i]=", input[i]));

                REQUIRE(Text::s_text[i] == input[i]);
            }
        }

        // ----------------------------------------------------------------
        // 3 - cleanup
        // ----------------------------------------------------------------

        ::remove(fname.c_str());
    }
}
