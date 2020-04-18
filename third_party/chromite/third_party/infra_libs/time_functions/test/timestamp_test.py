# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
from pytz import timezone
import unittest

from infra_libs.time_functions import timestamp


class TestTimestamps(unittest.TestCase):
  def testTimestamp(self):
    sec_diff = 1000
    my_date = datetime.datetime.utcfromtimestamp(sec_diff)
    self.assertEqual(timestamp.utctimestamp(my_date), float(sec_diff))

  def testTimeZoneTimestamp(self):
    sec_diff = 1000
    my_date = datetime.datetime.utcfromtimestamp(sec_diff)
    my_date = my_date.replace(tzinfo=timezone('Etc/GMT-8'))

    pst_diff = sec_diff - (8 * 3600.0)
    self.assertEqual(timestamp.utctimestamp(my_date), float(pst_diff))
