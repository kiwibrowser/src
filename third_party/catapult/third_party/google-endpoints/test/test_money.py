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

import sys
import unittest2
from expects import expect, equal, raise_error

from google.api.control import money, messages


class TestCheckValid(unittest2.TestCase):
    _BAD_CURRENCY = messages.Money(currencyCode='this-is-bad')
    _MISMATCHED_UNITS = (
        messages.Money(currencyCode='JPY', units=-1, nanos=1),
        messages.Money(currencyCode='JPY', units=1, nanos=-1),
    )
    _NANOS_OOB = messages.Money(currencyCode='EUR', units=0, nanos=9999999999)
    _OK = (
        messages.Money(currencyCode='JPY', units=1, nanos=1),
        messages.Money(currencyCode='JPY', units=-1, nanos=-1),
        messages.Money(currencyCode='EUR', units=0, nanos=money.MAX_NANOS),
    )

    def test_should_fail_if_not_really_money(self):
        expect(lambda: money.check_valid(object())).to(raise_error(ValueError))
        expect(lambda: money.check_valid(None)).to(raise_error(ValueError))

    def test_should_fail_when_no_currency_is_set(self):
        expect(lambda: money.check_valid(messages.Money())).to(
            raise_error(ValueError))

    def test_should_fail_when_the_currency_is_bad(self):
        expect(lambda: money.check_valid(self._BAD_CURRENCY)).to(
            raise_error(ValueError))

    def test_should_fail_when_the_units_and_nanos_are_mismatched(self):
        for m in self._MISMATCHED_UNITS:
            expect(lambda: money.check_valid(m)).to(raise_error(ValueError))

    def test_should_fail_when_nanos_are_oob(self):
        expect(lambda: money.check_valid(self._NANOS_OOB)).to(
            raise_error(ValueError))

    def test_should_succeed_for_ok_instances(self):
        for m in self._OK:
            money.check_valid(m)


class TestAdd(unittest2.TestCase):
    _SOME_YEN = messages.Money(currencyCode='JPY', units=3, nanos=0)
    _SOME_YEN_DEBT = messages.Money(currencyCode='JPY', units=-2, nanos=-1)
    _SOME_MORE_YEN = messages.Money(currencyCode='JPY', units=1, nanos=3)
    _SOME_USD = messages.Money(currencyCode='USD', units=1, nanos=0)
    _INT64_MAX = sys.maxint
    _INT64_MIN = -sys.maxint - 1
    _LARGE_YEN = messages.Money(currencyCode='JPY', units=_INT64_MAX -1, nanos=0)
    _LARGE_YEN_DEBT = messages.Money(currencyCode='JPY', units=-_INT64_MAX + 1, nanos=0)

    def test_should_fail_if_non_money_is_used(self):
        testfs = [
            lambda: money.add(self._SOME_YEN, object()),
            lambda: money.add(object(), self._SOME_USD),
            lambda: money.add(None, self._SOME_USD),
            lambda: money.add(self._SOME_YEN, None),
        ]
        for testf in testfs:
            expect(testf).to(raise_error(ValueError))

    def test_should_fail_on_currency_mismatch(self):
        testf = lambda: money.add(self._SOME_YEN, self._SOME_USD)
        expect(testf).to(raise_error(ValueError))

    def test_should_fail_on_unallowed_positive_overflows(self):
        testf = lambda: money.add(self._SOME_YEN, self._LARGE_YEN)
        expect(testf).to(raise_error(OverflowError))

    def test_should_allow_positive_overflows(self):
        overflowing = money.add(self._SOME_YEN, self._LARGE_YEN,
                                allow_overflow=True)
        expect(overflowing.units).to(equal(self._INT64_MAX))
        expect(overflowing.nanos).to(equal(money.MAX_NANOS))

    def test_should_fail_on_unallowed_negative_overflows(self):
        testf = lambda: money.add(self._SOME_YEN_DEBT, self._LARGE_YEN_DEBT)
        expect(testf).to(raise_error(OverflowError))

    def test_should_allow_negative_overflows(self):
        overflowing = money.add(self._SOME_YEN_DEBT, self._LARGE_YEN_DEBT,
                                allow_overflow=True)
        expect(overflowing.units).to(equal(self._INT64_MIN))
        expect(overflowing.nanos).to(equal(-money.MAX_NANOS))

    def test_should_add_ok_when_nanos_have_same_sign(self):
        the_sum = money.add(self._SOME_YEN, self._SOME_YEN)
        expect(the_sum.units).to(equal(2 * self._SOME_YEN.units))

    def test_should_add_ok_when_nanos_have_different_signs(self):
        the_sum = money.add(self._SOME_YEN, self._SOME_YEN_DEBT)
        want_units = self._SOME_YEN_DEBT.units + self._SOME_YEN.units - 1
        expect(the_sum.units).to(equal(want_units))
        expect(the_sum.nanos).to(equal(money.MAX_NANOS))
        the_sum = money.add(self._SOME_MORE_YEN, self._SOME_YEN_DEBT)
        want_units = self._SOME_YEN_DEBT.units + self._SOME_YEN.units - 1
        expect(the_sum.units).to(equal(want_units))
        expect(the_sum.nanos).to(equal(1 - money.MAX_NANOS))
