#!/usr/bin/env python2

import argparse
import math
import os
import re
import signal
import subprocess

class Runner(object):
  def __init__(self, input_cmd, timeout, comma_join, template, find_all):
    self._input_cmd = input_cmd
    self._timeout = timeout
    self._num_tries = 0
    self._comma_join = comma_join
    self._template = template
    self._find_all = find_all

  def estimate(self, included_ranges):
    result = 0
    for i in included_ranges:
      if isinstance(i, int):
        result += 1
      else:
        if i[1] - i[0] > 2:
          result += int(math.log(i[1] - i[0], 2))
        else:
          result += (i[1] - i[0])
    if self._find_all:
      return 2 * result
    else:
      return result

  def Run(self, included_ranges):
    def timeout_handler(signum, frame):
      raise RuntimeError('Timeout')

    self._num_tries += 1
    cmd_addition = ''
    for i in included_ranges:
      if isinstance(i, int):
        range_str = str(i)
      else:
        range_str = '{start}:{end}'.format(start=i[0], end=i[1])
      if self._comma_join:
        cmd_addition += ',' + range_str
      else:
        cmd_addition += ' -i ' + range_str

    if self._template:
      cmd = cmd_addition.join(re.split(r'%i' ,self._input_cmd))
    else:
      cmd = self._input_cmd + cmd_addition

    print cmd
    p = subprocess.Popen(cmd, shell = True, cwd = None,
      stdout = subprocess.PIPE, stderr = subprocess.PIPE, env = None)
    if self._timeout != -1:
      signal.signal(signal.SIGALRM, timeout_handler)
      signal.alarm(self._timeout)

    try:
      _, _ = p.communicate()
      if self._timeout != -1:
        signal.alarm(0)
    except:
      try:
        os.kill(p.pid, signal.SIGKILL)
      except OSError:
        pass
      print 'Timeout'
      return -9
    print '===Return Code===: ' + str(p.returncode)
    print '===Remaining Steps (approx)===: ' \
      + str(self.estimate(included_ranges))
    return p.returncode

def flatten(tree):
  if isinstance(tree, list):
    result = []
    for node in tree:
      result.extend(flatten(node))
    return result
  else:
    return [tree] # leaf

def find_failures(runner, current_interval, include_ranges, find_all):
  if current_interval[0] == current_interval[1]:
    return []
  mid = (current_interval[0] + current_interval[1]) / 2

  first_half = (current_interval[0], mid)
  second_half = (mid, current_interval[1])

  exit_code_2 = 0

  exit_code_1 = runner.Run([first_half] + include_ranges)
  if find_all or exit_code_1 == 0:
    exit_code_2 = runner.Run([second_half] + include_ranges)

  if exit_code_1 == 0 and exit_code_2 == 0:
    # Whole range fails but both halves pass
    # So, some conjunction of functions cause a failure, but none individually.
    partial_result = flatten(find_failures(runner, first_half, [second_half]
                             + include_ranges, find_all))
    # Heavy list concatenation, but this is insignificant compared to the
    # process run times
    partial_result.extend(flatten(find_failures(runner, second_half,
                          partial_result + include_ranges, find_all)))
    return [partial_result]
  else:
    result = []
    if exit_code_1 != 0:
      if first_half[1] == first_half[0] + 1:
        result.append(first_half[0])
      else:
        result.extend(find_failures(runner, first_half,
                                     include_ranges, find_all))
    if exit_code_2 != 0:
      if second_half[1] == second_half[0] + 1:
        result.append(second_half[0])
      else:
        result.extend(find_failures(runner, second_half,
                                     include_ranges, find_all))
    return result


def main():
  '''
  Helper Script for Automating Bisection Debugging

  Example Invocation:
  bisection-tool.py --cmd 'bisection-test.py -c 2x3' --end 1000 --timeout 60

  This will invoke 'bisection-test.py -c 2x3' starting with the range -i 0:1000
  If that fails, it will subdivide the range (initially 0:500 and 500:1000)
  recursively to pinpoint a combination of singletons that are needed to cause
  the input to return a non zero exit code or timeout.

  For investigating an error in the generated code:
  bisection-tool.py --cmd './pydir/szbuild_spec2k.py --run 188.ammp'

  For Subzero itself crashing,
  bisection-tool.py --cmd 'pnacl-sz -translate-only=' --comma-join=1
  The --comma-join flag ensures the ranges are formatted in the manner pnacl-sz
  expects.

  If the range specification is not to be appended on the input:
  bisection-tool.py --cmd 'echo %i; cmd-main %i; cmd-post' --template=1

  '''
  argparser = argparse.ArgumentParser(main.__doc__)
  argparser.add_argument('--cmd', required=True,  dest='cmd',
                           help='Runnable command')

  argparser.add_argument('--start', dest='start', default=0,
                           help='Start of initial range')

  argparser.add_argument('--end', dest='end', default=50000,
                           help='End of initial range')

  argparser.add_argument('--timeout', dest='timeout', default=60,
                           help='Timeout for each invocation of the input')

  argparser.add_argument('--all', type=int, choices=[0,1], default=1,
                           dest='all', help='Find all failures')

  argparser.add_argument('--comma-join', type=int, choices=[0,1], default=0,
                           dest='comma_join', help='Use comma to join ranges')

  argparser.add_argument('--template', type=int, choices=[0,1], default=0,
                           dest='template',
                           help='Replace %%i in the cmd string with the ranges')


  args = argparser.parse_args()

  fail_list = []

  initial_range = (int(args.start), int(args.end))
  timeout = int(args.timeout)
  runner = Runner(args.cmd, timeout, args.comma_join, args.template, args.all)
  if runner.Run([initial_range]) != 0:
    fail_list = find_failures(runner, initial_range, [], args.all)
  else:
    print 'Pass'
    # The whole input range works, maybe check subzero build flags?
    # Also consider widening the initial range (control with --start and --end)

  if fail_list:
    print 'Failing Items:'
    for fail in fail_list:
      if isinstance(fail, list):
        fail.sort()
        print '[' + ','.join(str(x) for x in fail) + ']'
      else:
        print fail
  print 'Number of tries: ' + str(runner._num_tries)

if __name__ == '__main__':
  main()
