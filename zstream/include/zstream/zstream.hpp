// zstream.hpp

#pragma once

#include "zstreambuf.hpp"
#include <iostream>
#include <fstream>
#include <list>

/* note: We want to allow out-of-memory-order initialization here.
 *       1. We (presumably) must initialize .rdbuf before passing it to basic_iostream's ctor
 *       2. Since we inherit basic_iostream,  its memory will precede .rdbuf
 *
 * Example 1 (compress)
 *
 *   // zstream = basic_zstream<char>,  in this file following basic_zstream decl
 *   zstream zs(64*1024, "path/to/foo.gz", ios::out);
 *
 *   zs << "some text to be compressed" << endl;
 *
 *   zs.close();
 *
 * Example 2 (uncompress)
 *
 *   zstream zs(64*1024, "path/to/foo.gz", ios::in);
 *
 *   while (!zs.eof()) {
 *     std::string x;
 *     zs >> x;
 *
 *     cout << "input: [" << x << "]" << endl;
 *   }
 *
 * Example 3 (deferred open)
 *
 *   zstream zs(0, nullptr, mode);  // or zstream zs;
 *   zs.alloc(buf_z); // create buffers
 *
 *   std::unique_ptr<std::streambuf> sbuf = ...;  // streambuf for compressed data
 *   int fd = -1;  // or file descriptor if known
 *   zs.rdbuf()->adopt_native_sbuf(std::move(streambuf), fd)
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_zstream : public std::basic_iostream<CharT, Traits> {
public:
    using char_type = CharT;
    using traits_type = Traits;
    using int_type = typename Traits::int_type;
    using pos_type = typename Traits::pos_type;
    using off_type = typename Traits::off_type;
    using zstreambuf_type = basic_zstreambuf<CharT, Traits>;
    using native_handle_type = int;

    static constexpr native_handle_type c_empty_native_handle = -1;
    static constexpr std::streamsize c_default_buffer_size = zstreambuf_type::c_default_buf_z;

public:
    /* caller must establish .rdbuf.native_sbuf before using zstream.
     * either
     * - .rdbuf()->adopt_native_sbuf(sbuf, native_fd)
     * - .rdbuf()->open(fname, openmode)
     */
    basic_zstream(std::streamsize buf_z = c_default_buffer_size,
                  std::ios::openmode mode = std::ios::in)
        : rdbuf_(buf_z,
                 c_empty_native_handle /*native_fd*/,
                 nullptr /*native_sbuf*/,
                 mode),
          std::basic_iostream<CharT, Traits>(&rdbuf_)
        {}

    basic_zstream(std::streamsize buf_z,
                  std::unique_ptr<std::streambuf> native_sbuf,
                  std::ios::openmode mode)
        : rdbuf_(buf_z,
                 c_empty_native_handle /*native_fd*/,
                 std::move(native_sbuf),
                 mode),
          std::basic_iostream<CharT, Traits>(&rdbuf_)
           {}
    /* convenience ctor;  apply default buffer size */
    basic_zstream(std::unique_ptr<std::streambuf> native_sbuf,
                  std::ios::openmode mode)
        : basic_zstream(c_default_buffer_size,
                        c_empty_native_handle /*native_fd*/,
                        std::move(native_sbuf),
                        mode)
        {}
    /* convenience ctor;  creates filebuf attached to filename and opens it.
     *
     * It might look like an oversight that we use xfilebuf here instead of basic_xfilebuf<CharT, Traits>.
     * It's on purpose since refers to the compressed stream
     */
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
                }
            }
        }

    /* convenience ctor;  apply default buffer size */
    basic_zstream(char const * filename,
                  std::ios::openmode mode = std::ios::in)
        : basic_zstream(c_default_buffer_size, filename, mode) {}

    ~basic_zstream() = default;

    zstreambuf_type * rdbuf() { return &rdbuf_; }
    std::ios::openmode openmode() const { return rdbuf_.openmode(); }
    // bool eof() const;   // editor bait: inherited
    bool is_open() const { return rdbuf_.is_open(); }
    bool is_closed() const { return rdbuf_.is_closed(); }
    bool is_binary() const { return rdbuf_.is_binary(); }
    native_handle_type native_handle() const { return rdbuf_.native_handle(); }

    /* Allocate buffer space.  May use before reading/writing any data,  after calling ctor with 0 buf_z.
     * Does not preserve any existing buffer contents.
     * Not inteended to be used after beginning inflation/deflation work
     */
    void alloc(std::streamsize buf_z = c_default_buffer_size) { rdbuf_.alloc(buf_z); }

    /* (Re)open stream,  connected to a .gz file */
    void open(char const * filename,
              std::ios::openmode mode = std::ios::in)
        {
            /* clear state bits,  in case we previously used this stream for i/o */
            this->clear();

            this->rdbuf_.open(filename, mode);
        }

    /* read into s[0]..s[n-1] until either:
     * 1. s[] is full
     * 2. check_delim_flag is true and reached character delim.
     *    In this case behavior is equivalent to .get(s, n, delim),
     *    except that .read_until() returns the #of characters read,
     *    insteead of *this.
     *
     * We need .read_until() as building block for overload that does not require caller
     * to supply buffer size.
     */
    std::streamsize read_until(char_type * s,
                               std::streamsize n,
                               bool check_delim_flag,
                               char_type delim)
        {
            if (check_delim_flag) {
                this->get(s, n, delim);
                std::streamsize nr = this->gcount();

                return nr;
            }


            if (n <= 0)
                return 0;

            std::streamsize retval = this->rdbuf_.read_until(s, n,
                                                             check_delim_flag, delim);

            if (retval == 0)
                this->setstate(std::ios_base::eofbit);

            return retval;
        }

    /* if check_delim_flag is true: read up to first occurence of delim
     * otherwise:  read up to eof.
     *
     * Implementation is O(n) for return value of length n
     *
     * Allocates pages for input in units of block_z
     */
    std::string read_until(bool check_delim_flag,
                           char_type delim,
                           uint32_t block_z = 4095)
        {
            /* list of complete blocks read;  each string in this list has block_z chars */
            std::list<std::string> block_l;

            /* index# of next block to read */
            uint32_t i_block = 0;

            /* last block read.   0..block_z chars */
            std::string last_block;

            for (;; ++i_block) {
                last_block = std::string();
                last_block.resize(block_z + 1);

                std::streamsize n = this->read_until(last_block.data(), block_z, check_delim_flag, delim);

                if ((n < block_z)
                    || ((n == block_z) && check_delim_flag && (last_block[block_z - 1] == delim))) {

                    last_block.resize(n);
                    break;
                }

                block_l.push_back(last_block);
            }

            /* final result is concatenation of:
             *   - strings in block_l
             *   - followed by last_block
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
                      retval.data() + block_l.size() * block_z);

            return retval;
        }

    /* move-assignment */
    basic_zstream & operator=(basic_zstream && x) {
        /* assign any base-class state */
        std::basic_iostream<CharT, Traits>::operator=(x);

        this->rdbuf_ = std::move(x.rdbuf_);
        return *this;
    }

    /* exchange state with x */
    void swap(basic_zstream & x) {
        /* swap any base-class state */
        std::basic_iostream<CharT, Traits>::swap(x);
        /* swap streambuf state */
        this->rdbuf_.swap(x.rdbuf_);
    }

    /* finishes writing compressed output */
    void final_sync() {
        this->rdbuf_.final_sync();
    }

    /* closes stream;  incorporates .final_sync() */
    void close() {
        this->rdbuf_.close();

        /* clear state bits:  in particular need to clear any of {eofbit, failbit, badbit} */
        this->clear();
    }

#  ifndef NDEBUG
    void set_debug_flag(bool x) { rdbuf_.set_debug_flag(x); }
#  endif

private:
    basic_zstreambuf<CharT, Traits> rdbuf_;
}; /*basic_zstream*/
#pragma GCC diagnostic pop

using zstream = basic_zstream<char>;

namespace std {
    template <typename CharT, typename Traits>
    void swap(basic_zstream<CharT, Traits> & lhs,
              basic_zstream<CharT, Traits> & rhs)
    {
        lhs.swap(rhs);
    }
}
