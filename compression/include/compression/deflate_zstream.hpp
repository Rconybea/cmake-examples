// deflate_zstream.hpp

#pragma once

#include "base_zstream.hpp"
#include "buffer.hpp"
#include <zlib.h>
#include <ios>
#include <cstring>

/* accept input and compress (aka deflate).
 * customer is responsible for providing input+output buffer space.
 * can use buffered_deflate_zstream to provision buffer space.
 */
class deflate_zstream : public base_zstream {
public:
    using span_type = span<std::uint8_t>;

public:
    deflate_zstream();
    /* move-only */
    deflate_zstream(deflate_zstream const & x) = delete;
    ~deflate_zstream();

    /* cleanup+reestablish nominal .p_native_zs state */
    void rebuild();

    /* compress some output,  return #of compressed bytes obtained
     *
     * final_flag.  must set to true end of uncompressed input reached,
     *              so that .zstream knows to flush compressed state
     *
     * .first = span for uncompressed bytes consumed
     * .second = span for compressed bytes produced
     */
    std::pair<span_type, span_type> deflate_chunk(bool final_flag);

    void swap(deflate_zstream & x) {
        base_zstream::swap(x);
    }

    deflate_zstream & operator= (deflate_zstream && x) {
        base_zstream::operator=(std::move(x));
        return *this;
    }

private:
    /* calls ::deflateInit2() */
    void setup();
    /* calls ::deflateEnd() */
    void teardown();
};

inline void
swap(deflate_zstream & lhs, deflate_zstream & rhs) noexcept {
    lhs.swap(rhs);
}
