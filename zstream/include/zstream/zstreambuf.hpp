// zstreambuf.hpp

#pragma once

#include "compression/buffered_inflate_zstream.hpp"
#include "compression/buffered_deflate_zstream.hpp"
#include "compression/tostr.hpp"
#include "zlib.h"

#include <iostream>
#include <string>
#include <memory>

struct hex {
    hex(std::uint8_t x, bool w_char = false) : x_{x}, with_char_{w_char} {}

    std::uint8_t x_;
    bool with_char_;
};

struct hex_view {
    hex_view(std::uint8_t const * lo, std::uint8_t const * hi, bool as_text) : lo_{lo}, hi_{hi}, as_text_{as_text} {}
    hex_view(char const * lo, char const * hi, bool as_text)
        : lo_{reinterpret_cast<std::uint8_t const *>(lo)},
          hi_{reinterpret_cast<std::uint8_t const *>(hi)},
          as_text_{as_text} {}

    std::uint8_t const * lo_;
    std::uint8_t const * hi_;
    bool as_text_;
};

inline std::ostream &
operator<< (std::ostream & os, hex const & ins) {
    std::uint8_t lo = ins.x_ & 0xf;
    std::uint8_t hi = ins.x_ >> 4;

    char lo_ch = (lo < 10) ? '0' + lo : 'a' + lo - 10;
    char hi_ch = (hi < 10) ? '0' + hi : 'a' + hi - 10;

    os << hi_ch << lo_ch;

    if (ins.with_char_) {
        os << "(";
        if (std::isprint(ins.x_))
            os << (char)ins.x_;
        else
            os << "?";
        os << ")";
    }

    return os;
}

inline std::ostream &
operator<< (std::ostream & os, hex_view const & ins) {
    os << "[";
    std::size_t i = 0;
    for (std::uint8_t const * p = ins.lo_; p < ins.hi_; ++p) {
        if (i > 0)
            os << " ";
        os << hex(*p, ins.as_text_);
        ++i;
    }
    os << "]";
    return os;
}

/* implementation of streambuf that provides output to, and input from, a compressed stream
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_zstreambuf : public std::basic_streambuf<CharT, Traits> {
public:
    using size_type = std::uint64_t;
    using int_type = typename Traits::int_type;

public:
    basic_zstreambuf(size_type buf_z = 64 * 1024,
                     std::unique_ptr<std::streambuf> native_sbuf = std::unique_ptr<std::streambuf>())
        :
        in_zs_{aligned_upper_bound(buf_z), alignment()},
        out_zs_{aligned_upper_bound(buf_z), alignment()},
        native_sbuf_{std::move(native_sbuf)}
    {
        this->setg_span(in_zs_.uc_contents());
        this->setp_span(out_zs_.uc_avail());
    }
    ~basic_zstreambuf() {
        this->close();
    }

    std::uint64_t n_z_in_total() const { return in_zs_.n_in_total(); }
    /* note: z input side of zstreambuf = output from inflating-zstream */
    std::uint64_t n_uc_in_total() const { return in_zs_.n_out_total(); }

    /* note: uc output side of zstreambuf = input to deflating-zstream */
    std::uint64_t n_uc_out_total() const { return out_zs_.n_in_total(); }
    std::uint64_t n_z_out_total() const { return out_zs_.n_out_total(); }

    std::streambuf * native_sbuf() const { return native_sbuf_.get(); }

    void adopt_native_sbuf(std::unique_ptr<std::streambuf> x) { native_sbuf_ = std::move(x); }

    /* we have a problem writing compressed output:  compression algorithm in general
     * doesn't know how to compress byte n until it has seem byte n+1, .., n+k
     */
    void close() {
        if (!closed_flag_) {
            this->sync_impl(true /*final_flag*/);

            this->closed_flag_ = true;

            /* .native_sbuf may need to flush (e.g. if it's actually a filebuf).
             * The only way to invoke that behavior through the basic_streambuf api
             * is to invoke destructor,  so that's what we do here
             */
            this->native_sbuf_.reset();
        }
    }

    /* move-assignment */
    basic_zstreambuf & operator= (basic_zstreambuf && x) {
        /* assign any base-class state */
        std::basic_streambuf<CharT, Traits>::operator=(x);

        closed_flag_ = x.closed_flag_;
        in_zs_ = std::move(x.in_zs_);
        out_zs_ = std::move(x.out_zs_);

        native_sbuf_ = std::move(x.native_sbuf_);

        return *this;
    }

    void swap(basic_zstreambuf & x) {
        /* swap any base-class state */
        std::basic_streambuf<CharT, Traits>::swap(x);

        std::swap(closed_flag_, x.closed_flag_);

        std::swap(in_zs_, x.in_zs_);
        std::swap(out_zs_, x.out_zs_);

        std::swap(native_sbuf_, x.native_sbuf_);
    }

#  ifndef NDEBUG
    /* control per-instance debug output */
    void set_debug_flag(bool x) { debug_flag_ = x; }
#  endif

protected:
    /* estimates #of characters n available for input -- .underflow() will not be called
     * or throw exception until at least n chars are extracted.
     *
     * -1 if .showmanyc() can prove input has reached eof
     */
    //virtual std::streamsize showmanyc() override;

    /* attempt to read n chars from input,  and store in s.
     * (will call .uflow() as needed if less than n chars are immediately available)
     */
    //virtual std::streamsize xs_getn(char_type * s, std::streamsize n) override;

    /* ensure at least one character available in input area.
     * may update .gptr .egptr .eback to define input data location
     *
     * returns next input character (target of get-pointer)
     */
    virtual int_type underflow() override final {
        /* control here: .input buffer (i.e. .in_zs.uc_input_buf) has been entirely consumed */

#      ifndef NDEBUG
        if (debug_flag_)
            std::cerr << "zstreambuf::underflow: enter" << std::endl;
#      endif

        std::streambuf * nsbuf = native_sbuf_.get();

        /* any previous output from .in_zs has already been consumed (otherwise not in underflow state) */
        in_zs_.uc_consume_all();

        while (true) {
            /* zspan: available (unused) buffer space for compressed input */
            auto zspan = in_zs_.z_avail();

            std::streamsize n = 0;

            /* try to fill compressed-input buffer space */
            if (zspan.size()) {
                n = nsbuf->sgetn(reinterpret_cast<char *>(zspan.lo()),
                                 zspan.size());

                /* .in_zs needs to know how much we filled */
                in_zs_.z_produce(zspan.prefix(n));

#              ifndef NDEBUG
                if(debug_flag_)
                    std::cerr << "zstreambuf::underflow: read " << n << " compressed bytes (allowing space for " << zspan.size() << ")" << std::endl;
#              endif
            } else {
                /* it's possible previous inflate_chunk filled uncompressed output
                 * without consuming any compressed input,  in which case can have z_avail empty
                 */
            }

            /* do some decompression work */
            in_zs_.inflate_chunk();

            /* loop until uncompressed buffer filled,  or reached end of compressed input
             *
             * note this implies we always have whole-number-of-CharT in .uc_contents
             */
            if (in_zs_.uc_avail().empty() || (n < static_cast<std::streamsize>(zspan.size())))
                break;
        }

        /* ucspan: uncompressed output */
        auto ucspan = in_zs_.uc_contents();

        /* streambuf pointers need to know content
         *
         * see comment on loop above -- ucspan always aligned for CharT
         */
        this->setg_span(ucspan);

        if (ucspan.size())
            return Traits::to_int_type(*ucspan.lo());
        else
            return Traits::eof();
    }

    /* write contents of .output to .native_sbuf.
     * 0 on success, -1 on failure
     *
     * NOTE: After .sync() returns may still have un-synced output in .output_zs;
     *       tradeoff is that if we insist on writing that output,  will change the contents
     *       of comppressed output + degrade compression quality.
     */
    virtual int
    sync() override final {
#      ifndef NDEBUG
        if (debug_flag_)
            std::cerr << "zstreambuf::sync: enter" << std::endl;
#      endif

        return this->sync_impl(false /*!final_flag*/);
    }

    /* attempt to write n bytes starting at s[] to this streambuf.
     * returns the #of bytes actually written
     */
    virtual std::streamsize
    xsputn(CharT const * s, std::streamsize n_arg) override final {
#      ifndef NDEBUG
        if (debug_flag_) {
            std::cerr << "zstreambuf::xsputn: enter" << std::endl;
            std::cerr << hex_view(s, s+n_arg, true) << std::endl;
        }
#      endif

        if (closed_flag_)
            throw std::runtime_error("basic_zstreambuf::xsputn: attempted write to closed stream");

        std::streamsize n = n_arg;

        std::size_t i_loop = 0;

        while (n > 0) {
            std::streamsize buf_avail = this->epptr() - this->pptr();

            if (buf_avail == 0) {
                /* deflate some more output + free up buffer space */
                this->sync();
            } else {
                std::streamsize n_copy = std::min(n, buf_avail);

                ::memcpy(this->pptr(), s, n_copy);
                this->pbump(n_copy);

                s += n_copy;
                n -= n_copy;
            }

            ++i_loop;
        }

        return n_arg;
    }

    virtual int_type
    overflow(int_type new_ch) override final {
        if (this->sync() != 0) {
            throw std::runtime_error("basic_zstreambuf::overflow: sync failed to create buffer space");
        };

        if (Traits::eq_int_type(new_ch, Traits::eof()) != true) {
            *(this->pptr()) = Traits::to_char_type(new_ch);
            this->pbump(1);
        }

        return new_ch;
    }

private:
    /* write contents of .output to .native_sbuf.
     * 0 on success, -1 on failure.
     *
     * final_flag = true:  compressed stream is irrevocably complete -- no further output may be written
     * final_flag = false: after .sync_impl() returns may still have un-synced output in .output_zs
     *
     * TODO: sync for input (e.g. consider tailing a file)
     */
    int
    sync_impl(bool final_flag) {
#      ifndef NDEBUG
        if (debug_flag_)
        std::cerr << "zstreambuf::sync_impl: enter: :final_flag " << final_flag << std::endl;
#      endif

        if (closed_flag_) {
            /* implies attempt to write more output after call to .close() promised not to */
            return -1;
        }

        std::streambuf * nsbuf = native_sbuf_.get();

        /* consume all available uncompressed output
         *
         * note: converting from CharT* -> uint8_t* ok here.
         *       we are always starting with a properly-CharT*-aligned value,
         *       and in any case destination pointer used only with deflate(),
         *       which imposes no alignment requirements
         */

        out_zs_.uc_produce(span<std::uint8_t>(reinterpret_cast<std::uint8_t *>(this->pbase()),
                                              reinterpret_cast<std::uint8_t *>(this->pptr())));

        for (bool progress = true; progress;) {
            out_zs_.deflate_chunk(final_flag);
            auto zspan = out_zs_.z_contents();

            if (nsbuf->sputn(reinterpret_cast<char *>(zspan.lo()), zspan.size()) < static_cast<std::streamsize>(zspan.size()))
                throw std::runtime_error("zstreambuf::sync_impl: partial write!");

            out_zs_.z_consume(zspan);

            progress = (zspan.size() > 0);
        }

        /* uncompressed output buffer is empty,  since everything was sent to deflate;
         * can recycle it
         */
        this->setp_span(out_zs_.uc_avail());

        std::streamsize buf_avail = this->epptr() - this->pptr();

        if (buf_avail > 0) {
            /* control always here */
            return 0;
        } else {
            /* something crazy - maybe .output.buf_z == 0 ? */
            return -1;
        }
    }

    void setg_span(span<std::uint8_t> const & ucspan) {
        this->setg(reinterpret_cast<CharT *>(ucspan.lo()),
                   reinterpret_cast<CharT *>(ucspan.lo()),
                   reinterpret_cast<CharT *>(ucspan.hi()));
    }


    void setp_span(span<std::uint8_t> const & ucspan) {
        this->setp(reinterpret_cast<CharT *>(ucspan.lo()),
                   reinterpret_cast<CharT *>(ucspan.hi()));
    }

    static constexpr size_type alignment() {
        /* note: we can't support alignof(CharT) > sizeof(CharT),
         *       since we assume CharT's in a stream are packed
         */
        return sizeof(CharT);
    }

    /* returns #of bytes equal to a multiple of {CharT alignment,  sizeof(CharT)},
     * whichever is larger.  Use this to round up buffer sizes
     */
    static size_type aligned_upper_bound(size_type z) {
        constexpr size_type m = alignment();

        size_type extra = z % m;

        if (extra == 0)
            return z;
        else
            return z + (m - extra);
    }

private:
    /* Input:
     *                                       .inflate_chunk();
     *                   .sgetn()            .uc_contents()
     *    .native_sbuf -----------> .in_zs -------------------> .gptr, .egptr
     *
     * Output:
     *                                       .sync();
     *                                       .deflate_chunk();
     *                   .sputn()            .z_contents()          .sputn
     *   .pbeg, .pend ------------> .out_zs -------------------------------> .native_sbuf
     */

    /* set irrevocably on .close() */
    bool closed_flag_ = false;

    /* reminder:
     * 1. .eback() <= .gptr() <= .egptr()
     * 2. input buffer pointers .eback() .gptr() .egptr() are owned by basic_streambuf,
     *    and these methods are non-virtual.
     * 3. it's required that [.eback .. .egptr] represent contiguous memory
     */

    buffered_inflate_zstream in_zs_;
    buffered_deflate_zstream out_zs_;

    /* i/o for compressed data */
    std::unique_ptr<std::streambuf> native_sbuf_;

#  ifndef NDEBUG
    bool debug_flag_ = false;
#  endif
}; /*basic_zstreambuf*/

using zstreambuf = basic_zstreambuf<char>;

namespace std {
    template <typename CharT, typename Traits>
    void swap(basic_zstreambuf<CharT, Traits> & lhs,
              basic_zstreambuf<CharT, Traits> & rhs)
    {
        lhs.swap(rhs);
    }
}
