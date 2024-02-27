from .zstream_testutil import *
import zstream
import unittest
import logging
import pathlib

# Test for: python wrapper for pyzstream.zstream
#
class Test_ZstreamBase(unittest.TestCase):
    """
    ZstreamBase unit tests (see pyzstream/zstream.py for python impl)

    Methods in this class that begin with 'test_'
    automatically get plugged in to unittest.main()
    """

    log = logging.getLogger("Test_ZstreamBase")
    tmpdir = 'Test_ZstreamBase'

    def __init__(self, *args):
        import os

        print("Test_ZstreamBase ctor: args={args}".format(args=args))

        # create directory for temporary files
        os.makedirs(self.tmpdir, exist_ok=True)

        super(Test_ZstreamBase, self).__init__(*args)

    def test_empty_deflate(self):
        fname = pathlib.Path(self.tmpdir, 'empty2.gz')

        test_empty_deflate_aux(self,
                               ioclass=zstream.ZstreamBase,
                               fname=str(fname))

    def test_small_deflate(self):
        text = b'I see... stars!'
        fname = pathlib.Path(self.tmpdir, 'hello2.gz')

        test_single_deflate_aux(self,
                                ioclass=zstream.ZstreamBase,
                                text=text,
                                fname=fname)

    def test_multiline_deflate(self):
        text_l = get_lorem_bytes_l()
        fname = pathlib.Path(self.tmpdir, 'lorem.gz')

        test_multiline_deflate_aux(self,
                                   ioclass=zstream.ZstreamBase,
                                   text_l=text_l,
                                   fname=fname)

    def test_read(self):
        text = get_lorem_str()
        fname = pathlib.Path(self.tmpdir, 'read.gz')

        test_read_aux(self,
                      ioclass=zstream.ZstreamBase,
                      text=text,
                      fname = fname)

