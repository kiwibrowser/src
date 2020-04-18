# -*- coding: utf-8 -*-
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility functions for dealing with iterators."""

from __future__ import print_function

import itertools


def IntersectIntervals(intervals):
  """Gets the intersection of a set of intervals.

  Args:
    intervals: A list of interval groups, where each interval group is itself
               a list of (start, stop) tuples (ordered by start time and
               non-overlapping).

  Returns:
    An interval group, as a list of (start, stop) tuples, corresponding to the
    intersection (i.e. overlap) of the given |intervals|.
  """
  if not intervals:
    return []

  intersection = []
  indices = [0] * len(intervals)
  lengths = [len(i) for i in intervals]
  while all(i < l for i, l in zip(indices, lengths)):
    current_intervals = [intervals[i][j] for (i, j) in
                         zip(itertools.count(), indices)]
    start = max([s[0] for s in current_intervals])
    end, end_index = min([(e[1], i) for e, i in
                          zip(current_intervals, itertools.count())])
    if start < end:
      intersection.append((start, end))
    indices[end_index] += 1

  return intersection


def SplitToChunks(input_iter, chunk_size):
  """Split an iterator into chunks.

  This function walks 1 entire chunk of |input_iter| at a time, but does not
  walk past that chunk until necessary.

  Example usage
    list(SplitToChunks([1, 2, 3, 4, 5], 3)) -> [[1, 2, 3], [4, 5]

  Args:
    input_iter: iterable or generator to be split into chunks
    chunk_size: the maximum size of each chunk

  Returns:
    An iterable of chunks, where each chunk is of maximum size chunk_size.
  """
  count = 0
  l = []
  for item in input_iter:
    l.append(item)
    count += 1
    if count == chunk_size:
      yield l
      l = []
      count = 0

  if l:
    yield l
