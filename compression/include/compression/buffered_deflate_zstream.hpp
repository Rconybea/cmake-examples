/** @file buffered_deflate_zstream.hpp **/

#include "deflate_zstream.hpp"

/**
   @class buffered_deflate_zstream compression/buffered_deflate_zstream.hpp

   @brief accept input and deflate (i.e. decompress) it.

   Creates and manages buffer space for input and (inflated) output.

   Memory allocation occurs only in:
   1. constructor
   2. buffered_inflate_stream::alloc()

   Other stateful operations write to buffers established in those two entry points

   Example
   @code
      ifstream ucfs("path/to/uncompressedfile");
      buffered_deflate_zstream zs;
      ofstream zfs("path/to/compressedfile.z", ios::binary);

      if (!ucfs)
          error...
      if (!zfs)
          error...

      for (bool progress = true, final_flag = false; progress;) {
          streamsize nread = 0;

          if (ucfs.eof()) {
              // end of uncompressed stream

              final = true;
          } else {
              span<char> uc_span = zs.uc_avail();
              ucfs.get(uc_span.lo(), uc_span.size(), '\n');
              nread = ucfs.gcount();

              zs.uc_produce(uc_span.prefix(nread));
         }

         zs.deflate_chunk(final);

         span<uint8_t> z_span = zs.z_contents();
         zfs.write(z_span.lo(), z_span.size());

         zs.z_consume(z_span);

         progress = (nread > 0) || (z_span.size() > 0);
     }
   @endcode
 **/
class buffered_deflate_zstream {
public:
    /** @brief typealias for span of compressed bytes **/
    using z_span_type = span<std::uint8_t>;
    /** @brief typealias for stream size (will be in bytes) **/
    using size_type = std::uint64_t;

    /** @brief default buffer size (64k) **/
    static constexpr size_type c_default_buf_z = 64UL * 1024UL;

public:
    /** @brief Constructor;  if @p buf_z > 0,  allocate two buffers of size @p buf_z for compressed and uncompressed content.
        Otherwise defer allocation.

        @param buf_z    Buffer size,  for both compressed and uncompressed input.
        @param align_z  Alignment for uncompressed buffer memory, if allocating.  Otherwise ignored.
     */
    buffered_deflate_zstream(size_type buf_z = c_default_buf_z,
                             size_type align_z = 1)
        : uc_in_buf_{buf_z, align_z},
          z_out_buf_{buf_z, sizeof(std::uint8_t)}
        {
            zs_algo_.provide_output(z_out_buf_.avail());
        }
    /** @brief not copyable (since member @c zs_algo_ is not) **/
    buffered_deflate_zstream(buffered_deflate_zstream const & x) = delete;

    /** @brief number of bytes of uncompressed input (according to @c zlib) consumed since this stream created **/
    size_type n_in_total() const { return zs_algo_.n_in_total(); }
    /** @brief number of bytes of compressed output (according to @c zlib) provided since this stream created **/
    size_type n_out_total() const { return zs_algo_.n_out_total(); }

    /** @brief space current available for more uncompressed input **/
    z_span_type uc_avail() const { return uc_in_buf_.avail(); }
    /** @brief uncompressed content currently buffered for compression (see .uc_produce()) **/
    z_span_type uc_contents() const { return uc_in_buf_.contents(); }
    /** @brief space currently available for more compressed output */
    z_span_type z_avail() const { return z_out_buf_.avail(); }
    /** @brief compressed content currently available for consumption.

        If non-empty: consuming at least 1 byte allows @c zlib to guarantee progress.
        Consume by calling @c .uc_consume() with a non-empty prefix of @c .z_contents()
     **/
    z_span_type z_contents() const { return z_out_buf_.contents(); }

    /** @brief Allocate buffer space;  may use after calling ctor with zero @c buf_z,  but before initating deflation work.

        Does not preserve existing buffer contents!
        Not intended for use after beginning deflation work
     **/
    void alloc(size_type buf_z = c_default_buf_z, size_type align_z = sizeof(char)) {
        uc_in_buf_.alloc(buf_z, align_z);
        z_out_buf_.alloc(buf_z, sizeof(std::uint8_t));
        zs_algo_.provide_output(z_out_buf_.avail());
    }

    /** @brief reset input/output buffers to empty state,  in case want to reuse *this for different input.

        May call @c zlib @c deflateEnd() and @c deflateInit2().
    **/
    void clear2empty(bool zero_buffer_flag) {
        uc_in_buf_.clear2empty(zero_buffer_flag);
        z_out_buf_.clear2empty(zero_buffer_flag);
        zs_algo_.rebuild();
        zs_algo_.provide_output(z_out_buf_.avail());
    }

    /** @brief Introduce new uncompressed input to @c zlib for deflation.

        @param span   Memory range of new uncompressed input.

        @pre @p span must be a prefix of @ref uc_avail()
    **/
    void uc_produce(z_span_type const & span) {
        if (span.size()) {
            uc_in_buf_.produce(span);

            /* note whenever we call .deflate,  we consume from .uc_output_buf,
             * so .uc_output_buf and .output_zs are kept synchronized
             */
            zs_algo_.provide_input(uc_in_buf_.contents());
        }
    }

    /** @brief consume some deflated output;  consumed buffer space can eventually be reused

        @param span   Memory range of now-consumed compressed output.

        @pre @p span must be a prefix of @ref z_avail()
    **/
    void z_consume(z_span_type const & span) {
        if (span.size()) {
            z_out_buf_.consume(span);
        }

        if (z_out_buf_.empty()) {
            /* can recycle output */
            zs_algo_.provide_output(z_out_buf_.avail());
        }
    }

    /** @brief consume all buffered compressed content **/
    void z_consume_all() { this->z_consume(this->z_contents()); }

    /** @brief attempt some deflation work.

        Deflate some input data previously provided via @ref uc_produce()

        @param final_flag   When true,  pass @c Z_FINAL to @c deflate(),
        to ensure latent compression state is flushed.
        Optimally set exactly once at end of stream.
        May be called earlier,  at the cost of less efficient compression.

        @return number of b ytes of deflated data appended to @ref z_out_buf_
        as a result of this call.
    **/
    size_type deflate_chunk(bool final_flag);

    /** @brief swap state with another buffered_deflate_stream object **/
    void swap (buffered_deflate_zstream & x) {
        ::swap(uc_in_buf_, x.uc_in_buf_);
        ::swap(zs_algo_, x.zs_algo_);
        ::swap(z_out_buf_, x.z_out_buf_);
    }

    /** @brief move-assignemnt: annex state from another buffered_deflate_zstream object.

        Except for its destructor, @p x cannot be used after this method returns.
    **/
    buffered_deflate_zstream & operator= (buffered_deflate_zstream && x) {
        uc_in_buf_ = std::move(x.uc_in_buf_);
        zs_algo_ = std::move(x.zs_algo_);
        z_out_buf_ = std::move(x.z_out_buf_);

        return *this;
    }

private:
    /** @brief buffer for uncompressed input **/
    buffer<std::uint8_t> uc_in_buf_;

    /** @brief deflation-state (holds @c zlib @c z_stream control object) */
    deflate_zstream zs_algo_;

    /** @brief buffer for deflated output **/
    buffer<std::uint8_t> z_out_buf_;
}; /*buffered_deflate_zstream*/

/** @brief overload for @c swap(),  so that @ref buffered_deflate_zstream is swappable **/
inline void
swap(buffered_deflate_zstream & lhs,
     buffered_deflate_zstream & rhs)
{
    lhs.swap(rhs);
}
