/** @file compression.hpp **/

#pragma once

#include <vector>
#include <string>
#include <cstdint>

/** @class compression compression/compression.hpp

    @brief Using class-as-namespace idiom to scope some global, non-streaming functions.

    More memory-efficient streaming versions in neighboring files
    @see inflate_zstream
    @see buffered_inflate_zstream
    @see deflate_zstream
    @see buffered_deflate_zstream

    Thanks to:
    - <a href="https://bobobobo.wordpress.com/2008/02/23/how-to-use-zlib">Bobobobo's blog for zlib introduction</a>
    - <a href="https://zlib.net/zlib_how.html">zlib.net howto</a>
 **/
class compression {
public:
    /** @brief uncompress contents of z_data_v[],  return uncompressed data.
     *  caller expected to remember original uncompressed size + supply in og_data_z,
     *  (or supply a sufficiently-large value)
     **/
    static std::vector<std::uint8_t> inflate(std::vector<std::uint8_t> const & z_data_v,
                                             std::uint64_t og_data_z);

    /** @brief compress contents of og_data_v[],  return compressed data **/
    static std::vector<std::uint8_t> deflate(std::vector<std::uint8_t> const & og_data_v);

    /** @brief compress file with path .in_file,  putting output in .out_file **/
    static void inflate_file(std::string const & in_file,
                             std::string const & out_file,
                             bool keep_flag = true,
                             bool verbose_flag = false);

    /** @brief uncompress file with path .in_file,  putting uncompressed output in .out_file **/
    static void deflate_file(std::string const & in_file,
                             std::string const & out_file,
                             bool keep_flag = true,
                             bool verbose_flag = false);
};
