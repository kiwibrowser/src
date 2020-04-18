# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import bisect
import collections


class _Bucketer(object):
  """Bucketing function for histograms recorded by the Distribution class."""

  def __init__(self, width, growth_factor, num_finite_buckets, scale=1.0):
    """The bucket sizes are controlled by width and growth_factor, and the total
    number of buckets is set by num_finite_buckets:

    Args:
      width: fixed size of each bucket (ignores |scale|).
      growth_factor: if non-zero, the size of each bucket increases by another
          multiplicative factor of this factor (see lower bound formula below).
      num_finite_buckets: the number of finite buckets.  There are two
          additional buckets - an underflow and an overflow bucket - that have
          lower and upper bounds of Infinity.
      scale: overall scale factor to apply to buckets, if using geometric
          buckets.

    Specify a width for fixed-size buckets or specify a growth_factor for bucket
    sizes that follow a geometric progression.  Specifying both is not valid.

    For fixed-size buckets::

      The i'th bucket covers the interval [(i-1) * width, i * width),  where i
      ranges from 1 to num_finite_buckets, inclusive:

      bucket number                   lower bound      upper bound
      i == 0 (underflow)              -inf             0
      1 <= i <= num_buckets           (i-1) * width    i * width
      i == num_buckets+1 (overflow)   (i-1) * width    +inf

    For geometric buckets::

      The i'th bucket covers the interval [factor^(i-1), factor^i) * scale
      where i ranges from 1 to num_finite_buckets inclusive.

      bucket number                   lower bound            upper bound
      i == 0 (underflow)              -inf                   scale
      1 <= i <= num_buckets           factor^(i-1) * scale   factor^i * scale
      i == num_buckets+1 (overflow)   factor^(i-1) * scale   +inf
    """

    if num_finite_buckets < 0:
      raise ValueError('num_finite_buckets must be >= 0 (was %d)' %
          num_finite_buckets)
    if width != 0 and growth_factor != 0:
      raise ValueError('a Bucketer must be created with either a width or a '
                       'growth factor, not both')

    self.width = width
    self.growth_factor = growth_factor
    self.num_finite_buckets = num_finite_buckets
    self.total_buckets = num_finite_buckets + 2
    self.underflow_bucket = 0
    self.overflow_bucket = self.total_buckets - 1
    self.scale = scale

    if width != 0:
      self._lower_bounds = [float('-Inf')] + self._linear_bounds()
    else:
      self._lower_bounds = [float('-Inf')] + self._exponential_bounds()

    # Sanity check the bucket lower bounds we created.
    assert len(self._lower_bounds) == self.total_buckets
    assert all(x < y for x, y in zip(
        self._lower_bounds, self._lower_bounds[1:])), (
        'bucket boundaries must be monotonically increasing')

  def _linear_bounds(self):
    return [self.width * i for i in xrange(self.num_finite_buckets + 1)]

  def _exponential_bounds(self):
    return [
        self.scale * self.growth_factor ** i
        for i in xrange(self.num_finite_buckets + 1)]

  def bucket_for_value(self, value):
    """Returns the index of the bucket that this value belongs to."""

    # bisect.bisect_left is wrong because the buckets are of [lower, upper) form
    return bisect.bisect(self._lower_bounds, value) - 1

  def bucket_boundaries(self, bucket):
    """Returns a tuple that is the [lower, upper) bounds of this bucket.

    The lower bound of the first bucket is -Infinity, and the upper bound of the
    last bucket is +Infinity.
    """

    if bucket < 0 or bucket >= self.total_buckets:
      raise IndexError('bucket %d out of range' % bucket)
    if bucket == self.total_buckets - 1:
      return (self._lower_bounds[bucket], float('Inf'))
    return (self._lower_bounds[bucket], self._lower_bounds[bucket + 1])


def FixedWidthBucketer(width, num_finite_buckets=100):
  """Convenience function that returns a fixed width Bucketer."""
  return _Bucketer(width=width, growth_factor=0.0,
      num_finite_buckets=num_finite_buckets)


def GeometricBucketer(growth_factor=10**0.2, num_finite_buckets=100,
                      scale=1.0):
  """Convenience function that returns a geometric progression Bucketer."""
  return _Bucketer(width=0, growth_factor=growth_factor,
      num_finite_buckets=num_finite_buckets, scale=scale)


class Distribution(object):
  """Holds a histogram distribution.

  Buckets are chosen for values by the provided Bucketer.
  """

  def __init__(self, bucketer):
    self.bucketer = bucketer
    self.sum = 0
    self.count = 0
    self.buckets = collections.defaultdict(int)

  def add(self, value):
    self.buckets[self.bucketer.bucket_for_value(value)] += 1
    self.sum += value
    self.count += 1
