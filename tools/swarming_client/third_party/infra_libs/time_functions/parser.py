# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility functions to parse time/duration arguments."""


import argparse
import datetime
import re


_TIMEDELTA_CONV = {
  'us': datetime.timedelta(microseconds=1),
  'ms': datetime.timedelta(milliseconds=1),
  's': datetime.timedelta(seconds=1),
  'm': datetime.timedelta(minutes=1),
  'h': datetime.timedelta(hours=1),
  'd': datetime.timedelta(days=1),
}
_TIMEDELTA_RE = re.compile(r'(\d+)(\w+)')


def parse_timedelta(v):
  """Returns (datetime.timedelta) The parsed timedelta.

  Args:
    v (str): The time delta string, a comma-delimited set of <count><unit>
        tokens comprising the timedelta (e.g., 10d,2h is 10 days, 2 hours).

  Raises:
    ValueError: If parsing failed.
  """
  result = datetime.timedelta()
  for comp in v.split(','):
    match = _TIMEDELTA_RE.match(comp)
    if match is None:
      raise ValueError('Invalid timedelta token (%s)' % (comp,))
    count, unit = int(match.group(1)), match.group(2)
    unit_value = _TIMEDELTA_CONV.get(unit)
    if unit_value is None:
      raise ValueError('Invalid timedelta token unit (%s)' % (unit,))
    result += (unit_value * count)
  return result


def argparse_timedelta_type(v):
  """Returns (datetime.timedelta) The parsed timedelta.

  This is an argparse-compatible version of `parse_timedelta` that raises an
  argparse.ArgumentTypeError on failure instead of a ValueError.

  Raises:
    argparse.ArgumentTypeError: If parsing failed.
  """
  try:
    return parse_timedelta(v)
  except ValueError as e:
    raise argparse.ArgumentTypeError(e.message)
