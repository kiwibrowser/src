#!/usr/bin/python
# Copyright 2008 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.



"""
Filter service runtime logging output and compute system call statistics.

To use this script, define the BENCHMARK symbol to be zero (default)
in nacl_syscall_hook.c.  Next, run the service runtime with NACLLOG
set to an output file name.  When the run is complete, run this script
with that file as input.

"""

import math
import re
import sys


class Stats:
  """
  Compute basic statistics.
  """
  def __init__(self):
    self._sum_x = 0.0
    self._sum_x_squared = 0.0
    self._n = 0

  def Enter(self, val):
    """Enter a new value.

    Args:
      val: the new (floating point) value
    """
    self._sum_x += val
    self._sum_x_squared += val * val
    self._n += 1

  def Mean(self):
    """Returns the mean of entered values.
    """
    return self._sum_x / self._n

  def Variance(self):
    """Returns the variance of entered values.
    """
    mean = self.Mean()
    return self._sum_x_squared / self._n - mean * mean

  def Stddev(self):
    """Returns the standard deviation of entered values.
    """
    return math.sqrt(self.Variance())

  def NumEntries(self):
    """Returns the number of data points entered.
    """
    return self._n


class PeakStats:
  """Compute min and max for a data set.  While far less efficient
  than using a reduce, this class makes streaming data handling
  easier.
  """

  def __init__(self):
    self._min = 1L << 64
    self._max = -1

  def Enter(self, val):
    """Enter a new datum.

    Args:
      val: the new datum to be entered.
    """
    if val > self._max:
      self._max = val
    if val < self._min:
      self._min = val

  def Max(self):
    """Returns the maximum value found so far.
    """
    return self._max

  def Min(self):
    """Returns the minimum value found so far.
    """
    return self._min


class WindowedRate:

  """Class for computing statistics on events based on counting the
  number of occurrences in a time interval.  Statistcs on these
  bucketed counts are then available.

  """
  def __init__(self, duration):
    self._t_start = -1
    self._t_duration = duration
    self._t_end = -1
    self._event_count = 0
    self._rate_stats = Stats()
    self._peak_stats = PeakStats()

  def Enter(self, t):
    """Enter in a new event that occurred at time t.

    Args:
      t: the time at which an event occurred.
    """
    if self._t_start == -1:
      self._t_start = t
      self._t_end = t + self._t_duration
      return
    # [ t_start, t_start + duration )
    if t < self._t_end:
      self._event_count += 1
      return
    self.Compute()
    self._event_count = 1
    next_end = self._t_end
    while next_end < t:
      next_end += self._t_duration
    self._t_end = next_end
    self._t_start = next_end - self._t_duration

  def Compute(self):
    """Finalize the last bucket.

    """
    self._rate_stats.Enter(self._event_count)
    self._peak_stats.Enter(self._event_count)
    self._event_count = 0

  def RateStats(self):
    """Returns the event rate statistics object.

    """
    return self._rate_stats

  def PeakStats(self):
    """Returns the peak event rate statistics object.

    """
    return self._peak_stats


class TimestampParser:
  """
  A class to parse timestamp strings.  This is needed because there is
  implicit state: the timestamp string is HH:MM:SS.fract and may cross
  a 24 hour boundary -- we do not log the date since that would make
  the log file much larger and generally it is not needed (implicit in
  file modification time) -- so we convert to a numeric representation
  that is relative to an arbitrary epoch start, and the state enables
  us to correctly handle midnight.

  This code assumes that the timestamps are monotonically
  non-decreasing.

  """
  def __init__(self):
    self._min_time = -1

  def Convert(self, timestamp):
    """Converts a timestamp string into a numeric timestamp value.

    Args:
      timestamp: A timestamp string in HH:MM:SS.fraction format.

    Returns:
      a numeric timestamp (arbitrary epoch)
    """
    (hh, mm, ss) = map(float,timestamp.split(':'))
    t = ((hh * 60) + mm) * 60 + ss
    if self._min_time == -1:
      self._min_time = t
    while t < self._min_time:
      t += 24 * 60 * 60
    self._min_time = t
    return t


def ReadFileHandle(fh, duration):
  """Reads log data from the provided file handle, and compute and
  print various statistics on the system call rate based on the log
  data.

  """
  # log format "[pid:timestamp] msg" where the timestamp is
  log_re = re.compile(r'\[[0-9,]+:([:.0-9]+)\] system call [0-9]+')
  parser = TimestampParser()
  inter_stats = Stats()
  rate_stats = Stats()
  windowed = WindowedRate(duration)
  prev_time = -1
  start_time = 0
  for line in fh:  # generator
    m = log_re.search(line)
    if m is not None:
      timestamp = m.group(1)
      t = parser.Convert(timestamp)

      windowed.Enter(t)

      if prev_time != -1:
        elapsed = t - prev_time
        inter_stats.Enter(elapsed)
        rate_stats.Enter(1.0/elapsed)
      else:
        start_time = t
      prev_time = t

  print '\nInter-syscall time'
  print 'Mean:   %g' % inter_stats.Mean()
  print 'Stddev: %g' % inter_stats.Stddev()
  print '\nInstantaneous Syscall Rate (unweighted!)'
  print 'Mean :  %g' % rate_stats.Mean()
  print 'Stddev: %g' % rate_stats.Stddev()
  print '\nAvg Syscall Rate: %g' % (rate_stats.NumEntries()
                                    / (prev_time - start_time))

  print '\nSyscalls in %f interval' % duration
  print 'Mean:   %g' % windowed.RateStats().Mean()
  print 'Stddev: %g' % windowed.RateStats().Stddev()
  print 'Min:    %g' % windowed.PeakStats().Min()
  print 'Max:    %g' % windowed.PeakStats().Max()


def main(argv):
  if len(argv) > 1:
    print >>sys.stderr, 'no arguments expected\n'
    return 1
  ReadFileHandle(sys.stdin, 0.010)
  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv))
