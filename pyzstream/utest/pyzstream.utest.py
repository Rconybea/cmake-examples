# python script to exercise zstream

import pyzstream
import zstream
import array
import subprocess
import unittest
import logging
import sys
import os

# Test pybind11-mediated zstream
#
class Test_zstream(unittest.TestCase):
    """
    zstream unit tests (see pyzstream/pyzstream.cpp for c++ impl)

    Methods in this class that begin with 'test_'
    automatically get plugged in to unittest.main()
    """

    log = logging.getLogger("Test_pyzstream")


if __name__ == '__main__':
    #logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)
    logging.basicConfig(stream=sys.stderr, level=logging.WARNING)

    Test_zstream.log=logging.getLogger("Test_pyzstream")
    Test_zstream.log.debug("logging enabled")

    #print("pyzstream.utest.py\n")
    #print("cwd=", os.getcwd(), "\n")

    unittest.main()
