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

    def test_ctor1(self):
        from pyzstream import openmode

        self.log.debug("\ntest_ctor1: present")

        # default will have:
        #   bufz = 64K  (4x this amount allocated)
        #   openmode = input
        #
        zs = pyzstream.zstream()

        self.assertEqual(zs.openmode(), openmode.input)
        self.assertEqual(zs.is_readable(), True)
        self.assertEqual(zs.is_writable(), False)
        self.assertEqual(zs.is_open(), False)
        self.assertEqual(zs.is_closed(), True)
        self.assertEqual(zs.eof(), True)
        self.assertEqual(zs.tellg(), -1)
        self.assertEqual(zs.tellp(), -1)
        self.assertEqual(zs.native_handle(), -1)

        return

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
        self.assertEqual(zs.is_readable(), False)
        self.assertEqual(zs.is_writable(), True)
        self.assertEqual(zs.is_open(), True)
        self.assertEqual(zs.is_closed(), False)
        self.assertEqual(zs.eof(), False)
        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), 0)
        self.assertTrue(zs.native_handle() >= 0)

        zs.close()

        self.assertEqual(zs.tellg(), -1)
        self.assertEqual(zs.tellp(), -1)
        self.assertEqual(zs.native_handle(), -1)

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
        self.assertEqual(zs.is_readable(), False)
        self.assertEqual(zs.is_writable(), True)
        self.assertEqual(zs.is_open(), True)
        self.assertEqual(zs.is_closed(), False)
        self.assertEqual(zs.eof(), False)
        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), 0)
        self.assertTrue(zs.native_handle() >= 0)

        s = 'hello, world!\n'
        n = zs.write(s)

        self.assertEqual(zs.is_open(), True)
        self.assertEqual(zs.is_closed(), False)
        self.assertEqual(n, len(s))
        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), n)
        self.assertTrue(zs.native_handle() >= 0)

        zs.close()

        self.assertEqual(zs.is_open(), False)
        self.assertEqual(zs.is_closed(), True)
        self.assertEqual(zs.tellg(), -1)
        self.assertEqual(zs.tellp(), -1)
        self.assertEqual(zs.native_handle(), -1)

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
        self.assertEqual(zs.is_readable(), False)
        self.assertEqual(zs.is_writable(), True)
        self.assertEqual(zs.is_open(), True)
        self.assertEqual(zs.is_closed(), False)
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

        self.assertEqual(zs.is_open(), False)
        self.assertEqual(zs.is_closed(), True)

        self.assertEqual(zs.tellg(), -1)
        self.assertEqual(zs.tellp(), -1)
        self.assertEqual(zs.native_handle(), -1)

        # reopen stream,  this time for reading

        zs.open('hello.gz', openmode.input)

        self.assertEqual(zs.openmode(), openmode.input)
        self.assertTrue(zs.native_handle() >= 0)
        self.assertEqual(zs.is_readable(), True)
        self.assertEqual(zs.is_writable(), False)
        self.assertEqual(zs.is_open(), True)
        self.assertEqual(zs.is_closed(), False)
        self.assertEqual(zs.eof(), False)
        self.assertEqual(zs.fail(), False)
        self.assertEqual(zs.gcount(), 0)
        self.assertEqual(zs.tellg(), 0)
        self.assertEqual(zs.tellp(), 0)

        # Note: zs.read() would set failbit since <32 chars available
        #s2 = zs.get(32, '\0')
        s2=zs.readline()

        # although we're at eof,  we won't find out until next attempt to read.

        self.assertEqual(s2, s)
        self.assertEqual(zs.eof(), False)
        self.assertEqual(zs.fail(), False)
        # zs.gcount() won't include final newline.
        self.assertEqual(zs.gcount(), n)
        self.assertEqual(zs.tellg(), n)
        self.assertEqual(zs.tellp(), 0)
        self.assertTrue(zs.native_handle() >= 0)

        zs.close()

        self.assertEqual(zs.native_handle(), -1)
        self.assertEqual(s2, s)

        self.assertEqual(zs.gcount(), n)
        self.assertEqual(zs.tellg(), -1)
        self.assertEqual(zs.tellp(), -1)

        self.assertEqual(zs.openmode(), openmode.input)
        self.assertEqual(zs.is_readable(), True)
        self.assertEqual(zs.is_writable(), False)
        self.assertEqual(zs.is_open(), False)
        self.assertEqual(zs.is_closed(), True)
        # want eof state for closed stream
        self.assertEqual(zs.eof(), True)

        # attempt .readline() on closed stream
        s2=zs.readline()

        self.assertEqual(s2, '')
        self.assertEqual(zs.eof(), True)
        self.assertEqual(zs.fail(), True)
        self.assertEqual(zs.native_handle(), -1)

        zs.close()

        self.assertEqual(zs.is_open(), False)
        self.assertEqual(zs.is_closed(), True)
        self.assertEqual(zs.eof(), True)
        self.assertEqual(zs.native_handle(), -1)

    def test_deflate(self):
        from pyzstream import openmode

        zs = pyzstream.zstream(16384,
                               "multiline.gz",
                               openmode.output)

        s = """
Lorem ipsum dolor sit amet, consectetur adipiscing elit
, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua
. Nulla pharetra diam sit amet nisl. Non arcu risus quis varius
. Amet risus nullam eget felis eget nunc lobortis mattis
. Maecenas accumsan lacus vel facilisis volutpat
. At lectus urna duis convallis
. Arcu felis bibendum ut tristique et egestas quis
. Amet massa vitae tortor condimentum lacinia quis vel
. Auctor eu augue ut lectus arcu bibendum
. Sit amet nulla facilisi morbi tempus iaculis urna
. Netus et malesuada fames ac turpis egestas integer
. Suspendisse interdum consectetur libero id faucibus nisl
. Nunc consequat interdum varius sit amet mattis
. Orci porta non pulvinar neque laoreet suspendisse
. Adipiscing commodo elit at imperdiet dui

        """

        n = zs.write(s)
        zs.close()

        zs = pyzstream.zstream(16384,
                               "multiline.gz",
                               openmode.input)

        l = zs.readlines()

        self.assertEqual(len(l), s.count('\n') + 1)
        self.assertEqual(''.join(l), s)

        #print("l={l}".format(l=l))

