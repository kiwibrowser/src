#!/usr/bin/env python2
import argparse
import sys

def main():
  desc = 'Crash simulator script, useful for testing the bisection tool.\
          bisection-tool.py --cmd "./pydir/bisection-test.py -c 2x3" \
          --end 1000 --timeout 60'
  argparser = argparse.ArgumentParser(description=desc)
  argparser.add_argument('--include', '-i', default=[], dest='include',
    action='append',
    help='Include list, single values or ranges')
  argparser.add_argument('--exclude', '-e', default=[], dest='exclude',
    action='append',
    help='Exclude list, single values or ranges')
  argparser.add_argument('--crash', '-c', default=[], dest='crash',
    action='append',
    help='Crash list, single values or x-separated combinations like 2x4')

  args = argparser.parse_args()

  included = {-1}
  for string in args.include:
    include_range = string.split(':')
    if len(include_range) == 1:
      included.add(int(include_range[0]))
    else:
      for num in range(int(include_range[0]), int(include_range[1])):
        included.add(num)

  for string in args.exclude:
    exclude_range = string.split(':')
    if len(exclude_range) == 1:
      try:
        included.remove(int(exclude_range[0]))
      except KeyError:
        pass # Exclude works without a matching include
    else:
      for num in range(int(exclude_range[0]), int(exclude_range[1])):
        included.remove(num)

  for string in args.crash:
    crash_combination = string.split('x')
    fail = True
    for crash in crash_combination:
      if not int(crash) in included:
        fail = False
    if fail:
      print 'Fail'
      exit(1)
  print 'Success'
  exit(0)

if __name__ == '__main__':
  main()
