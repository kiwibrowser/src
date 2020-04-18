# -*- coding: utf-8 -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Infrastructure for collecting statistics about retries."""

from __future__ import print_function

import collections
import datetime

from chromite.lib import parallel
from chromite.lib import retry_util


# Well known categories we gather stats for.
CIDB = 'CIDB'
GSUTIL = 'Google Storage'


class UnconfiguredStatsCategory(Exception):
  """We tried to use a Stats Category without configuring it."""


# Create one of these for each retry call.
#   attempts: a list of all attempts to perform the action.
StatEntry = collections.namedtuple(
    'StatEntry',
    ('category', 'attempts'))

# Create one of these for each attempt to call the function.
#  time: The time for this attempt in seconds.
#  exception: None for a successful attempt, or a string exception description.
Attempt = collections.namedtuple(
    'Attempt',
    ('time', 'exception'))


# After Setup, contains a multiprocess proxy array.
# The array holds StatEntry values for each event seen.
_STATS_COLLECTION = None


def SetupStats():
  """Prepare a given category to collect stats.

  This must be called BEFORE any new processes that might read or write to
  these stat values are created. It is safe to call this more than once,
  but most efficient to only make a single call.
  """
  # Pylint thinks our manager has no members.
  # pylint: disable=E1101
  m = parallel.Manager()

  # pylint: disable=W0603
  # Create a new stats collection structure that is multiprocess usable.
  global _STATS_COLLECTION
  _STATS_COLLECTION = m.list()


def _SuccessFilter(entry):
  """Returns True if the StatEntry succeeded (perhaps after retries)."""
  # If all attempts contain an exception, they all failed.
  return not all(a.exception for a in entry.attempts)


def _RetryCount(entry):
  """Returns the number of retries in this StatEntry."""
  # If all attempts contain an exception, they all failed.
  return max(len(entry.attempts) - 1, 0)


def CategoryStats(category):
  """Return stats numbers for a given category.

  success is the number of times a given command succeeded, even if it had to be
  retried.

  failure is the number of times we exhausting all retries without success.

  retry is the total number of times we retried a command, unrelated to eventual
  success or failure.

  Args:
    category: A string that defines the 'namespace' for these stats.

  Returns:
    succuess, failure, retry values as integers.
  """
  # Convert the multiprocess proxy list into a local simple list.
  local_stats_collection = list(_STATS_COLLECTION)

  # Extract the values for the category we care about.
  stats = [e for e in local_stats_collection if e.category == category]

  success = len([e for e in stats if _SuccessFilter(e)])
  failure = len(stats) - success
  retry = sum([_RetryCount(e) for e in stats])

  return success, failure, retry

def ReportCategoryStats(out, category):
  """Dump stats reports for a given category.

  Args:
    out: Output stream to write to (e.g. sys.stdout).
    category: A string that defines the 'namespace' for these stats.
  """
  success, failure, retry = CategoryStats(category)

  line = '*' * 60 + '\n'
  edge = '*' * 2

  out.write(line)
  out.write(edge + ' Performance Statistics for %s' % category + '\n')
  out.write(edge + '\n')
  out.write(edge + ' Success: %d' % success + '\n')
  out.write(edge + ' Failure: %d' % failure + '\n')
  out.write(edge + ' Retries: %d' % retry + '\n')
  out.write(edge + ' Total: %d' % (success + failure) + '\n')
  out.write(line)


def ReportStats(out):
  """Dump stats reports for a given category.

  Args:
    out: Output stream to write to (e.g. sys.stdout).
    category: A string that defines the 'namespace' for these stats.
  """
  categories = sorted(set([e.category for e in _STATS_COLLECTION]))

  for category in categories:
    ReportCategoryStats(out, category)


def RetryWithStats(category, handler, max_retry, functor, *args, **kwargs):
  """Wrapper around retry_util.GenericRetry that collects stats.

  This wrapper collects statistics about each failure or retry. Each
  category is defined by a unique string. Each category should be setup
  before use (actually, before processes are forked).

  All other arguments are blindly passed to retry_util.GenericRetry.

  Args:
    category: A string that defines the 'namespace' for these stats.
    handler: See retry_util.GenericRetry.
    max_retry: See retry_util.GenericRetry.
    functor: See retry_util.GenericRetry.
    args: See retry_util.GenericRetry.
    kwargs: See retry_util.GenericRetry.

  Returns:
    See retry_util.GenericRetry raises.

  Raises:
    See retry_util.GenericRetry raises.
  """
  statEntry = StatEntry(category, attempts=[])

  # Wrap the work method, so we can gather info.
  def wrapper(*args, **kwargs):
    start = datetime.datetime.now()

    try:
      result = functor(*args, **kwargs)
    except Exception as e:
      end = datetime.datetime.now()
      e_description = '%s: %s' % (type(e).__name__, e)
      statEntry.attempts.append(Attempt(end - start, e_description))
      raise

    end = datetime.datetime.now()
    statEntry.attempts.append(Attempt(end - start, None))
    return result

  try:
    return retry_util.GenericRetry(handler, max_retry, wrapper,
                                   *args, **kwargs)
  finally:
    if _STATS_COLLECTION is not None:
      _STATS_COLLECTION.append(statEntry)
