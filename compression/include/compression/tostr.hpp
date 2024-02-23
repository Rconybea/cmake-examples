// tostr.hpp

#pragma once

#include <sstream>

/*   tostr(x1, x2, ...)
 *
 * is shorthand for something like:
 *
 *   {
 *     stringstream s;
 *     s << x1 << x2 << ...;
 *     return s.str();
 *   }
 */


//template<class Stream>
//Stream & tos(Stream & s) { return s; }

template <class Stream, typename T>
Stream & tos(Stream & s, T && x) { s << x; return s; }

template <class Stream, typename T1, typename... Tn>
Stream & tos(Stream & s, T1 && x, Tn && ...rest) {
    s << x;
    return tos(s, rest...);
}

template <typename... Tn>
std::string tostr(Tn && ...args) {
    std::stringstream ss;
    tos(ss, args...);
    return ss.str();
}
