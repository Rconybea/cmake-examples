// inflate_zstream.cpp

#include "compression/inflate_zstream.hpp"
#include "compression/tostr.hpp"

using namespace std;

inflate_zstream::inflate_zstream() {
    zstream_.zalloc    = Z_NULL;
    zstream_.zfree     = Z_NULL;
    zstream_.opaque    = Z_NULL;
    zstream_.avail_in  = 0;
    zstream_.next_in   = Z_NULL;
    zstream_.avail_out = 0;
    zstream_.next_out  = Z_NULL;

    int ret = ::inflateInit(&zstream_);

    if (ret != Z_OK)
        throw std::runtime_error("inflate_zstream: failed to initialize .zstream");
}

inflate_zstream::~inflate_zstream() {
    ::inflateEnd(&zstream_);
}

std::streamsize
inflate_zstream::inflate_chunk() {
    std::streamsize n_avail_pre = zstream_.avail_out;

    int err = ::inflate(&zstream_, Z_NO_FLUSH);

    switch(err) {
    case Z_NEED_DICT:
        err = Z_DATA_ERROR;
        /* fallthru */
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
        throw std::runtime_error(tostr("zstreambuf::inflate_chunk: error [", err, "] from zlib inflate"));
    }

    streamsize n_avail_post = zstream_.avail_out;
    streamsize n_read = (n_avail_pre - n_avail_post);

    return n_read;
}

std::pair<span<std::uint8_t>, span<std::uint8_t>>
inflate_zstream::inflate_chunk2() {
    uint8_t * z_pre = zstream_.next_in;
    uint8_t * uc_pre = zstream_.next_out;

    int err = ::inflate(&zstream_, Z_NO_FLUSH);

    switch(err) {
    case Z_NEED_DICT:
        err = Z_DATA_ERROR;
        /* fallthru */
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
        throw std::runtime_error(tostr("zstreambuf::inflate_chunk: error [", err, "] from zlib inflate"));
    }

    uint8_t * z_post = zstream_.next_in;
    uint8_t * uc_post = zstream_.next_out;

    return pair<span_type, span_type>(span_type(z_pre, z_post),
                                      span_type(uc_pre, uc_post));
} /*inflate_chunk2*/
