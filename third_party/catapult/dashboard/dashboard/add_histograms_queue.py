# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""URL endpoint to add new histograms to the datastore."""

import json
import logging
import sys

from google.appengine.ext import ndb

# TODO(eakuefner): Move these helpers so we don't have to import add_point or
# add_point_queue directly.
from dashboard import add_histograms
from dashboard import add_point
from dashboard import add_point_queue
from dashboard import find_anomalies
from dashboard import graph_revisions
from dashboard.common import datastore_hooks
from dashboard.common import histogram_helpers
from dashboard.common import request_handler
from dashboard.common import stored_object
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import graph_data
from dashboard.models import histogram
from tracing.value import histogram as histogram_module
from tracing.value import histogram_set
from tracing.value.diagnostics import diagnostic
from tracing.value.diagnostics import diagnostic_ref
from tracing.value.diagnostics import reserved_infos

DIAGNOSTIC_NAMES_TO_ANNOTATION_NAMES = {
    reserved_infos.CHROMIUM_COMMIT_POSITIONS.name: 'r_commit_pos',
    reserved_infos.V8_COMMIT_POSITIONS.name: 'r_v8_commit_pos',
    reserved_infos.CHROMIUM_REVISIONS.name: 'r_chromium',
    reserved_infos.V8_REVISIONS.name: 'r_v8_rev',
    # TODO(eakuefner): Add r_catapult_git to Dashboard revision_info map (see
    # https://github.com/catapult-project/catapult/issues/3545).
    reserved_infos.CATAPULT_REVISIONS.name: 'r_catapult_git',
    reserved_infos.ANGLE_REVISIONS.name: 'r_angle_git',
    reserved_infos.WEBRTC_REVISIONS.name: 'r_webrtc_git',
    reserved_infos.FUCHSIA_GARNET_REVISIONS.name: 'r_fuchsia_garnet_git',
    reserved_infos.FUCHSIA_PERIDOT_REVISIONS.name: 'r_fuchsia_peridot_git',
    reserved_infos.FUCHSIA_TOPAZ_REVISIONS.name: 'r_fuchsia_topaz_git',
    reserved_infos.FUCHSIA_ZIRCON_REVISIONS.name: 'r_fuchsia_zircon_git'
}


class BadRequestError(Exception):
  pass


def _CheckRequest(condition, msg):
  if not condition:
    raise BadRequestError(msg)


class AddHistogramsQueueHandler(request_handler.RequestHandler):
  """Request handler to process a histogram and add it to the datastore.

  This request handler is intended to be used only by requests using the
  task queue; it shouldn't be directly from outside.
  """

  def get(self):
    self.post()

  def post(self):
    """Adds a single histogram or sparse shared diagnostic to the datastore.

    The |data| request parameter can be either a histogram or a sparse shared
    diagnostic; the set of diagnostics that are considered sparse (meaning that
    they don't normally change on every upload for a given benchmark from a
    given bot) is shown in add_histograms.SPARSE_DIAGNOSTIC_TYPES.

    See https://goo.gl/lHzea6 for detailed information on the JSON format for
    histograms and diagnostics.

    Request parameters:
      data: JSON encoding of a histogram or shared diagnostic.
      revision: a revision, given as an int.
      test_path: the test path to which this diagnostic or histogram should be
          attached.
    """
    datastore_hooks.SetPrivilegedRequest()

    bot_whitelist_future = stored_object.GetAsync(
        add_point_queue.BOT_WHITELIST_KEY)

    params = json.loads(self.request.body)

    _PrewarmGets(params)

    bot_whitelist = bot_whitelist_future.get_result()

    # Roughly, the processing of histograms and the processing of rows can be
    # done in parallel since there are no dependencies.

    futures = []

    for p in params:
      futures.extend(_ProcessRowAndHistogram(p, bot_whitelist))

    ndb.Future.wait_all(futures)


def _GetStoryFromDiagnosticsDict(diagnostics):
  if not diagnostics:
    return None

  story_name = diagnostics.get(reserved_infos.STORIES.name)
  if not story_name:
    return None

  story_name = diagnostic.Diagnostic.FromDict(story_name)
  if story_name and len(story_name) == 1:
    return story_name.GetOnlyElement()
  return None


def _PrewarmGets(params):
  keys = set()

  for p in params:
    test_path = p['test_path']
    path_parts = test_path.split('/')

    keys.add(ndb.Key('Master', path_parts[0]))
    keys.add(ndb.Key('Bot', path_parts[1]))

    test_parts = path_parts[2:]
    test_key = '%s/%s' % (path_parts[0], path_parts[1])
    for p in test_parts:
      test_key += '/%s' % p
      keys.add(ndb.Key('TestMetadata', test_key))

  ndb.get_multi_async(list(keys))


def _ProcessRowAndHistogram(params, bot_whitelist):
  revision = int(params['revision'])
  test_path = params['test_path']
  benchmark_description = params['benchmark_description']
  data_dict = params['data']

  logging.info('Processing: %s', test_path)

  hist = histogram_module.Histogram.FromDict(data_dict)

  if hist.num_values == 0:
    return []

  test_path_parts = test_path.split('/')
  master = test_path_parts[0]
  bot = test_path_parts[1]
  benchmark_name = test_path_parts[2]
  histogram_name = test_path_parts[3]
  if len(test_path_parts) > 4:
    rest = '/'.join(test_path_parts[4:])
  else:
    rest = None
  full_test_name = '/'.join(test_path_parts[2:])
  internal_only = add_point_queue.BotInternalOnly(bot, bot_whitelist)
  extra_args = GetUnitArgs(hist.unit)

  unescaped_story_name = _GetStoryFromDiagnosticsDict(params.get('diagnostics'))

  # TDOO(eakuefner): Populate benchmark_description once it appears in
  # diagnostics.
  # https://github.com/catapult-project/catapult/issues/4096
  parent_test = add_point_queue.GetOrCreateAncestors(
      master, bot, full_test_name, internal_only=internal_only,
      unescaped_story_name=unescaped_story_name,
      benchmark_description=benchmark_description, **extra_args)
  test_key = parent_test.key

  statistics_scalars = hist.statistics_scalars
  legacy_parent_tests = {}

  # TODO(#4213): Stop doing this.
  if histogram_helpers.IsLegacyBenchmark(benchmark_name):
    statistics_scalars = {}

  for stat_name, scalar in statistics_scalars.iteritems():
    if histogram_helpers.ShouldFilterStatistic(
        histogram_name, benchmark_name, stat_name):
      continue
    extra_args = GetUnitArgs(scalar.unit)
    suffixed_name = '%s/%s_%s' % (
        benchmark_name, histogram_name, stat_name)
    if rest is not None:
      suffixed_name += '/' + rest
    legacy_parent_tests[stat_name] = add_point_queue.GetOrCreateAncestors(
        master, bot, suffixed_name, internal_only=internal_only,
        unescaped_story_name=unescaped_story_name, **extra_args)

  return [
      _AddRowsFromData(params, revision, parent_test, legacy_parent_tests),
      _AddHistogramFromData(params, revision, test_key, internal_only)]


@ndb.tasklet
def _AddRowsFromData(params, revision, parent_test, legacy_parent_tests):
  data_dict = params['data']
  test_key = parent_test.key

  stat_names_to_test_keys = {k: v.key for k, v in
                             legacy_parent_tests.iteritems()}
  rows = CreateRowEntities(
      data_dict, test_key, stat_names_to_test_keys, revision)
  if not rows:
    raise ndb.Return()

  yield ndb.put_multi_async(rows) + [r.UpdateParentAsync() for r in rows]

  tests_keys = []
  is_monitored = parent_test.sheriff and parent_test.has_rows
  if is_monitored:
    tests_keys.append(parent_test.key)

  for legacy_parent_test in legacy_parent_tests.itervalues():
    is_monitored = legacy_parent_test.sheriff and legacy_parent_test.has_rows
    if is_monitored:
      tests_keys.append(legacy_parent_test.key)

  tests_keys = [
      k for k in tests_keys if not add_point_queue.IsRefBuild(k)]

  # Updating of the cached graph revisions should happen after put because
  # it requires the new row to have a timestamp, which happens upon put.
  futures = [
      graph_revisions.AddRowsToCacheAsync(rows),
      find_anomalies.ProcessTestsAsync(tests_keys)]
  yield futures


@ndb.tasklet
def _AddHistogramFromData(params, revision, test_key, internal_only):
  data_dict = params['data']
  guid = data_dict['guid']
  diagnostics = params.get('diagnostics')
  new_guids_to_existing_diagnostics = yield ProcessDiagnostics(
      diagnostics, revision, test_key, internal_only)

  # TODO(eakuefner): Move per-histogram monkeypatching logic to Histogram.
  hs = histogram_set.HistogramSet()
  hs.ImportDicts([data_dict])
  # TODO(eakuefner): Share code for replacement logic with add_histograms
  for new_guid, existing_diagnostic in (
      new_guids_to_existing_diagnostics.iteritems()):
    hs.ReplaceSharedDiagnostic(
        new_guid, diagnostic_ref.DiagnosticRef(
            existing_diagnostic['guid']))
  data = hs.GetFirstHistogram().AsDict()

  entity = histogram.Histogram(
      id=guid, data=data, test=test_key, revision=revision,
      internal_only=internal_only)
  yield entity.put_async()


@ndb.tasklet
def ProcessDiagnostics(diagnostic_data, revision, test_key, internal_only):
  if not diagnostic_data:
    raise ndb.Return({})

  diagnostic_entities = []
  for name, diagnostic_datum in diagnostic_data.iteritems():
    # TODO(eakuefner): Pass map of guid to dict to avoid overhead
    guid = diagnostic_datum['guid']
    diagnostic_entities.append(histogram.SparseDiagnostic(
        id=guid, name=name, data=diagnostic_datum, test=test_key,
        start_revision=revision, end_revision=sys.maxint,
        internal_only=internal_only))
  new_guids_to_existing_diagnostics = yield (
      add_histograms.DeduplicateAndPutAsync(
          diagnostic_entities, test_key, revision))

  raise ndb.Return(new_guids_to_existing_diagnostics)


def GetUnitArgs(unit):
  unit_args = {
      'units': unit
  }
  # TODO(eakuefner): Port unit system to Python and use that here
  histogram_improvement_direction = unit.split('_')[-1]
  if histogram_improvement_direction == 'biggerIsBetter':
    unit_args['improvement_direction'] = anomaly.UP
  elif histogram_improvement_direction == 'smallerIsBetter':
    unit_args['improvement_direction'] = anomaly.DOWN
  else:
    unit_args['improvement_direction'] = anomaly.UNKNOWN
  return unit_args


def CreateRowEntities(
    histogram_dict, test_metadata_key, stat_names_to_test_keys, revision):
  h = histogram_module.Histogram.FromDict(histogram_dict)
  # TODO(eakuefner): Move this check into _PopulateNumericalFields once we
  # know that it's okay to put rows that don't have a value/error (see
  # https://github.com/catapult-project/catapult/issues/3564).
  if h.num_values == 0:
    return None

  rows = []

  row_dict = _MakeRowDict(revision, test_metadata_key.id(), h)
  properties = add_point.GetAndValidateRowProperties(row_dict)
  test_container_key = utils.GetTestContainerKey(test_metadata_key)
  rows.append(graph_data.Row(id=revision, parent=test_container_key,
                             **properties))

  for stat_name, suffixed_key in stat_names_to_test_keys.iteritems():
    row_dict = _MakeRowDict(revision, suffixed_key.id(), h, stat_name=stat_name)
    properties = add_point.GetAndValidateRowProperties(row_dict)
    test_container_key = utils.GetTestContainerKey(suffixed_key)
    rows.append(graph_data.Row(
        id=revision, parent=suffixed_key, **properties))

  return rows

def _MakeRowDict(revision, test_path, tracing_histogram, stat_name=None):
  d = {}
  test_parts = test_path.split('/')
  d['master'] = test_parts[0]
  d['bot'] = test_parts[1]
  d['test'] = '/'.join(test_parts[2:])
  d['revision'] = revision
  d['supplemental_columns'] = {}

  # TODO(#3628): Remove this annotation when the frontend displays the full
  # histogram and all its diagnostics including the full set of trace urls.
  trace_url_set = tracing_histogram.diagnostics.get(
      reserved_infos.TRACE_URLS.name)
  # We don't show trace URLs for summary values in the legacy pipeline
  is_summary = reserved_infos.SUMMARY_KEYS.name in tracing_histogram.diagnostics
  if trace_url_set and not is_summary:
    d['supplemental_columns']['a_tracing_uri'] = list(trace_url_set)[-1]

  for diag_name, annotation in DIAGNOSTIC_NAMES_TO_ANNOTATION_NAMES.iteritems():
    revision_info = tracing_histogram.diagnostics.get(diag_name)
    value = list(revision_info) if revision_info else None
    # TODO(eakuefner): Formalize unique-per-upload diagnostics to make this
    # check an earlier error. RevisionInfo's fields have to be lists, but there
    # should be only one revision of each type per upload.
    if not value:
      continue
    _CheckRequest(
        len(value) == 1,
        'RevisionInfo fields must contain at most one revision')

    d['supplemental_columns'][annotation] = value[0]

  _AddStdioUris(tracing_histogram, d)

  if stat_name is not None:
    d['value'] = tracing_histogram.statistics_scalars[stat_name].value
    d['error'] = 0.0
    if stat_name == 'avg':
      d['error'] = tracing_histogram.standard_deviation
  else:
    _PopulateNumericalFields(d, tracing_histogram)
  return d


def _AddStdioUris(tracing_histogram, row_dict):
  log_urls = tracing_histogram.diagnostics.get(reserved_infos.LOG_URLS.name)
  if not log_urls:
    return

  if len(log_urls) == 1:
    _AddStdioUri('a_stdio_uri', log_urls.GetOnlyElement(), row_dict)


def _AddStdioUri(name, link_list, row_dict):
  # TODO(#4397): Change this to an assert after infra-side fixes roll
  if isinstance(link_list, list):
    row_dict['supplemental_columns'][name] = '[%s](%s)' % tuple(link_list)
  # Support busted format until infra changes roll
  elif isinstance(link_list, str):
    row_dict['supplemental_columns'][name] = link_list


def _PopulateNumericalFields(row_dict, tracing_histogram):
  statistics_scalars = tracing_histogram.statistics_scalars
  for name, scalar in statistics_scalars.iteritems():
    # We'll skip avg/std since these are already stored as value/error in rows.
    if name in ('avg', 'std'):
      continue

    row_dict['supplemental_columns']['d_%s' % name] = scalar.value

  row_dict['value'] = tracing_histogram.average
  row_dict['error'] = tracing_histogram.standard_deviation
