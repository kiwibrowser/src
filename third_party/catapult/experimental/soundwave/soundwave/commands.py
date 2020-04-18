# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import pandas  # pylint: disable=import-error
import sqlite3

from soundwave import dashboard_api
from soundwave import pandas_sqlite
from soundwave import tables
from soundwave import worker_pool


def _FetchBugsWorker(args):
  api = dashboard_api.PerfDashboardCommunicator(args)
  con = sqlite3.connect(args.database_file, timeout=10)

  def Process(bug_id):
    bugs = tables.bugs.DataFrameFromJson(api.GetBugData(bug_id))
    pandas_sqlite.InsertOrReplaceRecords(con, 'bugs', bugs)

  worker_pool.Process = Process


def FetchAlertsData(args):
  api = dashboard_api.PerfDashboardCommunicator(args)
  con = sqlite3.connect(args.database_file)
  try:
    tables.CreateIfNeeded(con)
    alerts = tables.alerts.DataFrameFromJson(
        api.GetAlertData(args.benchmark, args.sheriff, args.days))
    print '%d alerts found!' % len(alerts)
    pandas_sqlite.InsertOrReplaceRecords(con, 'alerts', alerts)

    bug_ids = set(alerts['bug_id'].unique())
    bug_ids.discard(0)  # A bug_id of 0 means untriaged.
    print '%d bugs found!' % len(bug_ids)
    if args.use_cache:
      known_bugs = set(
          b for b in bug_ids if tables.bugs.Get(con, b) is not None)
      if known_bugs:
        print '(skipping %d bugs already in the database)' % len(known_bugs)
        bug_ids.difference_update(known_bugs)
  finally:
    con.close()

  total_seconds = worker_pool.Run(
      'Fetching data of %d bugs: ' % len(bug_ids),
      _FetchBugsWorker, args, bug_ids)
  print '[%.1f bugs per second]' % (len(bug_ids) / total_seconds)


def _IterStaleTestPaths(con, test_paths):
  """Iterate over test_paths yielding only those with stale or absent data.

  A test_path is considered to be stale if the most recent data point we have
  for it in the db is more than a day older.
  """
  a_day_ago = pandas.Timestamp.utcnow() - pandas.Timedelta(days=1)
  a_day_ago = a_day_ago.tz_convert(tz=None)

  for test_path in test_paths:
    latest = tables.timeseries.GetMostRecentPoint(con, test_path)
    if latest is None or latest['timestamp'] < a_day_ago:
      yield test_path


def _FetchTimeseriesWorker(args):
  api = dashboard_api.PerfDashboardCommunicator(args)
  con = sqlite3.connect(args.database_file, timeout=10)

  def Process(test_path):
    data = api.GetTimeseries(test_path, days=args.days)
    timeseries = tables.timeseries.DataFrameFromJson(data)
    pandas_sqlite.InsertOrReplaceRecords(con, 'timeseries', timeseries)

  worker_pool.Process = Process


def _ReadTestPathsFromFile(filename):
  with open(filename, 'rU') as f:
    for line in f:
      line = line.strip()
      if line and not line.startswith('#'):
        yield line


def FetchTimeseriesData(args):
  def _MatchesAllFilters(test_path):
    return all(f in test_path for f in args.filters)

  if args.benchmark is not None:
    api = dashboard_api.PerfDashboardCommunicator(args)
    test_paths = api.ListTestPaths(args.benchmark, sheriff=args.sheriff)
  elif args.input_file is not None:
    test_paths = list(_ReadTestPathsFromFile(args.input_file))
  else:
    raise NotImplementedError('Expected --benchmark or --input-file')

  if args.filters:
    test_paths = filter(_MatchesAllFilters, test_paths)
  num_found = len(test_paths)
  print '%d test paths found!' % num_found

  con = sqlite3.connect(args.database_file)
  try:
    tables.CreateIfNeeded(con)
    if args.use_cache:
      test_paths = list(_IterStaleTestPaths(con, test_paths))
      num_skipped = num_found - len(test_paths)
      if num_skipped:
        print '(skipping %d test paths already in the database)' % num_skipped
  finally:
    con.close()

  total_seconds = worker_pool.Run(
      'Fetching data of %d timeseries: ' % len(test_paths),
      _FetchTimeseriesWorker, args, test_paths)
  print '[%.1f test paths per second]' % (len(test_paths) / total_seconds)
