// span.hpp

#pragma once

#include <cstdint>

/* A span of un-owned memory */
template <typename CharT>
class span {
public:
    using size_type = std::uint64_t;

public:
    span(CharT * lo, CharT * hi) : lo_{lo}, hi_{hi} {}

    /* cast with different element type.  Note this may change .size */
    template <typename OtherT>
    span<OtherT>
    cast() const { return span<OtherT>(reinterpret_cast<OtherT *>(lo_),
                                       reinterpret_cast<OtherT *>(hi_)); }

    span prefix(size_type z) const { return span(lo_, lo_ + z); }

    bool empty() const { return lo_ == hi_; }
    size_type size() const { return hi_ - lo_; }

    CharT * lo() const { return lo_; }
    CharT * hi() const { return hi_; }

private:
    CharT * lo_ = nullptr;
    CharT * hi_ = nullptr;
};
