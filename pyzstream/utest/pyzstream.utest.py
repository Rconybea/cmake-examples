# python script to exercise zstream

import pyzstream
#import zstream
import array
import subprocess
import unittest
import logging
import sys
import os

class Test_openmode(unittest.TestCase):
    """
    openmode unit tests (see pyzstream/pyzstream.cpp for c++ impl)

    Methods in this class that begin with 'test_'
    automatically get plugged in to unittest.main()
    """

    def test_eq(self):
        from pyzstream import openmode

        self.assertFalse(openmode.input == openmode.output)
        self.assertFalse(openmode.input == openmode.binary)
        self.assertFalse(openmode.output == openmode.binary)

        self.assertFalse(openmode.input
                         == openmode.input | openmode.output)
        self.assertFalse(openmode.output
                         == openmode.input | openmode.output)

# Test pybind11-mediated zstream
#
class Test_zstream(unittest.TestCase):
    """
    zstream unit tests (see pyzstream/pyzstream.cpp for c++ impl)

    Methods in this class that begin with 'test_'
    automatically get plugged in to unittest.main()
    """

    log = logging.getLogger("Test_zstream")

    def test_empty_deflate(self):
        from pyzstream import openmode

        self.log.debug("")
        self.log.debug("test_empty_deflate1: start")
        self.log.debug(" cwd [{dir}]".format(dir=os.getcwd()))

        self.log.debug("create zstream")

        zs = pyzstream.zstream(16384,
                               "empty.gz",
                               openmode.output)

        zs.close()


        # expect file empty.gz with just zlib header (20 bytes)
        with subprocess.Popen(["gunzip", "-c", "empty.gz"], shell=False, stdout=subprocess.PIPE) as subp:
            s = subp.stdout.readline()

            subp.wait()

            self.log.debug(" gunzip [{s}]".format(s=s))

            # gunzip successfully produced empty output
            self.assertEqual(s, b'')
            self.assertEqual(subp.returncode, 0)

    def test_small_deflate(self):
        from pyzstream import openmode

        self.log.debug("\ntest_small_deflate")

        zs = pyzstream.zstream(16384,
                               "hello.gz",
                               openmode.output)

        s = 'hello, world!\n'
        n = zs.write(s)

        zs.close()

        # expect file hello.gz containing 'hello, world\n' (34 bytes 'compressed')
        with subprocess.Popen(["gunzip", "-c", "hello.gz"], shell=False, stdout=subprocess.PIPE) as subp:
            s2 = subp.stdout.readline()

            subp.wait()

            self.log.debug(" gunzip [{s2}]".format(s2=s2))

            # gunzip recovered original input
            self.assertEqual(s2, s.encode('utf-8'))

        self.log.debug("done")

    def test_small_inflate(self):
        from pyzstream import openmode

        zs = pyzstream.zstream(16384,
                               "hello.gz",
                               openmode.output)

        s = 'hello, world!\n'
        n = zs.write(s)

        zs.close()

        # reopen stream,  this time for reading

        zs = pyzstream.zstream(16384,
                               "hello.gz",
                               openmode.input)
        # zs.open('hello.gz', openmode.input)

        s2 = zs.read(32)
        #s2=zs.readline()

        self.assertEqual(s2, s)

        s2 = zs.read(32)
        #s2=zs.readline()

        self.assertEqual(s2, '')

        zs.close()

if __name__ == '__main__':
    #logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)
    logging.basicConfig(stream=sys.stderr, level=logging.WARNING)

    Test_zstream.log=logging.getLogger("Test_zstream")
    Test_zstream.log.debug("logging enabled")

    #print("pyzstream.utest.py\n")
    #print("cwd=", os.getcwd(), "\n")

    unittest.main()
