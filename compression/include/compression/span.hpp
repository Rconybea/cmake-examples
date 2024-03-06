/** @file span.hpp **/

#pragma once

#include <cstdint>

/** @class span compression/span.hpp
 *
 *  @brief Represents a contiguous memory range,  without ownership.
 *
 *  @tparam CharT type for elements referred to by this span.
 **/
template <typename CharT>
class span {
public:
    /** @brief typealias for span size (in units of CharT) **/
    using size_type = std::uint64_t;

public:
    /** @brief create span for the contiguous memory range [@p lo, @p hi) **/
    span(CharT * lo, CharT * hi) : lo_{lo}, hi_{hi} {}

    ///@{

    /** @name getters **/

    CharT * lo() const { return lo_; } /*!< get member span::lo_ */
    CharT * hi() const { return hi_; } /*!< get member span::hi_ */

    ///@}

    /** @brief create new span over supplied type, with identical (possibly misaligned) endpoints.
     *
     *  @warning
     *  1. New span uses exactly the same memory addresses.
     *     Endpoint pointers may not be aligned.
     *  2. Implementation assumes code compiled with @code -fno-strict-aliasing @endcode enabled.
     *
     *  @tparam OtherT element type for new span
     **/
    template <typename OtherT>
    span<OtherT>
    cast() const { return span<OtherT>(reinterpret_cast<OtherT *>(lo_),
                                       reinterpret_cast<OtherT *>(hi_)); }

    /** @brief create span including the first @p z members of this span. **/
    span prefix(size_type z) const { return span(lo_, lo_ + z); }

    /** @brief true iff this span is empty (comprises 0 elements). **/
    bool empty() const { return lo_ == hi_; }
    /** @brief report the number of elements (of type CharT) in this span. **/
    size_type size() const { return hi_ - lo_; }

private:
    ///@{

    /** @brief start of span
        Span comprises memory address between @p lo (inclusive) and @p hi (exclusive)
     **/
    CharT * lo_ = nullptr;
    /** @brief end of span
        Span comprises memory address between @p lo (inclusive) and @p hi (exclusive)
     **/
    CharT * hi_ = nullptr;

    ///@}
};
