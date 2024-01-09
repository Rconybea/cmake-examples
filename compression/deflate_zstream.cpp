// deflate_zstream.cpp

#include "compression/deflate_zstream.hpp"

using namespace std;

deflate_zstream::deflate_zstream()
{
    zstream_.zalloc    = Z_NULL;
    zstream_.zfree     = Z_NULL;
    zstream_.opaque    = Z_NULL;
    zstream_.avail_in  = 0;
    zstream_.next_in   = Z_NULL;
    zstream_.avail_out = 0;
    zstream_.next_out  = Z_NULL;

    int ret = ::deflateInit(&zstream_, Z_DEFAULT_COMPRESSION);

    if (ret != Z_OK)
        throw runtime_error("deflate_zstream: failed to initialize .zstream");
}

deflate_zstream::~deflate_zstream() {
    ::deflateEnd(&zstream_);
}

pair<span<uint8_t>, span<uint8_t>>
deflate_zstream::deflate_chunk(bool final_flag) {
    /* U = uncompressed data
     * Z = compressed   data
     *
     *                          (pre) zstream
     *         /--------------    .next_in
     *         |                  .next_out    -----------\
     *         |                                          |
     *         |                                          |
     *         v    (pre)                                 v    (pre)
     *              zstream                                    zstream
     *         <--  .avail_in ----------->                <--  .avail_out ------------------>
     *
     * input:  UUUUUUUUUUUUUUUUUUUUUUUUUUU       output:  ZZZZZZZZZZZZZ......................
     *         ^        ^                                 ^            ^
     *         uc_pre   uc_post                           z_pre        z_post
     *
     *                  <--- (post)  ---->                             <--- (post)   ------->
     *                       zstream                                        zstream
     *                  ^    .avail_in                                 ^    .avail_in
     *                  |                                              |
     *                  |       (post) zstream                         |
     *                  \------   .next_in                             |
     *                            .next_out ---------------------------/
     *
     *         < retval >                                 <  retval    >
     *         < .first >                                 <  .second   >
     *
     */

    uint8_t * uc_pre = zstream_.next_in;
    uint8_t * z_pre = zstream_.next_out;

    int err = ::deflate(&zstream_,
                        (final_flag ? Z_FINISH : 0) /*flush*/);

    if (err == Z_STREAM_ERROR)
        throw runtime_error("basic_zstreambuf::sync: impossible zlib deflate returned Z_STREAM_ERROR");

    uint8_t * uc_post = zstream_.next_in;
    uint8_t * z_post = zstream_.next_out;

    return pair<span_type, span_type>(span_type(uc_pre, uc_post),
                                      span_type(z_pre, z_post));
}
