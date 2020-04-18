# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import absolute_import

import datetime

import unittest2
from expects import be_below_or_equal, expect, equal, raise_error

from google.api.control import timestamp


class TestToRfc3339(unittest2.TestCase):
    A_LONG_TIME_AGO = datetime.datetime(1971, 12, 31, 21, 0, 20, 21000)
    TESTS = [
        (A_LONG_TIME_AGO, '1971-12-31T21:00:20.021Z'),
        (A_LONG_TIME_AGO - datetime.datetime(1970, 1, 1),
         '1971-12-31T21:00:20.021Z')
    ]

    def test_should_converts_correctly(self):
        for t in self.TESTS:
            expect(timestamp.to_rfc3339(t[0])).to(equal(t[1]))

    def test_should_fail_on_invalid_input(self):
        testf = lambda: timestamp.to_rfc3339('this will not work')
        expect(testf).to(raise_error(ValueError))


class TestFromRfc3339(unittest2.TestCase):
    TOLERANCE = 10000  # 1e-5 * 1e9
    TESTS = [
        # Simple
        ('1971-12-31T21:00:20.021Z',
         datetime.datetime(1971, 12, 31, 21, 0, 20, 21000)),
        # different timezone
        ('1996-12-19T16:39:57-08:00',
         datetime.datetime(1996, 12, 20, 0, 39, 57, 0)),
        # microseconds
        ('1996-12-19T16:39:57.123456-08:00',
         datetime.datetime(1996, 12, 20, 0, 39, 57, 123456)),
        # Beyond 2038
        ('2100-01-01T00:00:00Z',
         datetime.datetime(2100, 01, 01, 0, 0, 0, 0))
    ]

    NANO_TESTS = [
        # Simple
        ('1971-12-31T21:00:20.021Z',
         (datetime.datetime(1971, 12, 31, 21, 0, 20, 21000), 21000000)),
        # different timezone
        ('1996-12-19T16:39:57-08:00',
         (datetime.datetime(1996, 12, 20, 0, 39, 57, 0), 0)),
        # microseconds
        ('1996-12-19T16:39:57.123456789-08:00',
         (datetime.datetime(1996, 12, 20, 0, 39, 57, 123457), 123456789)),
    ]

    def test_should_convert_correctly_without_nanos(self):
        for t in self.TESTS:
            expect(timestamp.from_rfc3339(t[0])).to(equal(t[1]))

    def test_should_convert_correctly_with_nanos(self):
        for t in self.NANO_TESTS:
            dt, nanos = timestamp.from_rfc3339(t[0], with_nanos=True)
            expect(dt).to(equal(t[1][0]))
            epsilon = abs(nanos - t[1][1])
            # expect(epsilon).to(equal(0))
            expect(epsilon).to(be_below_or_equal(self.TOLERANCE))


class TestCompare(unittest2.TestCase):
    TESTS = [
        # Strings
        ('1971-10-31T21:00:20.021Z', '1971-11-30T21:00:20.021Z', -1),
        ('1971-11-30T21:00:20.021Z', '1971-10-30T21:00:20.021Z', 1),
        ('1971-11-30T21:00:20Z', '1971-11-30T21:00:20Z', 0),
        ('1971-11-30T21:00:20.021Z', '1971-11-30T21:00:20.041Z', -1),
        ('1971-11-30T21:00:20.021Z', '1971-11-30T21:00:20.001Z', 1),
        # Datetimes
        (datetime.datetime(1996, 10, 20, 0, 39, 57, 0),
         datetime.datetime(1996, 11, 20, 0, 39, 57, 0),
         -1),
        (datetime.datetime(1996, 10, 20, 0, 39, 57, 0),
         datetime.datetime(1996, 10, 20, 0, 39, 57, 0),
         0),
        (datetime.datetime(1996, 11, 20, 0, 39, 57, 0),
         datetime.datetime(1996, 10, 20, 0, 39, 57, 0),
         1)
    ]

    def test_should_compare_correctly(self):
        for t in self.TESTS:
            a, b, want = t
            expect(timestamp.compare(a, b)).to(equal(want))

    def test_should_fail_if_inputs_do_not_have_the_same_type(self):
        testf = lambda: timestamp.compare(self.TESTS[0][0],
                                          datetime.datetime.utcnow())
        expect(testf).to(raise_error(ValueError))
        testf = lambda: timestamp.compare(self.TESTS[0],
                                          datetime.datetime.utcnow())
        expect(testf).to(raise_error(ValueError))
