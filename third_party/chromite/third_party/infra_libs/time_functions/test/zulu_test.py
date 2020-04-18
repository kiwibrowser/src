# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
from pytz import timezone
import unittest

from infra_libs.time_functions import zulu


class TestZuluTime(unittest.TestCase):
  def _assert_parses_to(self, timestring, utc_ts_equivalent):
    zuluparse = zulu.parse_zulu_ts(timestring)
    self.assertIsInstance(zuluparse, float)
    self.assertEqual(zuluparse, utc_ts_equivalent)


  def testParseNonFractional(self):
    timestring = '2015-06-11T23:17:26Z'
    utc_ts_equivalent = 1434064646.0
    self._assert_parses_to(timestring, utc_ts_equivalent)

  def testParseFractional(self):
    timestring = '2015-06-11T23:17:26.5Z'
    utc_ts_equivalent = 1434064646.5
    self._assert_parses_to(timestring, utc_ts_equivalent)

  def testInvalidParse(self):
    timestring = '2015-06-11T23:YOLO:17:26.5Z'
    zuluparse = zulu.parse_zulu_ts(timestring)
    self.assertIsNone(zuluparse)

  def testNaiveDTZuluStringNonFractional(self):
    dt = datetime.datetime(2015, 06, 11, 23, 17, 26)
    timestring = '2015-06-11T23:17:26.0Z'
    self.assertEqual(zulu.to_zulu_string(dt), timestring)

  def testNaiveDTZuluStringFractional(self):
    dt = datetime.datetime(2015, 06, 11, 23, 17, 26, 123)
    timestring = '2015-06-11T23:17:26.000123Z'
    self.assertEqual(zulu.to_zulu_string(dt), timestring)

  def testTZAwareDTZuluString(self):
    # If you're confused why GMT+8 is -08:00, see
    # http://askubuntu.com/questions/519550/
    # why-is-the-8-timezone-called-gmt-8-in-the-filesystem
    dt = datetime.datetime(2015, 06, 11, 10, 17, 26, 123,
                           tzinfo=timezone('Etc/GMT+8'))
    timestring = '2015-06-11T18:17:26.000123Z'
    self.assertEqual(zulu.to_zulu_string(dt), timestring)
