// base_zstream.hpp

#pragma once

#include "span.hpp"
#include <zlib.h>
#include <ios>
#include <utility>
#include <memory>
#include <cstring>

/* Shared base class for {deflate_zstream, inflate_zstream}
 */
class base_zstream {
public:
    using span_type = span<std::uint8_t>;

public:
    /* not copyable or moveable (beause z_stream isn't) */
    base_zstream(base_zstream const & x) = delete;

    /* true iff no input work remaining (as far as zlib control struct concerned) */
    bool input_empty() const { return (p_native_zs_->avail_in == 0); }
    /* true iff input work remaining for zlib control struct */
    bool have_input() const { return (p_native_zs_->avail_in > 0); }
    /* true iff no output work available from zlib control struct) */
    bool output_empty() const { return (p_native_zs_->avail_out == 0); }

    std::uint64_t n_in_total() const { return p_native_zs_->total_in; }
    std::uint64_t n_out_total() const { return p_native_zs_->total_out; }

    /* Require: .input_empty() */
    void provide_input(std::uint8_t * buf, std::streamsize buf_z) {
        if (! this->input_empty())
            throw std::runtime_error("base_zstream::provide_input: prior input work not complete");

        p_native_zs_->next_in = buf;
        p_native_zs_->avail_in = buf_z;
    }

    /* attach input span to zlib control struct */
    void provide_input(span_type const & span) {
        this->provide_input(span.lo(), span.size());
    }

    /* attach output buffer space to zlib stream control state */
    void provide_output(uint8_t * buf, std::streamsize buf_z) {
        p_native_zs_->next_out = buf;
        p_native_zs_->avail_out = buf_z;
    }

    /* attach output span to zlib control struct */
    void provide_output(span_type const & span) {
        this->provide_output(span.lo(), span.size());
    }

    /* swap two base_zstream objects */
    void swap(base_zstream & x) {
        std::swap(p_native_zs_, x.p_native_zs_);
    }

protected:
    base_zstream() : p_native_zs_(new z_stream) {
        /* derived class must call ::inflateInit() / ::inflateInit2() / ::deflateInit() / ::deflateInit2() on *p_native_zstream */
    }

    base_zstream & operator= (base_zstream && x) {
        p_native_zs_ = std::move(x.p_native_zs_);
        return *this;
    }

    friend void swap(base_zstream & x, base_zstream & y) noexcept;

protected:
    /* zlib control state.  contains heap-allocated memory;
     * not copyable or movable (determined empirically :)
     */
    std::unique_ptr<z_stream> p_native_zs_;
};

inline void
swap(base_zstream & x, base_zstream & y) noexcept {
    x.swap(y);
}
