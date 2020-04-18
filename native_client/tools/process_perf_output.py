#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import sys

# Process log output from various components (e.g., sel_ldr, browser tester),
# line-by-line and prints out a summary of TIME performance measurements.
#
# ================
# LOG INPUTS:
# ================
# Logs can contain information from any number of Sources.
#
# For example, the sel_ldr logs and browser test logs might be Sources.
#
# This processor expects to match at least one line of info from each
# Source. If a source does not provide any events, then this is an error
# (the motivation being that we will pick up on log format changes this way).
# TODO(jvoung): Allow an escape hatch for some Sources.
#
# Each Eventful line with time info should contain:
#
# (a) an event name
# (b) a time value
#
# The time values can be in any unit, but will be converted to millisecs.
#
# NOTE: If multiple events with the same event name are present, then
# the time values will be summed. This is useful, for example, to get the
# total validation time of all dynamic libraries that are loaded.
# However, this means that if a nexe is loaded twice, then the two time
# values will get merged into one.
# TODO(jvoung): provide a mechanism to split multiple merged event streams.
#
# ================
# SUMMARY OUTPUT:
# ================
# Should be formatted according to the chrome buildbot format.

def GetEventPatterns():
  return [
      # NaClPerfCounterInterval (${SERIES} ${EVENT_A}:*${EVENT_B}): N microsecs
      # -->
      # RESULT ${GRAPH}: ${EVENT_B}_${SETUP_INFO}= N/1000 ms
      # Thus, this assumes that the EVENT_B provides the useful name
      # E.g., EVENT_A might be "Pre-Validation"
      # while EVENT_B is "Validation" (so this times validation)
      # Note, there is an asterisk in EVENT_B to indicate that it is important
      # enough to be included here.
      Pattern('SelLdr',
              'NaClPerfCounterInterval\(.*:\*(.*)\): (\d+) microsecs',
              1, 2, 0.001),

      # NaClPerf [${EVENT_NAME}]: N millisecs
      # -->
      # RESULT ${GRAPH}: ${EVENT_NAME}_${SETUP_INFO}= N ms
      Pattern('BrowserTester',
              'NaClPerf \[(.*)\] (\d+\.*\d*) millisecs',
              1, 2, 1.0),
      ]

class Pattern(object):
  def __init__(self,
               name,
               patternRE,
               eventIndex,
               timeIndex,
               scaleToMilli):
    self.name = name
    self.re = re.compile(patternRE)
    self.eventIndex = eventIndex
    self.timeIndex = timeIndex
    self.scaleToMilli = scaleToMilli
    self.didMatch = False
    self.accumulatedTimes = {}

  def _eventLabel(self, match):
    return match.group(self.eventIndex)

  def _timeInMillis(self, match):
    return float(match.group(self.timeIndex)) * self.scaleToMilli

  def _match(self, s):
    return self.re.search(s)

  def ProcessLine(self, line):
    match = self._match(line)
    if not match:
      return False
    self.didMatch = True
    event = self._eventLabel(match)
    time = self._timeInMillis(match)
    # Sum up the times for a particular event.
    self.accumulatedTimes[event] = self.accumulatedTimes.get(event, 0) + time
    return True

  def SanityCheck(self):
    # Make sure all patterns matched at least once.
    if not self.didMatch:
      sys.stderr.write(('Pattern for %s did not match anything.\n'
                        'Perhaps the format has changed.\n') % self.name)
      assert False

  def PrintSummary(self, graph_label, trace_label_extra):
    for event, time in self.accumulatedTimes.iteritems():
      sys.stdout.write('RESULT %s: %s_%s= %f ms\n' %
                       (graph_label, event, trace_label_extra, time))

def Main():
  usage = 'usage: %prog graphLabel traceExtra < stdin\n'
  if len(sys.argv) != 3:
    sys.stderr.write(usage)
    return 1
  graph_label = sys.argv[1]
  trace_label_extra = sys.argv[2]
  event_patterns = GetEventPatterns()
  for line in sys.stdin.readlines():
    for pat in event_patterns:
      if pat.ProcessLine(line):
        continue
    # Also echo the original data for debugging.
    sys.stdout.write(line)
  # Print the summary in the end.
  for pat in event_patterns:
    pat.SanityCheck()
    pat.PrintSummary(graph_label, trace_label_extra)
  return 0

if __name__ == '__main__':
  sys.exit(Main())
