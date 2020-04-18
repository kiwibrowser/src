#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Calculate reasonable timeout for each step as analysed by the actual runtimes
on the Swarming server.
"""

import Queue
import argparse
import json
import os
import subprocess
import sys
import threading
import time
import urllib


THIS_DIR = os.path.dirname(os.path.abspath(__file__))


def human_int(s):
  """Returns human readable time rounded to the second."""
  s = int(round(s))
  if s <= 60:
    return '%ds' % s
  m = s/60
  if m <= 60:
    return '%dm%02ds' % (m, s%60)
  return '%dh%02dm%02ds' % (m/60, m%60, s%60)


def human(s):
  """Returns human readable time rounded to the tenth of second."""
  if s <= 60:
    return '%.1fs' % s
  m = int(round(s/60))
  if m <= 60:
    return '%dm%04.1fs' % (m, s%60)
  return '%dh%02dm%04.1fs' % (m/60, m%60, s%60)


class Stats(object):
  """Holds runtimes statistics for a step run on a builder."""
  def __init__(self, builder, step, durations):
    self.builder = builder
    self.step = step
    self.durations = durations
    self.avg = sum(durations) / float(len(durations))
    self.len = len(durations)
    self.max = max(durations)
    self.timeout = max(120, int(round(self.max / 60.)) * 120)

  def __str__(self):
    return 'avg: %4ds   max: %4ds   timeout: %4ds' % (
        round(self.avg), round(self.max), self.timeout)


class Pool(object):
  def __init__(self, size):
    self._durations = []
    self._inputs = Queue.Queue()
    self._lock = threading.Lock()
    self._outputs = []
    self._start = time.time()
    self._total = 0
    self._threads = [
        threading.Thread(name=str(i), target=self._run) for i in xrange(size)
    ]
    for t in self._threads:
      t.start()

  def put(self, f):
    self._inputs.put(f)
    with self._lock:
      self._total += 1

  def join(self):
    for _ in xrange(len(self._threads)):
      self._inputs.put(None)
    try:
      for t in self._threads:
        while t.isAlive():
          t.join(0.1)
          self._print_eta()
    except KeyboardInterrupt:
      sys.stderr.write('\nInterrupted!\n')
    with self._lock:
      return self._outputs[:]

  def _print_eta(self):
    elapsed = human(time.time() - self._start)
    with self._lock:
      out = '\r%d/%d  Elapsed: %s' % (len(self._outputs), self._total, elapsed)
      if self._durations:
        avg = sum(self._durations) / float(len(self._durations))
        rem = self._total - len(self._outputs)
        eta = avg * rem / float(len(self._threads))
        out += '  ETA: %s   ' % human_int(eta)
    sys.stderr.write(out)
    sys.stderr.flush()

  def _run(self):
    while True:
      f = self._inputs.get()
      if not f:
        return
      s = time.time()
      o = f()
      e = time.time() - s
      with self._lock:
        self._durations.append(e)
        self._outputs.append(o)


def query(server, number, builder, step):
  q = 'tasks/list?%s' % urllib.urlencode([
      ('tags', 'buildername:%s' % builder),
      ('tags', 'name:%s' % step),
  ])
  cmd = [
      sys.executable, '../../tools/swarming_client/swarming.py', 'query',
      '-S', server, '--limit', str(number), q,
  ]
  out = subprocess.check_output(cmd, stderr=subprocess.PIPE)
  try:
    data = json.loads(out)
  except ValueError:
    sys.stderr.write(out)
    return None
  if not 'items' in data:
    # No task with this pattern.
    return None
  durations = [i['duration'] for i in data['items'] if i.get('duration')]
  if not durations:
    # There was tasks but none completed correctly, i.e. internal_failure.
    return None
  return Stats(builder, step, durations)


def extract_tags(data, test_name):
  """Returns all the tags that should be queried from a json file."""
  out = []
  for b, d in sorted(data.iteritems()):
    if not 'gtest_tests' in d:
      continue
    for t in d['gtest_tests']:
      if not t.get('swarming', {}).get('can_use_on_swarming_builders'):
        continue
      if test_name and t['test'] != test_name:
        continue
      out.append((b, t['test']))
  return out


def query_server(server, number, data):
  """Query the Swarming server to steps durations."""
  def _get_func(builder, step):
    return lambda: query(server, number, builder, step)
  # Limit to 256 threads, otherwise some OSes have trouble with it.
  p = Pool(min(len(data), 256))
  for builder, step in data:
    p.put(_get_func(builder, step))
  return p.join()


def main():
  os.chdir(THIS_DIR)
  parser = argparse.ArgumentParser(description=sys.modules[__name__].__doc__)
  parser.add_argument(
      '-f', metavar='chromium.foo.json', help='file to open', required=True)
  parser.add_argument('-s', metavar='foo_unittest', help='step to process')
  parser.add_argument(
      '-N', metavar='200', default=200, type=int,
      help='number of executions to look at')
  parser.add_argument(
      '-S', metavar='chromium-swarm.appspot.com',
      default='chromium-swarm.appspot.com', help='server to use')
  args = parser.parse_args()

  with open(args.f) as f:
    d = json.load(f)
  tags = extract_tags(d, args.s)
  if not tags:
    print('No step to process found')
    return 1
  out = [i for i in query_server(args.S, args.N, tags) if i]
  print('')
  maxbuilder = max(len(i.builder) for i in out)
  maxstep = max(len(i.step) for i in out)
  for i in sorted(out, key=lambda i: (i.builder, i.step)):
    print('%-*s / %-*s %s' % (maxbuilder, i.builder, maxstep, i.step, i))
  return 0


if __name__ == "__main__":
  sys.exit(main())
