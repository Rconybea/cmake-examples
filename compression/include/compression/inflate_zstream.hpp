// inflate_zstream.hpp

#pragma once

#include "base_zstream.hpp"
#include "buffer.hpp"
#include <ios>
#include <cstring>

class inflate_zstream : public base_zstream {
public:
    using span_type = span<std::uint8_t>;

public:
    inflate_zstream();
    ~inflate_zstream();

    /* decompress some input,  return #of uncompressed bytes obtained */
    std::streamsize inflate_chunk();

    /* .first  = span for compressed bytes consumed
     * .second = span for uncompressed bytes produced
     */
    std::pair<span_type, span_type> inflate_chunk2();

    void swap (inflate_zstream & x) {
        base_zstream::swap(x);
    }

    /* move-assignment */
    inflate_zstream & operator= (inflate_zstream && x) {
        base_zstream::operator=(std::move(x));
        return *this;
    }
};

namespace std {
    inline void
    swap(inflate_zstream & lhs, inflate_zstream & rhs) {
        lhs.swap(rhs);
    }
}
