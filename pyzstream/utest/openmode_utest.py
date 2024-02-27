import pyzstream
import unittest

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

