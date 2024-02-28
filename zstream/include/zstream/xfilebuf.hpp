// xfilebuf.hpp

#pragma once

#include <fstream>

/* Simple extension of std::basic_filebuf<>,  to expose file descriptor
 *
 * Lament:
 * 1. for file i/o,  we want to know the file descriptor associated with a stream:
 *    a. so that it's possible to use epoll or similar mechanism to schedule i/o activity on linux
 *    b. to implement python IOBase.fileno() when using stack
 *       zstream.py / pyzstream / ::zstream / ::zstreambuf
 *       (python)     (pybind11)  (c++)       (c++)
 * 2. GNU 12.2.0 stdc++ library has (under include/c++/12.2.0):
 *     [x86_64-unknown-linux-gnu/bits/c++io.h]        typedef FILE __c_file
 *     [x86_64-unknown-linux-gnu/bits/basic_file.h]   __c_file * __basic_file<char>::_M_cfile (private)
 *                                                    int __basic_file<char>::fd()            (public)
 *     [include/fstream]                              __basic_file<char> basic_filebuf<..>::_M_file (protected)
 * 3. we can't get this from a filebuf via public API (at least, prior to c++26 filebuf::native_handle()):
 * 4. there's also no public API path to associate a separately-created file descriptor with a filebuf.
 * 5. this leaves us a narrow path to squeeze through:
 *    a. write class inheriting std::filebuf
 *    b. in this class,  provide wrapper for filebuf::open()
 *       wrapper can access file descriptor (after successful .open()) using self->_M_file.fd()
 *    c. record file descriptor separately;  after that can revert to vanilla filebuf api
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_xfilebuf : public std::basic_filebuf<CharT, Traits>
{
public:
    using basic_filebuf_type = std::basic_filebuf<CharT, Traits>;
    using basic_xfilebuf_type = basic_xfilebuf<CharT, Traits>;

    /* might as well make a reasonable effort to follow c++26 conventions here */
    using native_handle_type = int;


public:
    basic_xfilebuf() = default;
    basic_xfilebuf(basic_xfilebuf const & x) = delete;
    basic_xfilebuf(basic_xfilebuf &&);

    /* the point of writing this class:  to provide public API access here */
    native_handle_type fd() const {
        /* __basic_file_type::fd() isn't declared const in gcc 12.2.0 */
        return const_cast<typename std::basic_filebuf<CharT, Traits>::__file_type &>(this->_M_file).fd();
    }
    native_handle_type native_handle() const { return this->fd(); }

    /* .open() methods parallel basic_filebuf<> versions,
     * but return basic_xfilebuf* instead of basic_filebuf*
     */

    basic_xfilebuf_type * open(char const * s, std::ios_base::openmode mode) {
        if (basic_filebuf_type::open(s, mode))
            return this;
        else
            return nullptr;
    }

#if _GLIBCXX_HAVE__WFOPEN && _GLIBCXX_USE_WCHAR_T
    basic_xfilebuf_type * open(wchar_t const * s, std::ios_base::openmode mode) {
        if (basic_filebuf_type::open(s, mode))
            return this;
        else
            return nullptr;
    }
#endif

    basic_xfilebuf_type * open(std::string const & s, std::ios_base::openmode mode) {
        if (basic_filebuf_type::open(s, mode))
            return this;
        else
            return nullptr;
    }

    template<typename Path>
    std::_If_fs_path<Path, basic_xfilebuf_type*>
    open(Path const & s, std::ios_base::openmode mode) {
        if (basic_xfilebuf_type::open(s, mode))
            return this;
        else
            return nullptr;
    }

    void swap(basic_xfilebuf & x) {
        std::basic_filebuf<CharT, Traits>::swap(x);
    }

    basic_xfilebuf & operator=(basic_xfilebuf const & x) = delete;
    basic_xfilebuf & operator=(basic_xfilebuf &&);
};

using xfilebuf = basic_xfilebuf<char>;

namespace std {
    template<typename CharT, typename Traits>
    void swap(basic_xfilebuf<CharT, Traits> & lhs,
              basic_xfilebuf<CharT, Traits> & rhs)
    {
        lhs.swap(rhs);
    }
}
