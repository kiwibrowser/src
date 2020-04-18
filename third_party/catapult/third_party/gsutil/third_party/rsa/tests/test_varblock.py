# -*- coding: utf-8 -*-
#
#  Copyright 2011 Sybren A. Stüvel <sybren@stuvel.eu>
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

"""Tests varblock operations."""

try:
    from StringIO import StringIO as BytesIO
except ImportError:
    from io import BytesIO
import unittest

from rsa._compat import b
from rsa import varblock


class VarintTest(unittest.TestCase):
    def test_read_varint(self):
        encoded = b('\xac\x02crummy')
        infile = BytesIO(encoded)

        (decoded, read) = varblock.read_varint(infile)

        # Test the returned values
        self.assertEqual(300, decoded)
        self.assertEqual(2, read)

        # The rest of the file should be untouched
        self.assertEqual(b('crummy'), infile.read())

    def test_read_zero(self):
        encoded = b('\x00crummy')
        infile = BytesIO(encoded)

        (decoded, read) = varblock.read_varint(infile)

        # Test the returned values
        self.assertEqual(0, decoded)
        self.assertEqual(1, read)

        # The rest of the file should be untouched
        self.assertEqual(b('crummy'), infile.read())

    def test_write_varint(self):
        expected = b('\xac\x02')
        outfile = BytesIO()

        written = varblock.write_varint(outfile, 300)

        # Test the returned values
        self.assertEqual(expected, outfile.getvalue())
        self.assertEqual(2, written)

    def test_write_zero(self):
        outfile = BytesIO()
        written = varblock.write_varint(outfile, 0)

        # Test the returned values
        self.assertEqual(b('\x00'), outfile.getvalue())
        self.assertEqual(1, written)


class VarblockTest(unittest.TestCase):
    def test_yield_varblock(self):
        infile = BytesIO(b('\x01\x0512345\x06Sybren'))

        varblocks = list(varblock.yield_varblocks(infile))
        self.assertEqual([b('12345'), b('Sybren')], varblocks)


class FixedblockTest(unittest.TestCase):
    def test_yield_fixedblock(self):
        infile = BytesIO(b('123456Sybren'))

        fixedblocks = list(varblock.yield_fixedblocks(infile, 6))
        self.assertEqual([b('123456'), b('Sybren')], fixedblocks)
