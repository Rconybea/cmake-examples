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
        .def("native_handle",
             &zstream::native_handle,
             py::doc("Return stream file descriptor, if defined and known."))
        .def("open",
             &zstream::open,
             py::arg("filename"), py::arg("openmode"),
             py::doc("Connect stream to filename,  open for input/output according to openmode"))
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
        .def("readinto",
             [](zstream & zs, py::object const & x) -> int64_t
                 {
                     Py_buffer buffer_details;

                     if (PyObject_GetBuffer(x.ptr(), &buffer_details, PyBUF_C_CONTIGUOUS | PyBUF_WRITABLE) != 0)
                         throw py::error_already_set();

                     zstream::pos_type p0 = zs.tellg();

                     /* here: can read buffer_details.len bytes into buffer_details.buf */
                     if (buffer_details.len > 0) {
                         zs.read(reinterpret_cast<char *>(buffer_details.buf), buffer_details.len);
                     }

                     zstream::pos_type p1 = zs.tellg();

                     PyBuffer_Release(&buffer_details);

                     return (p1 - p0);

#ifdef OBSOLETE
                     char * buf_data = PyByteArray_AS_STRING(buf.ptr());
                     size_t buf_z = buf.size();

                     if (buf_data && (buf_z > 0)) {
                         zs.read(buf_data, buf_z);
                         return zs.gcount();
                     } else {
                         return 0;
                     }
#endif
                 },
             py::arg("buf"),
             py::doc("Read into writable python bytes-like object."))
        .def("readline",
             [](zstream & zs, std::streamsize z)
                 {
                     string retval;

                     if (z >= 0) {
                         retval.resize(z);

                         std::streamsize n = zs.read_until(retval.data(),
                                                           z,
                                                           true /*check_delim_flag*/,
                                                           '\n' /*delim*/);

                         retval.resize(n+1);
                     } else {
                         retval = zs.read_until(true /*check_delim_flag*/,
                                                '\n' /*delim*/);
                     }

                     return retval;
                 },
             py::arg("z") = -1,
             py::doc("Read one line of (uncompressed) text;"
                     " or if z>=0, up to z characters or newline, whichever comes first."))
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
        .def("readlines",
             [](zstream & zs, std::streamsize hint = -1)
                 {
                     list<string> retval;

                     bool using_hint_flag = (hint >= 0);

                     while (!zs.eof() && (!using_hint_flag || (hint >= 0))) {
                         std::string s = zs.read_until(true /*check_delim_flag*/, '\n', 4095 /*block_size*/);

                         if (s.empty()) {
                             /* since we're reading lines,
                              * a non-endofline delimited line implies end-of-file
                              */
                             break;
                         }

                         hint -= s.size();

                         retval.push_back(s);
                     }

                     return retval;
                 },
             py::arg("hint") = -1,
             py::doc("Read stream content (uncompressing), splitting into lines on each newline.\n"
                     "If hint >= 0,  stop once total number of chars read reaches hint.\n"))
        .def("write",
             [](zstream & zs, py::object const & x) -> uint64_t
                 {
                     uint64_t retval = 0;

                     if (zs.is_binary()) {
                         /* For binary streams, want to be able to accept any python bytes-like object here:
                          * i.e. an object that:
                          * - supports the Buffer Protocol
                          * - can export a C-contiguous buffer;
                          * This includes bytes, bytearray, array.array, some memoryview objects.
                          *
                          * From
                          *   https://docs.python.org/3/c-api/buffer.html#bufferobjects:
                          * Use PyObject_GetBuffer() to acquire buffer assoc'd with target object;
                          * then PyBuffer_Release() when done.
                          *
                          * flags: see
                          *   https://docs.python.org/3/c-api/buffer.html
                          *
                          *   PyBUF_STRIDES PyBUF_FORMAT PyBUF_WRITABLE
                          */

                         Py_buffer buffer_details;

                         if (PyObject_GetBuffer(x.ptr(), &buffer_details, PyBUF_C_CONTIGUOUS) != 0)
                             throw py::error_already_set();

                         try {
                             zstream::pos_type p0 = zs.tellp();

                             /* note: contiguous array --> .buf refers to beginning of memory block */
                             zs.write(static_cast<char const *>(buffer_details.buf), buffer_details.len);

                             /* ostream.write() doesn't report #bytes written
                              * (though may set state flags failbit/badbit)
                              */
                             zstream::pos_type p1 = zs.tellp();

                             /* report #of uncompressed chars written */
                             retval = (p1 - p0);

                             PyBuffer_Release(&buffer_details);  /*must call exactly once*/
                         } catch(...) {
                             PyBuffer_Release(&buffer_details);  /*must call exactly once*/

                             throw;
                         }
                     } else {
                         /* For text streams,  python string could use any of utf-8, utf-16, utf-32;
                          * convert to utf-8 for output to zstream.
                          *
                          * We could write a shorter version using py::cast<std::string>(); but
                          * std::string ctor would make extra copy of string contents.
                          *
                          * Starting with python 3.10,  could perhaps use PyUnicode_AsUTF8StringAndSize() here instead;
                          * caches utf-8 representation in source object and takes responsibility for storage
                          */
                         py::object tmp = x;

                         if (PyUnicode_Check(tmp.ptr())) {
                             tmp = py::reinterpret_steal<py::object>(PyUnicode_AsUTF8String(x.ptr()));
                             if (!tmp)
                                 throw py::error_already_set();
                         }

                         char * buf = nullptr;
                         int64_t buf_z = 0;

                         if (PyBytes_AsStringAndSize(tmp.ptr(), &buf, &buf_z) != 0)
                             throw py::error_already_set();

                         /* write buf[0 .. buf_z-1] to zs */

                         zstream::pos_type p0 = zs.tellp();

                         zs.write(buf, buf_z);

                         zstream::pos_type p1 = zs.tellp();

                         retval = (p1 - p0);
                     }

                     return retval;
                 },
             py::arg("x"),
             py::doc("Write x onto this stream.\n"
                     "x must be a str (for stream opened in text mode),\n"
                     "or bytes-like object (for stream opened in binary mode).\n"
                     "Returns the number of (uncompressed) bytes written\n"))
        .def("sync",
             &zstream::sync,
             py::doc("Sync stream state with filesystem (i.e. flush output)."))
        .def("close",
             &zstream::close,
             py::doc("Close stream and any associated file;"
                     " revert stream to closed state with empty buffers.\n"
                     "Can reopen stream (perhaps connected to a different file) with zstream.open()."))
        .def("__repr__",
             [](zstream & /*zs*/)
                 {
                     return "<zstream>";
                 })
        ;
}
