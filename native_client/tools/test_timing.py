#!/usr/bin/python2.4
# Copyright 2008 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Program to parse hammer output and sort tests by how long they took to run.

The output should be from the CommandTest in SConstruct.
"""

import getopt
import re
import sys


class Analyzer(object):
  """Basic run time analysis (sorting, so far)."""

  def __init__(self):
    self._data = []  # list of (time, test-name) tuples

  def AddData(self, execution_time, test_name, mode):
    self._data.append((execution_time, test_name, mode))

  def Sort(self):
    self._data.sort(None, lambda x: x[0], True)

  def Top(self, n):
    return self._data[:n]


def TrimTestName(name):
  s = '/scons-out/'
  ix = name.find(s)
  if ix < 0:
    return name[ix + len(s):]
  return name


def Usage():
  print >>sys.stderr, 'Usage: test_timing [-n top-n-to-print]'


def main(argv):
  top_n = 10

  try:
    optlist, argv = getopt.getopt(argv[1:], 'n:')
  except getopt.error, e:
    print >>sys.stderr, str(e)
    Usage()
    return 1

  for opt, val in optlist:
    if opt == '-n':
      try:
        top_n = int(val)
      except ValueError:
        print >>sys.stderr, 'test_timing: -n arg should be an integer'
        Usage()
        return 1

  mode = 'Unknown'
  mode_nfa = re.compile(r'^running.*scons-out/((opt|dbg)-linux)')
  nfa = re.compile(r'^Test (.*) took ([.0-9]*) secs')
  analyzer = Analyzer()

  for line in sys.stdin:
    mobj = mode_nfa.match(line)
    if mobj is not None:
      mode = mobj.group(1)
      continue
    mobj = nfa.match(line)
    if mobj is not None:
      analyzer.AddData(float(mobj.group(2)), mobj.group(1), mode)
  analyzer.Sort()

  print '%-12s %-9s %s' % ('Time', 'Mode', 'Test Name')
  print '%-12s %-9s %s' % (12*'-', 9*'-', '---------')
  for time, name, mode in analyzer.Top(top_n):
    print '%12.8f %-9s %s' % (time, mode, TrimTestName(name))
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
