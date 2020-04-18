# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Basic statistics-related math functions used by the dashboard."""

import math


def Mean(values):
  """Returns the arithmetic mean, or NaN if the input is empty."""
  if not values:
    return float('nan')
  return sum(values) / float(len(values))


def Median(values):
  """Returns the arithmetic mean, or NaN if the input is empty."""
  if not values:
    return float('nan')
  sorted_values = sorted(values)
  mid = len(values) / 2
  if len(values) % 2 == 1:
    return float(sorted_values[mid])
  return (sorted_values[mid - 1] + sorted_values[mid]) / 2.0


def Variance(values):
  """Returns the population variance, or NaN if the input is empty."""
  if not values:
    return float('nan')
  mean = Mean(values)
  return Mean([(x - mean) ** 2 for x in values])


def StandardDeviation(values):
  """Returns the population std. dev., or NaN if the input is empty."""
  if not values:
    return float('nan')
  return math.sqrt(Variance(values))


def Divide(a, b):
  """Returns the quotient, or NaN if the divisor is zero."""
  if b == 0:
    return float('nan')
  return a / float(b)


def RelativeChange(before, after):
  """Returns the absolute value of the relative change between two values.

  Args:
    before: First value.
    after: Second value.

  Returns:
    Relative change from the first to the second value, or infinity if the
    first value is zero. This is guaranteed to be non-negative.
  """
  return abs((after - before) / float(before)) if before else float('inf')
