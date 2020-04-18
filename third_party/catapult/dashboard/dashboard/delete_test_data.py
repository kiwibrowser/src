# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Task queue task which deletes a TestMetadata, its subtests, and all their
   Rows.

A delete consists of listing all TestMetadata entities which match the name
pattern, and then, for each, recursively deleting all child Row entities and
then TestMetadatas.

For any delete, there could be hundreds of TestMetadatas and many thousands of
Rows. Datastore operations often time out after a few hundred deletes(), so this
task is split up using the task queue.
"""

from google.appengine.api import mail
from google.appengine.api import taskqueue
from google.appengine.ext import ndb

from dashboard import list_tests
from dashboard.common import datastore_hooks
from dashboard.common import request_handler
from dashboard.common import utils
from dashboard.models import graph_data
from dashboard.models import histogram

_ROWS_TO_DELETE_AT_ONCE = 500
_MAX_DELETIONS_PER_TASK = 30

_SHERIFF_ALERT_EMAIL_BODY = """
The test %(test_path)s has been DELETED.

It was previously sheriffed by %(sheriff)s.

Please ensure this is intended!
"""

# Queue name needs to be listed in queue.yaml.
_TASK_QUEUE_NAME = 'delete-tests-queue'


class BadInputPatternError(Exception):
  pass


class DeleteTestDataHandler(request_handler.RequestHandler):
  """Deletes the data for a test."""

  def get(self):
    """Displays a simple UI form to kick off migrations."""
    self.RenderHtml('delete_test_data.html', {})

  def post(self):
    """Recursively deletes TestMetadata and Row data.

    The form that's used to kick off migrations will give the parameter
    pattern, which is a test path pattern string.

    When this handler is called from the task queue, however, it will be given
    the parameter test_key, which should be a key of a TestMetadata entity in
    urlsafe form.
    """
    datastore_hooks.SetPrivilegedRequest()

    pattern = self.request.get('pattern')
    test_key = self.request.get('test_key')
    notify = self.request.get('notify', 'true')
    if notify.lower() == 'true':
      notify = True
    else:
      notify = False

    if pattern:
      try:
        _AddTasksForPattern(pattern, notify)
        self.RenderHtml('result.html', {
            'headline': 'Test deletion task started.'
        })
      except BadInputPatternError as error:
        self.ReportError('Error: %s' % error.message, status=400)
    elif test_key:
      _DeleteTest(test_key, notify)
    else:
      self.ReportError('Missing required parameters of /delete_test_data.')


def _AddTasksForPattern(pattern, notify):
  """Enumerates individual test deletion tasks and enqueues them.

  Typically, this function is called by a request initiated by the user.
  The purpose of this function is to queue up a set of requests which will
  do all of the actual work.

  Args:
    pattern: Test path pattern for TestMetadatas to delete.
    notify: If true, send an email notification for monitored test deletion.

  Raises:
    BadInputPatternError: Something was wrong with the input pattern.
  """
  tests = list_tests.GetTestsMatchingPattern(pattern, list_entities=True)
  for test in tests:
    _AddTaskForTest(test, notify)


def _AddTaskForTest(test, notify):
  """Adds a task to the task queue to delete a TestMetadata and its descendants.

  Args:
    test: A TestMetadata entity.
    notify: If true, send an email notification for monitored test deletion.
  """
  task_params = {
      'test_key': test.key.urlsafe(),
      'notify': 'true' if notify else 'false',
  }
  taskqueue.add(
      url='/delete_test_data',
      params=task_params,
      queue_name=_TASK_QUEUE_NAME)


def _DeleteTest(test_key_urlsafe, notify):
  """Deletes data for one TestMetadata.

  This gets all the descendant TestMetadata entities, and deletes their Row
  entities, then when the Row entities are all deleted, deletes the
  TestMetadata. This is often too much work to do in a single task, so if it
  doesn't finish, it will re-add itself to the same task queue and retry.
  """
  test_key = ndb.Key(urlsafe=test_key_urlsafe)
  finished = _DeleteTestData(test_key, notify)
  if not finished:
    task_params = {
        'test_key': test_key_urlsafe,
        'notify': 'true' if notify else 'false',
    }
    taskqueue.add(
        url='/delete_test_data',
        params=task_params,
        queue_name=_TASK_QUEUE_NAME)


def _DeleteTestData(test_key, notify):
  futures = []
  num_tests_processed = 0
  more = False
  descendants = list_tests.GetTestDescendants(test_key)
  for descendant in descendants:
    rows = graph_data.GetLatestRowsForTest(
        descendant, _ROWS_TO_DELETE_AT_ONCE, keys_only=True)
    if rows:
      futures.extend(ndb.delete_multi_async(rows))
      more = True
      num_tests_processed += 1
      if num_tests_processed > _MAX_DELETIONS_PER_TASK:
        break

    if not more:
      more = _DeleteTestHistogramData(descendant)

  # Only delete TestMetadata entities after all Row entities have been deleted.
  if not more:
    descendants = ndb.get_multi(descendants)
    for descendant in descendants:
      _SendNotificationEmail(descendant, notify)
      futures.append(descendant.key.delete_async())

  ndb.Future.wait_all(futures)
  return not more


@ndb.synctasklet
def _DeleteTestHistogramData(test_key):
  @ndb.tasklet
  def _DeleteHistogramClassData(cls):
    query = cls.query(cls.test == test_key)
    keys = yield query.fetch_async(
        limit=_MAX_DELETIONS_PER_TASK,
        use_cache=False, use_memcache=False, keys_only=True)
    yield ndb.delete_multi_async(keys)
    raise ndb.Return(bool(keys))

  result = yield (
      _DeleteHistogramClassData(histogram.SparseDiagnostic),
      _DeleteHistogramClassData(histogram.Histogram),
  )

  raise ndb.Return(any(result))


def _SendNotificationEmail(test, notify):
  """Send a notification email about the test deletion.

  Args:
    test_key: Key of the TestMetadata that's about to be deleted.
    notify: If true, send an email notification for monitored test deletion.
  """
  if not test or not test.sheriff or not notify:
    return
  body = _SHERIFF_ALERT_EMAIL_BODY % {
      'test_path': utils.TestPath(test.key),
      'sheriff': test.sheriff.string_id(),
  }
  mail.send_mail(sender='gasper-alerts@google.com',
                 to='chrome-performance-monitoring-alerts@google.com',
                 subject='Sheriffed Test Deleted',
                 body=body)
