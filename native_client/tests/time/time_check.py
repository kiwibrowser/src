#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import re
import subprocess
import sys
import time

def TimeCommand(cmd_and_argv, output_predicate):
  p = subprocess.Popen(cmd_and_argv, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
  return output_predicate(p.stdout)

def Main(argv):
  parser = optparse.OptionParser()
  parser.add_option('-s', '--time_slop', dest='time_slop', default=1.0,
                    help=('how far outside of starting / ending time' +
                          ' (as measured by python) can the output be'))
  # the time slop should be small -- just enough to take care of all
  # NTP adjustments.
  parser.add_option('-S', '--show_output', dest='show_output',
                    action='store_true', default=False,
                    help='show wrapped program\'s output')

  options, args = parser.parse_args(argv)

  fmt = '%15s: %20.6f\n'  # label: time_value output format to line things up

  fp_num_re = re.compile(r'time is (\d*\.\d*) seconds')
  time_start = time.time()
  sys.stdout.write(fmt % ('Error margin', options.time_slop))
  sys.stdout.write(fmt % ('Start time' , time_start))

  def OutputPredicate(output_stream):
    # consume output_stream looking for output time value
    time_output = None
    for line in output_stream:
      if options.show_output:
        sys.stdout.write(line + '\n')
      mobj = fp_num_re.search(line)
      if time_output is None and mobj is not None:
        time_output = float(mobj.group(1))
        sys.stdout.write(fmt % ('Output time', time_output))
    time_end = time.time()
    sys.stdout.write(fmt % ('End time', time_end))
    if time_output is None:
      sys.stderr.write('time_check: No time output found.\n')
      return False
    return (time_start - options.time_slop <= time_output and
            time_output <= time_end + options.time_slop)

  if TimeCommand(args, OutputPredicate):
    sys.stdout.write('OK\n')
    sys.exit(0)
  else:
    sys.stdout.write('FAILED\n')
    sys.exit(1)

if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  sys.stdout.write('\n\n')
  sys.exit(retval)
