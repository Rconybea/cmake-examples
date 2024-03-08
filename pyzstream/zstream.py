# zstream.py

import pyzstream
import io
import os

class ZstreamBase(io.IOBase):
    """I/O stream that operates with respect to a compressed (gzip-format) stream.

    Attributes:
      _zstream  implementation (pyzstream.zstream, see pyzstream.cpp)
    """
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

    def readinto(b):
        """
        Read into existing bytes-like object,
        and return number of bytes read.
        """
        return self._zstream.readinto(b)

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
        """
        zstreams do not support seek
        """
        return False

    @property
    def closed(self):
        """
        return true iff stream is in a closed state
        """
        return self._zstream.is_closed()

    def fileno(self):
        """
        return file descriptor associated with zstream,  if known
        """
        return self._zstream.native_handle()

    def isatty(self):
        """
        interactive compressed stream not supported
        """
        return False

    def readline(self, z = -1):
        """
        read up to z chars,  or until newline,  whichever comes first
        """
        return self._zstream.readline(z)

    def readlines(self, hint = -1):
        """
        read multiple complete lines of text,  returning as a list or strings;
        but stop reading additional lines after hint total characters.
        """
        return self._zstream.readlines(hint)

    def writelines(self, l):
        """
        write each string in l
        """
        for s in l:
            self.write(s)

    def truncate(self, z=None):
        """
        truncate not implemented for zstream
        """
        raise OSError(-1, "zstream: attempted .truncate() with non-truncatable stream")

    def tell(self):
        """
        Report current stream position -- assumes open for input-only or output-only

        Caveat: -1 once input stream reaches eof.   Want to be able to tail a file,  for example
        """
        # pyzstream has independent read + write positions.
        #
        # In expected case that stream was opened for input-only or output-only,
        # taking max always gives the right answer
        #
        return max(self._zstream.tellg(),
                   self._zstream.tellp())

    def seek(self, offset, whence):
        """
        seek not implemented for zstream
        """
        raise OSError(-1, "zstream: attempted .seek() with non-seekable stream")

    def close(self):
        """
        send zstream to closed state,  flush any remaining output.
        """
        self._zstream.close()

    def flush(self):
        """
        flush remaining output.

        NOTE: excludes output held internally by zlib for which compressed representation isn't yet determined
        (because it depends on further uncompressed data)

        Stream more output,  or flush this data using close()
        """
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
        """
        assuming utf-8 for encoding
        """
        return "utf-8"

    @property
    def errors(self):
        """
        errors property of regular open isn't supported here.
        if this is important,  suggest using TextIOWrapper to textify a BufferedZstream
        """
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

def zopen(filename : str | bytes | os.PathLike,
          mode : str = 'r',
          buffering : int = 64*1024,
          encoding = None,
          errors = None,
          newline = None):
    """Drop-in (with caveats) replacement for built-in open(), that works with a compressed file

    Caveats
      integer file descriptors for filename are not supported
      Accordingly the closefd and opener arguments to built-in open() are not provided.

    Args:
      filename    path-like-object of file to be opened.
                  integer file descriptors not supported.
      mode (str)  optional string specifying file open mode.
                  'r' (read), 'w' (write), 'b' (binary), 't' (text).
                  'x', 'a' not supported (as of mar 2024).
                  '+' is permitted but not usable absent some basic seek support.
                  For example, "rb" to open for reading in binary mode.  Default is "r".
      buffering   buffer size.  Floored at 1 byte.  Default 64k
      encoding    string encoding in text mode
      errors      optional string specifying how encoding and decoding errors should be handled.
                  See built-in open();  ignored in binary mode.
      newline     determines handling of newline characters in text mode.  See built-in open();
    """
    bufsize = max(1, buffering)

    zs = BufferedZstream(filename, mode, bufsize)

    if 'b' in mode:
        return zs
    else:
        return io.TextIOWrapper(zs,
                                encoding,
                                errors,
                                newline,
                                line_buffering=False,
                                write_through=False)
