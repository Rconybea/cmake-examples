#!/usr/bin/env python
# python script to exercise zstream

from .openmode_utest import *
from .zstream_utest import *
import unittest
import logging

def main():

# Test pybind11-mediated zstream
#
class Test_zstream(unittest.TestCase):


def main():
    """
    run tests explictly.
    Must first set PYTHONPATH to refer to build artifacts;
    see pystream.utest.in template in this directory
    """

    #logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)
>>>>>>> ex18:pyzstream/utest/pyzstream.utest.py
    #logging.basicConfig(stream=sys.stderr, level=logging.WARNING)

    #Test_zstream.log=logging.getLogger("Test_zstream")
    #Test_zstream.log.debug("logging enabled")

    #Test_ZstreamBase.log = logging.getLogger("Test_ZstreamBase")
    #Test_ZstreamBase.log.debug("logging enabled")

    Test_ZstreamBase.log = logging.getLogger("Test_ZstreamBase")
    Test_ZstreamBase.log.debug("logging enabled")

    #print("pyzstream.utest.py\n")
    #print("cwd=", os.getcwd(), "\n")

    unittest.main()

if __name__ == '__main__':
    main()
