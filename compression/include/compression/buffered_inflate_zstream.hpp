// buffered_inflate_zstream.hpp

#include "inflate_zstream.hpp"

/* Example
 *
 *   ifstream zfs("path/to/compressedfile.z", ios::binary);
 *   buffered_inflate_zstream zs;
 *   ofstream ucfs("path/to/uncompressedfile");
 *
 *   if (!ucfs)
 *       error...
 *   if (!zfs)
 *       error...
 *
 *   while (!zfs.eof()) {
 *       span<char> z_span = zs.z_avail();
 *       if (!zfs.read(z_span.lo(), z_span.size())) {
 *            error...
 *       }
 *       zs.z_produce(z_span.prefix(zfs.gcount()));
 *
 *       zs.inflate_chunk();
 *
 *       span<char> uc_span = zs.uc_contents();
 *       ucfs.write(uc_span.lo(), uc_span.size());
 *
 *       zs.uc_consume(uc_span);
 *   }
 */
class buffered_inflate_zstream {
public:
    using z_span_type = span<std::uint8_t>;
    using size_type = std::uint64_t;

    static constexpr size_type c_default_buf_z = 64UL * 1024UL;

public:
    /* buf_z :  buffer size for compressed + uncompressed input.
     *          If 0,  defer allocation
     * align_z :  alignment for uncompressed input, if allocating. Otherwise ignored
     */
    buffered_inflate_zstream(size_type buf_z = c_default_buf_z,
                             size_type align_z = sizeof(char))
        : z_in_buf_{buf_z, sizeof(std::uint8_t)},
          uc_out_buf_{buf_z, align_z}
        {
            zs_algo_.provide_output(uc_out_buf_.avail());
        }
    /* not copyable (since .inflate_zstream isn't) */
    buffered_inflate_zstream(buffered_inflate_zstream const & x) = delete;

    size_type n_in_total() const { return zs_algo_.n_in_total(); }
    size_type n_out_total() const { return zs_algo_.n_out_total(); }

    /* space available for more compressed input */
    z_span_type z_avail() const { return z_in_buf_.avail(); }
    /* space available for more uncompressed input (output of this object) */
    z_span_type uc_avail() const { return uc_out_buf_.avail(); }
    /* uncompressed content available */
    z_span_type uc_contents() const { return uc_out_buf_.contents(); }

<<<<<<< Updated upstream
=======
    /* Allocate buffer space.  May use before inflation, after calling ctor with 0 buf_z.
     * Does not preserve any existing buffer contents -> not intended to be used
     * after beginning inflation work.
     */
    void alloc(size_type buf_z = c_default_buf_z, size_type align_z = sizeof(char)) {
        z_in_buf_.alloc(buf_z, sizeof(std::uint8_t));
        uc_out_buf_.alloc(buf_z, align_z);
        zs_algo_.provide_output(uc_out_buf_.avail());
    }

>>>>>>> Stashed changes
    /* reset buffers to empty state,  in case want to reuse *this for different input.
     * Calls ::inflateInit2()
     */
    void clear2empty(bool zero_buffer_flag) {
        z_in_buf_.clear2empty(zero_buffer_flag);
        uc_out_buf_.clear2empty(zero_buffer_flag);
        /* reinitialize (ctor calls ::inflateInit2()) */
        zs_algo_.rebuild();
        zs_algo_.provide_output(uc_out_buf_.avail());
    }

    /* After populating some prefix of .z_avail(), make existence of that input known
     * so that it can be uncompressed
     */
    void z_produce(z_span_type const & span) {
        if (span.size()) {
            z_in_buf_.produce(span);

            /* note whenever we call .inflate,  we consume from .z_in_buf,
             * so .z_in_buf and .input_zs are kept synchronized
             */
            zs_algo_.provide_input(z_in_buf_.contents());
        }
    }

    /* Consume some uncompressed input -- allows that buffer space to be reused */
    void uc_consume(z_span_type const & span) {
        if (span.size()) {
            uc_out_buf_.consume(span);
        }

        if (uc_out_buf_.empty()) {
            /* can recycle output */
            zs_algo_.provide_output(uc_out_buf_.avail());
        }
    }

    void uc_consume_all() { this->uc_consume(this->uc_contents()); }

    /* inflate some input data (previously provided by .z_produce())
     * returns #of bytes of inflated data appended to .uc_out_buf by this call
     */
    size_type inflate_chunk();

    void swap(buffered_inflate_zstream & x) {
        ::swap(z_in_buf_, x.z_in_buf_);
        ::swap(zs_algo_, x.zs_algo_);
        ::swap(uc_out_buf_, x.uc_out_buf_);
    }

    buffered_inflate_zstream & operator= (buffered_inflate_zstream && x) {
        z_in_buf_ = std::move(x.z_in_buf_);
        zs_algo_ = std::move(x.zs_algo_);
        uc_out_buf_ = std::move(x.uc_out_buf_);

        return *this;
    }

private:
    /* compressed input */
    buffer<std::uint8_t> z_in_buf_;

    /* inflation-state (holds zlib z_stream) */
    inflate_zstream zs_algo_;

    /* uncompressed input */
    buffer<std::uint8_t> uc_out_buf_;
};

inline void
swap(buffered_inflate_zstream & lhs,
     buffered_inflate_zstream & rhs)
{
    lhs.swap(rhs);
}
