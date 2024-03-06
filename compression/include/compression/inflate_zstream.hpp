/** @file inflate_zstream.hpp **/

#pragma once

#include "base_zstream.hpp"
#include "buffer.hpp"
#include <ios>
#include <cstring>

/**
   @class inflate_zstream compression/inflate_zstream.hpp

   @brief accept compressed input and inflate (i.e. uncompress) it.

   Customer is responsible for supplying buffer space for
   compressed and uncompressed input.

   @see buffered_inflate_zstream for implementation that creates and manages i/o buffers.

   Example
   @code
   #include "compression/inflate_zstream.hpp"

   buffer<uint8_t> z_buf(4096);
   buffer<uint8_t> uc_buf(4096);

   // store some compressed data in z_buf.buf()
   uint64_t n_z_in = ...; // bytes stored in z_buf[]

   // attach buffers to native zstream
   inflate_zstream zs;
   zs.provide_input(span(z_buf, n_z_in));
   zs.provide_output(span(uc_buf, sizeof(uc_buf)));

   while (!z_buf.empty() || !uc_buf.empty()) {
       auto pr = zs.inflate_chunk();

       z_buf.consume(pr.first);
       uc_buf.produce(pr.second);

       if (!uc_buf.empty()) {
           // do something with uc_buf.contents()
           uint64_t n_uc_out
             = dosomethingwith(uc_buf.contents().buf(), uc_buf.contents().size());

           // Need n_uc_out>0 eventually, to guarantee progress.
           uc_buf.consume(uc_buf.contents().prefix(n_uc_out));

           if (uc_buf.empty()) {
               // can reuse output buffer space once it empties
               zs.provide_output(uc_buf.avail());
           }

           if (z_buf.empty()) {
               // get more input from somewhere,  store it starting from z_buf.buf()

               n_z_in = ...;
               zs.provide_input(z_buf.avail().prefix(n_z_in));
           }
       }
   }

   @endcode
**/
class inflate_zstream : public base_zstream {
public:
    /** typealias for span of (compressed or uncompressed) data **/
    using span_type = span<std::uint8_t>;

public:
    /** @brief create inflation-only zstream in empty state **/
    inflate_zstream();
    /** @brief inflate_zstream is not copyable **/
    inflate_zstream(inflate_zstream const & x) = delete;
    /** @brief destructor; calls @c zlib @c InflateEnd() **/
    virtual ~inflate_zstream();

    /** @brief cleanup and reestablish nominal @c z_stream state in @c *base_zstream::p_native_zs_.

        - Calls @c InflateEnd() (if necessary)
        - Calls @c InflateInit2() (always)
     **/
    void rebuild();

    /** @brief Inflate (uncompress) some input.

        @return pair with:
        @c .first  = span for compressed bytes consumed
        @c .second = span for uncompressed bytes produced

        After invoking @c inflate_chunk(),  caller should either:
        1. provide at least one byte of additional input (if @ref base_zstream::have_input is false)
        2. consume at least one byte of available output (if @ref base_zstream::output_empty is false)

        @pre
        Input data must have been attached by a preceding call to @p provide_input().
        @pre
        Output buffer space must have been provided by a preceding call to @p provide_output().
    **/
    std::pair<span_type, span_type> inflate_chunk();

    /** @brief swap state with another inflate_zstream object **/
    void swap(inflate_zstream & x) {
        base_zstream::swap(x);
    }

    /** @brief move assignment **/
    inflate_zstream & operator= (inflate_zstream && x) {
        base_zstream::operator=(std::move(x));
        return *this;
    }

private:
    /** @brief setup a new (or refurbished) instance in @c p_native_zs_.

        Calls @c zlib @c inflateInit2()
    **/
    void setup();

    /** @brief call when finished with instance in @c p_native_zs_.

        Calls @c zlib @c inflateEnd()
    **/
    void teardown();
};

/** @brief Provide overload for @c swap(), so that @ref inflate_zstream is swappable **/
inline void
swap(inflate_zstream & x, inflate_zstream & y) noexcept {
    x.swap(y);
}
