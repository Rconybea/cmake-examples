// compression.hpp

#pragma once

#include <vector>
#include <cstdint>

/* thanks to Bobobobo's blog for zlib introduction
 *   [[https://bobobobo.wordpress.comp/2008/02/23/how-to-use-zlib]]
 * also
 *   [[https://zlib.net/zlib_how.html]]
 */
struct compression {
    /* compress contents of og_data_v[],  return compressed data */
    static std::vector<std::uint8_t> deflate(std::vector<std::uint8_t> const & og_data_v);
    /* uncompress contents of z_data_v[],  return uncompressed data.
     * caller expected to remember original uncompressed size + supply in og_data_z,
     * (or supply a sufficiently-large value)
     */
    static std::vector<std::uint8_t> inflate(std::vector<std::uint8_t> const & z_data_v,
                                             std::uint64_t og_data_z);
};
