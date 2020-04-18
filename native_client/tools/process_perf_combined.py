#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import sys

# Combine chromium-style perf log output from multiple runs.
# Input is text *containing* the chrome buildbot perf format (may contain more)
# but the output is only the merged perf data (throws away the rest).

def ListToString(l):
  return '[%s]' % (','.join(l))


def Main():
  usage = 'usage: %prog < stdin\n'
  if len(sys.argv) != 1:
    sys.stderr.write(usage)
    sys.stderr.write('Instead, argv was %s\n' % str(sys.argv))
    return 1
  accumulated_times = {}
  result_matcher = re.compile(r'^RESULT (.*): (.*)= (.*) (.*)$')
  for line in sys.stdin.readlines():
    match = result_matcher.match(line)
    if match:
      graph, trace, value, unit = match.groups()
      key = (graph, trace)
      value_list, old_unit = accumulated_times.get(key, ([], None))
      if old_unit is not None:
        assert(unit == old_unit), (unit, old_unit)
      if isinstance(value, list):
        value_list += value
      else:
        value_list.append(value)
      accumulated_times[key] = (value_list, unit)
  for ((graph, trace), (values, unit)) in accumulated_times.iteritems():
    sys.stdout.write('RESULT %s: %s= %s %s\n' %
                     (graph, trace, ListToString(values), unit))
  return 0


if __name__ == '__main__':
  sys.exit(Main())
