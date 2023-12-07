// compression.cpp

#include "compression.hpp"
#include <zlib.h>
#include <stdexcept>

using namespace std;

vector<uint8_t>
compression::deflate(std::vector<uint8_t> const & og_data_v)
{
    /* required input space for zlib is (1.01 * input size) + 12;
     * add +1 byte to avoid thinking about rounding
     */
    uint64_t z_data_z = (1.01 * og_data_v.size()) + 12 + 1;

    vector<uint8_t> z_data_v(z_data_z);

    int32_t zresult = ::compress(z_data_v.data(),
                                 &z_data_z,
                                 og_data_v.data(),
                                 og_data_v.size());

    switch (zresult) {
    case Z_OK:
        break;
    case Z_MEM_ERROR:
        throw std::runtime_error("compression::deflate: out of memory");
    case Z_BUF_ERROR:
        throw std::runtime_error("compression::deflate: output buffer too small");
    }

    return z_data_v;
}

vector<uint8_t>
compression::inflate(vector<uint8_t> const & z_data_v,
                     uint64_t og_data_z)
{
    vector<uint8_t> og_data_v(og_data_z);

    int32_t zresult = ::uncompress(og_data_v.data(),
                                   &og_data_z,
                                   z_data_v.data(),
                                   z_data_v.size());

    switch (zresult) {
    case Z_OK:
        break;
    case Z_MEM_ERROR:
        throw std::runtime_error("compression::inflate: out of memory");
    case Z_BUF_ERROR:
        throw std::runtime_error("compression::inflate: output buffer too small");
    }

    og_data_v.resize(og_data_z);

    return og_data_v;
} /*inflate*/
