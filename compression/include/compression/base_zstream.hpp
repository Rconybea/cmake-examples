// base_zstream.hpp

#pragma once

#include "span.hpp"
#include <zlib.h>
#include <ios>
#include <utility>
#include <cstring>

/* Shared base class for {deflate_zstream, inflate_zstream}
 */
class base_zstream {
public:
    using span_type = span<std::uint8_t>;

public:
    bool input_empty() const { return (zstream_.avail_in == 0); }
    bool have_input() const { return (zstream_.avail_in > 0); }
    bool output_empty() const { return (zstream_.avail_out == 0); }

    std::uint64_t n_in_total() const { return zstream_.total_in; }
    std::uint64_t n_out_total() const { return zstream_.total_out; }

    /* Require: .input_empty() */
    void provide_input(std::uint8_t * buf, std::streamsize buf_z) {
        if (! this->input_empty())
            throw std::runtime_error("base_zstream::provide_input: prior input work not complete");

        zstream_.next_in = buf;
        zstream_.avail_in = buf_z;
    }

    void provide_input(span_type const & span) {
        this->provide_input(span.lo(), span.size());
    }

    void provide_output(uint8_t * buf, std::streamsize buf_z) {
        zstream_.next_out = buf;
        zstream_.avail_out = buf_z;
    }

    void provide_output(span_type const & span) {
        this->provide_output(span.lo(), span.size());
    }

protected:
    /* swap two base_zstream objects */
    void swap(base_zstream & x) {
        std::swap(zstream_, x.zstream_);
    }

    /* move-assignment */
    base_zstream & operator= (base_zstream && x) {
        zstream_ = x.zstream_;

        /* zero rhs to prevent ::inflateEnd() releasing memory in x dtor */
        ::memset(&x.zstream_, 0, sizeof(x.zstream_));

        return *this;
    }

protected:
    /* zlib control state.  contains heap-allocated memory */
    z_stream zstream_;
};

inline void
swap(base_zstream & x, base_zstream & y) noexcept {
    x.swap(y);
}
