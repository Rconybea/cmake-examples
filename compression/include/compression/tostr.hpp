/** @file tostr.hpp **/

#pragma once

#include <sstream>

/** @brief write @p x on @p s
 *  @param s stream on which to print
 *  @param x value to print (as per @c operator<<)
 **/
template <class Stream, typename T>
Stream &
tos(Stream & s, T && x) {
    s << x;
    return s;
}

/** @brief write @p x, then contents of @p rest on @p s
 *  @param s stream on which to print
 *  @param x value to print first (as per @c operator<<)
 *  @param rest remaining values to print (in order from left to right)
 **/
template <class Stream, typename T1, typename... Tn>
Stream &
tos(Stream & s, T1 && x, Tn && ...rest) {
    s << x;
    return tos(s, std::forward<Tn>(rest)...);
}

/**
 * @brief construct string from in-order printed value of arguments
 *
 * @code
 *   tostr(x1, x2, ...)
 * @endcode
 *
 * is shorthand for:
 *
 * @code
 *   {
 *     stringstream s;
 *     s << x1 << x2 << ...;
 *     return s.str();
 *   }
 * @endcode
 *
 * @param args (parameter pack) arguments to be printed
 * @return string constructed by writing each argument to a stringstrema.
 **/
template <typename... Tn>
std::string
tostr(Tn && ...args) {
    std::stringstream ss;
    tos(ss, std::forward<Tn>(args)...);
    return ss.str();
}
