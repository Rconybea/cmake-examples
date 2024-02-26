#!/usr/bin/env python
# python script to exercise zstream

import pyzstream
import zstream
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
    automatically get plugged in to unittest.main().

    Each test runs in its own independently-constructed instance
    """

    def __init__(self, *args):
        print("Test_openmode ctor: args={args}".format(args=args))

        super(Test_openmode, self).__init__(*args)

    def test_eq(self):
        from pyzstream import openmode

        self.assertTrue(openmode.none == openmode.none)
        self.assertTrue(openmode.input == openmode.input)
        self.assertTrue(openmode.output == openmode.output)
        self.assertTrue(openmode.binary == openmode.binary)

        self.assertFalse(openmode.none == openmode.input)
        self.assertFalse(openmode.input == openmode.output)
        self.assertFalse(openmode.input == openmode.binary)
        self.assertFalse(openmode.output == openmode.binary)

        self.assertFalse(openmode.input
                         == openmode.input | openmode.output)
        self.assertFalse(openmode.output
                         == openmode.input | openmode.output)
        self.assertTrue(openmode.input | openmode.output
                        == openmode.input | openmode.output)

    def test_and(self):
        from pyzstream import openmode

        self.assertEqual(openmode.input & openmode.output,
                         openmode.none)
        self.assertEqual(openmode.input & openmode.input,
                         openmode.input)
        self.assertEqual(openmode.input & (openmode.input | openmode.output),
                         openmode.input)
        self.assertEqual(openmode.output & (openmode.input | openmode.output),
                         openmode.output)

    def test_xor(self):
        from pyzstream import openmode

        self.assertEqual(openmode.input ^ openmode.input,
                         openmode.none)
        self.assertEqual(openmode.input ^ openmode.output,
                         openmode.input | openmode.output)

    def test_invert(self):
        from pyzstream import openmode

        self.assertEqual(~openmode.none, openmode.all)
        self.assertEqual(~openmode.all, openmode.none)
        self.assertEqual(~openmode.input, openmode.output | openmode.binary)
        self.assertEqual(~(openmode.input | openmode.binary), openmode.output)


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

# Test for: python wrapper for pyzstream.zstream
#
class Test_ZstreamBase(unittest.TestCase):
    """
    ZstreamBase unit tests (see pyzstream/zstream.py for python impl)

    Methods in this class that begin with 'test_'
    automatically get plugged in to unittest.main()
    """

    log = logging.getLogger("Test_ZstreamBase")

    def __init__(self, *args):
        print("Test_ZstreamBase ctor: args={args}".format(args=args))

        super(Test_ZstreamBase, self).__init__(*args)

    def test_empty_deflate(self):
        from pyzstream import openmode
        from zstream import ZstreamBase

        self.log.debug("{id}: enter".format(id=unittest.TestCase.id(self)))

        zs = ZstreamBase("empty2.gz", openmode.output)

        self.assertEqual(zs.isatty(), False)

        with self.assertRaises(OSError):
            zs.truncate()

        with self.assertRaises(OSError):
            zs.seek(0, os.SEEK_CUR)

        zs.close()

        zs = ZstreamBase("empty2.gz", openmode.input)

        # expect file empty2.gz with just zlib header (20 bytes)
        with subprocess.Popen(["gunzip", "-c", "empty2.gz"], shell=False, stdout=subprocess.PIPE) as subp:
            s = subp.stdout.readline()

            subp.wait()

            self.log.debug(" gunzip [{s}]".format(s=s))

            # gunzip successfully produced empty output
            self.assertEqual(s, b'')
            self.assertEqual(subp.returncode, 0)


    def test_small_deflate(self):
        from pyzstream import openmode
        from zstream import ZstreamBase

        self.log.debug("test_small_deflate: enter")

        zs = ZstreamBase("hello3.gz", openmode.output)

        self.assertEqual(zs.isatty(), False)

        with self.assertRaises(OSError):
            zs.truncate()

        with self.assertRaises(OSError):
            zs.seek(0, os.SEEK_CUR)

        s = b"I see... stars!\n"

        n = zs.write(s)

        self.assertEqual(n, len(s))

        zs.close()

        # expect file empty2.gz with just zlib header (20 bytes)
        with subprocess.Popen(["gunzip", "-c", "hello3.gz"], shell=False, stdout=subprocess.PIPE) as subp:
            s2 = subp.stdout.readline()

            subp.wait()

            self.log.debug("{id}: gunzip [{s2}]".format(id=unittest.TestCase.id(self), s2=s2))

            # gunzip successfully produced empty output
            self.assertEqual(s, s2)
            self.assertEqual(subp.returncode, 0)

    def test_multiline_deflate_readlines(self):
        from pyzstream import openmode
        from zstream import ZstreamBase

        self.log.debug("test_multi_deflate: enter")

        zs = ZstreamBase("lorem.gz", openmode.output)

        l = [b"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n",
             b"Nulla pharetra diam sit amet nisl. Non arcu risus quis varius.\n",
             b"Amet risus nullam eget felis eget nunc lobortis mattis.\n",
             b"Maecenas accumsan lacus vel facilisis volutpat.\n",
             b"At lectus urna duis convallis.\n",
             b"Arcu felis bibendum ut tristique et egestas quis.\n",
             b"Amet massa vitae tortor condimentum lacinia quis vel.\n",
             b"Auctor eu augue ut lectus arcu bibendum.\n",
             b"Sit amet nulla facilisi morbi tempus iaculis urna.\n",
             b"Netus et malesuada fames ac turpis egestas integer.\n",
             b"Suspendisse interdum consectetur libero id faucibus nisl.\n",
             b"Nunc consequat interdum varius sit amet mattis.\n",
             b"Orci porta non pulvinar neque laoreet suspendisse.\n",
             b"Adipiscing commodo elit at imperdiet dui.\n",
        ]

        zs.writelines(l)

        self.assertEqual(zs.tell(), 769) # total #of uncompressed chars output

        zs.close()

        # expect file hello3.gz containing (compressed) string s
        with subprocess.Popen(["gunzip", "-c", "lorem.gz"], shell=False, stdout=subprocess.PIPE) as subp:
            l2 = subp.stdout.readlines()

            subp.wait()

            self.log.debug("test {id}: gunzip [{l2}]".format(id=unittest.TestCase.id(self), l2=l2))

            # gunzip successfully reproduced uncompressed string
            self.assertEqual(l2, l)
            self.assertEqual(subp.returncode, 0)

    def test_multiline_deflate_read(self):
        from pyzstream import openmode
        from zstream import ZstreamBase

        self.log.debug("test_multi_deflate: enter")

        zs = ZstreamBase("lorem2.gz", openmode.output)

        l = ["Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n",
             "Nulla pharetra diam sit amet nisl. Non arcu risus quis varius.\n",
             "Amet risus nullam eget felis eget nunc lobortis mattis.\n",
             "Maecenas accumsan lacus vel facilisis volutpat.\n",
             "At lectus urna duis convallis.\n",
             "Arcu felis bibendum ut tristique et egestas quis.\n",
             "Amet massa vitae tortor condimentum lacinia quis vel.\n",
             "Auctor eu augue ut lectus arcu bibendum.\n",
             "Sit amet nulla facilisi morbi tempus iaculis urna.\n",
             "Netus et malesuada fames ac turpis egestas integer.\n",
             "Suspendisse interdum consectetur libero id faucibus nisl.\n",
             "Nunc consequat interdum varius sit amet mattis.\n",
             "Orci porta non pulvinar neque laoreet suspendisse.\n",
             "Adipiscing commodo elit at imperdiet dui.\n",
        ]

        s = ''.join(l)

        zs.write(s)

        self.assertEqual(zs.tell(), 769) # total #of uncompressed chars output

        zs.close()

        # now read from lorem2.gz

        zs = ZstreamBase("lorem2.gz", openmode.input)

        # zs.read(-1) not supported yet
        s2 = zs.read(16384)

        self.assertEqual(s2, s)

def main():
    logging.basicConfig(stream=sys.stderr, level=logging.DEBUG)
    #logging.basicConfig(stream=sys.stderr, level=logging.WARNING)

    Test_zstream.log=logging.getLogger("Test_zstream")
    Test_zstream.log.debug("logging enabled")

    Test_ZstreamBase.log = logging.getLogger("Test_ZstreamBase")
    Test_ZstreamBase.log.debug("logging enabled")

    #print("pyzstream.utest.py\n")
    #print("cwd=", os.getcwd(), "\n")

    unittest.main()

if __name__ == '__main__':
    main()
