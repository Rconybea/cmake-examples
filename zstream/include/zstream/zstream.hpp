// zstream.hpp

#pragma once

#include "zstreambuf.hpp"
#include <iostream>
#include <fstream>

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

public:
    basic_zstream(std::streamsize buf_z, std::unique_ptr<std::streambuf> native_sbuf)
        :
          rdbuf_(buf_z, std::move(native_sbuf)),
          std::basic_iostream<CharT, Traits>(&rdbuf_)
           {}
    /* convenience ctor;  creates filebuf attached to filename and opens it */
    basic_zstream(std::streamsize buf_z,
                  char const * filename,
                  std::ios::openmode mode = std::ios::in)
        : rdbuf_(buf_z,
                 std::unique_ptr<std::streambuf>((new std::filebuf())->open(filename,
                                                                            std::ios::binary | mode))),
          std::basic_iostream<CharT, Traits>(&rdbuf_)
        {}
    ~basic_zstream() = default;

    zstreambuf_type * rdbuf() { return &rdbuf_; }

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
    void close() {
        this->rdbuf_.close();
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
