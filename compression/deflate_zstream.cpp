// deflate_zstream.cpp

#include "compression/deflate_zstream.hpp"

using namespace std;

deflate_zstream::deflate_zstream()
{
    this->setup();
}

deflate_zstream::~deflate_zstream() {
    this->teardown();
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

void
deflate_zstream::setup() {
    z_stream * const pzs(p_native_zs_.get());

    pzs->zalloc    = Z_NULL;
    pzs->zfree     = Z_NULL;
    pzs->opaque    = Z_NULL;
    pzs->avail_in  = 0;
    pzs->next_in   = Z_NULL;
    pzs->avail_out = 0;
    pzs->next_out  = Z_NULL;

    int ret = ::deflateInit2(pzs,
                             Z_DEFAULT_COMPRESSION,
                             Z_DEFLATED,
                             MAX_WBITS + 16 /* +16 tells zlib to write .gzip header*/,
                             8 /*memlevel 1-9; higher to spend memory for more speed+compression. default=8*/,
                             Z_DEFAULT_STRATEGY);

    if (ret != Z_OK)
        throw runtime_error("deflate_zstream: failed to initialize .p_native_zs");
}

void
deflate_zstream::teardown() {
    ::deflateEnd(p_native_zs_.get());
}

