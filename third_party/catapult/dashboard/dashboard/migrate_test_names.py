# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Task queue task which migrates a TestMetadata and its Rows to a new name.

A rename consists of listing all TestMetadata entities which match the old_name,
and then, for each, completing these steps:
  * Create a new TestMetadata entity with the new name.
  * Re-parent all TestMetadata and Row entities from the old TestMetadata to
  * the new TestMetadata.
  * Update alerts to reference the new TestMetadata.
  * Delete the old TestMetadata.

For any rename, there could be hundreds of TestMetadatas and many thousands of
Rows. Datastore operations often time out after a few hundred puts(), so this
task is split up using the task queue.
"""

import re

from google.appengine.api import mail
from google.appengine.api import taskqueue
from google.appengine.ext import ndb

from dashboard import graph_revisions
from dashboard import list_tests
from dashboard.common import datastore_hooks
from dashboard.common import request_handler
from dashboard.common import utils
from dashboard.models import anomaly
from dashboard.models import graph_data
from dashboard.models import histogram

_MAX_DATASTORE_PUTS_PER_PUT_MULTI_CALL = 50

# Properties of TestMetadata that should not be copied when a TestMetadata is
# being copied.
_TEST_COMPUTED_PROPERTIES = [
    'bot',
    'parent_test',
    'test_path',
    'id',
    'master_name',
    'bot_name',
    'suite_name',
    'test_part1_name',
    'test_part2_name',
    'test_part3_name',
    'test_part4_name',
    'test_part5_name',
]
# The following shouldn't be copied because they were removed from the Model;
# Creating a new entity with one of these properties will result in an error.
_TEST_DEPRECATED_PROPERTIES = [
    'has_multi_value_rows',
    'important',
    'is_stacked',
    'last_added_revision',
    'overridden_gasp_modelset',
    'units_x',
    'buildername',
    'masterid',
    'stoppage_alert',
    'code',
    'command_line'
]
_TEST_EXCLUDE = _TEST_COMPUTED_PROPERTIES + _TEST_DEPRECATED_PROPERTIES

# Properties of Row that shouldn't be copied.
_ROW_EXCLUDE = ['parent_test', 'revision', 'id', 'internal_only']

_SHERIFF_ALERT_EMAIL_BODY = """
The test %(old_test_path)s has been migrated to %(new_test_path)s.

It was previously sheriffed by %(old_sheriff)s.

Please ensure the new test is properly sheriffed!
"""

# Match square brackets and group inside them.
_BRACKETS_REGEX = r'\[([^\]]*)\]'

# Queue name needs to be listed in queue.yaml.
_TASK_QUEUE_NAME = 'migrate-test-names-queue'
_TASK_INTERVAL = 1

_MIGRATE_TEST_LOOKUP_PATTERNS = 'migrate-lookup-patterns'
_MIGRATE_TEST_CREATE = 'migrate-test-create'
_MIGRATE_TEST_COPY_DATA = 'migrate-test-copy-data'


class BadInputPatternError(Exception):
  pass


class MigrateTestNamesHandler(request_handler.RequestHandler):
  """Migrates the data for a test which has been renamed on the buildbots."""

  def get(self):
    """Displays a simple UI form to kick off migrations."""
    self.RenderHtml('migrate_test_names.html', {})

  def post(self):
    """Starts migration of old TestMetadata entity names to new ones.

    The form that's used to kick off migrations will give the parameters
    old_pattern and new_pattern, which are both test path pattern strings.

    When this handler is called from the task queue, however, it will be given
    the parameters old_test_key and new_test_key, which should both be keys
    of TestMetadata entities in urlsafe form.
    """
    datastore_hooks.SetPrivilegedRequest()

    status = self.request.get('status')

    if not status:
      try:
        old_pattern = self.request.get('old_pattern')
        new_pattern = self.request.get('new_pattern')
        _MigrateTestBegin(old_pattern, new_pattern)
        self.RenderHtml('result.html', {
            'headline': 'Test name migration task started.'
        })
      except BadInputPatternError as error:
        self.ReportError('Error: %s' % error.message, status=400)
    elif status:
      if status == _MIGRATE_TEST_LOOKUP_PATTERNS:
        old_pattern = self.request.get('old_pattern')
        new_pattern = self.request.get('new_pattern')
        _MigrateTestLookupPatterns(old_pattern, new_pattern)
      elif status == _MIGRATE_TEST_CREATE:
        old_test_key = ndb.Key(urlsafe=self.request.get('old_test_key'))
        new_test_key = ndb.Key(urlsafe=self.request.get('new_test_key'))
        _MigrateTestCreateTest(old_test_key, new_test_key)
      elif status == _MIGRATE_TEST_COPY_DATA:
        old_test_key = ndb.Key(urlsafe=self.request.get('old_test_key'))
        new_test_key = ndb.Key(urlsafe=self.request.get('new_test_key'))
        _MigrateTestCopyData(old_test_key, new_test_key)
    else:
      self.ReportError('Missing required parameters of /migrate_test_names.')


def _MigrateTestBegin(old_pattern, new_pattern):
  _ValidateTestPatterns(old_pattern, new_pattern)

  _QueueTask({
      'old_pattern': old_pattern,
      'new_pattern': new_pattern,
      'status': _MIGRATE_TEST_LOOKUP_PATTERNS,
  }).get_result()


def _ValidateTestPatterns(old_pattern, new_pattern):
  tests = list_tests.GetTestsMatchingPattern(old_pattern, list_entities=True)
  for test in tests:
    old_path = utils.TestPath(test.key)
    _ValidateAndGetNewTestPath(old_path, new_pattern)


def _MigrateTestLookupPatterns(old_pattern, new_pattern):
  """Enumerates individual test migration tasks and enqueues them.

  Typically, this function is called by a request initiated by the user.
  The purpose of this function is to queue up a set of requests which will
  do all of the actual work.

  Args:
    old_pattern: Test path pattern for old names.
    new_pattern: Test path pattern for new names.

  Raises:
    BadInputPatternError: Something was wrong with the input patterns.
  """
  futures = []
  tests = list_tests.GetTestsMatchingPattern(old_pattern, list_entities=False)
  for test in tests:
    old_test_key = utils.TestKey(test)
    new_test_key = utils.TestKey(
        _ValidateAndGetNewTestPath(old_test_key.id(), new_pattern))
    futures.append(_QueueTask({
        'old_test_key': old_test_key.urlsafe(),
        'new_test_key': new_test_key.urlsafe(),
        'status': _MIGRATE_TEST_CREATE
    }))
  for f in futures:
    f.get_result()


def _ValidateAndGetNewTestPath(old_path, new_pattern):
  """Returns the destination test path that a test should be renamed to.

  The given |new_pattern| consists of a sequence of parts separated by slashes,
  and each part can be one of:
   (1) a single asterisk; in this case the corresponding part of the original
       test path will be used.
   (2) a string with brackets; which means that the corresponding part of the
       original test path should be used, but with the bracketed part removed.
   (3) a literal string; in this case this literal string will be used.

  The new_pattern can have fewer parts than the old test path; it can also be
  longer, but in this case the new pattern can't contain asterisks or brackets
  in the parts at the end.

  Args:
    old_path: A test path, e.g. ChromiumPerf/linux/sunspider/Total
    new_pattern: A destination path pattern.

  Returns:
    The new test path to use.

  Raises:
    BadInputPatternError: Something was wrong with the input patterns.
  """
  assert '*' not in old_path, '* should never appear in actual test paths.'
  old_path_parts = old_path.split('/')
  new_pattern_parts = new_pattern.split('/')
  new_path_parts = []
  for old_part, new_part in map(None, old_path_parts, new_pattern_parts):
    if not new_part:
      break  # In this case, the new path is shorter than the old.
    elif new_part == '*':
      # The old part field must exist.
      if not old_part:
        raise BadInputPatternError('* in new pattern has no corresponding '
                                   'part in old test path %s' % old_path)
      new_path_parts.append(old_part)
    elif re.search(_BRACKETS_REGEX, new_part):
      # A string contained in brackets in new path should be replaced by
      # old path with that string deleted. If the current part of old_path
      # is exactly that string, the new path rows are parented to the
      # previous part of old path.
      modified_old_part = _RemoveBracketedSubstring(old_part, new_part)
      if not modified_old_part:
        break
      new_path_parts.append(modified_old_part)
    else:
      if '*' in new_part:
        raise BadInputPatternError('Unexpected * in new test path pattern.')
      new_path_parts.append(new_part)
  return '/'.join(new_path_parts)


def _RemoveBracketedSubstring(old_part, new_part):
  """Returns the new name obtained by removing the given substring.

  Examples:
    _RemoveBracketedSubstring('asdf', '[sd]') => 'af'
    _RemoveBracketedSubstring('asdf', '[asdf]') => ''
    _RemoveBracketedSubstring('asdf', '[xy]') => Exception

  Args:
    old_part: A part of a test path.
    new_part: A string starting and ending with brackets, where the part
        inside the brackets is a substring of |old_part|.

  Returns:
    The |old_part| string with the substring removed.

  Raises:
    BadInputPatternError: The input was invalid.
  """
  substring_to_remove = re.search(_BRACKETS_REGEX, new_part).group(1)
  if substring_to_remove not in old_part:
    raise BadInputPatternError('Bracketed part not in %s.' % old_part)
  modified_old_part = old_part.replace(substring_to_remove, '', 1)
  return modified_old_part


def _QueueTask(task_params, countdown=None):
  queue = taskqueue.Queue(_TASK_QUEUE_NAME)
  return queue.add_async(taskqueue.Task(
      url='/migrate_test_names',
      params=task_params,
      countdown=countdown))


@ndb.synctasklet
def _MigrateTestCreateTest(old_test_key, new_test_key):
  """Migrates data (Row entities) from the old to the new test.

  Migrating all rows in one request is usually too much work to do before
  we hit a deadline exceeded error, so this function only does a limited
  chunk of work at one time.

  Args:
    old_test_key: The key of the TestMetadata to migrate data from.
    new_test_key: The key of the TestMetadata to migrate data to.

  Returns:
    True if finished or False if there is more work.
  """
  new_test_entity = yield _GetOrCreate(
      graph_data.TestMetadata, old_test_key.get(), new_test_key.id(),
      None, _TEST_EXCLUDE)

  yield (
      new_test_entity.put_async(),
      _MigrateTestScheduleChildTests(old_test_key, new_test_key))

  # Now migrate the actual row data and any associated data (ex. anomalies).
  # Do this in a seperate task that just spins on the row data.
  _QueueTask({
      'old_test_key': old_test_key.urlsafe(),
      'new_test_key': new_test_key.urlsafe(),
      'status': _MIGRATE_TEST_COPY_DATA
  }).get_result()


@ndb.tasklet
def _MigrateTestScheduleChildTests(old_test_key, new_test_key):
  # Try to re-parent children test first. We'll do this by creating a seperate
  # task for each child test.
  tests_to_reparent = yield graph_data.TestMetadata.query(
      graph_data.TestMetadata.parent_test == old_test_key).fetch_async(
          limit=_MAX_DATASTORE_PUTS_PER_PUT_MULTI_CALL,
          keys_only=True, use_cache=False, use_memcache=False)

  futures = []
  for old_child_test_key in tests_to_reparent:
    old_child_test_parts = old_child_test_key.id().split('/')
    new_child_test_key = utils.TestKey(
        '%s/%s' % (utils.TestPath(new_test_key), old_child_test_parts[-1]))
    futures.append(
        _QueueTask({
            'old_test_key': old_child_test_key.urlsafe(),
            'new_test_key': new_child_test_key.urlsafe(),
            'status': _MIGRATE_TEST_CREATE
        }))
  for f in futures:
    f.get_result()


@ndb.synctasklet
def _MigrateTestCopyData(old_test_key, new_test_key):

  more = yield (
      _MigrateTestRows(old_test_key, new_test_key),
      _MigrateAnomalies(old_test_key, new_test_key),
      _MigrateHistogramData(old_test_key, new_test_key))

  if any(more):
    # Rows at a specific test path are contained in a single entity group, thus
    # we space out writes to avoid data contention.
    _QueueTask({
        'old_test_key': old_test_key.urlsafe(),
        'new_test_key': new_test_key.urlsafe(),
        'status': _MIGRATE_TEST_COPY_DATA
    }, countdown=_TASK_INTERVAL).get_result()
    return

  _SendNotificationEmail(old_test_key, new_test_key)
  old_test_key.delete()


@ndb.tasklet
def _MigrateTestRows(old_parent_key, new_parent_key):
  """Copies Row entities from one parent to another, deleting old ones.

  Args:
    old_parent_key: TestMetadata entity key of the test to move from.
    new_parent_key: TestMetadata entity key of the test to move to.

  Returns:
    A tuple of (futures, more)
      futures: A list of Future objects for entities being put.
      more: Whether or not there's more work to do on the row data.
  """
  # In this function we'll build up lists of entities to put and delete
  # before returning Future objects for the entities being put and deleted.
  rows_to_put = []
  rows_to_delete = []

  # Add some Row entities to the lists of entities to put and delete.
  rows = graph_data.GetLatestRowsForTest(
      old_parent_key, _MAX_DATASTORE_PUTS_PER_PUT_MULTI_CALL)

  rows_to_put = yield [
      _GetOrCreate(
          graph_data.Row, r, r.key.id(), new_parent_key, _ROW_EXCLUDE)
      for r in rows]
  rows_to_delete = [r.key for r in rows]

  # Clear the cached revision range selector data for both the old and new
  # tests because it will no longer be valid after migration. The cache should
  # be updated with accurate data the next time it's set, which will happen
  # when someone views the graph.

  futures = ndb.put_multi_async(
      rows_to_put, use_cache=False, use_memcache=False)

  if rows_to_put:
    futures.append(rows_to_put[0].UpdateParentAsync())

  futures.extend(ndb.delete_multi_async(rows_to_delete))
  futures.append(
      graph_revisions.DeleteCacheAsync(utils.TestPath(old_parent_key)))
  futures.append(
      graph_revisions.DeleteCacheAsync(utils.TestPath(new_parent_key)))

  yield futures

  raise ndb.Return(bool(rows_to_put))


@ndb.tasklet
def _MigrateAnomalies(old_parent_key, new_parent_key):
  """Copies the Anomaly entities from one test to another.

  Args:
    old_parent_key: Source TestMetadata entity key.
    new_parent_key: Destination TestMetadata entity key.

  Returns:
    A list of Future objects for Anomaly entities to update.
  """
  anomalies_to_update = yield anomaly.Anomaly.GetAlertsForTestAsync(
      old_parent_key, limit=_MAX_DATASTORE_PUTS_PER_PUT_MULTI_CALL)
  if not anomalies_to_update:
    raise ndb.Return([])

  for anomaly_entity in anomalies_to_update:
    anomaly_entity.test = new_parent_key

  yield ndb.put_multi_async(anomalies_to_update)

  raise ndb.Return(bool(anomalies_to_update))


@ndb.tasklet
def _MigrateHistogramClassData(cls, old_parent_key, new_parent_key):
  query = cls.query(cls.test == old_parent_key)
  entities = yield query.fetch_async(
      limit=_MAX_DATASTORE_PUTS_PER_PUT_MULTI_CALL,
      use_cache=False, use_memcache=False)
  for e in entities:
    e.test = new_parent_key
  yield ndb.put_multi_async(entities)
  raise ndb.Return(bool(entities))


@ndb.tasklet
def _MigrateHistogramData(old_parent_key, new_parent_key):
  result = yield (
      _MigrateHistogramClassData(
          histogram.SparseDiagnostic, old_parent_key, new_parent_key),
      _MigrateHistogramClassData(
          histogram.Histogram, old_parent_key, new_parent_key),
  )

  raise ndb.Return(any(result))


def _SendNotificationEmail(old_test_key, new_test_key):
  """Send a notification email about the test migration.

  This function should be called after we have already found out that there are
  no new rows to move from the old test to the new test, but before we actually
  delete the old test.

  Args:
    old_test_key: TestMetadata key of the test that's about to be deleted.
    new_test_key: TestMetadata key of the test that's replacing the old one.
  """
  old_entity = old_test_key.get()
  if not old_entity or not old_entity.sheriff:
    return
  body = _SHERIFF_ALERT_EMAIL_BODY % {
      'old_test_path': utils.TestPath(old_test_key),
      'new_test_path': utils.TestPath(new_test_key),
      'old_sheriff': old_entity.sheriff.string_id(),
  }
  mail.send_mail(sender='gasper-alerts@google.com',
                 to='chrome-performance-monitoring-alerts@google.com',
                 subject='Sheriffed Test Migrated',
                 body=body)


@ndb.tasklet
def _GetOrCreate(
    cls, old_entity, new_name, parent_key, exclude):
  """Create an entity with the desired name if one does not exist.

  Args:
    cls: The class of the entity to create, either Row or TestMetadata.
    old_entity: The old entity to copy.
    new_name: The string id of the new entity.
    parent_key: The ndb.Key for the parent test of the new entity.
    exclude: Properties to not copy from the old entity.

  Returns:
    The new Row or TestMetadata entity (or the existing one).
  """
  new_entity = yield cls.get_by_id_async(new_name, parent=parent_key)
  if new_entity:
    raise ndb.Return(new_entity)
  if old_entity.key.kind() == 'Row':
    parent_key = utils.GetTestContainerKey(parent_key)
  create_args = {
      'id': new_name,
      'parent': parent_key,
  }
  for prop, val in old_entity.to_dict(exclude=exclude).iteritems():
    create_args[prop] = val
  new_entity = cls(**create_args)
  raise ndb.Return(new_entity)
