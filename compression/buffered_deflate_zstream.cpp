// buffered_deflate_zstream.cpp

#include "compression/buffered_deflate_zstream.hpp"

using namespace std;

auto
buffered_deflate_zstream::deflate_chunk(bool final_flag) -> size_type
{
    if (zs_algo_.have_input() || final_flag) {
        std::pair<z_span_type, z_span_type> x = zs_algo_.deflate_chunk2(final_flag);

        uc_in_buf_.consume(x.first);
        z_out_buf_.produce(x.second);

        return x.second.size();
    } else {
        return 0;
    }
}
