// myzip.cpp

#include "compression/compression.hpp"

#include <boost/program_options.hpp>
#include <zlib.h>
#include <iostream>
#include <fstream>

namespace po = boost::program_options;
using namespace std;

int
main(int argc, char * argv[]) {
    po::options_description po_descr{"Options"};
    po_descr.add_options()
        ("help,h",
         "this help")
        ("keep,k",
         "keep input files instead of deleting them")
        ("verbose,v",
         "enable to report progress messages to stderr")
        ("input-file",
         po::value<vector<string>>(),
         "input file(s) to compress/uncompress")
        ;

    po::variables_map vm;

    po::positional_options_description po_pos_args;
    po_pos_args.add("input-file", -1);
    po::store(po::command_line_parser(argc, argv)
              .options(po_descr)
              .positional(po_pos_args)
              .run(),
              vm);
    po::notify(vm);

    bool keep_flag = vm.count("keep");
    bool verbose_flag = vm.count("verbose");

    try {
        if (vm.count("help")) {
            cerr << po_descr << endl;
        } else {
            vector<string> input_file_l = vm["input-file"].as<vector<string>>();

            for (string const & fname : input_file_l) {
                if (verbose_flag)
                    cerr << "myzip: consider file [" << fname << "]" << endl;

                constexpr int32_t sfx_z = 3;

                if ((fname.size() > sfx_z) && (fname.substr(fname.size() - sfx_z, sfx_z) == ".mz")) {
                    /* uncompress */

                    string fname_mz = fname;
                    string fname = fname_mz.substr(0, fname_mz.size() - sfx_z);

                    compression::inflate_file(fname_mz, fname, keep_flag, verbose_flag);
                } else {
                    /* compress */
                    string fname_mz = fname + ".mz";

                    compression::deflate_file(fname, fname_mz, keep_flag, verbose_flag);
                }
            }
        }
    } catch(exception & ex) {
        cerr << "error: myzip: " << ex.what() << endl;
        return 1;
    }

    return 0;
}
