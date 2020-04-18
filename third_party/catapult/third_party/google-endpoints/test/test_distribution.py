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

from google.api.control import distribution, messages


class TestCreateExponential(unittest2.TestCase):

    def test_should_fail_if_num_finite_buckets_is_bad(self):
        testf = lambda: distribution.create_exponential(0, 1.1, 0.1)
        expect(testf).to(raise_error(ValueError))

    def test_should_fail_if_growth_factor_is_bad(self):
        testf = lambda: distribution.create_exponential(1, 0.9, 0.1)
        expect(testf).to(raise_error(ValueError))

    def test_should_fail_if_scale_is_bad(self):
        testf = lambda: distribution.create_exponential(1, 1.1, -0.1)
        expect(testf).to(raise_error(ValueError))

    def test_should_succeed_if_inputs_are_ok(self):
        num_finite_buckets = 1
        got = distribution.create_exponential(num_finite_buckets, 1.1, 0.1)
        expect(len(got.bucketCounts)).to(equal(num_finite_buckets + 2))


class TestCreateLinear(unittest2.TestCase):

    def test_should_fail_if_num_finite_buckets_is_bad(self):
        testf = lambda: distribution.create_linear(0, 1.1, 0.1)
        expect(testf).to(raise_error(ValueError))

    def test_should_fail_if_growth_factor_is_bad(self):
        testf = lambda: distribution.create_linear(1, -0.1, 0.1)
        expect(testf).to(raise_error(ValueError))

    def test_should_succeed_if_inputs_are_ok(self):
        num_finite_buckets = 1
        got = distribution.create_linear(num_finite_buckets, 0.1, 0.1)
        expect(len(got.bucketCounts)).to(equal(num_finite_buckets + 2))


class TestCreateExplicit(unittest2.TestCase):

    def test_should_fail_if_there_are_matching_bounds(self):
        testf = lambda: distribution.create_explicit([0.0, 0.1, 0.1])
        expect(testf).to(raise_error(ValueError))

    def test_should_succeed_if_inputs_are_ok(self):
        want = [0.1, 0.2, 0.3]
        got = distribution.create_explicit([0.1, 0.2, 0.3])
        expect(got.explicitBuckets.bounds).to(equal(want))
        expect(len(got.bucketCounts)).to(equal(len(want) + 1))

    def test_should_succeed_if_input_bounds_are_unsorted(self):
        want = [0.1, 0.2, 0.3]
        got = distribution.create_explicit([0.3, 0.1, 0.2])
        expect(got.explicitBuckets.bounds).to(equal(want))


def _make_explicit_dist():
    return distribution.create_explicit([0.1, 0.3, 0.5, 0.7])


def _make_linear_dist():
    return distribution.create_linear(3, 0.2, 0.1)


def _make_exponential_dist():
    return distribution.create_exponential(3, 2, 0.1)

_UNDERFLOW_SAMPLE = 1e-5
_LOW_SAMPLE = 0.11
_HIGH_SAMPLE = 0.5
_OVERFLOW_SAMPLE = 1e5

_TEST_SAMPLES_AND_BUCKETS = [
    {
        'samples': [_UNDERFLOW_SAMPLE],
        'want': [1, 0, 0, 0, 0]
    },
    {
        'samples': [_LOW_SAMPLE] * 2,
        'want': [0, 2, 0, 0, 0]
    },
    {
        'samples': [_LOW_SAMPLE, _HIGH_SAMPLE, _HIGH_SAMPLE],
        'want': [0, 1, 0, 2, 0]
    },
    {
        'samples': [_OVERFLOW_SAMPLE],
        'want': [0, 0, 0, 0, 1]
    },
]


def _expect_stats_eq_direct_calc_from_samples(d, samples):
    # pylint: disable=fixme
    # TODO: update this the sum of rho-squared
    want_mean = sum(samples) / len(samples)
    expect(d.mean).to(equal(want_mean))
    expect(d.maximum).to(equal(max(samples)))
    expect(d.minimum).to(equal(min(samples)))


class TestAddSample(unittest2.TestCase):
    NOTHING_SET = messages.Distribution()

    def test_should_fail_if_no_buckets_are_set(self):
        testf = lambda: distribution.add_sample(_UNDERFLOW_SAMPLE,
                                                self.NOTHING_SET)
        expect(testf).to(raise_error(ValueError))

    def expect_adds_test_samples_ok(self, make_dist_func):
        for t in _TEST_SAMPLES_AND_BUCKETS:
            d = make_dist_func()
            samples = t['samples']
            for s in samples:
                distribution.add_sample(s, d)
            expect(d.bucketCounts).to(equal(t['want']))
            _expect_stats_eq_direct_calc_from_samples(d, samples)

    def test_update_explict_buckets_ok(self):
        self.expect_adds_test_samples_ok(_make_explicit_dist)

    def test_update_exponential_buckets_ok(self):
        self.expect_adds_test_samples_ok(_make_exponential_dist)

    def test_update_linear_buckets_ok(self):
        self.expect_adds_test_samples_ok(_make_linear_dist)


class TestMerge(unittest2.TestCase):

    def setUp(self):
        self.merge_triples = (
            (
                distribution.create_exponential(3, 2, 0.1),
                distribution.create_exponential(3, 2, 0.1),
                distribution.create_exponential(4, 2, 0.1),
            ),(
                distribution.create_linear(3, 0.2, 0.1),
                distribution.create_linear(3, 0.2, 0.1),
                distribution.create_linear(4, 0.2, 0.1)
            ),(
                distribution.create_explicit([0.1, 0.3]),
                distribution.create_explicit([0.1, 0.3]),
                distribution.create_explicit([0.1, 0.3, 0.5]),
            )
        )
        for d1, d2, _ in self.merge_triples:
            distribution.add_sample(_LOW_SAMPLE, d1)
            distribution.add_sample(_HIGH_SAMPLE, d2)

    def test_should_fail_on_dissimilar_bucket_options(self):
        explicit = _make_explicit_dist()
        linear = _make_linear_dist()
        exponential = _make_exponential_dist()
        pairs = (
            (explicit, linear),
            (explicit, exponential),
            (linear, exponential)
        )
        for p in pairs:
            testf = lambda: distribution.merge(*p)
            expect(testf).to(raise_error(ValueError))

    def test_should_fail_on_dissimilar_bucket_counts(self):
        for _, d2, d3 in self.merge_triples:
            testf = lambda: distribution.merge(d2, d3)
            expect(testf).to(raise_error(ValueError))

    def test_should_merge_stats_correctly(self):
        # TODO(add a check of the variance)
        for d1, d2, _ in self.merge_triples:
            distribution.merge(d1, d2)
            expect(d2.count).to(equal(2))
            expect(d2.mean).to(equal((_HIGH_SAMPLE + _LOW_SAMPLE) / 2))
            expect(d2.maximum).to(equal(_HIGH_SAMPLE))
            expect(d2.minimum).to(equal(_LOW_SAMPLE))

    def test_should_merge_bucket_counts_correctly(self):
        for d1, d2, _ in self.merge_triples:
            d1_start = list(d1.bucketCounts)
            d2_start = list(d2.bucketCounts)
            want = [x + y for (x,y) in zip(d1_start, d2_start)]
            distribution.merge(d1, d2)
            expect(d2.bucketCounts).to(equal(want))
