# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Implements an integer set compression algorithm based on delta
encoded varints, which is then deflate'd.

The algorithm is intentionally simple.

This only works with sorted list of integers. The resulting compression level
can be very high for monotonically increasing sets.
"""

import cStringIO
import zlib


def pack(values):
  """Returns a deflate'd buffer of delta encoded varints.

  Arguments:
    values: sorted list of int.

  Returns:
    compressed buffer as a str.
  """
  out = ''
  if not values:
    return ''
  last = 0
  max_value = 2L**63
  assert 0 <= values[0] < max_value, 'Values must be between 0 and 2**63'
  assert 0 <= values[-1] < max_value, 'Values must be between 0 and 2**63'
  for value in values:
    v = value
    value -= last
    assert value >= 0, 'List must be sorted ascending'
    last = v
    while value > 127:
      out += chr((1 << 7) | (value & 0x7F))
      value >>=  7
    out += chr(value)
  return zlib.compress(out)


def unpack(data):
  """Decompresses a deflate'd delta encoded list of varints.

  Arguments:
    compressed buffer as a str. Accepts None to simplify call sites.

  Returns:
    values: sorted list of int.
  """
  out = []
  if data == '':
    return out
  value = 0
  base = 1
  last = 0
  for d in zlib.decompress(data):
    val_byte = ord(d)
    value += (val_byte & 0x7F) * base
    if val_byte & 0x80:
      base <<= 7
    else:
      out.append(value + last)
      last += value
      value = 0
      base = 1
  return out
