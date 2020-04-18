#!/usr/bin/python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script produces csv data from multiple benchmarking runs with the
# spec2k harness.
#
# A typical usage would be
#
# export SPEC_RUN_REPETITIONS=3
# ./run_all.sh RunTimedBenchmarks  SetupPnaclX8664Opt  ref > ../timings.setting1
# [change the compiler settings]
# ./run_all.sh RunTimedBenchmarks  SetupPnaclX8664Opt  ref > ../timings.setting2
#
# tests/spec2k/extract_timings.py time.inline time.noinline time.lowinline
#
#  which produces output like:
# name                 , inline , noinline , lowinline
# ammp                 ,  250.47 ,  263.83 ,  262.20
# art                  ,  222.12 ,  219.36 ,  259.28
# bzip2                ,  179.05 ,  194.05 , missing
# crafty               ,   60.24 ,   73.33 , missing
# ...
#
# Alternatively, if your data already has the form:
#
# <bechmark> <setting> <value>
#
# You can run the tool like so:
#  tests/spec2k/extract_timings.py < <data-file>


import sys

# The name the individual settings derived from the filename in the order
# they were given on the command-line
SETTINGS = []
# dictionary of dictionaries accessed like so:
#     BENCHMARKS['benchmark']['setting']
BENCHMARKS = {}


def AddDataPoint(benchmark, setting, v):
  if setting not in SETTINGS:
    # TODO: linear search is slightly inefficient
    SETTINGS.append(setting)
  values = BENCHMARKS.get(benchmark, {})
  values[setting] = v
  BENCHMARKS[benchmark] = values


def ExtractResults(name, inp):
  for line in inp:
    if not line.startswith('RESULT'):
      continue
    tokens = line.split()
    # NOTE: the line we care about look like this:
    # 'RESULT runtime_equake: pnacl.opt.x8664= [107.36,116.28,116.4] secs'
    assert tokens[0] == 'RESULT'
    assert tokens[1].endswith(':')
    assert tokens[2].endswith('=')
    assert tokens[3].startswith('[')
    assert tokens[3].endswith(']')
    benchmark = tokens[1][:-1].split('_')[-1]
    data =  tokens[3][1:][:-1].split(',')
    data = [float(d) for d in data]
    m = min(data)
    AddDataPoint(benchmark, name, m)

# Note: we are intentionally not using the csv module
# as it does not provide nicely formatted output
def DumpRow(row):
  sys.stdout.write('%-20s' % row[0])
  for val in row[1:]:
    if type(val) == str:
      sys.stdout.write(', %10s' % val)
    else:
      sys.stdout.write(', %10.2f' % val)
  sys.stdout.write('\n')


def DumpCsv():
  row = ['name'] + SETTINGS
  DumpRow(row)

  for k in sorted(BENCHMARKS.keys()):
    row = [k]
    values = BENCHMARKS[k]
    for s in SETTINGS:
      if s in values:
        row.append(values[s])
      else:
        row.append('missing')
    DumpRow(row)


if len(sys.argv) > 1:
  for f in sys.argv[1:]:
    setting = f.split('.')[-1]
    fin = open(f)
    ExtractResults(setting, fin)
    fin.close()
else:
  for line in sys.stdin:
    tokens = line.split()
    if not tokens: continue
    assert len(tokens) == 3
    AddDataPoint(tokens[0], tokens[1], float(tokens[2]))

DumpCsv()
