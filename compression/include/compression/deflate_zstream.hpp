/** @file deflate_zstream.hpp **/

#pragma once

#include "base_zstream.hpp"
#include "buffer.hpp"
#include <zlib.h>
#include <ios>
#include <cstring>

/**
   @class deflate_zstream compression/deflate_zstream.hpp

   @brief accept uncompressed output and deflate (i.e. compress) it.

   Customer is responsible for supplying buffer space for
   compressed and uncompressed output.

   @see buffered_deflate_zstream for implementation that creates and manages i/o buffers.

   Example
   @code
   #include "compression/deflate_zstream.hpp"

   buffer<uint8_t> uc_buf(4096);
   buffer<uint8_t> z_buf(4096);

   // store some to-be-compressed data in uc_buf.buf()
   uint64_t n_uc_in = ...; // content stored in uc_buf[]

   // attach buffers to native zstream
   deflate_zstream zs;
   zs.provide_input(span(uc_buf, n_uc_in));
   zs.provide_output(span(z_buf, sizeof(z_buf));

   while (!uc_buf.empty() || !z_buf.empty()) {
       auto pr = zs.deflate_chunk();

       uc_buf.consume(pr.first);
       z_buf.produce(pr.second);

       if (!z_buf.empty()) {
           // do something with z_buf.contents()
           uint64_t n_z_out
             = dosomethingwith(z_buf.contents().buf(), z_buf.contents().size());

           // Need z_uc_out>0 eventually, to guarantee progress.
           z_buf.consume(z_buf.contents().prefix(n_z_out));

           if (z_buf.empty()) {
               // can reuse output buffer space once it empties
               zs.provide_output(z_buf.avail());
           }

           if (uc_buf.empty()) {
               // get more input from somewhere,  store it starting from uc_buf.buf()

               n_uc_in = ...;
               zs.provide_input(uc_buf.avail().prefix(n_uc_in));
           }
       }
   }
   @endcode
**/
class deflate_zstream : public base_zstream {
public:
    /** @brief typealias for span of compressed data **/
    using span_type = span<std::uint8_t>;

public:
    /** @brief create deflation-only zstream in empty state **/
    deflate_zstream();
    /** @brief deflate_zstream is not copyable **/
    deflate_zstream(deflate_zstream const & x) = delete;
    /** @brief destructor; calls @c zlib @c DeflateEnd() **/
    virtual ~deflate_zstream();

    /** @brief cleanup and reestablish nominal @c z_stream state in @c *base_zstream::p_native_zs_

        1. Calls @c DeflateEnd() (if necessary)
        2. Calls @c DeflateInit2() (always)
    **/
    void rebuild();

    /** @brief Deflate (compress) some input.

        @param final_flag  True to provide @c Z_FINISH to @c deflate(),
        to flush pending compression state held in @c *base_zstream::p_native_zs_.
        Ideally set this exactly once, in last call to deflate_chunk, as uncompressed stream ends.

        Can set on earlier calls,  at the cost of restarting compression state with corresponding
        increase in size of compressed stream.

        @return pair with:
        @c .first  = span for uncompressed bytes consumed
        @c .second = span for compressed bytes produced

        After invoking @c deflate_chunk(),  caller should either:
        1. provide at least one byte of additional input (if @ref base_zstream::have_input is false)
        2. consume at least one byte of available output (if @ref base_zstream::output_empty is false)

        @pre
        Input data must have been attached by a preceding call to @p provide_input().
        @pre
        Output buffer space must have been provided by a preceding call to @p provide_output().
     **/
    std::pair<span_type, span_type> deflate_chunk(bool final_flag);

    /** @brief move assignment **/
    void swap(deflate_zstream & x) {
        base_zstream::swap(x);
    }

    /** @brief move assignment **/
    deflate_zstream & operator= (deflate_zstream && x) {
        base_zstream::operator=(std::move(x));
        return *this;
    }

private:
    /** @brief setup a new (or refurbished) instance in @c base_zstream::p_native_zs_.

        Calls @c zlib @c deflateInit2()
    **/
    void setup();
    /** @brief call when finished with instance in @c p_native_zs_.

        Calls @c zlib @c deflateEnd()
    **/
    void teardown();
};

/** @brief overload for @c swap(), so that @ref deflate_zstream is swappable **/
inline void
swap(deflate_zstream & lhs, deflate_zstream & rhs) noexcept {
    lhs.swap(rhs);
}
