.. _pyzstreamlib_ZstreamBase:

.. toctree::
   :maxdepth: 2

.. py:function:: zopen(file, mode = 'r', buffering = 64*1024, encoding = None, errors = None, newline = None)

   Drop-in (with some caveats) replacement for built-in `open <https://docs.python.org/3/library/functions.html#open>`_,
   that deflates (applies compression) on output,  and inflates (uncompresses) on input.
   Compression is built on top of ``zlib``,  and satsifies ``gzip`` format.

   Caveats:

    #. opening in read+write mode (``'+'``) is not useful,  since stream implementation is not seekable.
    #. opening in append (``'a'``) or exclusive (``'x'``) mode is not supported.  The ``'a'`` and ``'x'`` characters are ignored.
    #. unlike with ``open()``, file cannot be an integer file descriptor
    #. the optional *opener* and *closefd* arguments to ``open()`` are not supported.

   Otherwise arguments have the same meaning as in built-in ``open()``:

   *file* filename or path-like object identifying location for compressed stream.

   *mode* mode for opening.  Combination of characters ``'r'`` (read), ``'w'`` (write), ``'b'`` (binary), ``'t'`` (text).

   *buffering* buffer size.  A floor of 1 byte is applied.  Implementation uses 4x this value.
   Note that independently the inflate/deflate algorithm uses 32k memory internally (``zlib`` window size 15)

   *encoding* String encoding for text streams

   *errors* Error handling policy for encoding errors

   *newline* Newline translation policy

.. py:class:: ZstreamBase(filename, mode, bufsize)

   I/O class that works with compressed (``.gzip``) native data.
   Automatically deflates output and inflates input

   Inherits `io.IOBase <https://python.readthedocs.io/en/latest/library/io.html#i-o-base-classes>`_

   `io.IOBase` behavior:

   .. py:method:: seekable()

      ``False``: ZstreamBase does not support seek

   .. py:method:: truncate()

      raises ``OSError``: ZstreamBase does not support truncate

   .. py:method:: flush()

      Does not guarantee all output written to disk.
      That requirement would be problematic for zlib, since deflated representation up to some position **p**
      typically depends on output after **p**.

      All output *will* be flushed on ``IOBase.close()`` [#]_

   ZstreamBase provides these methods in addition to those from IOBase:

   .. py:method:: openmode()

      Return native (c++ iostream) bitmask recording flags set when this stream last opened.

   .. py:method:: eof()

      ``True`` if input position is at end-of-file

   .. py:method:: read(z = -1, /)

      Read up to *z* characters / bytes.   In text mode return `str`; in binary mode return `bytes`

   .. py:method:: readinto(b, /)

      Read bytes into a pre-allocated, writable *bytes-like object* *b* and return the number of bytes read.
      For example, this will work with python classes that support the buffer protocol.

   .. py:method:: write(obj, /)

      Write string (in text mode) or object (in binary mode) to stream.
      Returns the number of bytes used before compression.

   .. rubric:: Footnotes

   .. [#] For flush-on-close implementation,  see the ``Z_FINAL`` argument to zlib ``deflate()``
