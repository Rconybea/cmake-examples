/** @file buffered_inflate_zstream.hpp **/

#include "inflate_zstream.hpp"

/**
   @class buffered_inflate_zstream compression/buffered_inflate_zstream.hpp

   @brief accept compressed input and inflate (i.e. uncompress) it.

   Creates and manages buffer space for input and (inflateed) output.

   Memory allocation occurs only in:
   1. constructor
   2. buffered_inflate_stream::alloc()

   Other stateful operations write to buffers established in those two entry points.

   Example
   @code
      ifstream zfs("path/to/compressedfile.z", ios::binary);
      buffered_inflate_zstream zs;
      ofstream ucfs("path/to/uncompressedfile");

      if (!ucfs)
         error...
      if (!zfs)
         error...

      while (!zfs.eof()) {
          span<char> z_span = zs.z_avail();
          if (!zfs.read(z_span.lo(), z_span.size())) {
              error...
          }
          zs.z_produce(z_span.prefix(zfs.gcount()));

          zs.inflate_chunk();

          span<char> uc_span = zs.uc_contents();
          ucfs.write(uc_span.lo(), uc_span.size());

          zs.uc_consume(uc_span);
      }
   @endcode
**/
class buffered_inflate_zstream {
public:
    /** @brief typealias for span of compressed data **/
    using z_span_type = span<std::uint8_t>;
    /** @brief typealias for stream size (in bytes) **/
    using size_type = std::uint64_t;

    /** @brief default buffer size.  Used for both compressed+uncompressed stream (64k) **/
    static constexpr size_type c_default_buf_z = 64UL * 1024UL;

public:
    /** @brief Constructor;  if @p buf_z > 0, allocate two buffers of size @p buf_z for compressed and uncompressed content.
        Otherwise defer allocation.

        @param buf_z    buffer size, allocated separately for {compressed, uncompressed} input.
        @param align_z  alignment for uncompressed buffer memory, if allocating. Otherwise ignored.
     */
    buffered_inflate_zstream(size_type buf_z = c_default_buf_z,
                             size_type align_z = sizeof(char))
        : z_in_buf_{buf_z, sizeof(std::uint8_t)},
          uc_out_buf_{buf_z, align_z}
        {
            zs_algo_.provide_output(uc_out_buf_.avail());
        }
    /** @brief not copyable (since member @c zs_algo_ is not) **/
    buffered_inflate_zstream(buffered_inflate_zstream const & x) = delete;

    /** @brief number of bytes of compressed input (according to @c zlib) consumed since this stream created **/
    size_type n_in_total() const { return zs_algo_.n_in_total(); }
    /** @brief number of bytes of uncompressed output (according to @c zlib) provided since this stream created **/
    size_type n_out_total() const { return zs_algo_.n_out_total(); }

    /** @brief space currently available for more compressed input **/
    z_span_type z_avail() const { return z_in_buf_.avail(); }
    /** @brief space currently available for more uncompressed input (output of this buffered_inflate_zstream object) */
    z_span_type uc_avail() const { return uc_out_buf_.avail(); }
    /** @brief uncompressed content currently available for consumption.

        If non-empty: consuming at least 1 byte allows @c zlib to guarantee progress.
        Consume by calling @c .uc_consume() with some non-empty prefix of @c .uc_contents()
    **/
    z_span_type uc_contents() const { return uc_out_buf_.contents(); }

    /** @brief allocate buffer space;  may use after calling ctor with zero @c buf_z, but before initiating inflation work.

        Does not preserve any existing buffer contents!
        Not intended for use after beginning inflation work.
    **/
    void alloc(size_type buf_z = c_default_buf_z, size_type align_z = sizeof(char)) {
        z_in_buf_.alloc(buf_z, sizeof(std::uint8_t));
        uc_out_buf_.alloc(buf_z, align_z);
        zs_algo_.provide_output(uc_out_buf_.avail());
    }

    /** @brief reset input/output buffers to empty state,  in case want to reuse this stream for different input.

        May call @c zlib @c inflateEnd() and @c inflateInit2().
    **/
    void clear2empty(bool zero_buffer_flag) {
        z_in_buf_.clear2empty(zero_buffer_flag);
        uc_out_buf_.clear2empty(zero_buffer_flag);
        /* reinitialize (ctor calls ::inflateInit2()) */
        zs_algo_.rebuild();
        zs_algo_.provide_output(uc_out_buf_.avail());
    }

    /** @brief Introduce new compressed input to @c zlib for inflation.

        @param span   Memory range of new compressed input.

        @pre @p span must be a prefix of @ref z_avail()
     **/
    void z_produce(z_span_type const & span) {
        if (span.size()) {
            z_in_buf_.produce(span);

            /* note whenever we call .inflate,  we consume from .z_in_buf,
             * so .z_in_buf and .input_zs are kept synchronized
             */
            zs_algo_.provide_input(z_in_buf_.contents());
        }
    }

    /** @brief consume some inflated output;  consumed buffer space can eventually be reused

        @param span   Memory range of now-consumed uncompressed output.

        @pre @p span must be a prefix of @ref uc_avail()
     **/
    void uc_consume(z_span_type const & span) {
        if (span.size()) {
            uc_out_buf_.consume(span);
        }

        if (uc_out_buf_.empty()) {
            /* can recycle output */
            zs_algo_.provide_output(uc_out_buf_.avail());
        }
    }

    /** @brief consume all buffered uncompressed content

        @post @ref uc_avail() is empty
     **/
    void uc_consume_all() { this->uc_consume(this->uc_contents()); }

    /** @brief attempt some inflation work

        Inflate some input data previously provided via @ref z_produce()

        @return number of bytes of inflated data appended to @ref uc_out_buf_
        as a result of this call
    **/
    size_type inflate_chunk();

    /** @brief swap state with another buffered_inflate_zstream object **/
    void swap(buffered_inflate_zstream & x) {
        ::swap(z_in_buf_, x.z_in_buf_);
        ::swap(zs_algo_, x.zs_algo_);
        ::swap(uc_out_buf_, x.uc_out_buf_);
    }

    /** @brief move-assignment: annex state from another buffered_inflate_zstream object

        Except for its destructor, @p x cannot be used after this method returns.
     **/
    buffered_inflate_zstream & operator= (buffered_inflate_zstream && x) {
        z_in_buf_ = std::move(x.z_in_buf_);
        zs_algo_ = std::move(x.zs_algo_);
        uc_out_buf_ = std::move(x.uc_out_buf_);

        return *this;
    }

private:
    /** @brief buffer for compressed input **/
    buffer<std::uint8_t> z_in_buf_;

    /** @brief inflation-state (holds @c zlib @c z_stream control object) **/
    inflate_zstream zs_algo_;

    /** @brief buffer for inflated output **/
    buffer<std::uint8_t> uc_out_buf_;
};

/** @brief overload for @c swap(), so that @ref buffered_inflate_zstream is swappable **/
inline void
swap(buffered_inflate_zstream & lhs,
     buffered_inflate_zstream & rhs)
{
    lhs.swap(rhs);
}
