.. _pyzstreamlib_TextZstream:

.. toctree::
   :maxdepth: 2

.. py:class:: TextZstream(file, mode, bufsize)

   I/O class that works with compressed (``.gzip``) data
   representing a stream of text.

   Inherits :class:`ZstreamBase` and `io.TextIOBase <https://python.readthedocs.io/en/latest/library/io.html#id1>`_

   `io.TextIOBase` behavior:

   .. py:property:: detach

      Not supported (underlying buffers exist, but are not exposed to python)

   .. py:property:: encoding

      always ``"utf8"``

   .. py:property:: errors

      always ``"ignore"``
