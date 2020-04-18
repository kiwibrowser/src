# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""URL endpoint for adding new histograms to the dashboard."""

import json
import logging
import sys
import zlib

from google.appengine.api import taskqueue
from google.appengine.ext import ndb

from dashboard import add_point
from dashboard import add_point_queue
from dashboard.api import api_request_handler
from dashboard.common import datastore_hooks
from dashboard.common import histogram_helpers
from dashboard.common import stored_object
from dashboard.common import utils
from dashboard.models import histogram
from tracing.value import histogram_set
from tracing.value.diagnostics import diagnostic
from tracing.value.diagnostics import reserved_infos

SUITE_LEVEL_SPARSE_DIAGNOSTIC_NAMES = set([
    reserved_infos.ARCHITECTURES.name,
    reserved_infos.BENCHMARKS.name,
    reserved_infos.BENCHMARK_DESCRIPTIONS.name,
    reserved_infos.BOTS.name,
    reserved_infos.BUG_COMPONENTS.name,
    reserved_infos.GPUS.name,
    reserved_infos.MASTERS.name,
    reserved_infos.MEMORY_AMOUNTS.name,
    reserved_infos.OS_NAMES.name,
    reserved_infos.OS_VERSIONS.name,
    reserved_infos.OWNERS.name,
    reserved_infos.PRODUCT_VERSIONS.name,
    reserved_infos.TAG_MAP.name,
])

HISTOGRAM_LEVEL_SPARSE_DIAGNOSTIC_NAMES = set([
    reserved_infos.DEVICE_IDS.name,
    reserved_infos.RELATED_NAMES.name,
    reserved_infos.STORIES.name,
    reserved_infos.STORYSET_REPEATS.name,
    reserved_infos.STORY_TAGS.name,
])

SPARSE_DIAGNOSTIC_NAMES = SUITE_LEVEL_SPARSE_DIAGNOSTIC_NAMES.union(
    HISTOGRAM_LEVEL_SPARSE_DIAGNOSTIC_NAMES)


TASK_QUEUE_NAME = 'histograms-queue'


def _CheckRequest(condition, msg):
  if not condition:
    raise api_request_handler.BadRequestError(msg)


class AddHistogramsHandler(api_request_handler.ApiRequestHandler):

  def AuthorizedPost(self):
    datastore_hooks.SetPrivilegedRequest()

    try:
      data_str = zlib.decompress(self.request.body)
      logging.info('Recieved compressed data.')
    except zlib.error:
      data_str = self.request.get('data')
      logging.info('Recieved uncompressed data.')
    if not data_str:
      raise api_request_handler.BadRequestError('Missing "data" parameter')

    logging.info('Received data: %s', data_str[:200])

    histogram_dicts = json.loads(data_str)
    ProcessHistogramSet(histogram_dicts)


def _LogDebugInfo(histograms):
  hist = histograms.GetFirstHistogram()
  if not hist:
    logging.info('No histograms in data.')
    return

  log_urls = hist.diagnostics.get(reserved_infos.LOG_URLS.name)
  if log_urls:
    log_urls = list(log_urls)
    logging.info('Buildbot URL: %s', str(log_urls))
  else:
    logging.info('No LOG_URLS in data.')


def ProcessHistogramSet(histogram_dicts):
  if not isinstance(histogram_dicts, list):
    raise api_request_handler.BadRequestError(
        'HistogramSet JSON much be a list of dicts')

  bot_whitelist_future = stored_object.GetAsync(
      add_point_queue.BOT_WHITELIST_KEY)

  histograms = histogram_set.HistogramSet()
  histograms.ImportDicts(histogram_dicts)
  histograms.ResolveRelatedHistograms()
  histograms.DeduplicateDiagnostics()

  if len(histograms) == 0:
    raise api_request_handler.BadRequestError(
        'HistogramSet JSON must contain at least one histogram.')

  _LogDebugInfo(histograms)

  InlineDenseSharedDiagnostics(histograms)

  # TODO(eakuefner): Get rid of this.
  # https://github.com/catapult-project/catapult/issues/4242
  _PurgeHistogramBinData(histograms)

  master = _GetDiagnosticValue(
      reserved_infos.MASTERS.name, histograms.GetFirstHistogram())
  bot = _GetDiagnosticValue(
      reserved_infos.BOTS.name, histograms.GetFirstHistogram())
  benchmark = _GetDiagnosticValue(
      reserved_infos.BENCHMARKS.name, histograms.GetFirstHistogram())
  benchmark_description = _GetDiagnosticValue(
      reserved_infos.BENCHMARK_DESCRIPTIONS.name,
      histograms.GetFirstHistogram(), optional=True)

  _ValidateMasterBotBenchmarkName(master, bot, benchmark)

  suite_key = utils.TestKey('%s/%s/%s' % (master, bot, benchmark))

  logging.info('Suite: %s', suite_key.id())

  revision = ComputeRevision(histograms)

  bot_whitelist = bot_whitelist_future.get_result()
  internal_only = add_point_queue.BotInternalOnly(bot, bot_whitelist)

  # We'll skip the histogram-level sparse diagnostics because we need to
  # handle those with the histograms, below, so that we can properly assign
  # test paths.
  suite_level_sparse_diagnostic_entities = FindSuiteLevelSparseDiagnostics(
      histograms, suite_key, revision, internal_only)

  # TODO(eakuefner): Refactor master/bot computation to happen above this line
  # so that we can replace with a DiagnosticRef rather than a full diagnostic.
  new_guids_to_old_diagnostics = DeduplicateAndPut(
      suite_level_sparse_diagnostic_entities, suite_key, revision)
  for new_guid, old_diagnostic in new_guids_to_old_diagnostics.iteritems():
    histograms.ReplaceSharedDiagnostic(
        new_guid, diagnostic.Diagnostic.FromDict(old_diagnostic))

  tasks = _BatchHistogramsIntoTasks(
      suite_key.id(), histograms, revision, benchmark_description)

  _QueueHistogramTasks(tasks)


def _ValidateMasterBotBenchmarkName(master, bot, benchmark):
  for n in (master, bot, benchmark):
    if '/' in n:
      raise api_request_handler.BadRequestError('Illegal slash in %s' % n)


def _MakeTask(params):
  return taskqueue.Task(
      url='/add_histograms_queue', payload=json.dumps(params),
      _size_check=False)


def _BatchHistogramsIntoTasks(
    suite_path, histograms, revision, benchmark_description):
  params = []
  tasks = []

  base_size = _MakeTask([]).size
  estimated_size = 0

  duplicate_check = set()

  for hist in histograms:
    diagnostics = FindHistogramLevelSparseDiagnostics(hist.guid, histograms)

    # TODO(eakuefner): Don't compute full diagnostics, because we need anyway to
    # call GetOrCreate here and in the queue.
    test_path = ComputeTestPath(suite_path, hist.guid, histograms)

    if test_path in duplicate_check:
      logging.warning('Duplicate histogram detected: %s', test_path)
    duplicate_check.add(test_path)

    # TODO(eakuefner): Batch these better than one per task.
    task_dict = _MakeTaskDict(
        hist, test_path, revision, benchmark_description, diagnostics)

    estimated_size_dict = len(json.dumps(task_dict))
    estimated_size += estimated_size_dict

    # Creating the task directly and getting the size back is slow, so we just
    # keep a running total of estimated task size. A bit hand-wavy but the #
    # of histograms per task doesn't need to be perfect, just has to be under
    # the max task size.
    estimated_total_size = estimated_size * 1.05 + base_size + 1024
    if estimated_total_size > taskqueue.MAX_TASK_SIZE_BYTES:
      t = _MakeTask(params)
      tasks.append(t)
      params = []
      estimated_size = estimated_size_dict

    params.append(task_dict)

  if params:
    t = _MakeTask(params)
    tasks.append(t)

  return tasks


def _QueueHistogramTasks(tasks):
  queue = taskqueue.Queue(TASK_QUEUE_NAME)
  futures = []
  for i in xrange(0, len(tasks), taskqueue.MAX_TASKS_PER_ADD):
    f = queue.add_async(tasks[i:i + taskqueue.MAX_TASKS_PER_ADD])
    futures.append(f)
  for f in futures:
    f.get_result()


def _MakeTaskDict(
    hist, test_path, revision, benchmark_description, diagnostics):
  # TODO(simonhatch): "revision" is common to all tasks, as is the majority of
  # the test path
  params = {
      'test_path': test_path,
      'revision': revision,
      'benchmark_description': benchmark_description
  }

  # By changing the GUID just before serializing the task, we're making it
  # unique for each histogram. This avoids each histogram trying to write the
  # same diagnostic out (datastore contention), at the cost of copyin the
  # data. These are sparsely written to datastore anyway, so the extra
  # storage should be minimal.
  for d in diagnostics.itervalues():
    d.ResetGuid()

  diagnostics = {k: d.AsDict() for k, d in diagnostics.iteritems()}

  params['diagnostics'] = diagnostics
  params['data'] = hist.AsDict()

  return params


# TODO(eakuefner): Clean this up by making it accept raw diagnostics.
# TODO(eakuefner): Move this helper along with others to a common place.
@ndb.synctasklet
def DeduplicateAndPut(new_entities, test, rev):
  result = yield DeduplicateAndPutAsync(new_entities, test, rev)
  raise ndb.Return(result)


@ndb.tasklet
def DeduplicateAndPutAsync(new_entities, test, rev):
  query = histogram.SparseDiagnostic.query(
      ndb.AND(
          histogram.SparseDiagnostic.end_revision == sys.maxint,
          histogram.SparseDiagnostic.test == test))
  diagnostic_entities = yield query.fetch_async()
  entity_futures = []
  new_guids_to_existing_diagnostics = {}

  for new_entity in new_entities:
    old_entity = _GetDiagnosticEntityMatchingName(
        new_entity.name, diagnostic_entities)
    if old_entity is not None:
      # Case 1: One in datastore, different from new one.
      if old_entity.IsDifferent(new_entity):
        old_entity.end_revision = rev - 1
        entity_futures.append(old_entity.put_async())
        new_entity.start_revision = rev
        new_entity.end_revision = sys.maxint
        entity_futures.append(new_entity.put_async())
      # Case 2: One in datastore, same as new one.
      else:
        new_guids_to_existing_diagnostics[new_entity.key.id()] = old_entity.data
      continue
    # Case 3: Nothing in datastore.
    entity_futures.append(new_entity.put_async())
  yield entity_futures
  raise ndb.Return(new_guids_to_existing_diagnostics)


def _GetDiagnosticEntityMatchingName(name, diagnostic_entities):
  for entity in diagnostic_entities:
    if entity.name == name:
      return entity
  return None


def FindSuiteLevelSparseDiagnostics(
    histograms, suite_key, revision, internal_only):
  diagnostics = {}
  for hist in histograms:
    for name, diag in hist.diagnostics.iteritems():
      if name in SUITE_LEVEL_SPARSE_DIAGNOSTIC_NAMES:
        existing_entity = diagnostics.get(name)
        if existing_entity is None:
          diagnostics[name] = histogram.SparseDiagnostic(
              id=diag.guid, data=diag.AsDict(), test=suite_key,
              start_revision=revision, end_revision=sys.maxint, name=name,
              internal_only=internal_only)
        elif existing_entity.key.id() != diag.guid:
          raise ValueError(
              name + ' diagnostics must be the same for all histograms')
  return diagnostics.values()


def FindHistogramLevelSparseDiagnostics(guid, histograms):
  hist = histograms.LookupHistogram(guid)
  diagnostics = {}
  for name, diag in hist.diagnostics.iteritems():
    if name in HISTOGRAM_LEVEL_SPARSE_DIAGNOSTIC_NAMES:
      diagnostics[name] = diag
  return diagnostics


def ComputeTestPath(suite_path, guid, histograms):
  hist = histograms.LookupHistogram(guid)
  path = '%s/%s' % (suite_path, hist.name)

  # If a Histogram represents a summary across multiple stories, then its
  # 'stories' diagnostic will contain the names of all of the stories.
  # If a Histogram is not a summary, then its 'stories' diagnostic will contain
  # the singular name of its story.
  is_summary = list(
      hist.diagnostics.get(reserved_infos.SUMMARY_KEYS.name, []))

  tir_label = histogram_helpers.GetTIRLabelFromHistogram(hist)
  if tir_label and (
      not is_summary or reserved_infos.STORY_TAGS.name in is_summary):
    path += '/' + tir_label

  is_ref = hist.diagnostics.get(reserved_infos.IS_REFERENCE_BUILD.name)
  if is_ref and len(is_ref) == 1:
    is_ref = is_ref.GetOnlyElement()

  story_name = hist.diagnostics.get(reserved_infos.STORIES.name)
  if story_name and len(story_name) == 1 and not is_summary:
    escaped_story_name = add_point.EscapeName(story_name.GetOnlyElement())
    path += '/' + escaped_story_name
    if is_ref:
      path += '_ref'
  elif is_ref:
    path += '/ref'

  return path


def _GetDiagnosticValue(name, hist, optional=False):
  if optional:
    if name not in hist.diagnostics:
      return None

  _CheckRequest(
      name in hist.diagnostics,
      'Histogram [%s] missing "%s" diagnostic' % (hist.name, name))
  value = hist.diagnostics[name]
  _CheckRequest(
      len(value) == 1,
      'Histograms must have exactly 1 "%s"' % name)
  return value.GetOnlyElement()


def ComputeRevision(histograms):
  _CheckRequest(len(histograms) > 0, 'Must upload at least one histogram')
  commit_position = _GetDiagnosticValue(
      reserved_infos.CHROMIUM_COMMIT_POSITIONS.name,
      histograms.GetFirstHistogram())

  # TODO(eakuefner): Allow users to specify other types of revisions to be used
  # for computing revisions of dashboard points. See
  # https://github.com/catapult-project/catapult/issues/3623.
  if not type(commit_position) in (long, int):
    raise api_request_handler.BadRequestError(
        'Commit Position must be an integer.')
  return commit_position


def InlineDenseSharedDiagnostics(histograms):
  # TODO(eakuefner): Delete inlined diagnostics from the set
  for hist in histograms:
    diagnostics = hist.diagnostics
    for name, diag in diagnostics.iteritems():
      if name not in SPARSE_DIAGNOSTIC_NAMES:
        diag.Inline()


def _PurgeHistogramBinData(histograms):
  # We do this because RelatedEventSet and Breakdown data in bins is
  # enormous in their current implementation.
  for cur_hist in histograms:
    for cur_bin in cur_hist.bins:
      for dm in cur_bin.diagnostic_maps:
        keys = dm.keys()
        for k in keys:
          del dm[k]
