# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module to manage stage failure messages."""

from __future__ import print_function

import collections
import json
import re

from chromite.lib import cros_logging as logging

# Currently, an exception is reported to CIDB failureTabe using the exception
# class name as the exception_type. failure_message_lib.FailureMessageManager
# uses the exception_type to decide which StageFailureMessage class to use
# to rebuild the failure message. Whenever you need to change the names of these
# classes, please add the new class names to their corresponding type lists,
# and DO NOT remove the old class names from the type lists.
# TODO (nxia): instead of using the class name as the exception type when
# reporting an exception to CIDB, we need to have an attribute like
# EXCEPTION_CATEGORY (say EXCEPTION_TYPE) and this type cannot be changed or
# removed from EXCEPTION_TYPE_LIST. But we can add new types to the list.
BUILD_SCRIPT_FAILURE_TYPES = ('BuildScriptFailure',)
PACKAGE_BUILD_FAILURE_TYPES = ('PackageBuildFailure',)


# These keys must exist as column names from failureView in cidb.
FAILURE_KEYS = (
    'id', 'build_stage_id', 'outer_failure_id', 'exception_type',
    'exception_message', 'exception_category', 'extra_info',
    'timestamp', 'stage_name', 'board', 'stage_status', 'build_id',
    'master_build_id', 'builder_name', 'waterfall', 'build_number',
    'build_config', 'build_status', 'important', 'buildbucket_id')


# A namedtuple containing values fetched from CIDB failureView.
_StageFailure = collections.namedtuple('_StageFailure', FAILURE_KEYS)


class StageFailure(_StageFailure):
  """A class presenting values of a failure fetched from CIDB failureView."""

  @classmethod
  def GetStageFailureFromMessage(cls, stage_failure_message):
    """Create StageFailure from a StageFailureMessage instance.

    Args:
      stage_failure_message: An instance of StageFailureMessage.

    Returns:
      An instance of StageFailure.
    """
    return StageFailure(
        stage_failure_message.failure_id,
        stage_failure_message.build_stage_id,
        stage_failure_message.outer_failure_id,
        stage_failure_message.exception_type,
        stage_failure_message.exception_message,
        stage_failure_message.exception_category,
        stage_failure_message.extra_info, None,
        stage_failure_message.stage_name, None, None, None, None, None, None,
        None, None, None, None, None)

  @classmethod
  def GetStageFailureFromDicts(cls, failure_dict, stage_dict, build_dict):
    """Get StageFailure from value dictionaries.

    Args:
      failure_dict: A dict presenting values of a tuple from failureTable.
      stage_dict: A dict presenting values of a tuple from buildStageTable.
      build_dict: A dict presenting values of a tuple from buildTable.

    Returns:
      An instance of StageFailure.
    """
    return StageFailure(
        failure_dict['id'], failure_dict['build_stage_id'],
        failure_dict['outer_failure_id'], failure_dict['exception_type'],
        failure_dict['exception_message'], failure_dict['exception_category'],
        failure_dict['extra_info'], failure_dict['timestamp'],
        stage_dict['name'], stage_dict['board'], stage_dict['status'],
        build_dict['id'], build_dict['master_build_id'],
        build_dict['builder_name'], build_dict['waterfall'],
        build_dict['build_number'], build_dict['build_config'],
        build_dict['status'], build_dict['important'],
        build_dict['buildbucket_id'])


class StageFailureMessage(object):
  """Message class contains information of a general stage failure.

  Failed stages report stage failures to CIDB failureTable (see more details
  in failures_lib.ReportStageFailure). This class constructs a failure
  message instance from the stage failure information stored in CIDB.
  """

  def __init__(self, stage_failure, extra_info=None, stage_prefix_name=None):
    """Construct a StageFailureMessage instance.

    Args:
      stage_failure: An instance of StageFailure.
      extra_info: The extra info of the origin failure, default to None.
      stage_prefix_name: The prefix name (string) of the failed stage,
        default to None.
    """
    self.failure_id = stage_failure.id
    self.build_stage_id = stage_failure.build_stage_id
    self.stage_name = stage_failure.stage_name
    self.exception_type = stage_failure.exception_type
    self.exception_message = stage_failure.exception_message
    self.exception_category = stage_failure.exception_category
    self.outer_failure_id = stage_failure.outer_failure_id

    if extra_info is not None:
      self.extra_info = extra_info
    else:
      # No extra_info provided, decode extra_info from stage_failure.
      self.extra_info = self._DecodeExtraInfo(stage_failure.extra_info)

    if stage_prefix_name is not None:
      self.stage_prefix_name = stage_prefix_name
    else:
      # No stage_prefix_name provided, extra prefix name from stage_failure.
      self.stage_prefix_name = self._ExtractStagePrefixName(self.stage_name)

  def __str__(self):
    return ('[failure id] %s [stage name] %s [stage prefix name] %s '
            '[exception type] %s [exception category] %s [exception message] %s'
            ' [extra info] %s' %
            (self.failure_id, self.stage_name, self.stage_prefix_name,
             self.exception_type, self.exception_category,
             self.exception_message, self.extra_info))

  def _DecodeExtraInfo(self, extra_info):
    """Decode extra info json into dict.

    Args:
      extra_info: The extra_info of the origin exception, default to None.

    Returns:
      An empty dict if extra_info is None; extra_info itself if extra_info is
      a dict; else, load the json string into a dict and return it.
    """
    if not extra_info:
      return {}
    elif isinstance(extra_info, dict):
      return extra_info
    else:
      try:
        return  json.loads(extra_info)
      except ValueError as e:
        logging.error('Cannot decode extra_info: %s', e)
        return {}

  # TODO(nxia): Force format checking on stage names when they're created
  def _ExtractStagePrefixName(self, stage_name):
    """Extract stage prefix name given a full stage name.

    Format examples in our current CIDB buildStageTable:
      HWTest [bvt-arc] -> HWTest
      HWTest -> HWTest
      ImageTest -> ImageTest
      ImageTest [amd64-generic] -> ImageTest
      VMTest (attempt 1) -> VMTest
      VMTest [amd64-generic] (attempt 1) -> VMTest

    Args:
      stage_name: The full stage name (string) recorded in CIDB.

    Returns:
      The prefix stage name (string).
    """
    pattern = r'([^ ]+)( +\[([^]]+)\])?( +\(([^)]+)\))?'
    m = re.compile(pattern).match(stage_name)
    if m is not None:
      return m.group(1)
    else:
      return stage_name


class BuildScriptFailureMessage(StageFailureMessage):
  """Message class contains information of a BuildScriptFailure."""

  def __init__(self, stage_failure, **kwargs):
    """Construct a BuildScriptFailureMessage instance.

    Args:
      stage_failure: An instance of StageFailure.
      kwargs: Extra message information to pass to StageFailureMessage.
    """
    super(BuildScriptFailureMessage, self).__init__(stage_failure, **kwargs)

  def GetShortname(self):
    """Return the short name (string) of the run command."""
    return self.extra_info.get('shortname')


class PackageBuildFailureMessage(StageFailureMessage):
  """Message class contains information of a PackagebuildFailure."""

  def __init__(self, stage_failure, **kwargs):
    """Construct a PackageBuildFailureMessage instance.

    Args:
      stage_failure: An instance of StageFailure.
      kwargs: Extra message information to pass to StageFailureMessage.
    """
    super(PackageBuildFailureMessage, self).__init__(
        stage_failure, **kwargs)

  def GetShortname(self):
    """Return the short name (string) of the run command."""
    return self.extra_info.get('shortname')

  def GetFailedPackages(self):
    """Return a list of packages (strings) that failed to build."""
    return self.extra_info.get('failed_packages', [])


class CompoundFailureMessage(StageFailureMessage):
  """Message class contains information of a CompoundFailureMessage."""

  def __init__(self, stage_failure, **kwargs):
    """Construct a CompoundFailureMessage instance.

    Args:
      stage_failure: An instance of StageFailure.
      kwargs: Extra message information to pass to StageFailureMessage.
    """
    super(CompoundFailureMessage, self).__init__(stage_failure, **kwargs)

    self.inner_failures = []

  def __str__(self):
    msg_str = super(CompoundFailureMessage, self).__str__()

    for failure in self.inner_failures:
      msg_str += ('(Inner Stage Failure Message) %s' % str(failure))

    return msg_str

  @staticmethod
  def GetFailureMessage(failure_message):
    """Convert a regular failure message instance to CompoundFailureMessage.

    Args:
      failure_message: An instance of StageFailureMessage.

    Returns:
      A CompoundFailureMessage instance.
    """
    return CompoundFailureMessage(
        StageFailure.GetStageFailureFromMessage(failure_message),
        extra_info=failure_message.extra_info,
        stage_prefix_name=failure_message.stage_prefix_name)

  def HasEmptyList(self):
    """Check whether the inner failure list is empty.

    Returns:
      True if self.inner_failures is empty; else, False.
    """
    return not bool(self.inner_failures)

  def HasExceptionCategories(self, exception_categories):
    """Check whether any of the inner failures matches the exception categories.

    Args:
      exception_categories: A set of exception categories (members of
        constants.EXCEPTION_CATEGORY_ALL_CATEGORIES).

    Returns:
      True if any of the inner failures matches a memeber in
      exception_categories; else, False.
    """
    return any(x.exception_category in exception_categories
               for x in self.inner_failures)

  def MatchesExceptionCategories(self, exception_categories):
    """Check whether all of the inner failures matches the exception categories.

    Args:
      exception_categories: A set of exception categories (members of
        constants.EXCEPTION_CATEGORY_ALL_CATEGORIES).

    Returns:
      True if all of the inner failures match a memeber in
      exception_categories; else, False.
    """
    return (not self.HasEmptyList() and
            all(x.exception_category in exception_categories
                for x in self.inner_failures))


class FailureMessageManager(object):
  """Manager class to create a failure message or reconstruct messages."""

  @classmethod
  def CreateMessage(cls, stage_failure, **kwargs):
    """Create a failure message instance depending on the exception type.

    Args:
      stage_failure: An instance of StageFailure.
      kwargs: Extra message information to pass to StageFailureMessage.

    Returns:
      A failure message instance of StageFailureMessage class (or its
        sub-class)
    """
    if stage_failure.exception_type in BUILD_SCRIPT_FAILURE_TYPES:
      return BuildScriptFailureMessage(stage_failure, **kwargs)
    elif stage_failure.exception_type in PACKAGE_BUILD_FAILURE_TYPES:
      return PackageBuildFailureMessage(stage_failure, **kwargs)
    else:
      return StageFailureMessage(stage_failure, **kwargs)

  @classmethod
  def ReconstructMessages(cls, failure_messages):
    """Reconstruct failure messages by nesting messages.

    A failure message with not none outer_failure_id is an inner failure of its
    outer failure message(failure_id == outer_failure_id). This method takes a
    list of failure messages, reconstructs the list by 1) converting the outer
    failure message into a CompoundFailureMessage instance 2) insert the inner
    failure messages to the inner_failures list of their outer failure messages.
    CompoundFailures in CIDB aren't nested
    (see failures_lib.ReportStageFailure), so there isn't another
    inner failure list layer in a inner failure message and there're no circular
    dependencies.

    For example, given failure_messages list
      [A(failure_id=1),
       B(failure_id=2, outer_failure_id=1),
       C(failure_id=3, outer_failure_id=1),
       D(failure_id=4),
       E(failure_id=5, outer_failure_id=4),
       F(failure_id=6)]
    this method returns a reconstructed list:
      [A(failure_id=1, inner_failures=[B(failure_id=2, outer_failure_id=1),
                                       C(failure_id=3, outer_failure_id=1)]),
       D(failure_id=4, inner_failures=[E(failure_id=5, outer_failure_id=4)]),
       F(failure_id=6)]

    Args:
      failure_messages: A list a failure message instances not nested.

    Returns:
      A list of failure message instances of StageFailureMessage class (or its
        sub-class). Failure messages with not None outer_failure_id are nested
        into the inner_failures list of their outer failure messages.
    """
    failure_message_dict = {x.failure_id: x for x in failure_messages}

    for failure in failure_messages:
      if failure.outer_failure_id is not None:
        assert failure.outer_failure_id in failure_message_dict
        outer_failure = failure_message_dict[failure.outer_failure_id]
        if not isinstance(outer_failure, CompoundFailureMessage):
          outer_failure = CompoundFailureMessage.GetFailureMessage(
              outer_failure)
          failure_message_dict[outer_failure.failure_id] = outer_failure

        outer_failure.inner_failures.append(failure)
        del failure_message_dict[failure.failure_id]

    return failure_message_dict.values()

  @classmethod
  def ConstructStageFailureMessages(cls, stage_failures):
    """Construct stage failure messages from failure entries from CIDB.

    Args:
      stage_failures: A list of StageFailure instances.

    Returns:
      A list of stage failure message instances of StageFailureMessage class
      (or its sub-class). See return type of ReconstructMessages().
    """
    failure_messages = [cls.CreateMessage(f) for f in stage_failures]

    return cls.ReconstructMessages(failure_messages)
