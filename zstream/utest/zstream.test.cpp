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
