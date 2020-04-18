# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Fake CIDB for unit testing."""

from __future__ import print_function

import datetime
import itertools

from chromite.lib import build_requests
from chromite.lib import constants
from chromite.lib import cidb
from chromite.lib import clactions
from chromite.lib import failure_message_lib
from chromite.lib import hwtest_results


class FakeCIDBConnection(object):
  """Fake connection to a Continuous Integration database.

  This class is a partial re-implementation of CIDBConnection, using
  in-memory lists rather than a backing database.
  """

  NUM_RESULTS_NO_LIMIT = -1

  def __init__(self, fake_keyvals=None):
    self.buildTable = []
    self.clActionTable = []
    self.buildStageTable = {}
    self.failureTable = {}
    self.fake_time = None
    self.fake_keyvals = fake_keyvals or {}
    self.buildMessageTable = {}
    self.hwTestResultTable = {}
    self.buildRequestTable = {}

  def _TrimStatus(self, status):
    """Trims a build row to keys that should be returned by GetBuildStatus"""
    return {k: v for (k, v) in status.items()
            if k in cidb.CIDBConnection.BUILD_STATUS_KEYS}

  def SetTime(self, fake_time):
    """Sets a fake time to be retrieved by GetTime.

    Args:
      fake_time: datetime.datetime object.
    """
    self.fake_time = fake_time

  def GetTime(self):
    """Gets the current database time."""
    return self.fake_time or datetime.datetime.now()

  def InsertBuild(self, builder_name, waterfall, build_number,
                  build_config, bot_hostname, master_build_id=None,
                  timeout_seconds=None, status=constants.BUILDER_STATUS_PASSED,
                  important=None, buildbucket_id=None, milestone_version=None,
                  platform_version=None, start_time=None, build_type=None):
    """Insert a build row.

    Note this API slightly differs from cidb as we pass status to avoid having
    to have a later FinishBuild call in testing.
    """
    if start_time is None:
      start_time = datetime.datetime.now()

    deadline = None
    if timeout_seconds is not None:
      timediff = datetime.timedelta(seconds=timeout_seconds)
      deadline = start_time + timediff

    build_id = len(self.buildTable)
    row = {'id': build_id,
           'builder_name': builder_name,
           'buildbot_generation': constants.BUILDBOT_GENERATION,
           'waterfall': waterfall,
           'build_number': build_number,
           'build_config' : build_config,
           'bot_hostname': bot_hostname,
           'start_time': start_time,
           'master_build_id' : master_build_id,
           'deadline': deadline,
           'status': status,
           'finish_time': start_time,
           'important': important,
           'buildbucket_id': buildbucket_id,
           'final': False,
           'milestone_version': milestone_version,
           'platform_version': platform_version,
           'build_type': build_type}
    self.buildTable.append(row)
    return build_id

  def FinishBuild(self, build_id, status=None, summary=None, strict=True):
    """Update the build with finished status."""
    build = self.buildTable[build_id]

    if strict and build['final']:
      return 0

    values = {}
    if status is not None:
      values.update(status=status)
    if summary is not None:
      values.update(summary=summary)

    values.update(finish_time=datetime.datetime.now(), final=True)

    if values:
      build.update(values)
      return 1
    else:
      return 0

  def UpdateMetadata(self, build_id, metadata):
    """See cidb.UpdateMetadata.

    Args:
      build_id: The build to update.
      metadata: A cbuildbot metadata object. Or, a dictionary (note: using
                a dictionary is not supported by the base cidb API, but
                is provided for this fake class for ease of use in test
                set-up code).
    """
    d = metadata if isinstance(metadata, dict) else metadata.GetDict()
    versions = d.get('version') or {}
    self.buildTable[build_id].update(
        {'chrome_version': versions.get('chrome'),
         'milestone_version': versions.get('milestone'),
         'platform_version': versions.get('platform'),
         'full_version': versions.get('full'),
         'sdk_version': d.get('sdk-versions'),
         'toolchain_url': d.get('toolchain-url'),
         'build_type': d.get('build_type'),
         'important': d.get('important')})
    return 1

  def InsertCLActions(self, build_id, cl_actions, timestamp=None):
    """Insert a list of |cl_actions|."""
    if not cl_actions:
      return 0

    rows = []
    for cl_action in cl_actions:
      change_number = int(cl_action.change_number)
      patch_number = int(cl_action.patch_number)
      change_source = cl_action.change_source
      action = cl_action.action
      reason = cl_action.reason
      buildbucket_id = cl_action.buildbucket_id

      timestamp = cl_action.timestamp or timestamp or datetime.datetime.now()

      rows.append({
          'build_id' : build_id,
          'change_source' : change_source,
          'change_number': change_number,
          'patch_number' : patch_number,
          'action' : action,
          'timestamp': timestamp,
          'reason' : reason,
          'buildbucket_id': buildbucket_id})

    self.clActionTable.extend(rows)
    return len(rows)

  def InsertBuildStage(self, build_id, name, board=None,
                       status=constants.BUILDER_STATUS_PLANNED):
    build_stage_id = len(self.buildStageTable)
    row = {'build_id': build_id,
           'name': name,
           'board': board,
           'status': status}
    self.buildStageTable[build_stage_id] = row
    return build_stage_id

  def InsertBoardPerBuild(self, build_id, board):
    # TODO(akeshet): Fill this placeholder.
    pass

  def InsertFailure(self, build_stage_id, exception_type, exception_message,
                    exception_category=constants.EXCEPTION_CATEGORY_UNKNOWN,
                    outer_failure_id=None,
                    extra_info=None):
    failure_id = len(self.failureTable)
    values = {'id': failure_id,
              'build_stage_id': build_stage_id,
              'exception_type': exception_type,
              'exception_message': exception_message,
              'exception_category': exception_category,
              'outer_failure_id': outer_failure_id,
              'extra_info': extra_info,
              'timestamp': None}
    self.failureTable[failure_id] = values
    return failure_id

  def InsertBuildMessage(self, build_id,
                         message_type=None, message_subtype=None,
                         message_value=None, board=None):
    """Insert a build message.

    Args:
      build_id: primary key of build recording this message.
      message_type: Optional str name of message type.
      message_subtype: Optional str name of message subtype.
      message_value: Optional value of message.
      board: Optional str name of the board.

    Returns:
      The build message id (string).
    """
    if message_type:
      message_type = message_type[:240]
    if message_subtype:
      message_subtype = message_subtype[:240]
    if message_value:
      message_value = message_value[:480]
    if board:
      board = board[:240]

    build_message_id = len(self.buildMessageTable)
    values = {'build_id': build_id,
              'message_type': message_type,
              'message_subtype': message_subtype,
              'message_value': message_value,
              'board': board}
    self.buildMessageTable[build_message_id] = values
    return build_message_id

  def InsertHWTestResults(self, hwTestResults):
    """Insert HWTestResults into the hwTestResultTable.

    Args:
      hwTestResults: A list of HWTestResult instances.

    Returns:
      The number of inserted rows.
    """
    result_id = len(self.hwTestResultTable)
    for result in hwTestResults:
      values = {'id': result_id,
                'build_id': result.build_id,
                'test_name': result.test_name,
                'status': result.status}
      self.hwTestResultTable[result_id] = values
      result_id = result_id + 1

    return len(hwTestResults)

  def InsertBuildRequests(self, build_reqs):
    """Insert a list of build requests.

    Args:
      build_reqs: A list of build_requests.BuildRequest instances.

    Returns:
       The number of inserted rows.
    """
    request_id = len(self.buildRequestTable)
    for build_req in build_reqs:
      values = {
          'id': request_id,
          'build_id': build_req.build_id,
          'request_build_config': build_req.request_build_config,
          'request_build_args': build_req.request_build_args,
          'request_buildbucket_id': build_req.request_buildbucket_id,
          'request_reason': build_req.request_reason,
          'timestamp': build_req.timestamp or datetime.datetime.now()}
      self.buildRequestTable[request_id] = values
      request_id = request_id + 1

    return len(build_reqs)

  def GetBuildMessages(self, build_id, message_type=None, message_subtype=None):
    """Get the build messages of the given build id.

    Args:
      build_id: build id (string) of the build to get messages.
      message_type: Get messages with the specific message_type (string) if
        message_type is not None.
      message_subtype: Get messages with the specific message_subtype (stirng)
        if message_subtype is not None.

    Returns:
      A list of build messages (in the format of dict).
    """
    messages = []
    for v in self.buildMessageTable.values():
      if (v['build_id'] == build_id and
          (message_type is None or v['message_type'] == message_type) and
          (message_subtype is None or
           v['message_subtype'] == message_subtype)):
        messages.append(v)

    return messages

  def StartBuildStage(self, build_stage_id):
    if build_stage_id > len(self.buildStageTable):
      return

    self.buildStageTable[build_stage_id]['status'] = (
        constants.BUILDER_STATUS_INFLIGHT)

  def WaitBuildStage(self, build_stage_id):
    if build_stage_id > len(self.buildStageTable):
      return

    self.buildStageTable[build_stage_id]['status'] = (
        constants.BUILDER_STATUS_WAITING)

  def ExtendDeadline(self, build_id, timeout):
    # No sanity checking in fake object.
    now = datetime.datetime.now()
    timediff = datetime.timedelta(seconds=timeout)
    self.buildStageTable[build_id]['deadline'] = now + timediff

  def FinishBuildStage(self, build_stage_id, status):
    if build_stage_id > len(self.buildStageTable):
      return

    self.buildStageTable[build_stage_id]['status'] = status

  def GetActionsForChanges(self, changes, ignore_patch_number=True,
                           status=None, action=None, start_time=None):
    """Gets all the actions for the given changes.

    Args:
      changes: A list of GerritChangeTuple, GerritPatchTuple or GerritPatch
        specifying the changes to whose actions should be fetched.
      ignore_patch_number: Boolean indicating whether to ignore patch_number of
        the changes. If ignore_patch_number is False, only get the actions with
        matched patch_number. Default to True.
      status: If provided, only return the actions with build is |status| (a
        member of constants.BUILDER_ALL_STATUSES). Default to None.
      action: If provided, only return the actions is |action| (a member of
        constants.CL_ACTIONS). Default to None.
      start_time: If provided, only return the actions with timestamp >=
        start_time. Default to None.

    Returns:
      A list of CLAction instances, in action id order.
    """
    values = []
    for row in self.GetActionHistory():
      if start_time is not None and row.timestamp < start_time:
        continue
      if status is not None and row.status != status:
        continue
      if action is not None and row.action != action:
        continue

      for change in changes:
        change_source = 'internal' if change.internal else 'external'

        if (change_source != row.change_source or
            int(change.gerrit_number) != row.change_number):
          continue
        if (not ignore_patch_number and
            int(change.patch_number) != row.patch_number):
          continue

        values.append(row)
        break

    return values

  def GetActionsForBuild(self, build_id):
    """Gets all the actions associated with build |build_id|.

    Returns:
      A list of CLAction instance, in action id order.
    """
    return [row for row in self.GetActionHistory()
            if build_id == row.build_id]

  def GetActionHistory(self, *args, **kwargs):
    """Get all the actions for all changes."""
    # pylint: disable=W0613
    values = []
    for item, action_id in zip(self.clActionTable, itertools.count()):
      row = (
          action_id,
          item['build_id'],
          item['action'],
          item['reason'],
          self.buildTable[item['build_id']]['build_config'],
          item['change_number'],
          item['patch_number'],
          item['change_source'],
          item['timestamp'],
          item['buildbucket_id'],
          self.buildTable[item['build_id']]['status'])
      values.append(row)

    return clactions.CLActionHistory(clactions.CLAction(*row) for row in values)

  def GetBuildStatus(self, build_id):
    """Gets the status of the build."""
    try:
      return self._TrimStatus(self.buildTable[build_id])
    except IndexError:
      return None

  def GetBuildStatuses(self, build_ids):
    """Gets the status of the builds."""
    return [self._TrimStatus(self.buildTable[x]) for x in build_ids]

  def GetSlaveStatuses(self, master_build_id, buildbucket_ids=None):
    """Gets the slaves of given build."""
    if buildbucket_ids is None:
      return [self._TrimStatus(b) for b in self.buildTable
              if b['master_build_id'] == master_build_id]
    else:
      return [self._TrimStatus(b) for b in self.buildTable
              if b['master_build_id'] == master_build_id and
              b['buildbucket_id'] in buildbucket_ids]

  def GetBuildStage(self, build_stage_id):
    """Get build stage given the build_stage_id.

    Args:
      build_stage_id: The build_stage_id to get the stage.

    Returns:
      A dict prensenting the stage if the build_stage_id exists in
      the buildStageTable; else, None.
    """
    return self.buildStageTable.get(build_stage_id)

  def GetBuildStages(self, build_id):
    """Gets build stages given the build_id"""
    return [self.buildStageTable[_id]
            for _id in self.buildStageTable
            if self.buildStageTable[_id]['build_id'] == build_id]

  def GetSlaveStages(self, master_build_id, buildbucket_ids=None):
    """Get the slave stages of the given build.

    Args:
      master_build_id: The build id (string) of the master build.
      buildbucket_ids: A list of buildbucket ids (strings) of the slaves.

    Returns:
      A list of slave stages (in format of dicts).
    """
    slave_builds = []

    if buildbucket_ids is None:
      slave_builds = {b['id']: b for b in self.buildTable
                      if b['master_build_id'] == master_build_id}
    else:
      slave_builds = {b['id']: b for b in self.buildTable
                      if b['master_build_id'] == master_build_id and
                      b['buildbucket_id'] in buildbucket_ids}

    slave_stages = []
    for _id in self.buildStageTable:
      build_id = self.buildStageTable[_id]['build_id']
      if build_id in slave_builds:
        stage = self.buildStageTable[_id].copy()
        stage['build_config'] = slave_builds[build_id]['build_config']
        slave_stages.append(stage)

    return slave_stages

  def GetBuildHistory(self, build_config, num_results,
                      ignore_build_id=None, start_date=None, end_date=None,
                      milestone_version=None, platform_version=None,
                      starting_build_id=None, final=False, reverse=False):
    """Returns the build history for the given |build_config|."""
    return self.GetBuildsHistory(
        build_configs=[build_config], num_results=num_results,
        ignore_build_id=ignore_build_id, start_date=start_date,
        end_date=end_date, milestone_version=milestone_version,
        platform_version=platform_version, starting_build_id=starting_build_id,
        final=final, reverse=reverse)

  def GetBuildsHistory(self, build_configs, num_results,
                       ignore_build_id=None, start_date=None, end_date=None,
                       milestone_version=None,
                       platform_version=None, starting_build_id=None,
                       final=False, reverse=False):
    """Returns the build history for the given |build_configs|."""
    builds = sorted(self.buildTable, reverse=(not reverse))

    # Filter results.
    if build_configs:
      builds = [b for b in builds if b['build_config'] in build_configs]
    if ignore_build_id is not None:
      builds = [b for b in builds if b['id'] != ignore_build_id]
    if start_date is not None:
      builds = [b for b in builds
                if b['start_time'].date() >= start_date]
    if end_date is not None:
      builds = [b for b in builds
                if 'finish_time' in b and
                b['finish_time'] and
                b['finish_time'].date() <= end_date]
    if milestone_version is not None:
      builds = [b for b in builds
                if b.get('milestone_version') == milestone_version]
    if platform_version is not None:
      builds = [b for b in builds
                if b.get('platform_version') == platform_version]
    if starting_build_id is not None:
      builds = [b for b in builds if b['id'] >= starting_build_id]
    if final:
      builds = [b for b in builds
                if b.get('final') is True]

    if num_results != -1:
      return builds[:num_results]
    else:
      return builds

  def GetPlatformVersions(self, build_config, num_results=-1,
                          starting_milestone_version=None):
    """Get the platform versions for a build_config."""
    builds = [b for b in self.buildTable
              if b['build_config'] == build_config]

    if starting_milestone_version is not None:
      builds = [b for b in builds if int(b.get('milestone_version')) >=
                int(starting_milestone_version)]

    versions = [b['platform_version'] for b in builds]

    if num_results != -1:
      return versions[:num_results]
    else:
      return versions

  def GetTimeToDeadline(self, build_id):
    """Gets the time remaining until deadline."""
    now = datetime.datetime.now()
    deadline = self.buildTable[build_id]['deadline']
    return max(0, (deadline - now).total_seconds())

  def GetKeyVals(self):
    """Gets contents of keyvalTable."""
    return self.fake_keyvals

  def GetBuildStatusWithBuildbucketId(self, buildbucket_id):
    for row in self.buildTable:
      if row['buildbucket_id'] == buildbucket_id:
        return self._TrimStatus(row)

    return None

  def GetBuildsFailures(self, build_ids):
    """Gets the failure entries for all listed build_ids.

    Args:
      build_ids: list of build ids of the builds to fetch failures for.

    Returns:
      A list of failure_message_lib.StageFailure instances.
    """
    stage_failures = []
    for build_id in build_ids:
      b_dict = self.buildTable[build_id]
      bs_table = {k: v for k, v in self.buildStageTable.iteritems()
                  if v['build_id'] == build_id}

      for f_dict in self.failureTable.values():
        if f_dict['build_stage_id'] in bs_table:
          bs_dict = bs_table[f_dict['build_stage_id']]
          stage_failures.append(
              failure_message_lib.StageFailure.GetStageFailureFromDicts(
                  f_dict, bs_dict, b_dict))

    return stage_failures

  def GetHWTestResultsForBuilds(self, build_ids):
    """Get hwTestResults for builds.

    Args:
      build_ids: A list of build_id (strings) of build.

    Returns:
      A list of hwtest_results.HWTestResult instances.
    """
    results = []
    for value in self.hwTestResultTable.values():
      if value['build_id'] in build_ids:
        results.append(hwtest_results.HWTestResult(
            value['id'], value['build_id'], value['test_name'],
            value['status']))

    return results

  def GetBuildRequestsForBuildConfig(self, request_build_config,
                                     num_results=-1, start_time=None):
    """Get BuildRequests for a build_config.

    Args:
      request_build_config: build config (string) to request.
      num_results: number of results to return, default to -1.
      start_time: get build requests sent after start_time.

    Returns:
      A list of BuildRequest instances sorted by id in descending order.
    """
    return self.GetBuildRequestsForBuildConfigs(
        [request_build_config], num_results=num_results, start_time=start_time)

  def GetBuildRequestsForBuildConfigs(self, request_build_configs,
                                      num_results=-1, start_time=None):
    """Get BuildRequests for a list build_configs.

    Args:
      request_build_configs: build configs (string) to request.
      num_results: number of results to return, default to -1.
      start_time: get build requests sent after start_time.

    Returns:
      A list of BuildRequest instances sorted by id in descending order.
    """
    results = []
    for value in self.buildRequestTable.values():

      if start_time is not None and value['timestamp'] < start_time:
        continue

      if value['request_build_config'] in request_build_configs:
        results.append(build_requests.BuildRequest(
            value['id'], value['build_id'], value['request_build_config'],
            value['request_build_args'], value['request_buildbucket_id'],
            value['request_reason'], value['timestamp']))

    requests = sorted(results, key=lambda r: r.id, reverse=True)

    if num_results != -1:
      return requests[:num_results]
    else:
      return requests

  def GetLatestBuildRequestsForReason(self, request_reason,
                                      status=None,
                                      num_results=NUM_RESULTS_NO_LIMIT,
                                      n_days_back=7):
    """Gets the latest build_requests associated with the request_reason.

    Args:
      request_reason: The reason to filter by
      status: Whether to filter on status
      num_results: Number of results to return, default to
        self.NUM_RESULTS_NO_LIMIT.
      n_days_back: How many days back to look for build requests.

    Returns:
      A list of build_request.BuildRequest instances.
    """
    def _MatchesStatus(value):
      return status is None or value['status'] == status

    def _MatchesTimeConstraint(value):
      if n_days_back is None:
        return True

      # MySQL doesn't support timestamps with microsecond resolution
      now = datetime.datetime.now().replace(microsecond=0)
      then = now - datetime.timedelta(days=n_days_back)
      return then < value['timestamp']

    by_build_config = {}
    for value in self.buildRequestTable.values():
      if (value['request_reason'] == request_reason
          and _MatchesStatus(value)
          and _MatchesTimeConstraint(value)):
        by_build_config.setdefault(
            value['request_build_config'], []).append(value)

    max_in_group = [
        build_requests.BuildRequest(
            **max(group, key=lambda value: value['timestamp']))
        for group in by_build_config.values()]

    limit = None
    if num_results != self.NUM_RESULTS_NO_LIMIT:
      limit = num_results
    return max_in_group[:limit]

  def GetBuildRequestsForRequesterBuild(self, requester_build_id,
                                        request_reason=None):
    """Get the build_requests associated to the requester build.

    Args:
      requester_build_id: The build id of the requester build.
      request_reason: If provided, only return the build_request of the given
        request reason. Default to None.

    Returns:
      A list of build_request.BuildRequest instances.
    """
    results = []
    for value in self.buildRequestTable.values():
      if (value['build_id'] == requester_build_id and
          request_reason is None or request_reason == value['request_reason']):
        results.append(build_requests.BuildRequest(
            value['id'], value['build_id'], value['request_build_config'],
            value['request_build_args'], value['request_buildbucket_id'],
            value['request_reason'], value['timestamp']))

    return results

  def HasFailureMsgForStage(self, build_stage_id):
    """Determine whether a build stage has failure messages in failureTable.

    Args:
      build_stage_id: The id of the build_stage to query for.

    Returns:
      True if there're failures reported to failureTable for this build stage
      to cidb; else, False.
    """
    stages = self.failureTable.values()
    for stage in stages:
      if stage['build_stage_id'] == build_stage_id:
        return True

    return False

  def GetPreCQFlakeCounts(self, start_date=None, end_date=None):
    """Queries pre-CQ config flake & run counts.

    Args:
      start_date: The start date for the time window.
      end_date: The last day to include in the time window.

    Returns:
      A list of (config, flake, runs) tuples.
    """
    if start_date is None:
      start_date = datetime.datetime.now() - datetime.timedelta(days=7)

    def TimeConstraint(build):
      if end_date is not None and build['start_time'] >= end_date:
        return False
      return start_date <= build['start_time']

    def JoinWithBuild(actions):
      for action in actions:
        yield action, builds_by_id[action['build_id']]

    runs_by_build_config = {}
    builds_by_id = {}
    for build in self.buildTable:
      if TimeConstraint(build):
        runs_by_build_config.setdefault(build['build_config'], []).append(build)
        builds_by_id.setdefault(build['id'], []).append(build)

    actions_by_cl = {}
    for action in self.clActionTable:
      k = (action['change_number'],
           action['patch_number'],
           action['change_source'])
      actions_by_cl.setdefault(k, []).append(action)
    flakes = {}
    ignored_flake_build_configs = set(['pre-cq-launcher'])
    for _cl, actions in actions_by_cl.iteritems():
      has_precq_success = any(
          c2['action'] == 'pre_cq_fully_verified'
          for c2 in actions)
      if has_precq_success:
        # We must use a list here instead of a set, because we're counting
        # flakes. We want to count two failures of the same config as two
        # different instances of flake.
        pre_cq_failed_configs = list([
            b['build_config']
            for c, b in JoinWithBuild(actions)
            if c['action'] == 'pre_cq_failed'
            and b['build_config'] not in ignored_flake_build_configs])

        for config in pre_cq_failed_configs:
          flakes.setdefault(config, 0)
          flakes[config] += 1

    result = []
    for config in runs_by_build_config:
      result.append(
          (config, flakes.get(config, 0), len(runs_by_build_config[config])))
    return result
