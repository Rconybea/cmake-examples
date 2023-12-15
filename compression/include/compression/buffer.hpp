// buffer.hpp

#pragma once

#include "span.hpp"
#include <utility>
#include <cstdint>
#include <cassert>
#include <new>

/*
 *  .buf
 *
 *    +------------------------------------------+
 *    |  |  ...  |  | X|  ... | X|  |    ...  |  |
 *    +------------------------------------------+
 *     ^             ^            ^               ^
 *     0             .lo          .hi             .buf_z
 *
 *
 * buffer does not support wrapped content
 */
template <typename CharT>
class buffer {
public:
    using span_type = span<CharT>;
    using size_type = std::uint64_t;

public:
    buffer(size_type buf_z, size_type align_z = sizeof(char))
        : is_owner_{true},
          lo_pos_{0}, hi_pos_{0},
          buf_{new (std::align_val_t(align_z)) CharT [buf_z]},
          buf_z_{buf_z} {}
    ~buffer() { this->clear(); }

    CharT * buf() const { return buf_; }
    size_type buf_z() const { return buf_z_; }
    size_type lo_pos() const { return lo_pos_; }
    size_type hi_pos() const { return hi_pos_; }

    CharT const & operator[](size_type i) const { return buf_[i]; }

    span_type contents() const { return span_type(buf_ + lo_pos_, buf_ + hi_pos_); }
    span_type avail() const { return span_type(buf_ + hi_pos_, buf_ + buf_z_); }

    bool empty() const { return lo_pos_ == hi_pos_; }

    void produce(span_type const & span) {
        assert(span.lo() == buf_ + hi_pos_);

        hi_pos_ += span.size();
    }

    void consume(span_type const & span) {
        if (span.size()) {
            assert(span.lo() == buf_ + lo_pos_);

            lo_pos_ += span.size();
        } else {
            /* since .consume() that arrives at empty contents also resets .lo_pos .hi_pos,
             * we don't want to blow up when called with an empty span -- argument
             * may represent some pre-reset location in buffer
             */
        }

        if (lo_pos_ == hi_pos_) {
            lo_pos_ = 0;
            hi_pos_ = 0;
        }
    }

    void setbuf(CharT * buf, size_type buf_z) {
        /* properly reset any existing state */
        this->clear();

        is_owner_ = false;
        lo_pos_ = 0;
        hi_pos_ = 0;
        buf_ = buf;
        buf_z_ = buf_z;
    }

    void swap (buffer & x) {
        std::swap(is_owner_, x.is_owner_);
        std::swap(buf_, x.buf_);
        std::swap(buf_z_, x.buf_z_);
        std::swap(lo_pos_, x.lo_pos_);
        std::swap(hi_pos_, x.hi_pos_);
    }

    void clear() {
        if (is_owner_)
            delete [] buf_;

        is_owner_ = false;
        buf_ = nullptr;
        buf_z_ = 0;
        lo_pos_ = 0;
        hi_pos_ = 0;
    }

    /* move-assignment */
    buffer & operator= (buffer && x) {
        is_owner_ = x.is_owner_;
        buf_ = x.buf_;
        buf_z_ = x.buf_z_;
        lo_pos_ = x.lo_pos_;
        hi_pos_ = x.hi_pos_;

        x.is_owner_ = false;
        x.lo_pos_ = 0;
        x.hi_pos_ = 0;
        x.buf_ = nullptr;
        x.buf_z_ = 0;

        return *this;
    }

private:
    bool is_owner_ = false;
    CharT * buf_ = nullptr;
    size_type buf_z_ = 0;

    /* buffer locations [.lo_pos .. .hi_pos) are occupied;
     * remainder is available space
     */
    size_type lo_pos_ = 0;
    size_type hi_pos_ = 0;
};

namespace std {
    template <typename CharT>
    inline void
    swap(buffer<CharT> & lhs, buffer<CharT> & rhs) {
        lhs.swap(rhs);
    }
}
