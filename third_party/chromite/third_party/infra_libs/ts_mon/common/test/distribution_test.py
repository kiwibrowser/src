# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from infra_libs.ts_mon.common import distribution


class BucketerTestBase(unittest.TestCase):
  def assertLowerBounds(self, b, expected_lower_bounds):
    expected_total = len(expected_lower_bounds)

    # Test the instance attributes in the Bucketer.
    self.assertEquals(expected_total - 2, b.num_finite_buckets)
    self.assertEquals(expected_total, b.total_buckets)
    self.assertEquals(0, b.underflow_bucket)
    self.assertEquals(expected_total - 1, b.overflow_bucket)
    self.assertEquals(expected_lower_bounds, b._lower_bounds)

    # Test the bucket_boundaries and bucket_for_value methods.
    for i, lower_bound in enumerate(expected_lower_bounds):
      # This bucket's upper bound is the next bucket's lower bound, or Infinity
      # if this is the last bucket.
      try:
        upper_bound = expected_lower_bounds[i + 1]
      except IndexError:
        upper_bound = float('Inf')

      self.assertLess(lower_bound, upper_bound)
      self.assertEquals((lower_bound, upper_bound), b.bucket_boundaries(i))
      self.assertEquals(i, b.bucket_for_value(lower_bound))
      self.assertEquals(i, b.bucket_for_value(lower_bound + 0.5))
      self.assertEquals(i, b.bucket_for_value(upper_bound - 0.5))

    with self.assertRaises(IndexError):
      b.bucket_boundaries(-1)
    with self.assertRaises(IndexError):
      b.bucket_boundaries(len(expected_lower_bounds))



class FixedWidthBucketerTest(BucketerTestBase):
  def test_equality(self):
    b = distribution.FixedWidthBucketer(width=10, num_finite_buckets=8)
    self.assertEquals(b, b)

  def test_negative_size(self):
    with self.assertRaises(ValueError):
      distribution.FixedWidthBucketer(width=10, num_finite_buckets=-1)

  def test_negative_width(self):
    with self.assertRaises(AssertionError):
      distribution.FixedWidthBucketer(width=-1, num_finite_buckets=1)

  def test_zero_size(self):
    b = distribution.FixedWidthBucketer(width=10, num_finite_buckets=0)
    self.assertLowerBounds(b, [float('-Inf'), 0])

  def test_one_size(self):
    b = distribution.FixedWidthBucketer(width=10, num_finite_buckets=1)
    self.assertLowerBounds(b, [float('-Inf'), 0, 10])

  def test_bucket_for_value(self):
    b = distribution.FixedWidthBucketer(width=10, num_finite_buckets=5)
    self.assertEquals(0, b.bucket_for_value(float('-Inf')))
    self.assertEquals(0, b.bucket_for_value(-100))
    self.assertEquals(0, b.bucket_for_value(-1))
    self.assertEquals(1, b.bucket_for_value(0))
    self.assertEquals(5, b.bucket_for_value(45))
    self.assertEquals(6, b.bucket_for_value(51))
    self.assertEquals(6, b.bucket_for_value(100000))
    self.assertEquals(6, b.bucket_for_value(float('Inf')))


class GeometricBucketerTest(BucketerTestBase):
  def test_equality(self):
    b = distribution.GeometricBucketer(
        growth_factor=4, num_finite_buckets=4, scale=.1)
    self.assertEquals(b, b)

  def test_negative_size(self):
    with self.assertRaises(ValueError):
      distribution.GeometricBucketer(num_finite_buckets=-1)

  def test_bad_growth_factors(self):
    with self.assertRaises(AssertionError):
      distribution.GeometricBucketer(growth_factor=-1)
    with self.assertRaises(AssertionError):
      distribution.GeometricBucketer(growth_factor=0)
    with self.assertRaises(AssertionError):
      distribution.GeometricBucketer(growth_factor=1)

  def test_zero_size(self):
    b = distribution.GeometricBucketer(num_finite_buckets=0)
    self.assertLowerBounds(b, [float('-Inf'), 1])

  def test_large_size(self):
    b = distribution.GeometricBucketer(growth_factor=4, num_finite_buckets=4)
    self.assertLowerBounds(b, [float('-Inf'), 1, 4, 16, 64, 256])

  def test_scale(self):
    b = distribution.GeometricBucketer(growth_factor=4, num_finite_buckets=4,
                                       scale=.1)
    # bucket lower bounds will be approximately [float('-Inf'), .1, .4, 1.6,
    # 6.4, 25.6], but to avoid floating point errors affecting test assert on
    # bucket_for_value instead of using assertLowerBounds.
    self.assertEquals(0, b.bucket_for_value(float('-Inf')))
    self.assertEquals(0, b.bucket_for_value(.05))
    self.assertEquals(1, b.bucket_for_value(.2))
    self.assertEquals(5, b.bucket_for_value(float('Inf')))

  def test_bucket_for_value(self):
    b = distribution.GeometricBucketer(growth_factor=2, num_finite_buckets=5)
    self.assertEquals(0, b.bucket_for_value(float('-Inf')))
    self.assertEquals(0, b.bucket_for_value(-100))
    self.assertEquals(0, b.bucket_for_value(-1))
    self.assertEquals(0, b.bucket_for_value(0))
    self.assertEquals(1, b.bucket_for_value(1))
    self.assertEquals(5, b.bucket_for_value(31))
    self.assertEquals(6, b.bucket_for_value(32))
    self.assertEquals(6, b.bucket_for_value(100000))
    self.assertEquals(6, b.bucket_for_value(float('Inf')))


class CustomBucketerTest(BucketerTestBase):
  def test_boundaries(self):
    with self.assertRaises(ValueError):
      distribution._Bucketer(width=10, growth_factor=2, num_finite_buckets=4)


class DistributionTest(unittest.TestCase):
  def test_add(self):
    d = distribution.Distribution(distribution.GeometricBucketer())
    self.assertEqual(0, d.sum)
    self.assertEqual(0, d.count)
    self.assertEqual({}, d.buckets)

    d.add(1)
    d.add(10)
    d.add(100)

    self.assertEqual(111, d.sum)
    self.assertEqual(3, d.count)
    self.assertEqual({1: 1, 5: 1, 10: 1}, d.buckets)

    d.add(50)

    self.assertEqual(161, d.sum)
    self.assertEqual(4, d.count)
    self.assertEqual({1: 1, 5: 1, 9: 1, 10: 1}, d.buckets)

  def test_add_on_bucket_boundary(self):
    d = distribution.Distribution(distribution.FixedWidthBucketer(width=10))

    d.add(10)

    self.assertEqual(10, d.sum)
    self.assertEqual(1, d.count)
    self.assertEqual({2: 1}, d.buckets)

    d.add(0)

    self.assertEqual(10, d.sum)
    self.assertEqual(2, d.count)
    self.assertEqual({1: 1, 2: 1}, d.buckets)

  def test_underflow_bucket(self):
    d = distribution.Distribution(distribution.FixedWidthBucketer(width=10))

    d.add(-1)

    self.assertEqual(-1, d.sum)
    self.assertEqual(1, d.count)
    self.assertEqual({0: 1}, d.buckets)

    d.add(-1000000)

    self.assertEqual(-1000001, d.sum)
    self.assertEqual(2, d.count)
    self.assertEqual({0: 2}, d.buckets)

  def test_overflow_bucket(self):
    d = distribution.Distribution(
        distribution.FixedWidthBucketer(width=10, num_finite_buckets=10))

    d.add(100)

    self.assertEqual(100, d.sum)
    self.assertEqual(1, d.count)
    self.assertEqual({11: 1}, d.buckets)

    d.add(1000000)

    self.assertEqual(1000100, d.sum)
    self.assertEqual(2, d.count)
    self.assertEqual({11: 2}, d.buckets)
