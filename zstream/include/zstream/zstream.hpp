/** @file zstream.hpp **/

#pragma once

#include "zstreambuf.hpp"
#include <iostream>
#include <fstream>
#include <list>

/* note: need to allow out-of-memory-order initialization of basic_zstream
 * 1. basic_zstream::rdbuf needs to be constructed (so its in valid, nominal state)
 *    before passing it to (parent) basic_iostream ctor.
 * 2. This is out-of-memory order,  since memory for parent basic_iostream
 *    precedes memory for basic_zstream members,
 *
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"

/**
   @class basic_zstream zstream/zstream.hpp

   @brief iostream implementation with automatic compression/uncompression

   @tparam CharT character type for stream elements
   @tparam Traits character traits;  typically @c std::char_traits<CharT>

   Example 1 - create a @c .gzip file
   @code
   zstream zs(0, "path/to/foo.gz", ios::out);

   zs << "Some text to be compressed" << endl;
   zs.close();

   @endcode

   Example 2 - read from a @c .gzip file
   @code
   zstream zs(0, "path/to/foo.gz", ios::in);

   while (!zs.eof()) {
       string x;
       zs >> x;

       cout << "input: [" << x << "]" << endl;
   }
   @endcode

   Example 3 - attach existing streambuf
   @code
   ios::openmode mode = ...;

   zstream zs(0, nullptr, mode);

   unique_ptr<streambuf> sbuf = ...;  // streambuf for compressed data
   int fd = -1;                       // or actual file descriptor,  if known

   zs.rdbuf()->adopt_native_sbuf(std::move(sbuf), fd);
   @endcode
 **/
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_zstream : public std::basic_iostream<CharT, Traits> {
public:
    /** @brief imported from @p Traits **/
    ///@{

    using char_type = CharT; // = Traits::char_type
    using traits_type = Traits;
    using int_type = typename Traits::int_type;
    using pos_type = typename Traits::pos_type;
    using off_type = typename Traits::off_type;
    using zstreambuf_type = basic_zstreambuf<CharT, Traits>;
    /** @brief will import from @p Traits in c++26 **/
    using native_handle_type = int;

    ///@}

    /** @brief Invalid file descriptor value **/
    static constexpr native_handle_type c_empty_native_handle = -1;
    /** @brief Default buffer size.  Buffers are used to hold both compressed and uncompressed data. **/
    static constexpr std::streamsize c_default_buffer_size = zstreambuf_type::c_default_buf_z;

public:
    ///@{

    /**
       @brief Create zstream in closed state.

       Before using stream for i/o,  application code must invoke either
       - attach a streambuf for compressed i/o (call @c rdbuf() method @c adopt_native_sbuf(sbuf,native_fd))
       - call @ref open
    **/
    basic_zstream(std::streamsize buf_z = c_default_buffer_size,
                  std::ios::openmode mode = std::ios::in)
        : rdbuf_(buf_z,
                 c_empty_native_handle /*native_fd*/,
                 nullptr /*native_sbuf*/,
                 mode),
          std::basic_iostream<CharT, Traits>(&rdbuf_)
        {
            /* closed state = empty stream -> eof */
            this->setstate(std::ios_base::eofbit);
        }

    /**
       @brief Create zstream using supplied streambuf for compressed data

       @param buf_z  buffer size.  Implementation allocates 4 buffers of this size,  one each
       for {input, output} x {inflated, deflated}.
       @param native_sbuf  streambuf for compressed data.   Default is to allocate a @ref xfilebuf instance.
       @param mode  combination of @c ios::in, @c ios::out, @c ios::binary.  Other @c openmode bits ignored.
    **/
    basic_zstream(std::streamsize buf_z,
                  std::unique_ptr<std::streambuf> native_sbuf,
                  std::ios::openmode mode)
        : rdbuf_(buf_z,
                 c_empty_native_handle /*native_fd*/,
                 std::move(native_sbuf),
                 mode),
          std::basic_iostream<CharT, Traits>(&rdbuf_)
        {}

    /**
       @brief Create zstream using supplied streambuf for compressed data, with default buffer size
       @param native_sbuf  streambuf for compressed data.   Default is to allocate a @ref xfilebuf instance.
       @param mode  combination of @c ios::in, @c ios::out, @c ios::binary.  Other @c openmode bits ignored.
    **/
    basic_zstream(std::unique_ptr<std::streambuf> native_sbuf,
                  std::ios::openmode mode)
        : basic_zstream(c_default_buffer_size,
                        c_empty_native_handle /*native_fd (n/a or unknown)*/,
                        std::move(native_sbuf),
                        mode)
        {}

    /**
       @brief Create zstream attached to a (compressed) file
       @param buf_z  buffer size.  Implementation allocates 4 buffers of this size,  one each
       for {input, output} x {inflated, deflated}.
       @param filename  Open file with this path to hold compressed data.  File will be in gzip format.
       @param mode  combination of @c ios::in, @c ios::out, @c ios::binary.  Other @c openmode bits ignored.
     **/
    basic_zstream(std::streamsize buf_z,
                  char const * filename,
                  std::ios::openmode mode = std::ios::in)
        : rdbuf_(buf_z,
                 c_empty_native_handle /*native_fd*/,
                 nullptr /*native_sbuf*/,
                 mode),
          std::basic_iostream<CharT, Traits>(&rdbuf_)
        {
            if (filename
                && (::strlen(filename) > 0))
            {
                std::unique_ptr<xfilebuf> p(new xfilebuf());

                if (p->open(filename, std::ios::binary | mode)) {
                    this->rdbuf_.adopt_native_sbuf(std::move(p),
                                                   p->native_handle());
                } else {
                    /* open failed: want failbit set */
                    this->setstate(std::ios_base::failbit);
                }
            } else {
                /* no filename or empty filename:
                 * 1. do not create native sbuf
                 * 2. stream in eof state
                 */
                this->setstate(std::ios_base::eofbit);
            }
        }

    /* convenience ctor;  apply default buffer size */
    basic_zstream(char const * filename,
                  std::ios::openmode mode = std::ios::in)
        : basic_zstream(c_default_buffer_size, filename, mode) {}

    ///@}

    ~basic_zstream() = default;

    ///@{

    /** @name access methods **/

    zstreambuf_type * rdbuf() { return &rdbuf_; }
    std::ios::openmode openmode() const { return rdbuf_.openmode(); }
    // bool eof() const;   // editor bait: inherited
    /** @brief @c true iff this zstream is in an open state (available for i/o) **/
    bool is_open() const { return rdbuf_.is_open(); }
    /** @brief @c true iff this zstream is in a closed state (not available for i/o) **/
    bool is_closed() const { return rdbuf_.is_closed(); }
    /** @brief @c true iff this zstream was last opened with @c ios::binary set **/
    bool is_binary() const { return rdbuf_.is_binary(); }
    /** @brief report native handle (file descriptor), if known **/
    native_handle_type native_handle() const { return rdbuf_.native_handle(); }

    ///@}

    /** @brief  Allocate buffer space for inflation/deflation.

        May use before reading/writing any data,  after calling ctor with 0 buf_z.
        Does not preserve any existing buffer contents.
        Not inteended to be used after beginning inflation/deflation work
    **/
    void alloc(std::streamsize buf_z = c_default_buffer_size) { rdbuf_.alloc(buf_z); }

    /** @brief (Re)open zstream,  connected to a .gz file
        @param filename   Connect zstream to file with this pathname.
        @param mode       Openmode. If @c ios::out, provide empty file, creating or truncating as need be.

        @post if successful, @c is_open() = @c true
     **/
    void open(char const * filename,
              std::ios::openmode mode = std::ios::in)
        {
            /* clear state bits,  in case we previously used this stream for i/o */
            this->clear();

            this->rdbuf_.open(filename, mode);
        }

    /** @brief read characters up to delimiter

        Read up to @c n-1 chars, into memory @c s[0]..s[n-2]
        Stop when either:
        - @c s[] is full (contains @c n-1 chars)
        - @p check_delim_flag is true,  and reached character @p delim

        Similar to @c .get(s,n,delim) except that:
        - @c read_until returns the number of characters read instead of @c *this
        - @c read_until includes writes @p delim (if encountered) to @p s

        @c read_until ignores the @c noskipws flag

        @note We want @c read_until() as building block for overload that does not require
        caller to supply buffer size.

        @retval number of chars written to @c s[],  excluding trailing null.

        @post if @c read_until() returns @c nr with @c nr>0, then @c s[nr-1] is null (i.e. @c char_type())

        @param s   write characters to this array.
        @param n   available space in @p s.  Will not write past @c s[n-1]
        @param check_delim_flag  @c true to stop after first occurrence of @p delim
        @param delim  if @p check_delim_flag is @c true stop after first occurrence of @p delim in input
     **/
    std::streamsize read_until(char_type * s,
                               std::streamsize n,
                               bool check_delim_flag,
                               char_type delim)
        {
            using sentry_type = typename std::basic_istream<CharT, Traits>::sentry;

            if (n <= 0)
                return 0;

            sentry_type sentry(*this, true /*noskipws*/);

            std::streamsize nr = 0;

            if (sentry) {
                try {
                    if (check_delim_flag) {
                        /* notes on istream.get():
                         * 1. sets failbit if delim is first character in stream (!!)
                         * 2. mandates trailing null s[n-1], so only reads n-1 chars
                         * 3. excludes trailing delim.
                         *    We want to include it (since python requires it)
                         */

                        int_type nextc = this->rdbuf_.sgetc();

                        if (nextc == Traits::to_int_type(delim)) {
                            nr = 0;
                        } else {
                            this->get(s, n, delim);

                            nr = this->gcount();
                        }

                        if (nr < n-1) {
                            /* go directly to .rdbuf to ignore fmtflags. */

                            int_type nextc = this->rdbuf_.sgetc();

                            if (nextc == Traits::to_int_type(delim)) {
                                /* include delimiter in result */

                                ++(this->_M_gcount);
                                this->rdbuf_.sbumpc();

                                /* include delim in s[] */
                                s[nr]   = delim;
                                s[nr+1] = char_type();
                                nr = nr+1;
                            } else if (nextc == Traits::eof()) {
                                /* eof != delim -> nothing to do here */
                            }
                        }
                    } else /*!check_delim_flag*/ {
                        /* for consistency with the delim case, only fetch n-1 chars and null-terminate */

                        nr = this->rdbuf_.sgetn(s, n-1);

                        if (nr < n-1)
                            this->setstate(std::ios_base::eofbit);
                    }
                } catch(__cxxabiv1::__forced_unwind & x) {
                    this->setstate(std::ios::failbit);
                    throw;
                } catch(...) {
                    /* swallow i/o exceptions (except forced unwind) encountered while reading */
                    this->setstate(std::ios::failbit);
                }
            }

            return nr;
        }

    /** @brief read characters up to delimiter and package into a string

        @note implementation is O(n) for return value of length n.

        @param check_delim_flag If @c true read until after first occurrence of @p delim.
        Otherwise read until end of input.
        @param delim Stop after first occurrence of this character.  Ignored if @p check_delim_flag is @c false.
        @param block_z  Use temporary strings of this size to assemble result.

        @retval string containing characters read
    **/
    std::string read_until(bool check_delim_flag,
                           char_type delim,
                           uint32_t block_z = 4095)
        {
            if (block_z == 0) {
                /* heuristic:
                 * - approx size of 1 disk page
                 * - less 1 byte (in case string allocs 1 byte extra for null)
                 */
                block_z = 4095;
            }

            /* list of complete blocks read;  each string in this list has block_z chars */
            std::list<std::string> block_l;

            /* index# of next block to read */
            uint32_t i_block = 0;

            /* last block read.   0..block_z chars */
            std::string last_block;

            for (;; ++i_block) {
                last_block = std::string();
                last_block.resize(block_z + 1);

                /* reminder: .read_until() helper actually only reads block_z chars */
                std::streamsize n = this->read_until(last_block.data(), block_z + 1, check_delim_flag, delim);

                last_block.resize(n);

                if (n < block_z)
                    break;

                block_l.push_back(last_block);
            }

            /* final result is concatenation of:
             *   - strings collected in block_l (each string of length block_z)
             *   - leftover string in last_block
             */

            std::string retval;

            retval.resize((block_l.size() * block_z) + last_block.size());

            /* note: using form below so memory for each block is released
             *       as soon as its contents has been copied to retval.
             */
            i_block = 0;
            for (; !block_l.empty(); ++i_block) {
                std::string & s = block_l.front();

                std::copy(s.data(),
                          s.data() + s.size(),
                          retval.data() + (i_block * block_z));

                block_l.pop_front();
            }

            std::copy(last_block.data(),
                      last_block.data() + last_block.size(),
                      retval.data() + (i_block * block_z));

            return retval;
        }

    /** @brief Move assignment; zstream is movable **/
    basic_zstream & operator=(basic_zstream && x) {
        /* assign any base-class state */
        std::basic_iostream<CharT, Traits>::operator=(x);

        this->rdbuf_ = std::move(x.rdbuf_);
        return *this;
    }

    /** @brief exchange state with @p x **/
    void swap(basic_zstream & x) {
        /* swap any base-class state */
        std::basic_iostream<CharT, Traits>::swap(x);
        /* swap streambuf state */
        this->rdbuf_.swap(x.rdbuf_);
    }

    /** @brief flush any trailing compressed output;  promise not to write any more before closing

        Allow application code to read final counter values (e.g. @ref zstreambuf::n_z_in_total) before
        @ref close resets them.  Otherwise application code can ignore this method.
     **/
    void final_sync() {
        this->rdbuf_.final_sync();
    }

    /** @brief close stream,  ensuring all buffered compressed data is written

        @post @ref is_closed = @c true
        @post @c eof() = @c true
        @post @c fail() = @c false
     **/
    void close() {
        this->rdbuf_.close();

        /* clear state bits:  in particular need to clear any of {failbit, badbit} */
        this->clear(std::ios::eofbit);
    }

#  ifndef NDEBUG
    /** @brief in debug build, mark streambuf to log some behavior to cerr **/
    void set_debug_flag(bool x) { rdbuf_.set_debug_flag(x); }
#  endif

private:
    /** @brief streambuf implementation;  performs inflation (on input) and deflation (on output) **/
    basic_zstreambuf<CharT, Traits> rdbuf_;
}; /*basic_zstream*/
#pragma GCC diagnostic pop

/** @brief typealias for @c basic_zstream<char> **/
using zstream = basic_zstream<char>;

/** @brief Provide overload of @c swap(), so that @ref basic_zstream is swappable **/
template <typename CharT, typename Traits>
void swap(basic_zstream<CharT, Traits> & lhs,
          basic_zstream<CharT, Traits> & rhs)
{
    lhs.swap(rhs);
}
