# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides an interface for debugging the anomaly detection function."""

import json
import urllib

from dashboard import find_anomalies
from dashboard import find_change_points
from dashboard.common import datastore_hooks
from dashboard.common import request_handler
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import anomaly_config
from dashboard.models import graph_data

# Default number of points before and after a point to analyze.
_NUM_BEFORE = 40
_NUM_AFTER = 10


class QueryParameterError(Exception):
  pass


class DebugAlertHandler(request_handler.RequestHandler):
  """Request handler for the /debug_alert page."""

  def get(self):
    """Displays UI for debugging the anomaly detection function.

    Request parameters:
      test_path: Full test path (Master/bot/suite/chart) for test with alert.
      rev: A revision (Row id number) to center the graph on.
      num_before: Maximum number of points after the given revision to get.
      num_after: Maximum number of points before the given revision.
      config: Config parameters for in JSON form.

    Outputs:
      A HTML page with a chart (if test_path is given) and a form.
    """
    try:
      test = self._GetTest()
      num_before, num_after = self._GetNumBeforeAfter()
      config_name = self._GetConfigName(test)
      config_dict = anomaly_config.CleanConfigDict(self._GetConfigDict(test))
    except QueryParameterError as e:
      self.RenderHtml('debug_alert.html', {'error': e.message})
      return

    revision = self.request.get('rev')
    if revision:
      rows = _FetchRowsAroundRev(test, int(revision), num_before, num_after)
    else:
      rows = _FetchLatestRows(test, num_before)

    chart_series = _ChartSeries(rows)
    lookup = _RevisionList(rows)

    # Get the anomaly data from the new anomaly detection module. This will
    # also be passed to the template so that it can be shown on the page.
    change_points = SimulateAlertProcessing(chart_series, **config_dict)
    anomaly_indexes = [c.x_value for c in change_points]
    anomaly_points = [(i, chart_series[i][1]) for i in anomaly_indexes]
    anomaly_segments = _AnomalySegmentSeries(change_points)

    plot_data = _GetPlotData(chart_series, anomaly_points, anomaly_segments)

    # Render the debug_alert page with all of the parameters filled in.
    self.RenderHtml('debug_alert.html', {
        'test_path': test.test_path,
        'rev': revision or '',
        'num_before': num_before,
        'num_after': num_after,
        'sheriff_name': 'None' if not test.sheriff else test.sheriff.id(),
        'config_name': config_name,
        'config_json': json.dumps(config_dict, indent=2, sort_keys=True),
        'plot_data': json.dumps(plot_data),
        'lookup': json.dumps(lookup),
        'anomalies': json.dumps([c.AsDict() for c in change_points]),
        'csv_url': _CsvUrl(test.test_path, rows),
        'graph_url': _GraphUrl(test, revision),
        'stored_anomalies': _FetchStoredAnomalies(test, lookup),
    })

  def post(self):
    """A POST request to this endpoint does the same thing as a GET request."""
    return self.get()

  def _GetTest(self):
    test_path = self.request.get('test_path')
    if not test_path:
      raise QueryParameterError('No test specified.')
    test = utils.TestKey(test_path).get()
    if not test:
      raise QueryParameterError('Test "%s" not found.' % test_path)
    return test

  def _GetNumBeforeAfter(self):
    try:
      num_before = int(self.request.get('num_before', _NUM_BEFORE))
      num_after = int(self.request.get('num_after', _NUM_AFTER))
    except ValueError:
      raise QueryParameterError('Invalid "num_before" or "num_after".')
    return num_before, num_after

  def _GetConfigName(self, test):
    """Gets the name of the custom anomaly threshold, just for display."""
    if test.overridden_anomaly_config:
      return test.overridden_anomaly_config.string_id()
    if self.request.get('config'):
      return 'Custom config'
    return 'Default config'

  def _GetConfigDict(self, test):
    """Gets the name of the anomaly threshold dict to use."""
    input_config_json = self.request.get('config')
    if not input_config_json:
      return anomaly_config.GetAnomalyConfigDict(test)
    try:
      return json.loads(input_config_json)
    except ValueError:
      raise QueryParameterError('Invalid JSON.')


def SimulateAlertProcessing(chart_series, **config_dict):
  """Finds the same alerts as would be found normally as points are added.

  Each time a new point is added to a data series on dashboard, the
  FindChangePoints function is called with some points from that series.
  In order to simulate this here, we need to repeatedly call FindChangePoints.

  Args:
    chart_series: A list of (x, y) pairs.
    **config_dict: An alert threshold config dict.

  Returns:
    A list of find_change_points.ChangePoint objects, one for each alert found.
  """
  all_change_points = []
  highest_x = None  # This is used to avoid finding duplicate alerts.
  # The number of points that are passed in to FindChangePoints normally may
  # depend on either the specific "max_window_size" value or another default
  # used in find_anomalies.
  window = config_dict.get('max_window_size', find_anomalies.DEFAULT_NUM_POINTS)
  for end in range(1, len(chart_series)):
    start = max(0, end - window)
    series = chart_series[start:end]
    change_points = find_change_points.FindChangePoints(series, **config_dict)
    change_points = [c for c in change_points if c.x_value > highest_x]
    if change_points:
      highest_x = max(c.x_value for c in change_points)
      all_change_points.extend(change_points)
  return all_change_points


def _AnomalySegmentSeries(change_points):
  """Makes a list of data series for showing segments next to anomalies.

  Args:
    change_points: A list of find_change_points.ChangePoint objects.

  Returns:
    A list of data series (lists of pairs) to be graphed by Flot.
  """
  # We make a separate series for each anomaly, since segments may overlap.
  anomaly_series_list = []

  for change_point in change_points:
    anomaly_series = []

    # In a Flot data series, null is treated as a special value which
    # indicates a discontinuity. We want to end each segment with null
    # so that they show up as separate segments on the graph.
    anomaly_series.append([change_point.window_start, None])

    for x in range(change_point.window_start + 1, change_point.x_value):
      anomaly_series.append([x, change_point.median_before])
    anomaly_series.append([change_point.x_value, None])

    for x in range(change_point.x_value + 1, change_point.window_end + 1):
      anomaly_series.append([x, change_point.median_after])
    anomaly_series.append([change_point.window_end, None])
    anomaly_series_list.append(anomaly_series)

  return anomaly_series_list


def _GetPlotData(chart_series, anomaly_points, anomaly_segments):
  """Returns data to embed on the front-end for the chart.

  Args:
    chart_series: A series, i.e. a list of (index, value) pairs.
    anomaly_points: A series which contains the list of points where the
        anomalies were detected.
    anomaly_segments: A list of series, each of which represents one segment,
        which is a horizontal line across a range of values used in finding
        an anomaly.

  Returns:
    A list of data series, in the format accepted by Flot, which can be
    serialized as JSON and embedded on the page.
  """
  data = [
      {
          'data': chart_series,
          'color': '#666',
          'lines': {'show': True},
          'points': {'show': False},
      },
      {
          'data': anomaly_points,
          'color': '#f90',
          'lines': {'show': False},
          'points': {'show': True, 'radius': 4}
      },
  ]
  for series in anomaly_segments:
    data.append({
        'data': series,
        'color': '#f90',
        'lines': {'show': True},
        'points': {'show': False},
    })
  return data


def _ChartSeries(rows):
  """Returns a data series and index to revision map."""
  return [(i, r.value) for i, r in enumerate(rows)]


def _RevisionList(rows):
  """Returns a list of revisions."""
  return [r.revision for r in rows]


def _FetchLatestRows(test, num_points):
  """Does a query for the latest Row entities in the given test.

  Args:
    test: A TestMetadata entity to fetch Row entities for.
    num_points: Number of points to fetch.

  Returns:
    A list of Row entities, ordered by revision. The number to fetch is limited
    to the number that is expected to be processed at once by GASP.
  """
  assert utils.IsInternalUser() or not test.internal_only
  datastore_hooks.SetSinglePrivilegedRequest()
  return list(reversed(
      graph_data.GetLatestRowsForTest(test.key, num_points)))


def _FetchRowsAroundRev(test, revision, num_before, num_after):
  """Fetches Row entities before and after a given revision.

  Args:
    test: A TestMetadata entity.
    revision: A Row ID.
    num_before: Maximum number of Rows before |revision| to fetch.
    num_after: Max number of Rows starting from |revision| to fetch.

  Returns:
    A list of Row entities ordered by ID. The Row entities will have at least
    the "revision" and "value" properties, which are the only ones relevant
    to their use in this module.
  """
  assert utils.IsInternalUser() or not test.internal_only
  return graph_data.GetRowsForTestBeforeAfterRev(
      test.key, revision, num_before, num_after)


def _FetchStoredAnomalies(test, revisions):
  """Makes a list of data about Anomaly entities for a Test."""
  stored_anomalies = anomaly.Anomaly.GetAlertsForTest(test.key)

  stored_anomaly_dicts = []
  for a in stored_anomalies:
    if a.end_revision > revisions[0]:
      stored_anomaly_dicts.append({
          'revision': a.end_revision,
          'median_before': a.median_before_anomaly,
          'median_after': a.median_after_anomaly,
          'percent_changed': a.percent_changed,
          'bug_id': _GetDisplayBugId(a.bug_id),
          'timestamp': a.timestamp,
      })
  return stored_anomaly_dicts


def _CsvUrl(test_path, rows):
  """Constructs an URL for requesting data from /graph_csv for |rows|."""
  # Using a list of pairs ensures a predictable order for the parameters.
  params = [('test_path', test_path)]
  if rows:
    params += [
        ('num_points', len(rows)),
        ('rev', rows[-1].revision),
    ]
  return '/graph_csv?%s' % urllib.urlencode(params)


def _GraphUrl(test, revision):
  """Constructs an URL for requesting data from /graph_csv for |rows|."""
  params = [
      ('masters', test.master_name),
      ('bots', test.bot_name),
      ('tests', '/'.join(test.test_path.split('/')[2:])),
  ]
  if revision:
    params.append(('rev', revision))
  return '/report?%s' % urllib.urlencode(params)


def _GetDisplayBugId(bug_id):
  """Returns a display string for the given bug ID property of an anomaly."""
  special_ids = {-1: 'INVALID', -2: 'IGNORE', None: 'NONE'}
  return special_ids.get(bug_id, str(bug_id))
