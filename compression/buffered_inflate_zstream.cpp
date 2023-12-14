// buffered_inflate_zstream.cpp

#include "compression/buffered_inflate_zstream.hpp"

using namespace std;

auto
buffered_inflate_zstream::inflate_chunk() -> size_type
{
    if (zs_algo_.have_input()) {
        std::pair<z_span_type, z_span_type> x = zs_algo_.inflate_chunk2();

        z_in_buf_.consume(x.first);
        uc_out_buf_.produce(x.second);

        return x.second.size();
    } else {
        return 0;
    }
}
