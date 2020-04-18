#!/usr/bin/env python
# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import itertools
import os
import sys
import unittest
import StringIO

ROOT_DIR = os.path.dirname(os.path.abspath(os.path.join(
    __file__.decode(sys.getfilesystemencoding()),
    os.pardir, os.pardir, os.pardir)))
sys.path.insert(0, ROOT_DIR)

from libs.logdog import varint


class VarintTestCase(unittest.TestCase):

  def testVarintEncodingRaw(self):
    for base, exp in (
        (0, b'\x00'),
        (1, b'\x01'),
        (0x7F, b'\x7f'),
        (0x80, b'\x80\x01'),
        (0x81, b'\x81\x01'),
        (0x18080, b'\x80\x81\x06'),
    ):
      sio = StringIO.StringIO()
      count = varint.write_uvarint(sio, base)
      act = sio.getvalue()

      self.assertEqual(act, exp,
          "Encoding for %d (%r) doesn't match expected (%r)" % (base, act, exp))
      self.assertEqual(count, len(act),
          "Length of %d (%d) doesn't match encoded length (%d)" % (
              base, len(act), count))

  def testVarintEncodeDecode(self):
    seed = (b'\x00', b'\x01', b'\x55', b'\x7F', b'\x80', b'\x81', b'\xff')
    for perm in itertools.permutations(seed):
      perm = ''.join(perm).encode('hex')

      while len(perm) > 0:
        exp = int(perm.encode('hex'), 16)

        sio = StringIO.StringIO()
        count = varint.write_uvarint(sio, exp)
        sio.seek(0)
        act, count = varint.read_uvarint(sio)

        self.assertEqual(act, exp,
            "Decoded %r (%d) doesn't match expected (%d)" % (
                sio.getvalue().encode('hex'), act, exp))
        self.assertEqual(count, len(sio.getvalue()),
            "Decoded length (%d) doesn't match expected (%d)" % (
                count, len(sio.getvalue())))

        if perm == 0:
          break
        perm = perm[1:]


if __name__ == '__main__':
  unittest.main()
