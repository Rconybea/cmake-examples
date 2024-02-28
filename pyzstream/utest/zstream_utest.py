import pyzstream
import unittest
import subprocess
import logging
import os

# Test pybind11-mediated zstream
#
class Test_zstream(unittest.TestCase):
    """
    zstream unit tests (see pyzstream/pyzstream.cpp for c++ impl)

    Methods in this class that begin with 'test_'
    automatically get plugged in to unittest.main()
    """

    log = logging.getLogger("Test_zstream")

    def __init__(self, *args):
        print("Test_zstream ctor: args={args}".format(args=args))

        super(Test_zstream, self).__init__(*args)

    def test_empty_deflate(self):
        from pyzstream import openmode

        self.log.debug("")
        self.log.debug("test_empty_deflate1: start")
        self.log.debug(" cwd [{dir}]".format(dir=os.getcwd()))

        self.log.debug("create zstream")

        zs = pyzstream.zstream(16384,
                               "empty.gz",
                               openmode.output)

        self.assertEqual(zs.openmode(), openmode.output)
        self.assertEqual(zs.eof(), False)
        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), 0)
        zs.close()

        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), 0)

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

        self.log.debug("test_small_deflate")

        zs = pyzstream.zstream(16384,
                               "hello.gz",
                               openmode.output)

        self.assertEqual(zs.openmode(), openmode.output)
        self.assertEqual(zs.eof(), False)
        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), 0)
        s = 'hello, world!\n'
        n = zs.write(s)

        self.assertEqual(n, len(s))
        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), n)
        zs.close()

        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), 0)

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

        self.log.debug("test_small_inflate")

        zs = pyzstream.zstream(16384,
                               "hello.gz",
                               openmode.output)

        self.assertEqual(zs.openmode(), openmode.output)
        self.assertEqual(zs.eof(), False)
        self.assertEqual(zs.fail(), False)
        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), 0)
        s = 'hello, world!\n'
        n = zs.write(s)

        self.assertEqual(n, len(s))
        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), n)
        zs.close()

        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), 0)
        # reopen stream,  this time for reading

        zs = pyzstream.zstream(16384,
                               "hello.gz",
                               openmode.input)
        # zs.open('hello.gz', openmode.input)
        self.assertEqual(zs.openmode(), openmode.input)
        self.assertEqual(zs.eof(), False)
        self.assertEqual(zs.fail(), False)
        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), 0)

        # Note: zs.read() would set failbit since <32 chars available
        s2 = zs.get(32, '\0')
        #s2=zs.readline()

        self.assertEqual(zs.eof(), True)
        self.assertEqual(zs.fail(), False)
        self.assertEqual(s2, s)
        self.assertEqual(zs.gcount(), n)
        self.assertEqual(zs.tellg(), -1)
        self.assertEqual(zs.tellp(), -1)

        zs.close()

