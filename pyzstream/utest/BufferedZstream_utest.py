# BufferedZstream_utest.py

from .zstream_testutil import *

import zstream
import unittest
import pathlib
import logging
import os

class Test_BufferedZstream(unittest.TestCase):
    """
    BufferedZstream unit tests (see pyzstream/zstream.py for python impl)

    Methods in this class that begin with 'test_'
    automatically get plugged in to unittest.main()
    """

    tmpdir = 'Test_BufferedZstream'
    log = logging.getLogger("Test_BufferedZstream")

    def __init__(self, *args):
        print("Test_BufferedZstream ctor: args={args}".format(args=args))

        # create directory for temporary files
        os.makedirs(self.tmpdir, exist_ok=True)

        super(Test_BufferedZstream, self).__init__(*args)

    def test_empty_deflate(self):
        fname = pathlib.Path(self.tmpdir, 'empty3.gz')
        test_empty_deflate_aux(self,
                               ioclass=zstream.BufferedZstream,
                               fname=fname)

    def test_single_deflate(self):
        text = b'I see... stars!'
        fname = pathlib.Path(self.tmpdir, 'hello3.gz')
        test_single_deflate_aux(self,
                                ioclass=zstream.BufferedZstream,
                                text=text,
                                fname=fname)

    def test_multiline_deflate(self):
        text_l = get_lorem_bytes_l()
        fname = pathlib.Path(self.tmpdir, 'lorem3.gz')
        test_multiline_deflate_aux(self,
                                   ioclass=zstream.BufferedZstream,
                                   text_l=text_l,
                                   fname=fname)

    def test_read(self):
        text = get_lorem_bytes()
        fname = pathlib.Path(self.tmpdir, 'read3.gz')
        test_read_aux(self,
                      ioclass=zstream.BufferedZstream,
                      text=text,
                      fname=fname)
