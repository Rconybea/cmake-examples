# zstream.py

import pyzstream
import io
import os

class ZstreamBase(io.IOBase):
    """
    I/O stream that operates with respect to a compressed (gzip-format) stream.
    """

    # pyzstream.zstream (see pyzstream.cpp)
    _zstream = None

    def __init__(self,
                 filename : str | bytes | os.PathLike,
                 mode : str = 'r',
                 bufsize = 64*1024):
        """
        create ZstreamBase for i/o w.r.t. filename.

        Args:
          filename (str|bytes|os.PathLike)
                        path to file on disk.
          mode (str)    string comprising letter codes:
                        [r] open for reading, [w] open for writing,
                        [b] binary mode, [t] text mode (default)
                        (note: python [a], [x] not supported)
          bufsize (int) buffer size;  used for both compressed +
                        uncompressed data
        """

        zstream_mode = pyzstream.openmode.from_string(mode)

        self._zstream = pyzstream.zstream(bufsize,
                                          os.fspath(filename),
                                          zstream_mode)

        super(ZstreamBase, self).__init__()

    def openmode(self):
        """
        return native openmode bitmask (python wrapper for c++ std::ios::openmode)
        """
        return self._zstream.openmode()

    def eof(self):
        """
        True if readable stream has encountered eof
        """
        return self._zstream.eof()

    def read(self, z : int = -1):
        """
        Broken!
        1. zstream.read(z) will set failbit if less than z bytes available
        2. zstream.read() expects positive argument
        """
        return self._zstream.read(z)

    # def readinto(b):   # TODO

    def write(self, s : str):
        """
        write string (in text mode) or object (in binary mode) to stream
        """
        return self._zstream.write(s)

    # ----- inherited from IOBase -----

    def readable(self):
        """
        return true iff stream was last opened for reading.
        """
        return self._zstream.is_readable()

    def writable(self):
        """
        return true iff stream was last opened for writing.
        """
        return self._zstream.is_writable()

    def seekable(self):
        return False

    @property
    def closed(self):
        """
        return true iff stream is in a closed state
        """
        return self._zstream.is_closed()

    # def fileno(self):    # TODO

    def isatty(self):
        # interactive compressed stream not supported
        return False

    # def readline(self, z = -1):   # TODO
    # def readlines(self, hint = -1):    # TODO

    def writelines(self, l):
        for s in l:
            self.write(s)

    def truncate(self, z=None):
        raise OSError(-1, "zstream: attempted .truncate() with non-truncatable stream")

    def tell(self):
        """
        Broken!  -1 once input stream reaches eof.   Want to be able to tail a file,  for example
        """

        # pyzstream has independent read + write positions.
        #
        # In expected case that stream was opened for input-only or output-only,
        # taking max always gives the right answer
        #
        return max(self._zstream.tellg(),
                   self._zstream.tellp())

    def seek(self, offset, whence):
        raise OSError(-1, "zstream: attempted .seek() with non-seekable stream")

    def close(self):
        self._zstream.close()

    def flush(self):
        self._zstream.sync()


class BufferedZstream(ZstreamBase, io.BufferedIOBase):
    """
    Compressed I/O stream that satisfies python's BufferedIOBase api.
    BufferedIOBase is always binary;  BufferedZstream enforces a binary zstream implementation.

    Intended to be usable to support a TextIOWrapper,  for example

    ZstreamBase provides methods read(), write()
    """

    def __init__(self,
                 filename : str | bytes | os.PathLike,
                 mode : str = 'rb',
                 bufsize : int = 64*1024):
        # super() invokes ZstreamBase ctor first
        super(BufferedZstream, self).__init__(filename,
                                              mode + 'b',
                                              bufsize)

    # def read1(z = -1):   # TODO

    # def readinto1(b):    # TODO


class TextZstream(ZstreamBase, io.TextIOBase):
    """
    Text I/O stream implementation that performs I/O w.r.t. a gzip-compressed stream.

    Zstreams rely on internal buffering; however buffers are not accessible from python.
    Zstreams do not support seek or truncate.
    """

    def __init__(self,
                 filename : str | bytes | os.PathLike,
                 mode : str = 'r',
                 bufsize : int = 64*1024):
        # init ZstreamBase first
        super(TextZstream, self).__init__(filename, mode, bufsize)

    @property
    def encoding(self):
        return "utf-8"

    # errors property of regular open isn't supported here.
    # if this is important,  suggest using TextIOWrapper to textify a BufferedZstream
    #
    @property
    def errors(self):
        return "ignore"

    # .newlines : not supported
    # .buffer   : not supported

    # .detach : not supported

    # .read     : inherited from ZstreamBase

    # def read1(z = -1):   # TODO

    # .readinto: inherited from ZstreamBase

    # def readinto1(b):   # TODO

    # .seek : inherited from ZstreamBase
    # .tell : inherited from ZstreamBase
    # .write : inherited from ZstreamBase

# ..class Zstream
