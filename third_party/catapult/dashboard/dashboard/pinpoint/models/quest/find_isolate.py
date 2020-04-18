# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from dashboard.pinpoint.models import isolate
from dashboard.pinpoint.models.quest import execution
from dashboard.pinpoint.models.quest import quest
from dashboard.services import buildbucket_service


BUCKET = 'master.tryserver.chromium.perf'


class BuildError(Exception):
  """Raised when the build fails."""


class IsolateNotFoundError(StandardError):
  """Raised when the build succeeds, but Pinpoint can't find the isolate.

  This error is fatal to the Job.
  """


class FindIsolate(quest.Quest):

  def __init__(self, builder, target):
    self._builder_name = builder
    self._target = target

    self._previous_builds = {}

  def __eq__(self, other):
    return (isinstance(other, type(self)) and
            self._builder_name == other._builder_name)

  def __str__(self):
    return 'Build'

  def Start(self, change):
    return _FindIsolateExecution(self._builder_name, self._target, change,
                                 self._previous_builds)

  @classmethod
  def FromDict(cls, arguments):
    builder = arguments.get('builder')
    if not builder:
      raise TypeError('Missing "builder" argument.')

    target = arguments.get('target')
    if not target:
      raise TypeError('Missing "target" argument.')

    return cls(builder, target)


class _FindIsolateExecution(execution.Execution):

  def __init__(self, builder_name, target, change, previous_builds):
    super(_FindIsolateExecution, self).__init__()
    self._builder_name = builder_name
    self._target = target
    self._change = change
    # previous_builds is shared among all Executions of the same Quest.
    self._previous_builds = previous_builds

    self._build = None

  def _AsDict(self):
    return {
        'build': self._build,
        'builder': self._builder_name,
    }

  def _Poll(self):
    if self._CheckCompleted():
      return

    if self._build:
      self._CheckBuildStatus()
      return

    self._RequestBuild()

  def _CheckCompleted(self):
    """Checks the isolate cache to see if a build is already available.

    Returns:
      True iff the isolate was found, meaning the execution is completed.
    """
    try:
      isolate_server, isolate_hash = isolate.Get(
          self._builder_name, self._change, self._target)
    except KeyError:
      return False

    result_arguments = {
        'isolate_server': isolate_server,
        'isolate_hash': isolate_hash,
    }
    self._Complete(result_arguments=result_arguments)
    return True

  def _CheckBuildStatus(self):
    """Checks on the status of a previously requested build.

    Raises:
      BuildError: The build failed, was canceled, or didn't produce an isolate.
    """
    status = buildbucket_service.GetJobStatus(self._build)

    if status['build']['status'] != 'COMPLETED':
      return

    if status['build']['result'] == 'FAILURE':
      raise BuildError('Build failed: ' + status['build']['failure_reason'])
    elif status['build']['result'] == 'CANCELED':
      raise BuildError('Build was canceled: ' +
                       status['build']['cancelation_reason'])
    else:
      if self._CheckCompleted():
        return
      raise IsolateNotFoundError(
          'Buildbucket says the build completed successfully, '
          "but Pinpoint can't find the isolate hash.")

  def _RequestBuild(self):
    """Requests a build.

    If a previous Execution already requested a build for this Change, returns
    that build instead of requesting a new one.
    """
    if self._change in self._previous_builds:
      # If another Execution already requested a build, reuse that one.
      self._build = self._previous_builds[self._change]
    else:
      # Request a build!
      buildbucket_info = _RequestBuild(self._builder_name, self._change)
      self._build = buildbucket_info['build']['id']
      self._previous_builds[self._change] = self._build


def _RequestBuild(builder_name, change):
  deps_overrides = {dep.repository_url: dep.git_hash for dep in change.deps}
  parameters = {
      'builder_name': builder_name,
      'properties': {
          'clobber': True,
          'parent_got_revision': change.base_commit.git_hash,
          'deps_revision_overrides': deps_overrides,
      },
  }

  if change.patch:
    parameters['properties'].update(change.patch.BuildParameters())

  # TODO: Look up Buildbucket bucket from builder_name.
  return buildbucket_service.Put(BUCKET, parameters)
