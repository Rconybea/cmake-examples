# zstream_testutil.py

import unittest
import subprocess
import io
import os

def get_lorem_bytes_l():
    return [b"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n",
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

def get_lorem_str():
    return ''.join(["Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n",
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
                    ])

def test_empty_deflate_aux(self : unittest.TestCase,
                           ioclass : io.IOBase,
                           fname : str):
    """
    test empty deflate on behalf of
    {Test_ZstreamBase, Test_BufferedZstream, Test_TextZstream}

    Args:
      self (unittest.TestCase).   Test harness class invoking this function
      ioclass (io.IOBase).        The i/o class to be tested.
                                  (ZstreamBase | BufferedZstream | TextZstream)
      fname (str).                Filename to use for (empty) compressed stream
    """

    from pyzstream import openmode

    self.log.debug("{id}: enter".format(id=unittest.TestCase.id(self)))

    zs = ioclass(fname, openmode.output)

    # .isatty not actually known (needs isatty(fd))
    self.assertEqual(zs.isatty(), False)

    # .truncate not supported
    with self.assertRaises(OSError):
        zs.truncate()

    # .seek not supported
    with self.assertRaises(OSError):
        zs.seek(0, os.SEEK_CUR)

    zs.close()

    # expect file empty2.gz with just zlib header (20 bytes)
    with subprocess.Popen(["gunzip", "-c", fname], shell=False, stdout=subprocess.PIPE) as subp:
        s = subp.stdout.readline()

        subp.wait()

        self.log.debug(" gunzip [{s}]".format(s=s))

        # gunzip successfully produced empty output
        self.assertEqual(s, b'')
        self.assertEqual(subp.returncode, 0)
    
def test_single_deflate_aux(self : unittest.TestCase,
                            ioclass : io.IOBase,
                            text : bytes,
                            fname : str):
    """
    test deflate on behalf of
    {Test_ZstreamBase, Test_BufferedZstream, Test_TextZstream}

    test will make one call to write (compressed) text to fname,
    and verify content using gunzip.

    Args:
      self (unittest.TestCase).   Test harness class invoking this function
      ioclass (io.IOBase).        The i/o class to be tested.
                                  (ZstreamBase | BufferedZstream | TextZstream)
      text (str).                 plain text to write to file.
      fname (str).                Filename to use for (empty) compressed stream
    """
    from pyzstream import openmode

    self.log.debug("test_small_deflate: enter")

    zs = ioclass(fname, openmode.output)

    # needs file descriptor to implement
    self.assertEqual(zs.isatty(), False)

    # .truncate() not supported
    with self.assertRaises(OSError):
        zs.truncate()

    # .seek() not supported
    with self.assertRaises(OSError):
        zs.seek(0, os.SEEK_CUR)

    s = text

    n = zs.write(s)

    self.assertEqual(n, len(s))

    zs.close()

    # expect file empty2.gz with just zlib header (20 bytes)
    with subprocess.Popen(["gunzip", "-c", fname], shell=False, stdout=subprocess.PIPE) as subp:
        s2 = subp.stdout.read(-1)

        subp.wait()

        self.log.debug("{id}: gunzip [{s2}]".format(id=unittest.TestCase.id(self), s2=s2))

        # gunzip successfully produced empty output
        self.assertEqual(s, s2)
        self.assertEqual(subp.returncode, 0)

def test_multiline_deflate_aux(self,
                               ioclass : io.IOBase,
                               text_l : list[bytes],
                               fname : str):
    """
    Test multiline deflate on behalf of
    {Test_ZstreamBase, Test_BufferedZstream, Test_TextZstream}

    Test will make one call to .writelines() method on an instance of ioclass,
    and verify content using gunzip.

    Args:
      self (unittest.TestCase).   Test harness class invoking this function
      ioclass (io.IOBase).        The i/o class to be tested.
                                  (ZstreamBase | BufferedZstream | TextZstream)
      text (list(bytes)).         content to write to (compressed) file.
                                  each element must contain exactly one newline,
                                  at the end.
      fname (str).                Filename to use for (empty) compressed stream
    """

    from pyzstream import openmode
    from zstream import ZstreamBase

    self.log.debug("test_multiline_deflate_aux: enter")

    zs = ioclass(fname, openmode.output)

    zs.writelines(text_l)

    # count sum of bytes in text
    text = b''.join(text_l)
    n = len(text)
    
    self.assertEqual(zs.tell(), n) # total #of uncompressed chars output

    zs.close()

    # expect file hello3.gz containing (compressed) string s
    with subprocess.Popen(["gunzip", "-c", fname], shell=False, stdout=subprocess.PIPE) as subp:
        text2 = subp.stdout.read(-1)

        subp.wait()

        self.log.debug("test {id}: gunzip [{text2}]".format(id=unittest.TestCase.id(self), text2=text2))

        # gunzip successfully reproduced uncompressed string
        self.assertEqual(text2, text)
        self.assertEqual(subp.returncode, 0)

def test_read_aux(self,
                  ioclass : io.IOBase,
                  text : str | bytes,
                  fname : os.PathLike):
    """
    Test read() on behalf of compressed stream
    {Test_ZstreamBase, Test_BufferedZstream, Test_TextZstream}

    Test will create a compressed file,  then make
    one call to .read() method on an instance of ioclass,

    Args:
      self (unittest.TestCase).   Test harness class invoking this function
      ioclass (io.IOBase).        The i/o class to be tested.
                                  (ZstreamBase | BufferedZstream | TextZstream)
      text (str).                 content to write to (compressed) file.
                                  if string, must be utf-8 (for length test).
                                  Test fails is len(text) > 16384.
      fname (str).                Filename to use for (empty) compressed stream
    """
    from pyzstream import openmode
    from zstream import ZstreamBase

    self.log.debug("test_read_aux: enter")

    zs = ZstreamBase(fname, openmode.output)

    n = len(text)

    zs.write(text)

    self.assertEqual(zs.tell(), n) # total #of uncompressed chars output

    zs.close()

    # now read from lorem2.gz

    zs = ioclass(fname, openmode.input)

    # zs.read(-1) not supported yet
    s2 = zs.read(16384)

    self.assertEqual(s2, text)

