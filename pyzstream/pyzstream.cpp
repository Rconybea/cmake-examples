#include "zstream/zstream.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace std;

PYBIND11_MODULE(pyzstream, m) {
    // see https://docs.python.org/3/library/operator.html#mapping-operators-to-functions

    m.doc() = "pybind11 plugin for zstream";

    /* wrap ios::openmode */
    py::class_<std::ios::openmode>(m, "openmode")
        /* note: 'in' is a keyword in python, can't use here */
        .def_property_readonly_static("input", [](py::object /*self*/) { return std::ios::in; })
        .def_property_readonly_static("output", [](py::object /*self*/) { return std::ios::out; })
        .def_property_readonly_static("binary", [](py::object /*self*/) { return std::ios::binary; })
        .def("__or__", [](std::ios::openmode x, std::ios::openmode y) { return x|y; })
        .def("__and__", [](std::ios::openmode x, std::ios::openmode y) { return x&y; })
        .def("__repr__",
             [](std::ios::openmode & self)
                 {
                     std::stringstream ss;

                     ss << "<openmode ";
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
                     ss << ">";

                     return ss.str();
                 })
        ;

    /* The c++ style of iostream reading won't map nicely to python,
     * because expression like
     *   s >> x >> y
     * rely on type information from x, y.
     *
     * Instead plan to target the python File api.
     * Expect to wrap pyzstream into a python class that inherits from the python File class
     */
    py::class_<zstream>(m, "zstream")
        .def(py::init<std::streamsize, char const *, std::ios::openmode>())
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
                 })
        .def("write",
             [](zstream & zs, std::string const & x)
                 {
                     zs.write(x.data(), x.size());

                     /* cannot return this,  because don't know address of unique python wrapper object */

                     return zs.gcount();
                 })
        .def("close", &zstream::close)
        .def("__repr__",
             [](zstream & /*zs*/)
                 {
                     return "<zstream>";
                 })
        ;
}
