// compression.cpp

#include "compression.hpp"
#include "tostr.hpp"
#include <zlib.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <stdexcept>

using namespace std;

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
        throw std::runtime_error(tostr("compression::inflate: output buffer (size ", og_data_z, ") too small"));
    }

    og_data_v.resize(og_data_z);

    return og_data_v;
} /*inflate*/

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
        throw std::runtime_error(tostr("compression::deflate: output buffer (size ", z_data_z, ") too small"));
    }

    return z_data_v;
} /*deflate*/

void
compression::inflate_file(std::string const & in_file,
                          std::string const & out_file,
                          bool keep_flag,
                          bool verbose_flag)
{
    /* check output doesn't exist already */
    if (ifstream(out_file, ios::binary|ios::in))
        throw std::runtime_error(tostr("output file [", out_file, "] already exists"));

    if (verbose_flag)
        cerr << "compress::inflate_file: will compress [" << in_file << "] -> [" << out_file << "]" << endl;

    /* open target file (start at end) */
    ifstream fs(in_file, ios::binary|ios::ate);
    if (!fs)
        throw std::runtime_error(tostr("unable to open input file [", in_file, "]"));

    auto z = fs.tellg();

    /* read file content into memory */
    if (verbose_flag)
        cerr << "compress::inflate_file: read " << z << " bytes from [" << in_file << "] into memory" << endl;

    vector<uint8_t> fs_data_v(z);
    fs.seekg(0);
    if (!fs.read(reinterpret_cast<char *>(&fs_data_v[0]), z))
        throw std::runtime_error(tostr("unable to read contents of input file [", in_file, "]"));

    vector<uint8_t> z_data_v = compression::deflate(fs_data_v);

    /* write compresseed output */
    ofstream zfs(out_file, ios::out|ios::binary);
    zfs.write(reinterpret_cast<char *>(&(z_data_v[0])), z_data_v.size());

    if (!zfs.good())
        throw std::runtime_error(tostr("failed to write ", z_data_v.size(), " bytes to [", out_file, "]"));

    /* control here only if successfully wrote uncompressed output */
    if (!keep_flag)
        remove(in_file.c_str());
} /*inflate_file*/

void
compression::deflate_file(std::string const & in_file,
                          std::string const & out_file,
                          bool keep_flag,
                          bool verbose_flag)
{
    /* check output doesn't exist already */
    if (ifstream(out_file, ios::binary|ios::in))
        throw std::runtime_error(tostr("output file [", out_file, "] already exists"));

    if (verbose_flag)
        cerr << "compression::deflate_file will uncompress [" << in_file << "] -> [" << out_file << "]" << endl;

    /* open target file (start at end) */
    ifstream fs(in_file, ios::binary|ios::ate);
    if (!fs)
        throw std::runtime_error("unable to open input file");

    auto z = fs.tellg();

    /* read file contents into memory */
    if (verbose_flag)
        cerr << "compression::deflate_file: read " << z << " bytes from [" << in_file << "] into memory" << endl;

    vector<uint8_t> fs_data_v(z);
    fs.seekg(0);
    if (!fs.read(reinterpret_cast<char *>(&fs_data_v[0]), z))
        throw std::runtime_error(tostr("unable to read contents of input file [", in_file, "]"));

    /* uncompress */
    vector<uint8_t> og_data_v = compression::inflate(fs_data_v, 999999);

    /* write uncompressed output */
    ofstream ogfs(out_file, ios::out|ios::binary);
    ogfs.write(reinterpret_cast<char *>(&(og_data_v[0])), og_data_v.size());

    if (!ogfs.good())
        throw std::runtime_error(tostr("failed to write ", og_data_v.size(), " bytes to [", out_file, "]"));

    if (!keep_flag)
        remove(in_file.c_str());
} /*deflate_file*/
