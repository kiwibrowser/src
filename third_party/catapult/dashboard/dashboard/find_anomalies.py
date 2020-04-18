# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Processes tests and creates new Anomaly entities.

This module contains the ProcessTest function, which searches the recent
points in a test for potential regressions or improvements, and creates
new Anomaly entities.
"""

import logging

from google.appengine.ext import ndb

from dashboard import email_sheriff
from dashboard import find_change_points
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import anomaly_config
from dashboard.models import graph_data
from dashboard.models import histogram
from tracing.value.diagnostics import reserved_infos

# Number of points to fetch and pass to FindChangePoints. A different number
# may be used if a test has a "max_window_size" anomaly config parameter.
DEFAULT_NUM_POINTS = 50

@ndb.synctasklet
def ProcessTests(test_keys):
  """Processes a list of tests to find new anoamlies.

  Args:
    test_keys: A list of TestMetadata ndb.Key's.
  """
  yield ProcessTestsAsync(test_keys)


@ndb.tasklet
def ProcessTestsAsync(test_keys):
  # Using a parallel yield here let's the tasklets for each _ProcessTest run
  # in parallel.
  yield [_ProcessTest(k) for k in test_keys]


@ndb.tasklet
def _ProcessTest(test_key):
  """Processes a test to find new anomalies.

  Args:
    test_key: The ndb.Key for a TestMetadata.
  """
  test = yield test_key.get_async()

  sheriff = yield _GetSheriffForTest(test)
  if not sheriff:
    logging.error('No sheriff for %s', test_key)
    raise ndb.Return(None)

  config = yield anomaly_config.GetAnomalyConfigDictAsync(test)
  max_num_rows = config.get('max_window_size', DEFAULT_NUM_POINTS)
  rows = yield GetRowsToAnalyzeAsync(test, max_num_rows)
  # If there were no rows fetched, then there's nothing to analyze.
  if not rows:
    # In some cases (e.g. if some points are deleted) it might be possible
    # that last_alerted_revision is incorrect. In this case, reset it.
    highest_rev = yield _HighestRevision(test_key)
    if test.last_alerted_revision > highest_rev:
      logging.error('last_alerted_revision %d is higher than highest rev %d '
                    'for test %s; setting last_alerted_revision to None.',
                    test.last_alerted_revision, highest_rev, test.test_path)
      test.last_alerted_revision = None
      yield test.put_async()
    logging.error('No rows fetched for %s', test.test_path)
    raise ndb.Return(None)

  # Get anomalies and check if they happen in ref build also.
  change_points = FindChangePointsForTest(rows, config)
  change_points = yield _FilterAnomaliesFoundInRef(
      change_points, test_key, len(rows))

  anomalies = yield [_MakeAnomalyEntity(c, test, rows) for c in change_points]

  # If no new anomalies were found, then we're done.
  if not anomalies:
    return

  logging.info('Created %d anomalies', len(anomalies))
  logging.info(' Test: %s', test_key.id())
  logging.info(' Sheriff: %s', test.sheriff.id())

  # Update the last_alerted_revision property of the test.
  test.last_alerted_revision = anomalies[-1].end_revision
  yield test.put_async()

  yield ndb.put_multi_async(anomalies)

  # TODO(simonhatch): email_sheriff.EmailSheriff() isn't a tasklet yet, so this
  # code will run serially.
  # Email sheriff about any new regressions.
  for anomaly_entity in anomalies:
    if (anomaly_entity.bug_id is None and
        not anomaly_entity.is_improvement and
        not sheriff.summarize):
      email_sheriff.EmailSheriff(sheriff, test, anomaly_entity)


@ndb.synctasklet
def GetRowsToAnalyze(test, max_num_rows):
  """Gets the Row entities that we want to analyze.

  Args:
    test: The TestMetadata entity to get data for.
    max_num_rows: The maximum number of points to get.

  Returns:
    A list of the latest Rows after the last alerted revision, ordered by
    revision. These rows are fetched with t a projection query so they only
    have the revision and value properties.
  """
  result = yield GetRowsToAnalyzeAsync(test, max_num_rows)
  raise ndb.Return(result)


@ndb.tasklet
def GetRowsToAnalyzeAsync(test, max_num_rows):
  query = graph_data.Row.query(projection=['revision', 'value'])
  query = query.filter(
      graph_data.Row.parent_test == utils.OldStyleTestKey(test.key))

  # The query is ordered in descending order by revision because we want
  # to get the newest points.
  query = query.filter(graph_data.Row.revision > test.last_alerted_revision)
  query = query.order(-graph_data.Row.revision)

  # However, we want to analyze them in ascending order.
  rows = yield query.fetch_async(limit=max_num_rows)
  raise ndb.Return(list(reversed(rows)))


@ndb.tasklet
def _HighestRevision(test_key):
  """Gets the revision number of the Row with the highest ID for a test."""
  query = graph_data.Row.query(
      graph_data.Row.parent_test == utils.OldStyleTestKey(test_key))
  query = query.order(-graph_data.Row.revision)
  highest_row_key = yield query.get_async(keys_only=True)
  if highest_row_key:
    raise ndb.Return(highest_row_key.id())
  raise ndb.Return(None)


@ndb.tasklet
def _FilterAnomaliesFoundInRef(change_points, test_key, num_rows):
  """Filters out the anomalies that match the anomalies in ref build.

  Background about ref build tests: Variation in test results can be caused
  by changes in Chrome or changes in the test-running environment. The ref
  build results are results from a reference (stable) version of Chrome run
  in the same environment. If an anomaly happens in the ref build results at
  the same time as an anomaly happened in the test build, that suggests that
  the variation was caused by a change in the test-running environment, and
  can be ignored.

  Args:
    change_points: ChangePoint objects returned by FindChangePoints.
    test_key: ndb.Key of monitored TestMetadata.
    num_rows: Number of Rows that were analyzed from the test. When fetching
        the ref build Rows, we need not fetch more than |num_rows| rows.

  Returns:
    A copy of |change_points| possibly with some entries filtered out.
    Any entries in |change_points| whose end revision matches that of
    an anomaly found in the corresponding ref test will be filtered out.
  """
  # Get anomalies for ref build.
  ref_test = yield _CorrespondingRefTest(test_key)
  if not ref_test:
    raise ndb.Return(change_points[:])

  ref_config = anomaly_config.GetAnomalyConfigDict(ref_test)
  ref_rows = yield GetRowsToAnalyzeAsync(ref_test, num_rows)
  ref_change_points = FindChangePointsForTest(ref_rows, ref_config)
  if not ref_change_points:
    raise ndb.Return(change_points[:])

  # We need to still alert on benchmark_duration, even if the ref moves since
  # that can signal some blow-up in cycle time. If we decide to expand this
  # to a greater set of metrics, we should move this to something more
  # generic like stored_object.
  test_path = utils.TestPath(test_key)
  if test_path.split('/')[-1] == 'benchmark_duration':
    raise ndb.Return(change_points[:])

  change_points_filtered = []
  for c in change_points:
    # Log information about what anomaly got filtered and what did not.
    if not _IsAnomalyInRef(c, ref_change_points):
      logging.info('Nothing was filtered out for test %s, and revision %s',
                   test_path, c.x_value)
      change_points_filtered.append(c)
    else:
      logging.info('Filtering out anomaly for test %s, and revision %s',
                   test_path, c.x_value)
  raise ndb.Return(change_points_filtered)


@ndb.tasklet
def _CorrespondingRefTest(test_key):
  """Returns the TestMetadata for the corresponding ref build trace, or None."""
  test_path = utils.TestPath(test_key)
  possible_ref_test_paths = [test_path + '_ref', test_path + '/ref']
  for path in possible_ref_test_paths:
    ref_test = yield utils.TestKey(path).get_async()
    if ref_test:
      raise ndb.Return(ref_test)
  raise ndb.Return(None)


def _IsAnomalyInRef(change_point, ref_change_points):
  """Checks if anomalies are detected in both ref and non ref build.

  Args:
    change_point: A find_change_points.ChangePoint object to check.
    ref_change_points: List of find_change_points.ChangePoint objects
        found for a ref build series.

  Returns:
    True if there is a match found among the ref build series change points.
  """
  for ref_change_point in ref_change_points:
    if change_point.x_value == ref_change_point.x_value:
      return True
  return False


@ndb.tasklet
def _GetSheriffForTest(test):
  """Gets the Sheriff for a test, or None if no sheriff."""
  if test.sheriff:
    sheriff = yield test.sheriff.get_async()
    raise ndb.Return(sheriff)
  raise ndb.Return(None)


def _GetImmediatelyPreviousRevisionNumber(later_revision, rows):
  """Gets the revision number of the Row immediately before the given one.

  Args:
    later_revision: A revision number.
    rows: List of Row entities in ascending order by revision.

  Returns:
    The revision number just before the given one.
  """
  for row in reversed(rows):
    if row.revision < later_revision:
      return row.revision
  assert False, 'No matching revision found in |rows|.'


def _GetRefBuildKeyForTest(test):
  """TestMetadata key of the reference build for the given test, if one exists.

  Args:
    test: the TestMetadata entity to get the ref build for.

  Returns:
    A TestMetadata key if found, or None if not.
  """
  potential_path = '%s/ref' % test.test_path
  potential_test = utils.TestKey(potential_path).get()
  if potential_test:
    return potential_test.key
  potential_path = '%s_ref' % test.test_path
  potential_test = utils.TestKey(potential_path).get()
  if potential_test:
    return potential_test.key
  return None


def _GetDisplayRange(old_end, rows):
  """Get the revision range using a_display_rev, if applicable.

  Args:
    old_end: the x_value from the change_point
    rows: List of Row entities in asscending order by revision.

  Returns:
    A end_rev, start_rev tuple with the correct revision.
  """
  start_rev = end_rev = 0
  for row in reversed(rows):
    if (row.revision == old_end and
        hasattr(row, 'r_commit_pos')):
      end_rev = row.r_commit_pos
    elif (row.revision < old_end and
          hasattr(row, 'r_commit_pos')):
      start_rev = row.r_commit_pos + 1
      break
  if not end_rev or not start_rev:
    end_rev = start_rev = None
  return start_rev, end_rev


@ndb.tasklet
def _MakeAnomalyEntity(change_point, test, rows):
  """Creates an Anomaly entity.

  Args:
    change_point: A find_change_points.ChangePoint object.
    test: The TestMetadata entity that the anomalies were found on.
    rows: List of Row entities that the anomalies were found on.

  Returns:
    An Anomaly entity, which is not yet put in the datastore.
  """
  end_rev = change_point.x_value
  start_rev = _GetImmediatelyPreviousRevisionNumber(end_rev, rows) + 1
  display_start = display_end = None
  if test.master_name == 'ClankInternal':
    display_start, display_end = _GetDisplayRange(change_point.x_value, rows)
  median_before = change_point.median_before
  median_after = change_point.median_after

  queried_diagnostics = yield (
      histogram.SparseDiagnostic.GetMostRecentValuesByNamesAsync(
          test.key, set([reserved_infos.BUG_COMPONENTS.name,
                         reserved_infos.OWNERS.name])))

  bug_components = queried_diagnostics.get(reserved_infos.BUG_COMPONENTS.name)

  ownership_information = {
      'emails': queried_diagnostics.get(reserved_infos.OWNERS.name),
      'component': (bug_components[0] if bug_components else None)}

  new_anomaly = anomaly.Anomaly(
      start_revision=start_rev,
      end_revision=end_rev,
      median_before_anomaly=median_before,
      median_after_anomaly=median_after,
      segment_size_before=change_point.size_before,
      segment_size_after=change_point.size_after,
      window_end_revision=change_point.window_end,
      std_dev_before_anomaly=change_point.std_dev_before,
      t_statistic=change_point.t_statistic,
      degrees_of_freedom=change_point.degrees_of_freedom,
      p_value=change_point.p_value,
      is_improvement=_IsImprovement(test, median_before, median_after),
      ref_test=_GetRefBuildKeyForTest(test),
      test=test.key,
      sheriff=test.sheriff,
      internal_only=test.internal_only,
      units=test.units,
      display_start=display_start,
      display_end=display_end,
      ownership=ownership_information)
  raise ndb.Return(new_anomaly)

def FindChangePointsForTest(rows, config_dict):
  """Gets the anomaly data from the anomaly detection module.

  Args:
    rows: The Row entities to find anomalies for, sorted backwards by revision.
    config_dict: Anomaly threshold parameters as a dictionary.

  Returns:
    A list of find_change_points.ChangePoint objects.
  """
  data_series = [(row.revision, row.value) for row in rows]
  return find_change_points.FindChangePoints(data_series, **config_dict)


def _IsImprovement(test, median_before, median_after):
  """Returns whether the alert is an improvement for the given test.

  Args:
    test: TestMetadata to get the improvement direction for.
    median_before: The median of the segment immediately before the anomaly.
    median_after: The median of the segment immediately after the anomaly.

  Returns:
    True if it is improvement anomaly, otherwise False.
  """
  if (median_before < median_after and
      test.improvement_direction == anomaly.UP):
    return True
  if (median_before >= median_after and
      test.improvement_direction == anomaly.DOWN):
    return True
  return False
