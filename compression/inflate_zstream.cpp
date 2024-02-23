// inflate_zstream.cpp

#include "compression/inflate_zstream.hpp"
#include "compression/tostr.hpp"

using namespace std;

inflate_zstream::inflate_zstream() {
    this->setup();
}

inflate_zstream::~inflate_zstream() {
    this->teardown();
}

std::pair<span<std::uint8_t>, span<std::uint8_t>>
inflate_zstream::inflate_chunk () {
    /* Z = compressed data
     * U = uncompressed   data
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
     * input:  ZZZZZZZZZZZZZZZZZZZZZZZZZZZ       output:  UUUUUUUUUUUUU......................
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

    z_stream * const pzs(p_native_zs_.get());

    uint8_t * z_pre = pzs->next_in;
    uint8_t * uc_pre = pzs->next_out;

    int err = ::inflate(pzs, Z_NO_FLUSH);

    switch(err) {
    case Z_NEED_DICT:
        err = Z_DATA_ERROR;
        /* fallthru */
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
        throw std::runtime_error(tostr("inflate_zstream::inflate_chunk: error [", err, "] from zlib inflate"));
    }

    uint8_t * z_post = pzs->next_in;
    uint8_t * uc_post = pzs->next_out;

    return pair<span_type, span_type>(span_type(z_pre, z_post),
                                      span_type(uc_pre, uc_post));
}

void
inflate_zstream::setup() {
    z_stream * const pzs(p_native_zs_.get());

    pzs->zalloc    = Z_NULL;
    pzs->zfree     = Z_NULL;
    pzs->opaque    = Z_NULL;
    pzs->avail_in  = 0;
    pzs->next_in   = Z_NULL;
    pzs->avail_out = 0;
    pzs->next_out  = Z_NULL;

    int ret = ::inflateInit2(pzs,
                             MAX_WBITS + 32 /* +32 tells zlib to detect zlib/gzip encoding + handle either*/);

    if (ret != Z_OK)
        throw std::runtime_error("inflate_zstream: failed to initialize .zstream");
}

void
inflate_zstream::teardown() {
    ::inflateEnd(p_native_zs_.get());
}

