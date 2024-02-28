#include "zstream/zstream.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace std;

PYBIND11_MODULE(pyzstream, m) {
    // see pybind11/pytypes.h for c++ class wrappers for specific builtin python types
    // see https://docs.python.org/3/library/operator.html#mapping-operators-to-functions

    m.doc() = "pybind11 plugin for zstream";

    /* wrap ios::openmode */
    py::class_<std::ios::openmode>(m, "openmode")
        /* note: 'in' is a keyword in python, can't use here */
        .def_property_readonly_static("none",
                                      [](py::object /*self*/) { return std::ios::openmode(0); },
                                      py::doc("openmode with no bits set"))
        .def_property_readonly_static("all",
                                      [](py::object /*self*/) { return std::ios::in | std::ios::out | std::ios::binary; },
                                      py::doc("openmode with all bits set"))
        .def_property_readonly_static("input",
                                      [](py::object /*self*/) { return std::ios::in; },
                                      py::doc("set this bit to enable stream for input i/o"))
        .def_property_readonly_static("output",
                                      [](py::object /*self*/) { return std::ios::out; },
                                      py::doc("set this bit to enable stream for output i/o"))
        .def_property_readonly_static("binary",
                                      [](py::object /*self*/) { return std::ios::binary; },
                                      py::doc("set this bit to operate stream in binary mode"
                                              " (disables automatic character processing)"))
        /* note: for a zstream,  being open for both read/write is not useful without seek() */
        .def_static("from_string",
                    [](string const & s) {
                        std::ios::openmode retval = static_cast<std::ios::openmode>(0);

                        for (char ch : s) {
                            /* other openmode flags:
                             *   app (append), trunc (truncate), ate (at end), noreplace (exclusive)
                             */

                            if (ch == 'r') {
                                retval |=  std::ios::in;
                            } else if (ch == 'w') {
                                retval |= std::ios::out;
                            } else if (ch == '+') {
                                retval |= (std::ios::in | std::ios::out);
                            } else if (ch == 'b') {
                                retval |= std::ios::binary;
                            } else if (ch == 't') {
                                ; /* nothing to do,  text assumed */
                            }
                        }

                        return retval;
                    },
                    py::arg("s"),
                    py::doc("Convert string s to openmode. s may contain characters from {r,w,b}"
                            "; r=input (read), w=output (write), b=binary"))
        .def("__eq__",
             [](std::ios::openmode x, std::ios::openmode y) { return x==y; },
             py::arg("other"),
             py::doc("True if bitmask self equals bitmask other; otherwise False"))
        .def("__ne__", [](std::ios::openmode x, std::ios::openmode y) { return x!=y; },
             py::arg("other"),
             py::doc("True if bitmask self not equal to bitmask other; otherwise False"))
        .def("__or__",
             [](std::ios::openmode x, std::ios::openmode y) { return x|y; },
             py::arg("other"),
             py::doc("bitmask with bit set whenever that bit is set in either self or other"))
        .def("__and__",
             [](std::ios::openmode x, std::ios::openmode y) { return x&y; },
             py::arg("other"),
             py::doc("bitmask with bit set whenever that bit is set in both self and other"))
        .def("__xor__",
             [](std::ios::openmode x, std::ios::openmode y) { return x^y; },
             py::arg("other"),
             py::doc("bitmask with bit set when, and only when, that bit is set in exactly one of {self, other}"))
        .def("__invert__",
             [](std::ios::openmode x) {
                 /* keep only bits we're exposing: in, out, binary */
                 return  ~x & (std::ios::in | std::ios::out | std::ios::binary); },
             py::doc("openmode bitmask with bit set when, and only when, that bit is not set in self"))
        .def("__repr__",
             [](std::ios::openmode & self)
                 {
                     std::stringstream ss;

                     ss << "<openmode";

                     if (self != 0) {
                         ss << " ";

                         std::size_t nset = 0;
                         if (self & std::ios::in) {
                             ++nset;
                             ss << "input";
                         }
                         if (self & std::ios::out) {
                             if (nset)
                                 ss << "|";
                             ++nset;
                             ss << "output";
                         }
                         if (self & std::ios::binary) {
                             if (nset)
                                 ss << "|";
                             ++nset;
                             ss << "binary";
                         }
                     }

                     ss << ">";

                     return ss.str();
                 },
             py::doc("Get human-readable string representation,"
                     " for example '<openmode input|output|binary>'"))
        ;

    /* The c++ style of iostream reading won't map nicely to python,
     * because expression like
     *   s >> x >> y
     * rely on type information from x, y.
     *
     * Instead plan to target the python File api.
     * Expect to wrap pyzstream into a python class that inherits from the python File class
     *
     */
    py::class_<zstream>(m, "zstream")
        .def(py::init<std::streamsize, char const *, std::ios::openmode>(),
             py::arg("bufz") = zstream::c_default_buffer_size,
             py::arg("filename") = "",
             py::arg("openmode") = std::ios::in,
             py::doc("Create zstream instance.\n"
                     "Allocate 4x bufz bytes for buffer space,"
                     " for {input, output} x {compressed, uncompressed}.\n"
                     "If filename provided, attach to compressed file with that name.\n"
                     "Openmode bitmask enables resulting stream for input and/or output.\n"
                 ))
        .def("openmode",
             &zstream::openmode,
             py::doc("Mode bitmask.\n"
                     "Combination of input|output|binary."))
        .def("is_readable",
             [](zstream & zs)
                 {
                     return ((zs.openmode() & ios::in) == ios::in);
                 },
             py::doc("True if and only if stream is enabled for input."
                     " (openmode.input bit is set)"))
        .def("is_writable",
             [](zstream & zs)
                 {
                     return ((zs.openmode() & ios::out) == ios::out);
                 },
             py::doc("True if and only if stream is enabled for output."
                     " (openmode.output bit is set)"))
        .def("is_open",
             &zstream::is_open,
             py::doc("True if and only if stream is in an open state,\n"
                     "i.e. connected to a stream and available for IO according to openmode.\n"))
        .def("is_closed",
             &zstream::is_closed,
             py::doc("True if and only if stream is in a closed state."))
        .def("eof",
             [](zstream & zs) { return zs.eof(); },
             py::doc("True if and only if input stream has reached end of file."))
        .def("fail",
             [](zstream & zs) { return zs.fail(); },
             py::doc("True if and only if error has occurred (failbit | badbit) on associated stream."))
        .def("gcount",
             [](zstream & zs) -> int64_t { return zs.gcount(); },
             py::doc("return #of chars obtained on last input i/o operation"))
        .def("tellg",
             [](zstream & zs) -> int64_t { return zs.tellg(); },
             py::doc("Return current get position (position relative to 0=start for input sequence.\n"
                     "Warning! Non-monotonic: reports -1 once input reaches eof.  Consider using .gcount()\n"))
        .def("tellp",
             [](zstream & zs) -> int64_t { return zs.tellp(); },
             py::doc("Return current put position (position relative to 0=start for output sequence.\n"
                     "Warning! Non-monotonic: reports -1 once input reaches eof.\n"))
        .def("peek",
             [](zstream & zs) -> char { return zs.peek();  },
             py::doc("Return next input character,  without extracting it"))
        .def("read",
             [](zstream & zs, std::streamsize z)
                 {
                     /* here we assume we should think of input as being in text mode */
                     std::string retval;
                     retval.resize(z);

                     /* read into buffer */
                     zs.read(retval.data(), z);

                     std::streamsize n_read = zs.gcount();

                     retval.resize(n_read);

                     return retval;
                 },
             py::doc("Read z characters from stream.\n"
                     "Return string containing the characters read.\n"
                     "Set both fail and eof bits if less than z characters are available.\n"))
        .def("get",
             [](zstream & zs, std::streamsize z, char delim)
                 {
                     std::string retval;
                     retval.resize(z);

                     /* read into buffer */
                     zs.get(retval.data(), z, delim);

                     std::streamsize n_read = zs.gcount();

                     retval.resize(n_read);

                     return retval;
                 },
             py::doc("Read up to z characters from stream,\n"
                     " but stop on first occurence of delim.\n"
                     "Sets eof bit (but not fail bit) if less than z characters are available.\n"))
        .def("write",
             [](zstream & zs, std::string const & x)
                 {
                     zstream::pos_type p0 = zs.tellp();

                     zs.write(x.data(), x.size());

                     zstream::pos_type p1 = zs.tellp();

                     /* cannot return this,  because don't know address of unique python wrapper object */

                     return (p1 - p0);
                 })
        .def("close", &zstream::close)
        .def("sync",
             &zstream::sync,
             py::doc("Sync stream state with filesystem (i.e. flush output)."))
        .def("__repr__",
             [](zstream & /*zs*/)
                 {
                     return "<zstream>";
                 })
        ;
}
