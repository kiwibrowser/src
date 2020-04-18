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

"""distribution provides funcs for working with `Distribution` instances.

:func:`create_exponential`, :func:`create_linear`, :func:`create_linear`
construct new `Distribution` instances initialized with different types
of buckets a `Distribution` can have. They are factory functions that
include assertions that make sure that the Distribution instances are
in the correct state.

:func:`add_sample` adds a sample to an existing distribution instance

:func:`merge` merges two distribution instances

"""

from __future__ import absolute_import
from __future__ import division

import bisect
import logging
import math

from . import messages

logger = logging.getLogger(__name__)


_BAD_NUM_FINITE_BUCKETS = 'number of finite buckets should be > 0'
_BAD_FLOAT_ARG = '%s should be > %f'


def create_exponential(num_finite_buckets, growth_factor, scale):
    """Creates a new instance of distribution with exponential buckets

    Args:
       num_finite_buckets (int): initializes number of finite buckets
       growth_factor (float): initializes the growth factor
       scale (float): initializes the scale

    Return:
       :class:`google.api.gen.servicecontrol_v1_messages.Distribution`

    Raises:
       ValueError: if the args are invalid for creating an instance
    """
    if num_finite_buckets <= 0:
        raise ValueError(_BAD_NUM_FINITE_BUCKETS)
    if growth_factor <= 1.0:
        raise ValueError(_BAD_FLOAT_ARG % ('growth factor', 1.0))
    if scale <= 0.0:
        raise ValueError(_BAD_FLOAT_ARG % ('scale', 0.0))
    return messages.Distribution(
        bucketCounts=[0] * (num_finite_buckets + 2),
        exponentialBuckets=messages.ExponentialBuckets(
            numFiniteBuckets=num_finite_buckets,
            growthFactor=growth_factor,
            scale=scale))


def create_linear(num_finite_buckets, width, offset):
    """Creates a new instance of distribution with linear buckets.

    Args:
       num_finite_buckets (int): initializes number of finite buckets
       width (float): initializes the width of each bucket
       offset (float): initializes the offset

    Return:
       :class:`google.api.gen.servicecontrol_v1_messages.Distribution`

    Raises:
       ValueError: if the args are invalid for creating an instance
    """
    if num_finite_buckets <= 0:
        raise ValueError(_BAD_NUM_FINITE_BUCKETS)
    if width <= 0.0:
        raise ValueError(_BAD_FLOAT_ARG % ('width', 0.0))
    return messages.Distribution(
        bucketCounts=[0] * (num_finite_buckets + 2),
        linearBuckets=messages.LinearBuckets(
            numFiniteBuckets=num_finite_buckets,
            width=width,
            offset=offset))


def create_explicit(bounds):
    """Creates a new instance of distribution with explicit buckets.

    bounds is an iterable of ordered floats that define the explicit buckets

    Args:
       bounds (iterable[float]): initializes the bounds

    Return:
       :class:`google.api.gen.servicecontrol_v1_messages.Distribution`

    Raises:
       ValueError: if the args are invalid for creating an instance
    """
    safe_bounds = sorted(float(x) for x in bounds)
    if len(safe_bounds) != len(set(safe_bounds)):
        raise ValueError('Detected two elements of bounds that are the same')
    return messages.Distribution(
        bucketCounts=[0] * (len(safe_bounds) + 1),
        explicitBuckets=messages.ExplicitBuckets(bounds=safe_bounds))


def add_sample(a_float, dist):
    """Adds `a_float` to `dist`, updating its existing buckets.

    Args:
      a_float (float): a new value
      dist (:class:`google.api.gen.servicecontrol_v1_messages.Distribution`):
        the Distribution being updated

    Raises:
      ValueError: if `dist` does not have known bucket options defined
      ValueError: if there are not enough bucket count fields in `dist`
    """
    dist_type, _ = _detect_bucket_option(dist)
    if dist_type == 'exponentialBuckets':
        _update_general_statistics(a_float, dist)
        _update_exponential_bucket_count(a_float, dist)
    elif dist_type == 'linearBuckets':
        _update_general_statistics(a_float, dist)
        _update_linear_bucket_count(a_float, dist)
    elif dist_type == 'explicitBuckets':
        _update_general_statistics(a_float, dist)
        _update_explicit_bucket_count(a_float, dist)
    else:
        logger.error('Could not determine bucket option type for %s', dist)
        raise ValueError('Unknown bucket option type')


def merge(prior, latest):
    """Merge `prior` into `latest`.

    N.B, this mutates latest. It ensures that the statistics and histogram are
    updated to correctly include the original values from both instances.

    Args:
      prior (:class:`google.api.gen.servicecontrol_v1_messages.Distribution`):
        an instance
      latest (:class:`google.api.gen.servicecontrol_v1_messages.Distribution`):
        an instance to be updated

    Raises:
      ValueError: if the bucket options of `prior` and `latest` do not match
      ValueError: if the bucket counts of `prior` and `latest` do not match

    """
    if not _buckets_nearly_equal(prior, latest):
        logger.error('Bucket options do not match. From %s To: %s',
                     prior,
                     latest)
        raise ValueError('Bucket options do not match')
    if len(prior.bucketCounts) != len(latest.bucketCounts):
        logger.error('Bucket count sizes do not match. From %s To: %s',
                     prior,
                     latest)
        raise ValueError('Bucket count sizes do not match')
    if prior.count <= 0:
        return

    old_count = latest.count
    old_mean = latest.mean
    old_summed_variance = latest.sumOfSquaredDeviation
    bucket_counts = latest.bucketCounts

    # Update the latest
    latest.count += prior.count
    latest.maximum = max(prior.maximum, latest.maximum)
    latest.minimum = min(prior.minimum, latest.minimum)
    latest.mean = ((old_count * old_mean + prior.count * prior.mean) /
                   latest.count)
    latest.sumOfSquaredDeviation = (
        old_summed_variance + prior.sumOfSquaredDeviation +
        old_count * (latest.mean - old_mean) ** 2 +
        prior.count * (latest.mean - prior.mean) ** 2)
    for i, (x, y) in enumerate(zip(prior.bucketCounts, bucket_counts)):
        bucket_counts[i] = x + y

_EPSILON = 1e-5


def _is_close_enough(x, y):
    if x is None or y is None:
        return False
    return abs(x - y) <= _EPSILON * abs(x)


# This is derived from the oneof choices of the Distribution message's
# bucket_option field in google/api/servicecontrol/v1/distribution.proto, and
# should be kept in sync with that
_DISTRIBUTION_ONEOF_FIELDS = (
    'linearBuckets', 'exponentialBuckets', 'explicitBuckets')


def _detect_bucket_option(distribution):
    for f in _DISTRIBUTION_ONEOF_FIELDS:
        value = distribution.get_assigned_value(f)
        if value is not None:
            return f, value
    return None, None


def _linear_buckets_nearly_equal(a, b):
    return ((a.numFiniteBuckets == b.numFiniteBuckets) and
            _is_close_enough(a.width, b.width) or
            _is_close_enough(a.offset, b.offset))


def _exponential_buckets_nearly_equal(a, b):
    return ((a.numFiniteBuckets == b.numFiniteBuckets) and
            _is_close_enough(a.growthFactor, b.growthFactor) and
            _is_close_enough(a.scale, b.scale))


def _explicit_buckets_nearly_equal(a, b):
    if len(a.bounds) != len(b.bounds):
        return False
    for x, y in zip(a.bounds, b.bounds):
        if not _is_close_enough(x, y):
            return False
    return True


def _buckets_nearly_equal(a_dist, b_dist):
    """Determines whether two `Distributions` are nearly equal.

    Args:
      a_dist (:class:`Distribution`): an instance
      b_dist (:class:`Distribution`): another instance

    Return:
      boolean: `True` if the two instances are approximately equal, otherwise
        False

    """
    a_type, a_buckets = _detect_bucket_option(a_dist)
    b_type, b_buckets = _detect_bucket_option(b_dist)
    if a_type != b_type:
        return False
    elif a_type == 'linearBuckets':
        return _linear_buckets_nearly_equal(a_buckets, b_buckets)
    elif a_type == 'exponentialBuckets':
        return _exponential_buckets_nearly_equal(a_buckets, b_buckets)
    elif a_type == 'explicitBuckets':
        return _explicit_buckets_nearly_equal(a_buckets, b_buckets)
    else:
        return False


def _update_general_statistics(a_float, dist):
    """Adds a_float to distribution, updating the statistics fields.

    Args:
      a_float (float): a new value
      dist (:class:`google.api.gen.servicecontrol_v1_messages.Distribution`):
        the Distribution being updated

    """
    if not dist.count:
        dist.count = 1
        dist.maximum = a_float
        dist.minimum = a_float
        dist.mean = a_float
        dist.sumOfSquaredDeviation = 0
    else:
        old_count = dist.count
        old_mean = dist.mean
        new_mean = ((old_count * old_mean) + a_float) / (old_count + 1)
        delta_sum_squares = (a_float - old_mean) * (a_float - new_mean)
        dist.count += 1
        dist.mean = new_mean
        dist.maximum = max(a_float, dist.maximum)
        dist.minimum = min(a_float, dist.minimum)
        dist.sumOfSquaredDeviation += delta_sum_squares


_BAD_UNSET_BUCKETS = 'cannot update a distribution with unset %s'
_BAD_LOW_BUCKET_COUNT = 'cannot update a distribution with a low bucket count'


def _update_exponential_bucket_count(a_float, dist):
    """Adds `a_float` to `dist`, updating its exponential buckets.

    Args:
      a_float (float): a new value
      dist (:class:`google.api.gen.servicecontrol_v1_messages.Distribution`):
        the Distribution being updated

    Raises:
      ValueError: if `dist` does not already have exponential buckets defined
      ValueError: if there are not enough bucket count fields in `dist`
    """
    buckets = dist.exponentialBuckets
    if buckets is None:
        raise ValueError(_BAD_UNSET_BUCKETS % ('exponential buckets'))
    bucket_counts = dist.bucketCounts
    num_finite_buckets = buckets.numFiniteBuckets
    if len(bucket_counts) < num_finite_buckets + 2:
        raise ValueError(_BAD_LOW_BUCKET_COUNT)
    scale = buckets.scale
    factor = buckets.growthFactor
    if (a_float <= scale):
        index = 0
    else:
        index = 1 + int((math.log(a_float / scale) / math.log(factor)))
        index = min(index, num_finite_buckets + 1)
    bucket_counts[index] += 1
    logger.debug('scale:%f, factor:%f, sample:%f, index:%d',
                 scale, factor, a_float, index)


def _update_linear_bucket_count(a_float, dist):
    """Adds `a_float` to `dist`, updating the its linear buckets.

    Args:
      a_float (float): a new value
      dist (:class:`google.api.gen.servicecontrol_v1_messages.Distribution`):
        the Distribution being updated

    Raises:
      ValueError: if `dist` does not already have linear buckets defined
      ValueError: if there are not enough bucket count fields in `dist`
    """
    buckets = dist.linearBuckets
    if buckets is None:
        raise ValueError(_BAD_UNSET_BUCKETS % ('linear buckets'))
    bucket_counts = dist.bucketCounts
    num_finite_buckets = buckets.numFiniteBuckets
    if len(bucket_counts) < num_finite_buckets + 2:
        raise ValueError(_BAD_LOW_BUCKET_COUNT)
    width = buckets.width
    lower = buckets.offset
    upper = lower + (num_finite_buckets * width)
    if a_float < lower:
        index = 0
    elif a_float >= upper:
        index = num_finite_buckets + 1
    else:
        index = 1 + int(((a_float - lower) / width))
    bucket_counts[index] += 1
    logger.debug('upper:%f, lower:%f, width:%f, sample:%f, index:%d',
                 upper, lower, width, a_float, index)


def _update_explicit_bucket_count(a_float, dist):
    """Adds `a_float` to `dist`, updating its explicit buckets.

    Args:
      a_float (float): a new value
      dist (:class:`google.api.gen.servicecontrol_v1_messages.Distribution`):
        the Distribution being updated

    Raises:
      ValueError: if `dist` does not already have explict buckets defined
      ValueError: if there are not enough bucket count fields in `dist`
    """
    buckets = dist.explicitBuckets
    if buckets is None:
        raise ValueError(_BAD_UNSET_BUCKETS % ('explicit buckets'))
    bucket_counts = dist.bucketCounts
    bounds = buckets.bounds
    if len(bucket_counts) < len(bounds) + 1:
        raise ValueError(_BAD_LOW_BUCKET_COUNT)
    bucket_counts[bisect.bisect(bounds, a_float)] += 1
