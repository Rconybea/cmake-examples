/** @file base_zstream.hpp **/

#pragma once

#include "span.hpp"
#include <zlib.h>
#include <ios>
#include <utility>
#include <memory>
#include <cstring>

/**
   @class base_zstream compression/base_zstream.hpp

   @brief manage a @c z_stream struct (from @c zlib)

   See <a href="https://www.zlib.net/manual.html">Zlib manual</a> for @c z_stream API.

   This class used by @see deflate_zstream @see inflate_zstream.
**/
class base_zstream {
public:
    /** @brief span type for interaction with @c zlib functions.
        Using @c uint8_t since @c zlib functions operate exclusively on byte ranges.
     **/
    using span_type = span<std::uint8_t>;

public:
    /** @brief base_zstream is not copyable **/
    base_zstream(base_zstream const & x) = delete;

    /** @brief true iff no input work remaining (as far as @c *p_zstream control struct is concerned) **/
    bool input_empty() const { return (p_native_zs_->avail_in == 0); }
    /** @brief true iff zlib control struct has input work remaining **/
    bool have_input() const { return (p_native_zs_->avail_in > 0); }
    /** @brief true iff no output work available from zlib control struct **/
    bool output_empty() const { return (p_native_zs_->avail_out == 0); }

    /** @brief total number of bytes consumed (by @c zlib control struct) since this instance created **/
    std::uint64_t n_in_total() const { return p_native_zs_->total_in; }
    /** @brief total number of bytes produced (by @c zlib control struct) since this instance created **/
    std::uint64_t n_out_total() const { return p_native_zs_->total_out; }

    /**
       @brief provide a new input memory range for compression/decompression.

       Discards any unconsumed input.  To avoid, invoke only when @c .input_empty() is true.

       @param buf    start of input range
       @param buf_z  size of input range in bytes

    **/
    void provide_input(std::uint8_t * buf, std::streamsize buf_z) {
        if (! this->input_empty())
            throw std::runtime_error("base_zstream::provide_input: prior input work not complete");

        p_native_zs_->next_in = buf;
        p_native_zs_->avail_in = buf_z;
    }

    /**
       @brief same as @c provide_input(buf,buf_z) but use span
    **/
    void provide_input(span_type const & span) {
        this->provide_input(span.lo(), span.size());
    }

    /**
       @brief provide new output memory range for compression/decompression.

       Discards any unconsumed output.  To avoid,  invoke only when @c .output_empty() is true.

       @param buf    start of new output range
       @param buf_z  size of output range in bytes
    **/
    void provide_output(uint8_t * buf, std::streamsize buf_z) {
        p_native_zs_->next_out = buf;
        p_native_zs_->avail_out = buf_z;
    }

    /**
       @brief same as @c provide_output(buf,buf_z) but use span
    **/
    void provide_output(span_type const & span) {
        this->provide_output(span.lo(), span.size());
    }

    /** @brief swap with another base_zstream object **/
    void swap(base_zstream & x) {
        std::swap(p_native_zs_, x.p_native_zs_);
    }

protected:
    /** @brief ctor.  allocates a @c zlib @c z_stream struct in @c .p_native_zs_ **/
    base_zstream() : p_native_zs_(new z_stream) {
        /* derived class must call
         *   ::inflateInit() / ::inflateInit2() / ::deflateInit() / ::deflateInit2()
         * on *p_native_zstream
         */
    }

    /** @brief move assignment **/
    base_zstream & operator= (base_zstream && x) {
        p_native_zs_ = std::move(x.p_native_zs_);
        return *this;
    }

    /** @brief destructor.

        Must be virtual so that inheriting implementation can call
        @c deflateEnd() or @c inflateEnd() as appropriate.
    **/
    virtual ~base_zstream() = default;

    friend void swap(base_zstream & x, base_zstream & y) noexcept;

protected:
    /**
     *  @brief zlib control state.
     *
     *  Contains heap-allocated memory.
     *
     *  @warning @c z_stream is not movable (as determined by experiment)
     */
    std::unique_ptr<z_stream> p_native_zs_;
};

/** @brief swap two @ref base_zstream instances **/
inline void
swap(base_zstream & x, base_zstream & y) noexcept {
    x.swap(y);
}
