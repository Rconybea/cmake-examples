/** @file zstreambuf.hpp **/

#pragma once

#include "xfilebuf.hpp"
#include "compression/buffered_inflate_zstream.hpp"
#include "compression/buffered_deflate_zstream.hpp"
#include "compression/tostr.hpp"
#include "compression/hex.hpp"
#include "zlib.h"

#include <iostream>
#include <string>
#include <memory>

/**
   @class basic_zstreambuf zstream/zstreambuf.hpp

   @brief Implementation of @c std::basic_streambuf that provides automatic streaming inflation/deflation to gzip format

   Example - allocating buffer space from constructor.
   @code
     zstreambuf zbuf;

     std::unique_ptr<xfilebuf> p(new filebuf());
     if (p->open("path/to/file.gz", std::ios::in))
         zbuf.adopt_native_sbuf(std::move(p), -1)
   @endcode

   Example - allocating buffer space after constructor returns.
   @code
     zstreambuf zbuf(0);

     zbuf.alloc(64UL*1024UL);

     std::unique_ptr<xfilebuf> p(new filebuf());
     if (p->open("path/to/file.gz", std::ios::in))
         zbuf.adopt_native_sbuf(std::move(p), -1)
   @endcode

   @note Only uses calling thread;  not threadsafe.

   @tparam CharT.  Typename for (uncompressed) characters.  Compressed stream always comprises @c uint8_t's
   @tparam Traits.  Typename for streambuf traits object.  Typically this will be @c std::char_traits<CharT>.
**/
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_zstreambuf : public std::basic_streambuf<CharT, Traits> {
public:
    ///@{

    /** @brief type for buffer sizes **/
    using size_type = std::uint64_t;
    /** @brief type for character (the same as template parameter @c CharT) **/
    using char_type = typename Traits::char_type;
    /** @brief integral type large enough to hold a @c CharT **/
    using int_type = typename Traits::int_type;
    /** @brief integral type to represent a stream offset **/
    using off_type = typename Traits::off_type;
    /** @brief integral type to represent a stream position **/
    using pos_type = typename Traits::pos_type;
    /** @brief type to hold a native handle (i.e. file descriptor) Can get from @p Traits in c++26 **/
    using native_handle_type = int;

    ///@}

    /** @brief default buffer size for inflation/deflation (arbitrarily taking value from inflate side) **/
    static constexpr size_type c_default_buf_z = buffered_inflate_zstream::c_default_buf_z;

public:
    /**
     * @brief Creates new zstreambuf.
     *
     * @note Taking care with alignment: if @c CharT is a wide character type,  probably want/need aligned buffers
     * for uncompressed data.
     *
     * @param buf_z   Buffer size for inflation/deflation algorithm.  Actual buffer memory consumption is 4x this value,
     *        to account for:
     *             - @c .in_zs_.z_in_buf_   buffer compressed input
     *             - @c .in_zs_.uc_out_buf_ buffer uncompressed input
     *             - @c .out_zs_.uc_in_buf_ buffer uncompressed output
     *             - @c .out_zs_.z_out_buf_ buffer compressed output
     *        Can use 0 to defer buffer allocation
     * @param fd     Native handle (file descriptor), if known, that @ref native_sbuf is using.
     *        Providing to anticipate c++26,  and for the sake of python @c IOBase.fileno() when using this stream from python.
     * @param native_sbuf  @c streambuf for doing compressed i/o.
     *         If this is a filebuf,  it must be in an open state
     * @param mode    Openmode bitmask.  combination of @c std::ios::in, @c std::ios::out, @c std::ios::binary.
     *       Other openmode bits are ignored
     **/
    basic_zstreambuf(size_type buf_z = 64 * 1024,
                     native_handle_type fd = -1,
                     std::unique_ptr<std::streambuf> native_sbuf = std::unique_ptr<std::streambuf>(),
                     std::ios::openmode mode = std::ios::in)
        : openmode_{mode},
          in_zs_{aligned_upper_bound(buf_z), alignment()},
          out_zs_{aligned_upper_bound(buf_z), alignment()},
          native_sbuf_{std::move(native_sbuf)},
          native_fd_{fd}
    {
        this->closed_flag_ = (native_sbuf_ ? false : true);

        this->setg_span(in_zs_.uc_contents());
        this->setp_span(out_zs_.uc_avail());
    }
    /** @brief Destructor.  Recovers resources,  closing stream if necessary. **/
    ~basic_zstreambuf() {
        this->close();
    }

    ///@{

    /** @name Access methods **/

    /** @brief report openmode value recorded the last time this streambuf was opened **/
    std::ios::openmode openmode() const { return openmode_; }
    /** @brief @c true iff this zstreambuf is in an open state (available for i/o) **/
    bool is_open() const { return !closed_flag_; }
    /** @brief @c true iff this zstream is in a closed state (not available fopr i/o) **/
    bool is_closed() const { return closed_flag_; }
    /** @brief @c true iff this zstreambuf was last opened with @c ios::binary set **/
    bool is_binary() const { return (this->openmode_ & std::ios::binary); }
    /** @brief report native handle (file descriptor), if known **/
    native_handle_type native_handle() const { return native_fd_; }

    /** @brief number of bytes compressed input (according to @c zlib) consumed since this stream last opened **/
    std::uint64_t n_z_in_total() const { return in_zs_.n_in_total(); }
    /** @brief number of bytes of inflated input (according to @c zlib) produced since this stream last opened **/
    std::uint64_t n_uc_in_total() const { return in_zs_.n_out_total(); }

    /** @brief number of bytes of uncompressed output (according to @c zlib) consumed since this stream last opened **/
    std::uint64_t n_uc_out_total() const { return out_zs_.n_in_total(); }
    /** @brief number of bytes of deflated output (according to @c zlib) produced since this stream last opened **/
    std::uint64_t n_z_out_total() const { return out_zs_.n_out_total(); }

    /** @brief streambuf responsible for compressed version of this streambuf **/
    std::streambuf * native_sbuf() const { return native_sbuf_.get(); }

    ///@}

    /** @brief (Re)open streambuf,  connected to a file
     *
     *  If @p mode sets @c std::ios::out:  open @p filename for writing; if file with that name already exists, truncate it;  create new file if necessary.
     *  If @p mode does not set @c std::ios::out:  open file for reading.
     *
     *  @param filename   path to file.  If @c mode&ios::out, create file if necessary,
     *         and truncate if file already exists.
     *  @param mode       @c ios::in, @c ios::out only are supported;  @c ios::binary is assumed, whether or not specified.
     *         (in particular: @c ios::app, @c ios::trunc, @c ios::ate, @c ios::noreplace not supported)
     *
     *  @post
     *  @c .is_open() if file was successfully opened;  @c .is_closed() otherwise
     **/
    void open(char const * filename,
              std::ios::openmode mode = std::ios::in)
        {
            /* 1. cleanup any existing state
             *    (amongst other things,  destroys native sbuf.
             *     puts buffers (including streambuf position) into consistent nominal state)
             */
            this->close();

            /* 2. establish new state,  preserving buffer memory address ranges */

            this->openmode_ = mode;
            this->final_sync_flag_ = false;
            this->closed_flag_ = false;

            /* reminder: streambuf for compressed data always uses char + ignore CharT */
            std::unique_ptr<xfilebuf> p(new xfilebuf());
            if (p->open(filename, std::ios::binary | mode)) {
                this->adopt_native_sbuf(std::move(p), p->native_handle());
            }
        }

    /** @brief Allocate buffer space before first initiating i/o.

        @warning Not intended to be used after beginning inflation/deflation work.

        @param buf_z   Buffer size.  zstreambuf will create 4 buffers of this size.
     **/
    void alloc(size_type buf_z = c_default_buf_z) {
        in_zs_.alloc(buf_z, alignment());
        out_zs_.alloc(buf_z, alignment());

        this->setg_span(in_zs_.uc_contents());
        this->setp_span(out_zs_.uc_avail());
    }

    /** @brief Attach a streambuf for dealing with compressed data.

        If @p x is a @c filebuf,  it must be in an open state,  and it's openmode should include @c ios::binary.

        @param x  streambuf for compressed data, for example @c stringbuf or @c filebuf will work here.
        @param fd  native handle (file descriptor on linux), if known, for informational purposes

        @post @ref is_open = @c true
        @post @ref native_sbuf = address supplied with @p x
        @post @ref native_handle = @p fd
     */
    void adopt_native_sbuf(std::unique_ptr<std::streambuf> x,
                           native_handle_type fd = -1)
        {
            native_sbuf_ = std::move(x);

            if (native_sbuf_) {
                final_sync_flag_ = false;  /*redundant; to remove all doubt*/
                closed_flag_ = false;
            }

            /* stash file descriptor,  if available */
            native_fd_ = fd;
        }

#ifdef NOT_USING_AFTER_ALL
    /* read until n chars or delim reached,  whichever comes first
     * if this returns 0,  have reached eof
     */
    std::streamsize read_until(char_type * s,
                               std::streamsize n,
                               bool check_delim_flag,
                               char_type delim) {
        /*
         * r = content that's been read already
         * X = content that's available to be read
         *
         *     rrrrrrrrrrXXXXXXXXXXXXX
         *     ^         ^            ^
         *     .eback    .gptr        .egptr
         */

        char_type * p = s;

        char_type * gptr = this->gptr();
        char_type * egptr = this->egptr();

        std::streamsize i = 0;

        for (; i < n; ++i) {
            if (gptr == egptr) {
                this->underflow();
                gptr = this->gptr();
                egptr = this->egptr();

                if (gptr == egptr) {
                    /* exhausted input */
                    break;
                }
            }

            *p = *gptr;

            if (check_delim_flag && (*p == delim)) {
                this->setg(this->eback(), gptr+1, egptr);

                /* include delimiter in count */
                ++i;

                return i;
            }

            ++p;
            ++gptr;
        }

        if (i > 0) {
            this->setg(this->eback() /*preserving .eback*/,
                       gptr,
                       egptr);
        }

        //std::cerr << "zstreambuf::read_until: s=" << std::string_view(s, s + std::min(n, i)) << std::endl;

        return i;
    }
#endif

    /** @brief Flush remaining compressed data;  promise not to write again

        Given that there will be no more uncompressed output,
        commit remaining compressed portion to output stream.

        Exposed so that application code can observe final byte counters
        (@ref n_uc_out_total @ref n_z_out_total)
        before @ref close resets them
    **/
    void final_sync()
        {
            if (!final_sync_flag_)
                this->sync_impl(true /*final_flag*/);
        }

    /** @brief flush remaining ouput and put stream in a closed state.

        Stream can be reopened using @ref open
     **/
    void close() {
        this->final_sync();

        if (!this->closed_flag_) {
            this->closed_flag_ = true;

            this->in_uc_pos_ = 0;
            this->out_uc_pos_ = 0;

            /* invokes ::inflateEnd() then ::inflateInit2() */
            this->in_zs_.clear2empty(false /*zero_buffer_flag*/);
            /* invokes ::deflateEnd() then ::deflateInit2() */
            this->out_zs_.clear2empty(false /*zero_buffer_flag*/);

            /* .native_sbuf may need to flush (e.g. if it's actually a filebuf).
             * The only way to invoke that behavior through the basic_streambuf api
             * is to invoke destructor,  so that's what we do here
             */
            this->native_sbuf_.reset();
            this->native_fd_ = -1;

            /* also for consistency,  clear builtin streambuf pointers:
             * 1. no input (.setg_span())
             * 2. entire buffer available for output (.setp_span())
             */
            this->setg_span(this->in_zs_.uc_contents());
            this->setp_span(this->out_zs_.uc_avail());
        }
    }

    /** @brief Move-assignment **/
    basic_zstreambuf & operator= (basic_zstreambuf && x) {
        /* assign any base-class state */
        std::basic_streambuf<CharT, Traits>::operator=(x);

        final_sync_flag_ = x.final_sync_flag_;
        closed_flag_ = x.closed_flag_;

        in_uc_pos_ = x.in_uc_pos_;
        out_uc_pos_ = x.out_uc_pos_;

        in_zs_ = std::move(x.in_zs_);
        out_zs_ = std::move(x.out_zs_);

        native_sbuf_ = std::move(x.native_sbuf_);
        native_fd_ = x.native_fd_;

        return *this;
    }

    /** @brief swap state with another basic_zstreambuf **/
    void swap(basic_zstreambuf & x) {
        /* swap any base-class state */
        std::basic_streambuf<CharT, Traits>::swap(x);

        std::swap(final_sync_flag_, x.final_sync_flag_);
        std::swap(closed_flag_, x.closed_flag_);

        std::swap(in_uc_pos_, x.in_uc_pos_);
        std::swap(out_uc_pos_, x.out_uc_pos_);

        ::swap(in_zs_, x.in_zs_);
        ::swap(out_zs_, x.out_zs_);

        std::swap(native_sbuf_, x.native_sbuf_);
        std::swap(native_fd_, x.native_fd_);
    }

#  ifndef NDEBUG
    /** @brief in a debug build, control logging for this instance
        @param x  @c true to enable, @c false to disable
     **/
    void set_debug_flag(bool x) { debug_flag_ = x; }
#  endif

protected:
#ifdef NOT_USING
    /* "Estimates" #of characters n available for input
     * Promises .underflow() will not be called
     * or throw exception until at least n chars are extracted.
     *
     * 1.
     *   basic_istream.readsome()
     *    --calls-> streambuf.in_avail()
     *    --calls-> streambuf.showmanyc()  when get area is empty;
     *
     *   Therefore must implement .showmanyc() to support istream.readsome()
     *
     * 2.
     *   cppreference implies .showmanyc() is supposed to reach decision without blocking.
     *   Satisfying this requirement would mean we cannot actually use .readsome() as
     *   "alternative to .read() that does not set failbit on eof".
     *
     * We could choose to resolve this by having .showmanyc() attempt read when input area
     * is empty;  but won't do what we want -- we want variation on .read(z)
     * that always reads z chars when z chars exist on input before eof
     */
    virtual std::streamsize showmanyc() override {
        std::streamsize n = (this->egptr() - this->gptr());

        if (n == 0) {
            this->underflow(); /* always calls .setg_span() */
        }

        n = (this->egptr() - this->gptr());

        if (n == 0) {
            /* assuming input stream is blocking here.
             * if got 0 bytes + EWOULDBLOCK,  should return 0 instead
             */
            return -1;
        }

        return n;
    }
#endif

    /* attempt to read n chars from input,  and store in s.
     * (will call .uflow() as needed if less than n chars are immediately available)
     */
    //virtual std::streamsize xs_getn(char_type * s, std::streamsize n) override;

    /**
       @brief Ensure at least one character available for reading in input area.

       May update input pointers (@c gptr @c egptr @c eback) to define input data location

       @retval next input character (target of get-pointer)
    **/
    virtual int_type underflow() override final {
        /* control here: .input buffer (i.e. .in_zs.uc_input_buf) has been entirely consumed */

#      ifndef NDEBUG
        if (debug_flag_)
            std::cerr << "zstreambuf::underflow: enter" << std::endl;
#      endif

        if ((openmode_ & std::ios::in) == 0)
            throw std::runtime_error("basic_zstreambuf::underflow: expected ios::in bit set when reading from streambuf");

        if (this->gptr() < this->egptr()) {
            /* short-circuit unnecessary .underflow(),  so we don't trash state */
            return Traits::to_int_type(*this->gptr());
        }

        std::streambuf * nsbuf = native_sbuf_.get();

        if (!nsbuf) {
            throw std::runtime_error("basic_zstreambuf::underflow: attempt to read from closed stream");
        }

        /* read position associated with start of buffer needs to include
         * buffer extent that we're about to replace
         */
        in_uc_pos_ += in_zs_.uc_contents().size();

        /* any previous output from .in_zs must have already been consumed (otherwise not in underflow state) */
        in_zs_.uc_consume(in_zs_.uc_contents());

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

        /* ucspan: uncompressed input */
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

    /** @brief Write buffered contents of output to @ref native_sbuf

        @warning After sync returns may still have un-synced output in @ref out_zs_
        (held privately by @c zlib @c z_stream).
        Prematurely forcing @c zlib to such output will degrade compression quality.
        We choose not to do that until end of stream.

        @return @c 0 on success, @c -1 on failure (@c iostream will set @c failbit on failure).
     **/
    virtual int
    sync() override final {
#      ifndef NDEBUG
        if (debug_flag_)
            std::cerr << "zstreambuf::sync: enter" << std::endl;
#      endif

        return this->sync_impl(false /*!final_flag*/);
    }

    /** @brief attempt to write n bytes starting at s[] to this streambuf.

        Write characters @c s[0] .. @c s[n_arg-1] to output.

        @param s      write output starting from this address
        @param n_arg  number of characters to write
        @return       number of characters actually written
    **/
    virtual std::streamsize
    xsputn(CharT const * s, std::streamsize n_arg) override final {
#      ifndef NDEBUG
        if (debug_flag_) {
            std::cerr << "zstreambuf::xsputn: enter" << std::endl;
            std::cerr << hex_view(s, s+n_arg, true) << std::endl;
        }
#      endif

        if (final_sync_flag_)
            throw std::runtime_error("basic_zstreambuf::xsputn: attempted write after final sync");

        if (closed_flag_)
            throw std::runtime_error("basic_zstreambuf::xsputn: attempted write to closed stream");

        if ((openmode_ & std::ios::out) == 0)
            throw std::runtime_error("basic_zstreambuf::xsputn: expected ios::out bit set when writing to streambuf");

        std::streamsize n_remaining = n_arg;

        while (n_remaining > 0) {
            std::streamsize buf_avail = this->epptr() - this->pptr();

            if (buf_avail == 0) {
                /* deflate some more output + free up buffer space */
                this->sync();
            } else {
                std::streamsize n_copy = std::min(n_remaining, buf_avail);

                ::memcpy(this->pptr(), s, n_copy);
                this->pbump(n_copy);

                this->out_uc_pos_ += n_copy;

                s += n_copy;
                n_remaining -= n_copy;
            }
        }

        return n_arg;
    }

    /** @brief Report input or output position

        @note
        Minimal implementation, necessary to support @c zstream methods @c tellg() and @c tellp().
        - @c tellg(): @c seekoff(0,ios_base::cur,ios::in)
        - @c tellp(): @c seekoff(0,ios_base::cur,ios::out)

        @param offset   Must be @c 0
        (in @c streambuf: desired seek offset from reference position)
        @param way      Must be @c ios_base::cur
        (in @c streambuf: determines reference position for @p offset).
        @param which    Must be either @c ios::in or @c ios::out
        (in @c streambuf: whether to affect input position, output position, or both)
        @retval desired position;  -1 for unsupported argument combinations
    **/
    virtual pos_type seekoff(off_type offset,
                             std::ios_base::seekdir way,
                             std::ios_base::openmode which) override final
        {
            /* Infeasible to implement this in general
             * (at least without intimate understanding of deflated zlib format).
             *
             * Do need to implement special case offset=0,
             * since iostream uses this to implement .tellg() + .tellp()
             *
             * Can possibly implement some other special cases with effort,  see TODO below
             */
            if ((offset == 0)
                && (way == std::ios_base::cur))
            {
                /* here: not attempting to change stream positioning */

                if ((which & std::ios_base::out) == std::ios_base::out) {
                    return this->out_uc_pos_;
                } else {
                    return this->in_uc_pos_ + this->gptr() - this->eback();
                }
            }

            /* TODO: feasible to implement:
             *
             * - seek input to any position p,  with caveats:
             *   - seek forward from current position cost O(p - cur)
             *     since have to run subseq(cur..p) through inflation algorithm
             *   - seek backward from current position cost O(p),
             *     since have to restart from beginning of input stream,
             *     and (re)inflate subseq(0..p)
             * - seek output to position 0:
             *   - equivalent to discarding output + restarting compression
             *   - anything else considered too expensive (absent moving to
             *     some bespoke block-based format), since would have to rebuild compressed stream
             *     from scratch
             */

            return -1;
        }

    /* in practice this won't be used,  because .xsputn() calls .sync() as needed */
#ifdef NOT_IN_USE
    virtual int_type
    overflow(int_type new_ch) override final
        {
            if (this->sync() != 0) {
                throw std::runtime_error("basic_zstreambuf::overflow: sync failed to create buffer space");
            };

            if (Traits::eq_int_type(new_ch, Traits::eof()) != true) {
                *(this->pptr()) = Traits::to_char_type(new_ch);
                this->pbump(1);
            }

        return new_ch;
    }
#endif

private:
    /** @brief Commit available compressed output to @ref native_sbuf

        No effect if openmode does not set @c ios::out

        @param final_flag
        if @c true: compressed stream is complete; flush remainder and prevent further output
        if @c false: write committed data produced by @c zlib;  trailing suffix may remain pending
        in private @c zlib state,  to be determined as stream send @c zlib more output

        @return @c 0 on success, @c -1 on failure.

        @pre Stream is @ref open and @ref final_sync has not been called
    **/
    int
    sync_impl(bool final_flag) {
#      ifndef NDEBUG
        if (debug_flag_)
        std::cerr << "zstreambuf::sync_impl: enter: :final_flag " << final_flag << std::endl;
#      endif

        if (final_sync_flag_) {
            /* implied duplicate call to .sync_impl(true) */
            return -1;
        }

        if (closed_flag_) {
            /* implies attempt to write more output after call to .close() promised not to */
            return -1;
        }

        if (final_flag)
            this->final_sync_flag_ = true;

        if ((openmode_ & std::ios::out) == 0) {
            /* nothing to do if not using stream for output */
            return 0;
        }

        std::streambuf * nsbuf = native_sbuf_.get();

        /* Consume (i.e. deflate) all collected uncompressed output
         * We've contrived for this->pbase() .. this->pptr() to always refer to some subrange of
         * .out_zs.uc_in_buf;  the uc_produce() call needs only update .out_zs.uc_in_buf.hi_pos
         *
         * note: converting from CharT* -> uint8_t* ok here.
         *       we are always starting with a properly-CharT*-aligned value,
         *       and in any case destination pointer used only with deflate(),
         *       which imposes no alignment requirements
         */
        out_zs_.uc_produce(span<std::uint8_t>(reinterpret_cast<std::uint8_t *>(this->pbase()),
                                              reinterpret_cast<std::uint8_t *>(this->pptr())));

        for (bool progress = true; progress;) {
            this->out_zs_.deflate_chunk(final_flag);

            auto zspan = out_zs_.z_contents();

            /* send compressed output to native sbuf */
            std::streamsize n_written = nsbuf->sputn(reinterpret_cast<char *>(zspan.lo()),
                                                     zspan.size());

#          ifndef NDEBUG
            if (debug_flag_) {
                std::cerr << "zstreambuf::sync_impl: (compressed) zspan.lo=" << static_cast<void *>(zspan.lo())
                          << ", zspan.size=" << zspan.size() << ", n_written=" << n_written << std::endl;
            }
#          endif

            if (n_written < static_cast<std::streamsize>(zspan.size())) {
                throw std::runtime_error(tostr("zstreambuf::sync_impl: partial write",
                                               " :attempted ", zspan.size(),
                                               " :wrote ", n_written));
            }

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

    /** @brief set @c streambuf input positions to span endpoints
        @param ucspan  span comprising entirety of available-and-uncompressed input to be read.
     **/
    void setg_span(span<std::uint8_t> const & ucspan) {
        this->setg(reinterpret_cast<CharT *>(ucspan.lo()),
                   reinterpret_cast<CharT *>(ucspan.lo()),
                   reinterpret_cast<CharT *>(ucspan.hi()));
    }

    /** @brief set @c streambuf output positions to span endpoints
        @param ucspan  span comprising entirety of available-and-not-compressed output to be written
    **/
    void setp_span(span<std::uint8_t> const & ucspan) {
        this->setp(reinterpret_cast<CharT *>(ucspan.lo()),
                   reinterpret_cast<CharT *>(ucspan.hi()));
    }

    /** @brief alignment to be used for a stream of @p CharT

        This is @c sizeof(CharT).
        @c alignof(CharT)>sizeof(CharT) would not work since implementation assumes packed buffer arithmetic
     **/
    static constexpr size_type alignment() {
        return sizeof(CharT);
    }

    /** @brief round up size @p z to a whole multiple of CharT size.

        For example if @c sizeof(CharT)=4, and @c z=5,  @c aligned_upper_bound(z) = @c 8
     **/
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
     *
     * reminder:
     * 1. .eback() <= .gptr() <= .egptr()
     * 2. input buffer pointers .eback() .gptr() .egptr() are owned by basic_streambuf,
     *    and these methods are non-virtual.
     * 3. it's required that [.eback .. .egptr] represent contiguous memory
     */

    /** @brief openmode for compressed stream

        zstreambuf needs to know if intending to use this zstreambuf for output:
        compressing an empty input sequence produces non-empty output (since will create a 20-byte @c gzip header)
        Therefore:
        - zstream("foo.gz",ios::out) should create valid @c foo.gz representing an empty sequence.
        - sync_impl(true) needs to know whether it's responsible for achieving this; recall that it will also be called
          on an input-only stream
    **/
    std::ios::openmode openmode_;

    /** @brief @c true iff @ref final_sync has been called on this streambuf **/
    bool final_sync_flag_ = false;
    /** @brief @c true iff streambuf is in a closed state. **/
    bool closed_flag_ = false;

    /** @brief input position relative to beginning of stream,  after last call to @ref underflow

        @note EXCLUDES range [@c eback .. @c gptr],  since @c gptr updates non-virtually between calls
        to @ref underflow.
    **/
    pos_type in_uc_pos_ = 0;

    /** @brief output position relative to beginning of stream.

        Updates on every call to @ref xsputn.

        @note delegating this to @ref out_zs_ will not work,  because that state only updates via @ref sync,
        and @ref xsputn will defer that until output buffer is full.
    **/
    pos_type out_uc_pos_ = 0;

    /** @brief stream for inflating input from @ref native_sbuf_ **/
    buffered_inflate_zstream in_zs_;
    /** @brief stream for deflating output to @ref native_sbuf_ **/
    buffered_deflate_zstream out_zs_;

    /** @brief i/o for compressed data **/
    std::unique_ptr<std::streambuf> native_sbuf_;

    /** @brief native file descriptor (or with some effort, other os-dependent handle), if known.

        @note Will be able to get this from @c filebuf in c++26,  at cost of a dynamic cast.
     **/
    native_handle_type native_fd_ = -1;

#  ifndef NDEBUG
    /** @brief in debug build, remembers whether logging enabled for this instance **/
    bool debug_flag_ = false;
#  endif
}; /*basic_zstreambuf*/

/** @brief typealias for basic_zstreambuf<char> **/
using zstreambuf = basic_zstreambuf<char>;

/** @brief provide overload for @c swap(), so that @ref basic_zstreambuf is swappable **/
template <typename CharT, typename Traits>
void swap(basic_zstreambuf<CharT, Traits> & lhs,
          basic_zstreambuf<CharT, Traits> & rhs)
{
    lhs.swap(rhs);
}
