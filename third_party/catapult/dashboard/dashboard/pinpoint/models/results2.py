# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cloudstorage
import os

from google.appengine.api import taskqueue
from google.appengine.ext import ndb

from dashboard.pinpoint.models.quest import read_value
from tracing_build import render_histograms_viewer


class Results2Error(Exception):

  pass


class CachedResults2(ndb.Model):
  """Stores data on when a results2 was generated."""

  updated = ndb.DateTimeProperty(required=True, auto_now_add=True)
  job_id = ndb.StringProperty()


class _GcsFileStream(object):
  """Wraps a gcs file providing a FileStream like api."""

  # pylint: disable=invalid-name

  def __init__(self, *args, **kwargs):
    self._gcs_file = cloudstorage.open(*args, **kwargs)

  def seek(self, _):
    pass

  def truncate(self):
    pass

  def write(self, data):
    self._gcs_file.write(data)

  def close(self):
    self._gcs_file.close()


def _GetCloudStorageName(job_id):
  return '/results2-public/%s.html' % job_id


def GetCachedResults2(job):
  filename = _GetCloudStorageName(job.job_id)
  results = cloudstorage.listbucket(filename)

  for _ in results:
    return 'https://storage.cloud.google.com' + filename

  return None


def ScheduleResults2Generation(job):
  try:
    # Don't want several tasks creating results2, so create task with specific
    # name to deduplicate.
    task_name = 'results2-public-%s' % job.job_id
    taskqueue.add(
        queue_name='job-queue', url='/api/generate-results2/' + job.job_id,
        name=task_name)
  except taskqueue.TombstonedTaskError:
    return False
  except taskqueue.TaskAlreadyExistsError:
    pass
  return True


def GenerateResults2(job):
  histogram_dicts = _FetchHistogramsDataFromJobData(job)
  vulcanized_html = _ReadVulcanizedHistogramsViewer()

  CachedResults2(job_id=job.job_id).put()

  filename = _GetCloudStorageName(job.job_id)
  gcs_file = _GcsFileStream(
      filename, 'w', content_type='text/html',
      retry_params=cloudstorage.RetryParams(backoff_factor=1.1))

  render_histograms_viewer.RenderHistogramsViewer(
      histogram_dicts, gcs_file,
      reset_results=True, vulcanized_html=vulcanized_html)

  gcs_file.close()


def _ReadVulcanizedHistogramsViewer():
  viewer_path = os.path.join(
      os.path.dirname(__file__), '..', '..', '..',
      'vulcanized_histograms_viewer', 'vulcanized_histograms_viewer.html')
  with open(viewer_path, 'r') as f:
    return f.read()


def _FetchHistogramsDataFromJobData(job):
  # We fetch 1 setof histograms at a time, iterating over the list of isolate
  # hashes and then yielding each histogram. This prevents memory blowouts
  # since we only have 1 gig to work with, but at the cost of increased
  # task time.
  for isolate_server, isolate_hash in _GetAllIsolateHashesForJob(job):
    hs = read_value._RetrieveOutputJson(
        isolate_server, isolate_hash, 'chartjson-output.json')
    for h in hs:
      yield h
    del hs


def _GetAllIsolateHashesForJob(job):
  job_data = job.AsDict(options=('STATE',))

  quest_index = None
  for quest_index in xrange(len(job_data['quests'])):
    if job_data['quests'][quest_index] == 'Test':
      break
  else:
    raise Results2Error('No Test quest.')

  isolate_hashes = []

  # If there are differences, only include Changes with differences.
  for change_index in xrange(len(job_data['state'])):
    change_state = job_data['state'][change_index]
    if not _IsChangeDifferent(change_state):
      continue
    isolate_hashes += _GetIsolateHashesForChange(change_state, quest_index)

  # Otherwise, just include all Changes.
  if not isolate_hashes:
    for change_index in xrange(len(job_data['state'])):
      change_state = job_data['state'][change_index]
      isolate_hashes += _GetIsolateHashesForChange(change_state, quest_index)

  return isolate_hashes


def _IsChangeDifferent(change_state):
  if change_state['comparisons'].get('prev') == 'different':
    return True

  if change_state['comparisons'].get('next') == 'different':
    return True

  return False


def _GetIsolateHashesForChange(change_state, quest_index):
  isolate_hashes = []
  attempts = change_state['attempts']
  for attempt_info in attempts:
    executions = attempt_info['executions']
    if quest_index >= len(executions):
      continue

    result_arguments = executions[quest_index]['result_arguments']
    if 'isolate_hash' not in result_arguments:
      continue

    isolate_server = result_arguments.get(
        'isolate_server', 'https://isolateserver.appspot.com')
    isolate_hashes.append((isolate_server, result_arguments['isolate_hash']))

  return isolate_hashes
