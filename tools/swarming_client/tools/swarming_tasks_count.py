#!/usr/bin/env python
# Copyright 2015 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Calculate statistics about tasks, counts per day.

Saves the data fetched from the server into a json file to enable reprocessing
the data without having to always fetch from the server.
"""

import collections
import datetime
import json
import logging
import optparse
import os
import subprocess
import Queue
import threading
import sys
import urllib


CLIENT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(
    __file__.decode(sys.getfilesystemencoding()))))
sys.path.insert(0, CLIENT_DIR)


from third_party import colorama
from utils import graph
from utils import threading_utils


_EPOCH = datetime.datetime.utcfromtimestamp(0)


def parse_time_option(value):
  """Converts time as an option into a datetime.datetime.

  Returns None if not specified.
  """
  if not value:
    return None
  try:
    return _EPOCH + datetime.timedelta(seconds=int(value))
  except ValueError:
    pass
  for fmt in ('%Y-%m-%d',):
    try:
      return datetime.datetime.strptime(value, fmt)
    except ValueError:
      pass
  raise ValueError('Failed to parse %s' % value)


def _get_epoch(t):
  return int((t - _EPOCH).total_seconds())


def _run_json(key, process, cmd):
  """Runs cmd and calls process with the decoded json."""
  logging.info('Running %s', ' '.join(cmd))
  raw = subprocess.check_output(cmd)
  logging.info('- returned %d bytes', len(raw))
  return key, process(json.loads(raw))


def _get_cmd(swarming, endpoint, start, end, state, tags):
  """Returns the command line to query this endpoint."""
  cmd = [
    sys.executable, os.path.join(CLIENT_DIR, 'swarming.py'),
    'query', '-S', swarming, '--limit', '0',
  ]
  data = [('start', start), ('end', end), ('state', state)]
  data.extend(('tags', tag) for tag in tags)
  return cmd + [endpoint + '?' + urllib.urlencode(data)]


def _flatten_dimensions(dimensions):
  items = {i['key']: i['value'] for i in dimensions}
  return ','.join('%s=%s' % (k, v) for k, v in sorted(items.iteritems()))


def fetch_tasks(swarming, start, end, state, tags, parallel):
  """Fetches the data for each task.

  Fetch per hour because otherwise it's too slow.
  """
  def process(data):
    """Returns the list of flattened dimensions for these tasks."""
    items = data.get('items', [])
    logging.info('- processing %d items', len(items))
    return [_flatten_dimensions(t['properties']['dimensions']) for t in items]
  delta = datetime.timedelta(hours=1)
  return _fetch_daily_internal(
      delta, swarming, process, 'tasks/requests', start, end, state, tags,
      parallel)


def fetch_counts(swarming, start, end, state, tags, parallel):
  """Fetches counts from swarming and returns it."""
  def process(data):
    return int(data['count'])
  delta = datetime.timedelta(days=1)
  return _fetch_daily_internal(
      delta, swarming, process, 'tasks/count', start, end, state, tags,
      parallel)


def _fetch_daily_internal(
    delta, swarming, process, endpoint, start, end, state, tags, parallel):
  """Executes 'process' by parallelizing it once per day."""
  out = {}
  with threading_utils.ThreadPool(1, parallel, 0) as pool:
    while start < end:
      cmd = _get_cmd(
          swarming, endpoint, _get_epoch(start), _get_epoch(start + delta),
          state, tags)
      pool.add_task(0, _run_json, start.strftime('%Y-%m-%d'), process, cmd)
      start += delta
    for k, v in pool.iter_results():
      sys.stdout.write('.')
      sys.stdout.flush()
      out[k] = v
  print('')
  return out


def present_dimensions(items, daily_count):
  # Split items per group.
  per_dimension = collections.defaultdict(lambda: collections.defaultdict(int))
  for date, dimensions in items.iteritems():
    for d in dimensions:
      per_dimension[d][date] += 1
  for i, (dimension, data) in enumerate(sorted(per_dimension.iteritems())):
    print(
        '%s%s%s' % (
          colorama.Style.BRIGHT + colorama.Fore.MAGENTA,
          dimension,
          colorama.Fore.RESET))
    present_counts(data, daily_count)
    if i != len(per_dimension) - 1:
      print('')


def present_counts(items, daily_count):
  months = collections.defaultdict(int)
  for day, count in sorted(items.iteritems()):
    month = day.rsplit('-', 1)[0]
    months[month] += count

  years = collections.defaultdict(int)
  for month, count in months.iteritems():
    year = month.rsplit('-', 1)[0]
    years[year] += count
  total = sum(months.itervalues())
  maxlen = len(str(total))

  if daily_count:
    for day, count in sorted(items.iteritems()):
      print('%s: %*d' % (day, maxlen, count))

  if len(items) > 1:
    for month, count in sorted(months.iteritems()):
      print('%s   : %*d' % (month, maxlen, count))
  if len(months) > 1:
    for year, count in sorted(years.iteritems()):
      print('%s      : %*d' % (year, maxlen, count))
  if len(years) > 1:
    print('Total     : %*d' % (maxlen, total))
  if not daily_count:
    print('')
    graph.print_histogram(items)


STATES = (
    'PENDING',
    'RUNNING',
    'PENDING_RUNNING',
    'COMPLETED',
    'COMPLETED_SUCCESS',
    'COMPLETED_FAILURE',
    'EXPIRED',
    'TIMED_OUT',
    'BOT_DIED',
    'CANCELED',
    'ALL',
    'DEDUPED')


def main():
  colorama.init()
  parser = optparse.OptionParser(description=sys.modules['__main__'].__doc__)
  tomorrow = datetime.datetime.utcnow().date() + datetime.timedelta(days=1)
  year = datetime.datetime(tomorrow.year, 1, 1)
  parser.add_option(
      '-S', '--swarming',
      metavar='URL', default=os.environ.get('SWARMING_SERVER', ''),
      help='Swarming server to use')

  group = optparse.OptionGroup(parser, 'Filtering')
  group.add_option(
      '--start', default=year.strftime('%Y-%m-%d'),
      help='Starting date in UTC; defaults to start of year: %default')
  group.add_option(
      '--end', default=tomorrow.strftime('%Y-%m-%d'),
      help='End date in UTC (excluded); defaults to tomorrow: %default')
  group.add_option(
      '--state', default='ALL', type='choice', choices=STATES,
      help='State to filter on. Values are: %s' % ', '.join(STATES))
  group.add_option(
      '--tags', action='append', default=[],
      help='Tags to filter on; use this to filter on dimensions')
  parser.add_option_group(group)

  group = optparse.OptionGroup(parser, 'Presentation')
  group.add_option(
      '--show-dimensions', action='store_true',
      help='Show the dimensions; slower')
  group.add_option(
      '--daily-count', action='store_true',
      help='Show the daily count in raw number instead of histogram')
  parser.add_option_group(group)

  parser.add_option(
      '--json', default='counts.json',
      help='File containing raw data; default: %default')
  parser.add_option(
      '--parallel', default=100, type='int',
      help='Concurrent queries; default: %default')
  parser.add_option(
      '-v', '--verbose', action='count', default=0, help='Log')
  options, args = parser.parse_args()

  if args:
    parser.error('Unsupported argument %s' % args)
  logging.basicConfig(level=logging.DEBUG if options.verbose else logging.ERROR)
  start = parse_time_option(options.start)
  end = parse_time_option(options.end)
  print('From %s (%d) to %s (%d)' % (
      start, int((start- _EPOCH).total_seconds()),
      end, int((end - _EPOCH).total_seconds())))
  if options.swarming:
    if options.show_dimensions:
      data = fetch_tasks(
          options.swarming, start, end, options.state, options.tags,
          options.parallel)
    else:
      data = fetch_counts(
          options.swarming, start, end, options.state, options.tags,
          options.parallel)
    with open(options.json, 'wb') as f:
      json.dump(data, f)
  elif not os.path.isfile(options.json):
    parser.error('--swarming is required.')
  else:
    with open(options.json, 'rb') as f:
      data = json.load(f)

  print('')
  if options.show_dimensions:
    present_dimensions(data, options.daily_count)
  else:
    present_counts(data, options.daily_count)
  return 0


if __name__ == '__main__':
  sys.exit(main())
