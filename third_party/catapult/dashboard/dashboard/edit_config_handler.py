# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A common base class for pages that are used to edit configs."""

import json
import logging

from google.appengine.api import app_identity
from google.appengine.api import mail
from google.appengine.api import taskqueue
from google.appengine.api import users
from google.appengine.ext import deferred

from dashboard import list_tests
from dashboard.common import request_handler
from dashboard.common import utils
from dashboard.common import xsrf

# Max number of entities to put in one request to /put_entities_task.
_MAX_TESTS_TO_PUT_AT_ONCE = 25

# The queue to use to re-put tests. Should be present in queue.yaml.
_TASK_QUEUE_NAME = 'edit-sheriffs-queue'

# Minimum time before starting tasks, in seconds. It appears that the tasks
# may be executed before the sheriff is saved, so this is a workaround for that.
# See http://crbug.com/621499
_TASK_QUEUE_COUNTDOWN = 60

_NUM_PATTERNS_PER_TASK = 10

_NOTIFICATION_EMAIL_BODY = """
The configuration of %(hostname)s was changed by %(user)s.

Key: %(key)s

New test path patterns:
%(new_test_path_patterns)s

Old test path patterns
%(old_test_path_patterns)s
"""

# TODO(qyearsley): Make this customizable by storing the value in datastore.
# Make sure to send a notification to both old and new address if this value
# gets changed.
_NOTIFICATION_ADDRESS = 'chrome-performance-monitoring-alerts@google.com'
_SENDER_ADDRESS = 'gasper-alerts@google.com'


class EditConfigHandler(request_handler.RequestHandler):
  """Base class for handlers that are used to add or edit entities.

  Specifically, this is a common base class for EditSheriffsHandler
  and EditAnomalyConfigsHandler. Both of these kinds of entities
  represent a configuration that can apply to a set of tests, where
  the set of tests is specified with a list of test path patterns.
  """

  # The webapp2 docs say that custom __init__ methods should call initialize()
  # at the beginning of the method (rather than calling super __init__). See:
  # https://webapp-improved.appspot.com/guide/handlers.html#overriding-init
  # pylint: disable=super-init-not-called
  def __init__(self, request, response, model_class):
    """Constructs a handler object for editing entities of the given class.

    Args:
      request: Request object (implicitly passed in by webapp2).
      response: Response object (implicitly passed in by webapp2).
      model_class: A subclass of ndb.Model.
    """
    self.initialize(request, response)
    self._model_class = model_class

  @xsrf.TokenRequired
  def post(self):
    """Updates the user-selected anomaly threshold configuration.

    Request parameters:
      add-edit: Either 'add' if adding a new config, or 'edit'.
      add-name: A new anomaly config name, if adding one.
      edit-name: An existing anomaly config name, if editing one.
      patterns: Newline-separated list of test path patterns to monitor.

    Depending on the specific sub-class, this will also take other
    parameters for specific properties of the entity being edited.
    """
    try:
      edit_type = self.request.get('add-edit')
      if edit_type == 'add':
        self._AddEntity()
      elif edit_type == 'edit':
        self._EditEntity()
      else:
        raise request_handler.InvalidInputError('Invalid value for add-edit.')
    except request_handler.InvalidInputError as error:
      message = str(error) + ' Model class: ' + self._model_class.__name__
      self.RenderHtml('result.html', {'errors': [message]})

  def _AddEntity(self):
    """Adds adds a new entity according to the request parameters."""
    name = self.request.get('add-name')
    if not name:
      raise request_handler.InvalidInputError('No name given when adding new ')
    if self._model_class.get_by_id(name):
      raise request_handler.InvalidInputError(
          'Entity "%s" already exists, cannot add.' % name)
    entity = self._model_class(id=name)
    self._UpdateAndReportResults(entity)

  def _EditEntity(self):
    """Edits an existing entity according to the request parameters."""
    name = self.request.get('edit-name')
    if not name:
      raise request_handler.InvalidInputError('No name given.')
    entity = self._model_class.get_by_id(name)
    if not entity:
      raise request_handler.InvalidInputError(
          'Entity "%s" does not exist, cannot edit.' % name)
    self._UpdateAndReportResults(entity)

  def _UpdateAndReportResults(self, entity):
    """Updates the entity and reports the results of this updating."""
    new_patterns = _SplitPatternLines(self.request.get('patterns'))
    old_patterns = entity.patterns
    entity.patterns = new_patterns
    self._UpdateFromRequestParameters(entity)
    entity.put()

    self._RenderResults(entity, new_patterns, old_patterns)
    self._QueueChangeTestPatternsAndEmail(entity, new_patterns, old_patterns)

  def  _QueueChangeTestPatternsAndEmail(
      self, entity, new_patterns, old_patterns):
    deferred.defer(
        _QueueChangeTestPatternsTasks, old_patterns, new_patterns)

    user_email = users.get_current_user().email()
    subject = 'Added or updated %s: %s by %s' % (
        self._model_class.__name__, entity.key.string_id(), user_email)
    email_key = entity.key.string_id()

    email_body = _NOTIFICATION_EMAIL_BODY % {
        'key': email_key,
        'new_test_path_patterns': json.dumps(
            list(new_patterns), indent=2, sort_keys=True,
            separators=(',', ': ')),
        'old_test_path_patterns': json.dumps(
            list(old_patterns), indent=2, sort_keys=True,
            separators=(',', ': ')),
        'hostname': app_identity.get_default_version_hostname(),
        'user': user_email,
    }
    mail.send_mail(
        sender=_SENDER_ADDRESS,
        to=_NOTIFICATION_ADDRESS,
        subject=subject,
        body=email_body)


  def _UpdateFromRequestParameters(self, entity):
    """Updates the given entity based on query parameters.

    This method does not need to put() the entity.

    Args:
      entity: The entity to update.
    """
    raise NotImplementedError()

  def _RenderResults(self, entity, new_patterns, old_patterns):
    """Outputs results using the results.html template.

    Args:
      entity: The entity that was edited.
      new_patterns: New test patterns that this config now applies to.
      old_patterns: Old Test patterns that this config no longer applies to.
    """
    def ResultEntry(name, value):
      """Returns an entry in the results lists to embed on result.html."""
      return {'name': name, 'value': value, 'class': 'results-pre'}

    self.RenderHtml('result.html', {
        'headline': ('Added or updated %s "%s".' %
                     (self._model_class.__name__, entity.key.string_id())),
        'results': [
            ResultEntry('Entity', str(entity)),
            ResultEntry('New Patterns', '\n'.join(new_patterns)),
            ResultEntry('Old Patterns', '\n'.join(old_patterns)),
        ]
    })


def _SplitPatternLines(patterns_string):
  """Splits up the given newline-separated patterns and validates them."""
  test_path_patterns = sorted(p for p in patterns_string.splitlines() if p)
  _ValidatePatterns(test_path_patterns)
  return test_path_patterns


def _ValidatePatterns(test_path_patterns):
  """Raises an exception if any test path patterns are invalid."""
  for pattern in test_path_patterns:
    if not _IsValidTestPathPattern(pattern):
      raise request_handler.InvalidInputError(
          'Invalid test path pattern: "%s"' % pattern)


def _IsValidTestPathPattern(test_path_pattern):
  """Checks whether the given test path pattern string is OK."""
  if '[' in test_path_pattern or ']' in test_path_pattern:
    return False
  # Valid test paths will have a Master, bot, and test suite, and will
  # generally have a chart name and trace name after that.
  return len(test_path_pattern.split('/')) >= 3


def _QueueChangeTestPatternsTasks(old_patterns, new_patterns):
  """Updates tests that are different between old_patterns and new_patterns.

  The two arguments both represent sets of test paths (i.e. sets of data
  series). Any tests that are different between these two sets need to be
  updated.

  Some properties of TestMetadata entities are updated when they are put in the
  |_pre_put_hook| method of TestMetadata, so any TestMetadata entity that might
  need to be updated should be re-put.

  Args:
    old_patterns: An iterable of test path pattern strings.
    new_patterns: Another iterable of test path pattern strings.

  Returns:
    A pair (added_test_paths, removed_test_paths), which are, respectively,
    the test paths that are in the new set but not the old, and those that
    are in the old set but not the new.
  """
  added_patterns, removed_patterns = _ComputeDeltas(old_patterns, new_patterns)
  patterns = list(added_patterns) + list(removed_patterns)

  for i in xrange(0, len(patterns), _NUM_PATTERNS_PER_TASK):
    pattern_sublist = patterns[i:i+_NUM_PATTERNS_PER_TASK]

    deferred.defer(_GetTestPathsAndAddTask, pattern_sublist)


def _GetTestPathsAndAddTask(patterns):
  test_paths = _AllTestPathsMatchingPatterns(patterns)
  logging.info(test_paths)

  _AddTestsToPutToTaskQueue(test_paths)


def _ComputeDeltas(old_items, new_items):
  """Finds the added and removed items in a new set compared to an old one.

  Args:
    old_items: A collection of existing items. Could be a list or set.
    new_items: Another collection of items.

  Returns:
    A pair of sets (added, removed).
  """
  old, new = set(old_items), set(new_items)
  return new - old, old - new


def _RemoveOverlapping(added_items, removed_items):
  """Returns two sets of items with the common items removed."""
  added, removed = set(added_items), set(removed_items)
  return added - removed, removed - added


def _AllTestPathsMatchingPatterns(patterns_list):
  """Returns a list of all test paths matching the given list of patterns."""
  matching_patterns_futures = [
      list_tests.GetTestsMatchingPatternAsync(p) for p in patterns_list]

  test_paths = set()
  for i in xrange(len(patterns_list)):
    matching_patterns = matching_patterns_futures[i].get_result()
    test_paths |= set(matching_patterns)

  return sorted(test_paths)


def _AddTestsToPutToTaskQueue(test_paths):
  """Adds tests that we want to re-put in the datastore to a queue.

  We need to re-put the tests so that TestMetadata._pre_put_hook is run, so that
  the sheriff or alert threshold config of the TestMetadata is updated.

  Args:
    test_paths: List of test paths of tests to be re-put.
  """
  futures = []
  queue = taskqueue.Queue(_TASK_QUEUE_NAME)
  for start_index in range(0, len(test_paths), _MAX_TESTS_TO_PUT_AT_ONCE):
    group = test_paths[start_index:start_index + _MAX_TESTS_TO_PUT_AT_ONCE]
    urlsafe_keys = [utils.TestKey(t).urlsafe() for t in group]
    t = taskqueue.Task(
        url='/put_entities_task',
        params={'keys': ','.join(urlsafe_keys)},
        countdown=_TASK_QUEUE_COUNTDOWN)
    futures.append(queue.add_async(t))
  for f in futures:
    f.get_result()
