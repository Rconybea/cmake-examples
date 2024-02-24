// compression.cpp

#include "compression/compression.hpp"
#include "compression/buffered_inflate_zstream.hpp"
#include "compression/buffered_deflate_zstream.hpp"
#include "compression/tostr.hpp"
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
compression::deflate_file(std::string const & in_file,
                          std::string const & out_file,
                          bool keep_flag,
                          bool verbose_flag)
{
    /* check output doesn't exist already */
    if (ifstream(out_file, ios::binary|ios::in))
        throw std::runtime_error(tostr("output file [", out_file, "] already exists"));

    if (verbose_flag || true)
        cerr << "compress::deflate_file: will compress [" << in_file << "]"
             << " -> [" << out_file << "]" << endl;

    /* open target file -- binary mode since need not be text */
    ifstream fs(in_file, ios::in|ios::binary);
    if (!fs)
        throw std::runtime_error(tostr("unable to open input file [", in_file, "]"));

    buffered_deflate_zstream zstate;

    /* write compressed output */
    ofstream zfs(out_file, ios::out|ios::binary);

    for (bool progress = true, final_flag = false; progress;) {
        streamsize nread = 0;

        if (fs.eof()) {
            final_flag = true;
        } else {
            span<uint8_t> ucspan = zstate.uc_avail();

            fs.read(reinterpret_cast<char *>(ucspan.lo()), ucspan.size());
            nread = fs.gcount();
            zstate.uc_produce(ucspan.prefix(nread));
        }

        zstate.deflate_chunk(final_flag);

        /* write compressed output */
        span<uint8_t> zspan = zstate.z_contents();

        zfs.write(reinterpret_cast<char *>(zspan.lo()), zspan.size());
        if (!zfs.good())
            throw std::runtime_error(tostr("failed to write ", zspan.size(), " bytes"
                                           , " to [", out_file, "]"));

        zstate.z_consume(zspan);

        progress = (nread > 0) || (zspan.size() > 0);
    }

    fs.close();
    zfs.close();

    /* control here only if successfully wrote uncompressed output */
    if (!keep_flag)
        remove(in_file.c_str());
} /*deflate_file*/

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
        cerr << "compression::inflate_file will uncompress [" << in_file << "] -> [" << out_file << "]" << endl;

    /* open target file */
    ifstream fs(in_file, ios::binary);
    if (!fs)
        throw std::runtime_error("unable to open input file");

    buffered_inflate_zstream zstate(buffered_inflate_zstream::c_default_buf_z);

    /* write uncompressed output */
    ofstream ucfs(out_file, ios::out|ios::binary);

    while (!fs.eof()) {
        span<uint8_t> zspan = zstate.z_avail();

        fs.read(reinterpret_cast<char *>(zspan.lo()), zspan.size());
        std::streamsize n_read = fs.gcount();

        if (n_read == 0)
            throw std::runtime_error(tostr("inflate_file: unable to read contents of input file [", in_file, "]"));

        zstate.z_produce(zspan.prefix(n_read));

        /* uncompress some text */
        zstate.inflate_chunk();

        span<uint8_t> ucspan = zstate.uc_contents();

        ucfs.write(reinterpret_cast<char *>(ucspan.lo()), ucspan.size());

        zstate.uc_consume(ucspan);
    }

    if (!ucfs.good())
        throw std::runtime_error(tostr("inflate_file: failed to write ", zstate.n_out_total(), " bytes to [", out_file, "]"));

    fs.close();
    ucfs.close();

    if (!keep_flag)
        remove(in_file.c_str());
} /*inflate_file*/
