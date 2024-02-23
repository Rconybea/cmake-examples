// inflate_zstream.hpp

#pragma once

#include "base_zstream.hpp"
#include "buffer.hpp"
#include <ios>
#include <cstring>

/* accept input and uncompress (aka inflate).
 * customer is responsible for providing input+output buffer space.
 * can use buffered_inflate_zstream to provision buffer space.
 */
class inflate_zstream : public base_zstream {
public:
    using span_type = span<std::uint8_t>;

public:
    inflate_zstream();
    ~inflate_zstream();

    /* uncompress some input.
     *
     * .first  = span for compressed bytes consumed
     * .second = span for uncompressed bytes produced
     */
    std::pair<span_type, span_type> inflate_chunk();

    void swap(inflate_zstream & x) {
        base_zstream::swap(x);
    }

    /* move-assignment */
    inflate_zstream & operator= (inflate_zstream && x) {
        base_zstream::operator=(std::move(x));
        return *this;
    }

private:
    /* calls ::inflateInit2() */
    void setup();
    /* calls ::inflateEnd() */
    void teardown();
};

inline void
swap(inflate_zstream & x, inflate_zstream & y) noexcept {
    x.swap(y);
}
