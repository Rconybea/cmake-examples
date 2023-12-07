// hello.cpp

#include "compression.hpp"

#include <boost/program_options.hpp>
#include <zlib.h>
#include <iostream>

namespace po = boost::program_options;
using namespace std;

int
main(int argc, char * argv[]) {
    po::options_description po_descr{"Options"};
    po_descr.add_options()
        ("help,h",
         "this help")
        ("subject,s",
         po::value<string>()->default_value("world"),
         "say hello to this subject")
        ("compress,z",
         "compress hello output using zlib")
        ("hex",
         "convert compressed output to hex for display")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, po_descr), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cerr << po_descr << endl;
    } else {
        stringstream ss;
        ss << "Hello, " << vm["subject"].as<string>() << "!\n" << endl;

        if (vm.count("compress")) {
            /* compress output */

            string s = ss.str();
            std::vector<uint8_t> og_data_v(s.begin(), s.end());

            std::vector<uint8_t> z_data_v = compression::deflate(og_data_v);

            if (vm.count("hex")) {
                cout << "original   size:" << og_data_v.size() << endl;
                cout << "compressed size:" << z_data_v.size() << endl;
                cout << "compressed data:";
                for (uint64_t i = 0; i < z_data_v.size(); ++i) {
                    uint8_t zi = z_data_v[i];
                    uint8_t hi = (zi >> 4);
                    uint8_t lo = (zi & 0x0f);

                    char hi_ch = (hi < 10) ? '0' + hi : 'a' + hi - 10;
                    char lo_ch = (lo < 10) ? '0' + lo : 'a' + lo - 10;

                    cout << ' ' << hi_ch << lo_ch;
                }
                cout << endl;
            } else {
                cout.write(reinterpret_cast<char *>(z_data_v.data()),
                           z_data_v.size());
            }
        } else {
            cout << ss.str();
        }
    }
}
