# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import mock
import pytz
import unittest

from infra_libs.time_functions.testing import mock_datetime_utc, mock_timezone

original_datetime = datetime.datetime

class TestFreezeTime(unittest.TestCase):
  @mock_datetime_utc(2015, 11, 24, 7, 18, 23)
  @mock_timezone('US/Pacific')
  def test_mocks_datetime_and_timezone(self):
    self.assertEqual(datetime.datetime.utcnow(),
                     datetime.datetime(2015, 11, 24, 7, 18, 23))
    self.assertEqual(datetime.datetime.now(),
                     datetime.datetime(2015, 11, 23, 23, 18, 23))
    self.assertEqual(datetime.datetime.now(tz=pytz.timezone('Europe/Berlin')),
                     datetime.datetime(2015, 11, 24, 8, 18, 23))
    self.assertEqual(datetime.datetime.today(), datetime.date(2015, 11, 23))
    self.assertEqual(datetime.date.today(), datetime.date(2015, 11, 23))
    self.assertEqual(datetime.datetime.fromtimestamp(0),
                     datetime.datetime(1969, 12, 31, 16, 0, 0))
    self.assertEqual(
        datetime.datetime.fromtimestamp(0, tz=pytz.timezone('Europe/Berlin')),
        datetime.datetime(1970, 1, 1, 1, 0, 0))
    self.assertEqual(datetime.date.fromtimestamp(0),
                     datetime.date(1969, 12, 31))

    # Try accessing standard class attributes and methods.
    self.assertEqual(datetime.datetime.min, original_datetime.min)
    self.assertEqual(datetime.datetime.utcfromtimestamp(0),
                     datetime.datetime(1970, 1, 1, 0, 0, 0))

    # Check that creating normal datetimes and working with them works.
    dt = datetime.datetime(2015, 10, 1, 1, 1, 1)
    dt += datetime.timedelta(hours=2)
    self.assertEqual(dt, datetime.datetime(2015, 10, 1, 3, 1, 1))
    d = datetime.date(2015, 10, 1)
    d += datetime.timedelta(days=5)
    self.assertEqual(d, datetime.date(2015, 10, 6))

    # Check that isinstance method works as expected.
    dt = datetime.datetime.utcnow()  # dt is _MockDateTime
    self.assertTrue(isinstance(dt, datetime.datetime))
    dt += datetime.timedelta(hours=1)  # dt is vanilla datatime.datetime
    self.assertTrue(isinstance(dt, datetime.datetime))
    d = datetime.datetime.today()  # d is _MockDate
    self.assertTrue(isinstance(d, datetime.date))
    d += datetime.timedelta(days=1)  # d is vanilla datatime.date
    self.assertTrue(isinstance(d, datetime.date))


  @mock_datetime_utc(2015, 11, 24, 7, 0, 0)
  def test_nested_mock_works(self):
    self.assertEqual(datetime.datetime.utcnow().hour, 7)

    @mock_datetime_utc(2015, 11, 24, 10, 0, 0)
    def nested_func():
      self.assertEqual(datetime.datetime.utcnow().hour, 10)
    nested_func()

    self.assertEqual(datetime.datetime.utcnow().hour, 7)

  @mock_datetime_utc(2015, 11, 24, 9, 10, 15)
  @mock_timezone('US/Pacific')
  def test_winter_time_is_computed_correctly(self):
    self.assertEqual(datetime.datetime.now(),
                     datetime.datetime(2015, 11, 24, 1, 10, 15))

  @mock_datetime_utc(2015, 8, 24, 9, 10, 15)
  @mock_timezone('US/Pacific')
  def test_summer_time_is_computed_correctly(self):
    self.assertEqual(datetime.datetime.now(),
                     datetime.datetime(2015, 8, 24, 2, 10, 15))
