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

import unittest
import struct

from rsa._compat import is_bytes, byte


class TestByte(unittest.TestCase):
    def test_byte(self):
        for i in range(256):
            byt = byte(i)
            self.assertTrue(is_bytes(byt))
            self.assertEqual(ord(byt), i)

    def test_raises_StructError_on_overflow(self):
        self.assertRaises(struct.error, byte, 256)
        self.assertRaises(struct.error, byte, -1)
