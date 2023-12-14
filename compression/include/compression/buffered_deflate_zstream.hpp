// buffered_deflate_zstream.hpp

#include "deflate_zstream.hpp"

/* accept input (of type CharT) and compress (aka deflatee).
 * provides buffer for both uncompressed input and compressed output
 *
 * Example
 *
 *   ifstream ucfs("path/to/uncompressedfile");
 *   buffered_deflate_zstream<char> zs;
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
template <typename CharT>
class buffered_deflate_zstream {
public:
    using uc_span_type = span<CharT>;
    using z_span_type = span<std::uint8_t>;
    using size_type = std::uint64_t;

public:
    buffered_deflate_zstream(size_type buf_z = 64 * 1024)
        : uc_in_buf_{buf_z},
          z_out_buf_{buf_z}
        {
            zs_algo_.provide_output(z_out_buf_.avail());
        }

    size_type n_in_total() const { return zs_algo_.n_in_total(); }
    size_type n_out_total() const { return zs_algo_.n_out_total(); }

    /* space available for more uncompressed output (input of this object) */
    uc_span_type uc_avail() const { return uc_in_buf_.avail(); }
    /* spaec available for more compressed output */
    z_span_type z_avail() const { return z_out_buf_.avail(); }
    /* compressed content available */
    z_span_type z_contents() const { return z_out_buf_.contents(); }

    /* after populating some prefix of .uc_avail(),  make existence of that output
     * known to .output_zs so it can be compressed
     */
    void uc_produce(uc_span_type const & span) {
        if (span.size()) {
            uc_in_buf_.produce(span);

            /* note whenever we call .deflate,  we consume from .uc_output_buf,
             * so .uc_output_buf and .output_zs are kept synchronized
             */
            zs_algo_.provide_input(uc_in_buf_.contents().template cast<std::uint8_t>());
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

    /* return #of bytes compressed output available */
    size_type deflate_chunk(bool final_flag) {
        if (zs_algo_.have_input() || final_flag) {
            std::pair<z_span_type, z_span_type> x = zs_algo_.deflate_chunk2(final_flag);

            uc_in_buf_.consume(x.first.template cast<CharT>());

            z_out_buf_.produce(x.second);

            return x.second.size();
        } else {
            return 0;
        }
    }

    void swap (buffered_deflate_zstream & x) {
        std::swap(uc_in_buf_, x.uc_in_buf_);
        std::swap(zs_algo_, x.zs_algo_);
        std::swap(z_out_buf_, x.z_out_buf_);
    }

    buffered_deflate_zstream & operator= (buffered_deflate_zstream && x) {
        uc_in_buf_ = std::move(x.uc_in_buf_);
        zs_algo_ = std::move(x.zs_algo_);
        z_out_buf_ = std::move(x.z_out_buf_);

        return *this;
    }

private:
    /* uncompressed output */
    buffer<CharT> uc_in_buf_;

    /* deflate-state (holds zlib z_stream) */
    deflate_zstream zs_algo_;

    /* compressed output */
    buffer<std::uint8_t> z_out_buf_;
}; /*buffered_deflate_zstream*/

namespace std {
    template <typename CharT>
    void swap(buffered_deflate_zstream<CharT> & lhs,
              buffered_deflate_zstream<CharT> & rhs)
    {
        lhs.swap(rhs);
    }
}
