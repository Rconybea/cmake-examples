// buffered_deflate_zstream.hpp

#include "deflate_zstream.hpp"

/* accept input and compress (aka deflatee).
 * provides buffer for both uncompressed input and compressed output
 *
 * Example
 *
 *   ifstream ucfs("path/to/uncompressedfile");
 *   buffered_deflate_zstream zs;
 *   ofstream zfs("path/to/compressedfile.z", ios::binary);
 *
 *   if (!ucfs)
 *       error...
 *   if (!zfs)
 *       error...
 *
 *   for (bool progress = true, final_flag = false; progress;) {
 *       streamsize nread = 0;
 *
 *       if (ucfs.eof()) {
 *           final = true;
 *       } else {
 *           span<char> uc_span = zs.uc_avail();
 *           ucfs.read(uc_span.lo(), uc_span.size());
 *           nread = ucfs.gcount();
 *           zs.uc_produce(uc_span.prefix(nread));
 *       }
 *
 *       zs.deflate_chunk(final);
 *
 *       span<uint8_t> z_span = zs.z_contents();
 *       zfs.write(z_span.lo(), z_span.size());
 *       zs.z_consume(z_span);
 *
 *       progress = (nread > 0) || (z_span.size() > 0);
 *   }
 */
class buffered_deflate_zstream {
public:
    using z_span_type = span<std::uint8_t>;
    using size_type = std::uint64_t;

    static constexpr size_type c_default_buf_z = 64UL * 1024UL;

public:
    /* buf_z :    buffer size for compressed + uncompressed input.
     *            If 0,  defer allocation
     * align_z :  alignment for uncompressed output,  if allocating.  Otherwise ignored.
     */
    buffered_deflate_zstream(size_type buf_z = c_default_buf_z,
                             size_type align_z = 1)
        : uc_in_buf_{buf_z, align_z},
          z_out_buf_{buf_z, sizeof(std::uint8_t)}
        {
            zs_algo_.provide_output(z_out_buf_.avail());
        }

    size_type n_in_total() const { return zs_algo_.n_in_total(); }
    size_type n_out_total() const { return zs_algo_.n_out_total(); }

    /* space available for more uncompressed output (input of this object) */
    z_span_type uc_avail() const { return uc_in_buf_.avail(); }
    /* spaec available for more compressed output */
    z_span_type z_avail() const { return z_out_buf_.avail(); }
    /* compressed content available */
    z_span_type z_contents() const { return z_out_buf_.contents(); }

    /* after populating some prefix of .uc_avail(),  make existence of that output
     * known to .output_zs so it can be compressed
     */
    void uc_produce(z_span_type const & span) {
        if (span.size()) {
            uc_in_buf_.produce(span);

            /* note whenever we call .deflate,  we consume from .uc_output_buf,
             * so .uc_output_buf and .output_zs are kept synchronized
             */
            zs_algo_.provide_input(uc_in_buf_.contents());
        }
    }

    /* recognize some consumed compressed output -- allows that buffer space to be reused */
    void z_consume(z_span_type const & span) {
        if (span.size()) {
            z_out_buf_.consume(span);
        }

        if (z_out_buf_.empty()) {
            /* can recycle output */
            zs_algo_.provide_output(z_out_buf_.avail());
        }
    }

    void z_consume_all() { this->z_consume(this->z_contents()); }

    /* deflate some portion of uncompressed input buffer.
     *
     * final_flag: must be set exactly once on last call for an output,  to flush any partially compressed state.
     *
     * return #of bytes compressed output available
     */
    size_type deflate_chunk(bool final_flag);

    void swap (buffered_deflate_zstream & x) {
        ::swap(uc_in_buf_, x.uc_in_buf_);
        ::swap(zs_algo_, x.zs_algo_);
        ::swap(z_out_buf_, x.z_out_buf_);
    }

    buffered_deflate_zstream & operator= (buffered_deflate_zstream && x) {
        uc_in_buf_ = std::move(x.uc_in_buf_);
        zs_algo_ = std::move(x.zs_algo_);
        z_out_buf_ = std::move(x.z_out_buf_);

        return *this;
    }

private:
    /* uncompressed output */
    buffer<std::uint8_t> uc_in_buf_;

    /* deflate-state (holds zlib z_stream) */
    deflate_zstream zs_algo_;

    /* compressed output */
    buffer<std::uint8_t> z_out_buf_;
}; /*buffered_deflate_zstream*/

inline void
swap(buffered_deflate_zstream & lhs,
     buffered_deflate_zstream & rhs)
{
    lhs.swap(rhs);
}
