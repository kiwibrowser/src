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

import collections
import datetime
import unittest2

from expects import be, be_a, be_none, equal, expect, raise_error

from google.api.control import caches, report_request


_TEST_NUM_ENTRIES = 3  # arbitrary


class TestDequeOutLRUCache(unittest2.TestCase):

    def test_constructor_should_set_up_a_default_deque(self):
        c = caches.DequeOutLRUCache(_TEST_NUM_ENTRIES)
        expect(c.out_deque).to(be_a(collections.deque))

    def test_constructor_should_fail_on_bad_deques(self):
        testf = lambda: caches.DequeOutLRUCache(_TEST_NUM_ENTRIES,
                                                out_deque=object())
        expect(testf).to(raise_error(ValueError))

    def test_constructor_should_accept_deques(self):
        a_deque = collections.deque()
        c = caches.DequeOutLRUCache(_TEST_NUM_ENTRIES, out_deque=a_deque)
        expect(c.out_deque).to(be(a_deque))

    def test_lru(self):
        lru_limit = 2
        cache = caches.DequeOutLRUCache(lru_limit)
        cache[1] = 1
        cache[2] = 2
        cache[3] = 3
        expect(len(cache)).to(equal(2))
        expect(cache[2]).to(equal(2))
        expect(cache[3]).to(equal(3))
        expect(cache.get(1)).to(be_none)
        expect(len(cache.out_deque)).to(be(1))
        cache[4] = 4
        expect(cache.get(2)).to(be_none)
        expect(len(cache.out_deque)).to(be(2))


class _Timer(object):
    def __init__(self, auto=False):
        self.auto = auto
        self.time = 0

    def __call__(self):
        if self.auto:
            self.tick()
        return self.time

    def tick(self):
        self.time += 1


_TEST_TTL = 3  # arbitrary


class TestDequeOutTTLCache(unittest2.TestCase):
    # pylint: disable=fixme
    #
    # TODO: add a ttl test based on the one in cachetools testsuite

    def test_constructor_should_set_up_a_default_deque(self):
        c = caches.DequeOutTTLCache(_TEST_NUM_ENTRIES, _TEST_TTL)
        expect(c.out_deque).to(be_a(collections.deque))

    def test_constructor_should_fail_on_bad_deques(self):
        testf = lambda: caches.DequeOutTTLCache(_TEST_NUM_ENTRIES, _TEST_TTL,
                                                out_deque=object())
        expect(testf).to(raise_error(ValueError))

    def test_constructor_should_accept_deques(self):
        a_deque = collections.deque()
        c = caches.DequeOutTTLCache(3, 3, out_deque=a_deque)
        expect(c.out_deque).to(be(a_deque))

    def test_lru(self):
        lru_limit = 2
        expiry = 100
        cache = caches.DequeOutTTLCache(lru_limit, expiry)
        cache[1] = 1
        cache[2] = 2
        cache[3] = 3
        expect(len(cache)).to(equal(2))
        expect(cache[2]).to(equal(2))
        expect(cache[3]).to(equal(3))
        expect(cache.get(1)).to(be_none)
        expect(len(cache.out_deque)).to(be(1))
        cache[4] = 4
        expect(cache.get(2)).to(be_none)
        expect(len(cache.out_deque)).to(be(2))

    def test_ttl(self):
        cache = caches.DequeOutTTLCache(2, ttl=1, timer=_Timer())
        expect(cache.timer()).to(equal(0))
        expect(cache.ttl).to(equal(1))

        cache[1] = 1
        expect(set(cache)).to(equal({1}))
        expect(len(cache)).to(equal(1))
        expect(cache[1]).to(equal(1))

        cache.timer.tick()
        expect(set(cache)).to(equal({1}))
        expect(len(cache)).to(equal(1))
        expect(cache[1]).to(equal(1))

        cache[2] = 2
        expect(set(cache)).to(equal({1, 2}))
        expect(len(cache)).to(equal(2))
        expect(cache[1]).to(equal(1))
        expect(cache[2]).to(equal(2))

        cache.timer.tick()
        expect(set(cache)).to(equal({2}))
        expect(len(cache)).to(equal(1))
        expect(cache[2]).to(equal(2))
        expect(cache.get(1)).to(be_none)


class _DateTimeTimer(object):
    def __init__(self, auto=False):
        self.auto = auto
        self.time = datetime.datetime(1970, 1, 1)

    def __call__(self):
        if self.auto:
            self.tick()
        return self.time

    def tick(self):
        self.time += datetime.timedelta(seconds=1)


class TestCreate(unittest2.TestCase):

    def test_should_fail_if_bad_options_are_used(self):
        should_fail = [
            lambda: caches.create(object()),
        ]
        for testf in should_fail:
            expect(testf).to(raise_error(ValueError))

    def test_should_return_none_if_options_is_none(self):
        expect(caches.create(None)).to(be_none)

    def test_should_return_none_if_cache_size_not_positive(self):
        should_be_none = [
            lambda: caches.create(caches.CheckOptions(num_entries=0)),
            lambda: caches.create(caches.CheckOptions(num_entries=-1)),
            lambda: caches.create(caches.ReportOptions(num_entries=0)),
            lambda: caches.create(caches.ReportOptions(num_entries=-1)),
        ]
        for testf in should_be_none:
            expect(testf()).to(be_none)

    def test_should_return_ttl_cache_if_flush_interval_is_positive(self):
        delta = datetime.timedelta(seconds=1)
        should_be_ttl = [
            lambda timer: caches.create(
                caches.CheckOptions(num_entries=1, flush_interval=delta),
                timer=timer
            ),
            lambda timer: caches.create(
                caches.ReportOptions(num_entries=1, flush_interval=delta),
                timer=timer
            ),
        ]
        for testf in should_be_ttl:
            timer = _DateTimeTimer()
            sync_cache = testf(timer)
            expect(sync_cache).to(be_a(caches.LockedObject))
            with sync_cache as cache:
                expect(cache).to(be_a(caches.DequeOutTTLCache))
                expect(cache.timer()).to(equal(0))
                cache[1] = 1
                expect(set(cache)).to(equal({1}))
                expect(cache.get(1)).to(equal(1))
                timer.tick()
                expect(cache.get(1)).to(equal(1))
                timer.tick()
                expect(cache.get(1)).to(be_none)

            # Is still TTL without the custom timer
            sync_cache = testf(None)
            expect(sync_cache).to(be_a(caches.LockedObject))
            with sync_cache as cache:
                expect(cache).to(be_a(caches.DequeOutTTLCache))

    def test_should_return_a_lru_cache_if_flush_interval_is_negative(self):
        delta = datetime.timedelta(seconds=-1)
        should_be_ttl = [
            lambda: caches.create(
                caches.CheckOptions(num_entries=1, flush_interval=delta),
            ),
            lambda: caches.create(
                caches.ReportOptions(num_entries=1, flush_interval=delta)),
        ]
        for testf in should_be_ttl:
            sync_cache = testf()
            expect(sync_cache).to(be_a(caches.LockedObject))
            with sync_cache as cache:
                expect(cache).to(be_a(caches.DequeOutLRUCache))


class TestReportOptions(unittest2.TestCase):

    def test_should_create_with_defaults(self):
        options = caches.ReportOptions()
        expect(options.num_entries).to(equal(
            caches.ReportOptions.DEFAULT_NUM_ENTRIES))
        expect(options.flush_interval).to(equal(
            caches.ReportOptions.DEFAULT_FLUSH_INTERVAL))


class TestCheckOptions(unittest2.TestCase):
    AN_INTERVAL = datetime.timedelta(milliseconds=2)
    A_LOWER_INTERVAL = datetime.timedelta(milliseconds=1)

    def test_should_create_with_defaults(self):
        options = caches.CheckOptions()
        expect(options.num_entries).to(equal(
            caches.CheckOptions.DEFAULT_NUM_ENTRIES))
        expect(options.flush_interval).to(equal(
            caches.CheckOptions.DEFAULT_FLUSH_INTERVAL))
        expect(options.expiration).to(equal(
            caches.CheckOptions.DEFAULT_EXPIRATION))

    def test_should_ignores_lower_expiration(self):
        wanted_expiration = (
            self.AN_INTERVAL + datetime.timedelta(milliseconds=1))
        options = caches.CheckOptions(flush_interval=self.AN_INTERVAL,
                                      expiration=self.A_LOWER_INTERVAL)
        expect(options.flush_interval).to(equal(self.AN_INTERVAL))
        expect(options.expiration).to(equal(wanted_expiration))
        expect(options.expiration).not_to(equal(self.A_LOWER_INTERVAL))
