// hex.hpp

#pragma once

#include <iostream>

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
            os << static_cast<char>(ins.x_);
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
