# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import re
import urllib

from common.buildbot import network


StackTraceLine = collections.namedtuple(
    'StackTraceLine', ('file', 'function', 'line', 'source'))


class Step(object):

  def __init__(self, data, build_url):
    self._name = data['name']
    self._status = data['results'][0]
    self._start_time, self._end_time = data['times']
    self._url = '%s/steps/%s' % (build_url, urllib.quote(self._name))

    self._log_link = None
    self._results_link = None
    for link_name, link_url in data['logs']:
      if link_name == 'stdio':
        self._log_link = link_url + '/text'
      elif link_name == 'json.output':
        self._results_link = link_url + '/text'

    # Property caches.
    self._log = None
    self._results = None
    self._stack_trace = None

  def __str__(self):
    return self.name

  @property
  def name(self):
    return self._name

  @property
  def url(self):
    return self._url

  @property
  def status(self):
    return self._status

  @property
  def start_time(self):
    return self._start_time

  @property
  def end_time(self):
    return self._end_time

  @property
  def log_link(self):
    return self._log_link

  @property
  def results_link(self):
    return self._results_link

  @property
  def log(self):
    if self._log is None:
      if not self.log_link:
        return None

      self._log = network.FetchText(self.log_link)
    return self._log

  @property
  def results(self):
    if self._results is None:
      if not self.results_link:
        return None

      self._results = network.FetchData(self.results_link)
    return self._results

  @property
  def stack_trace(self):
    if self._stack_trace is None:
      self._stack_trace = _ParseTraceFromLog(self.log)
    return self._stack_trace


def _ParseTraceFromLog(log):
  """Searches the log for a stack trace and returns a structured representation.

  This function supports both default Python-style stacks and Telemetry-style
  stacks. It returns the first stack trace found in the log - sometimes a bug
  leads to a cascade of failures, so the first one is usually the root cause.

  Args:
    log: A string containing Python or Telemetry stack traces.

  Returns:
    Two values, or (None, None) if no stack trace was found.
    The first is a tuple of StackTraceLine objects, most recent call last.
    The second is a string with the type and description of the exception.
  """
  log_iterator = iter(log.splitlines())
  for line in log_iterator:
    if line == 'Traceback (most recent call last):':
      break
  else:
    return (None, None)

  stack_trace = []
  while True:
    line = log_iterator.next()
    match_python = re.match(r'\s*File "(?P<file>.+)", line (?P<line>[0-9]+), '
                            r'in (?P<function>.+)', line)
    match_telemetry = re.match(r'\s*(?P<function>.+) at '
                               r'(?P<file>.+):(?P<line>[0-9]+)', line)
    match = match_python or match_telemetry
    if not match:
      exception = line
      break
    trace_line = match.groupdict()
    # Use the base name, because the path will be different
    # across platforms and configurations.
    file_base_name = trace_line['file'].split('/')[-1].split('\\')[-1]
    source = log_iterator.next().strip()
    stack_trace.append(StackTraceLine(
        file_base_name, trace_line['function'], trace_line['line'], source))

  return tuple(stack_trace), exception
