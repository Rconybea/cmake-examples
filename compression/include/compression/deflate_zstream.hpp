// deflate_zstream.hpp

#pragma once

#include "base_zstream.hpp"
#include "buffer.hpp"
#include <zlib.h>
#include <ios>
#include <cstring>

class deflate_zstream : public base_zstream {
public:
    using span_type = span<std::uint8_t>;

public:
    deflate_zstream();
    ~deflate_zstream();

    /* compress some output,  return #of compressed bytes obtained
     *
     * final_flag.  must set to true end of uncompressed input reached,
     *              so that .zstream knows to flush compressed state
     */
    std::streamsize deflate_chunk(bool final_flag);

    /* .first = span for uncompressed bytes consumed
     * .second = span for compressed bytes produced
     */
    std::pair<span_type, span_type> deflate_chunk2(bool final_flag);

    void swap(deflate_zstream & x) {
        base_zstream::swap(x);
    }

    /* move-assignment */
    deflate_zstream & operator= (deflate_zstream && x) {
        base_zstream::operator=(std::move(x));
        return *this;
    }
};

namespace std {
    inline void
    swap(deflate_zstream & lhs, deflate_zstream & rhs) {
        lhs.swap(rhs);
    }
}