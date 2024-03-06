/** @file xfilebuf.hpp **/

#pragma once

#include <fstream>

/** @class basic_xfilebuf zstream/xfilebuf.hpp

   @brief Minimal extension of gcc's @c std::basic_filebuf,  for the sole purpose of exposing file descriptor.

   We want to support the @c iostream::native_handle() method,  in advance of c++26.

   @note
   1. For file i/o,  we want to know the file descriptor associated with a stream:
      - To permit using @c epoll(), @c io_uring_enter() or similar mechanism to schedule i/o activity on linux
      - To implement python @c IOBase.fileno() when using the stack:
        @code
         zstream.py / pyzstream / ::zstream / ::zstreambuf
         (python)     (pybind11)     (c++)        (c++)
        @endcode
   2. The GNU 12.3.0 stdc++ library has (under @c include/c++/12.3.0):
      @code
      [x86_64-unknown-linux-gnu/bits/c++io.h]        typedef FILE __c_file
      [x86_64-unknown-linux-gnu/bits/basic_file.h]   __c_file * __basic_file<char>::_M_cfile (private)
                                                     int __basic_file<char>::fd()            (public)
      [include/fstream]                              __basic_file<char> basic_filebuf<..>::_M_file (protected)
      @endcode
   3. We can't get this from a @c std::filebuf via public API (at least, prior to c++26 @c filebuf::native_handle()):
   4. There's also no public API path to attach a separately-created file descriptor with a filebuf.
   5. This leaves us the narrowest path to squeeze through,  followed here:
      - Write class inheriting @c std::filebuf
      - In this class,  provide wrapper for @c std::filebuf::open()
      - Wrapper can access file descriptor (after successful .open()) using @c self->_M_file.fd()
      - Record file descriptor separately (after that can use only the unextended @c std::filebuf api
**/
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_xfilebuf : public std::basic_filebuf<CharT, Traits>
{
public:
    /** @brief typealias for parent class **/
    using basic_filebuf_type = std::basic_filebuf<CharT, Traits>;
    /** @brief typealias for self **/
    using basic_xfilebuf_type = basic_xfilebuf<CharT, Traits>;

    /** @brief typealias for native handle type.

        @warning Only known to be correct on linux.

        (might as well make a reasonable effort to follow c++26 conventions here)
    **/
    using native_handle_type = int;

public:
    /** @brief Default constructor **/
    basic_xfilebuf() = default;
    /** @brief basic_xfilebuf is not copyable (since @c basic_filebuf isn't) **/
    basic_xfilebuf(basic_xfilebuf const & x) = delete;
    /** @brief basic_xfilebuf is not movable (since @c basic_filebuf isn't) **/
    basic_xfilebuf(basic_xfilebuf &&) = delete;

    /** @brief provide (readonly) access to file descriptor associated with this stream

        @warning Implementation tested only on linux.

        Asof c++26 can use @c std::basic_filebuf::native_handle()
        At that point basic_xfilebuf can be abandoned
     **/
    native_handle_type fd() const {
        /* __basic_file_type::fd() isn't declared const in gcc 12.2.0 */
        return const_cast<typename std::basic_filebuf<CharT, Traits>::__file_type &>(this->_M_file).fd();
    }
    /** @brief report O/S native handle associated with this streambuf. **/
    native_handle_type native_handle() const { return this->fd(); }

    /** @brief parallel @c std::basic_filebuf method with similar signature, except we use most-derived type for return pointer.

        @return @c this on successful open;  @c nullptr otherwise
     **/
    basic_xfilebuf_type * open(char const * s, std::ios_base::openmode mode) {
        if (basic_filebuf_type::open(s, mode))
            return this;
        else
            return nullptr;
    }

#if _GLIBCXX_HAVE__WFOPEN && _GLIBCXX_USE_WCHAR_T
    /** @brief parallel @c std::basic_filebuf method with similar signature, except we use most-derived type for return pointer.

        @return @c this on successful open;  @c nullptr otherwise
     **/
    basic_xfilebuf_type * open(wchar_t const * s, std::ios_base::openmode mode) {
        if (basic_filebuf_type::open(s, mode))
            return this;
        else
            return nullptr;
    }
#endif

    /** @brief parallel @c std::basic_filebuf method with similar signature, except we use most-derived type for return pointer.

        @return @c this on successful open;  @c nullptr otherwise
     **/
    basic_xfilebuf_type * open(std::string const & s, std::ios_base::openmode mode) {
        if (basic_filebuf_type::open(s, mode))
            return this;
        else
            return nullptr;
    }

    /** @brief parallel @c std::basic_filebuf method with similar signature, except we use most-derived type for return pointer.

        @return @c this on successful open;  @c nullptr otherwise
     **/
    template<typename Path>
    std::_If_fs_path<Path, basic_xfilebuf_type*>
    open(Path const & s, std::ios_base::openmode mode) {
        if (basic_xfilebuf_type::open(s, mode))
            return this;
        else
            return nullptr;
    }

    /** @brief swap state with another basic_xfilebuf instance **/
    void swap(basic_xfilebuf & x) {
        std::basic_filebuf<CharT, Traits>::swap(x);
    }

    /** @brief basic_xfilebuf is not copyable (since @c std::filebuf isn't) **/
    basic_xfilebuf & operator=(basic_xfilebuf const & x) = delete;
    /** @brief basic_xfilebuf is not movable **/
    basic_xfilebuf & operator=(basic_xfilebuf &&) = delete;
};

/** @brief convenience typealias; zstreambuf implementation uses this for compressed data **/
using xfilebuf = basic_xfilebuf<char>;

/** @brief Overload @c swap(), so that @ref basic_xfilebuf is swappable **/
template<typename CharT, typename Traits>
void swap(basic_xfilebuf<CharT, Traits> & lhs,
          basic_xfilebuf<CharT, Traits> & rhs)
{
    lhs.swap(rhs);
}
