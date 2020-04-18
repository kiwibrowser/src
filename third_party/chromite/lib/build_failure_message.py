# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module to manage failure message of builds."""

from __future__ import print_function

from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import failure_message_lib
from chromite.lib import hwtest_results
from chromite.lib import patch as cros_patch
from chromite.lib import portage_util
from chromite.lib import triage_lib


class BuildFailureMessage(object):
  """Message indicating that changes failed to be validated.

  A failure message for a failed build, which is used to trige failures and
  detect bad changes.
  """

  def __init__(self, message_summary, failure_messages, internal, reason,
               builder):
    """Create a BuildFailureMessage instance.

    Args:
      message_summary: The message summary string to print.
      failure_messages: A list of failure messages (instances of
        StageFailureMessage), if any.
      internal: Whether this failure occurred on an internal builder.
      reason: A string describing the failure.
      builder: The builder the failure occurred on.
    """
    self.message_summary = str(message_summary)
    self.failure_messages = failure_messages or []
    self.internal = bool(internal)
    self.reason = str(reason)
    # builder should match build_config, e.g. self._run.config.name.
    self.builder = str(builder)

  def __str__(self):
    return self.message_summary

  def BuildFailureMessageToStr(self):
    """Return a string presenting the information in the BuildFailureMessage."""
    to_str = ('[builder] %s [message summary] %s [reason] %s [internal] %s\n' %
              (self.builder, self.message_summary, self.reason, self.internal))
    for f in self.failure_messages:
      to_str += '[failure message] ' + str(f) + '\n'

    return to_str

  def GetFailingStages(self):
    """Get a list of the failing stage prefixes from failure_messages.

    Returns:
      A list of failing stage prefixes if there are failure_messages; None
      otherwise.
    """
    failing_stages = None
    if self.failure_messages:
      failing_stages = set(x.stage_prefix_name for x in self.failure_messages)
    return failing_stages

  def MatchesExceptionCategories(self, exception_categories):
    """Check if all of the failure_messages match the exception_categories.

    Args:
      exception_categories: A set of exception categories (members of
        constants.EXCEPTION_CATEGORY_ALL_CATEGORIES).

    Returns:
      True if all of the failure_messages match a member in
      exception_categories; else, False.
    """
    for failure in self.failure_messages:
      if failure.exception_category not in exception_categories:
        if (isinstance(failure, failure_message_lib.CompoundFailureMessage) and
            failure.MatchesExceptionCategories(exception_categories)):
          continue
        else:
          return False

    return True

  def HasExceptionCategories(self, exception_categories):
    """Check if any of the failure_messages match the exception_categories.

    Args:
      exception_categories: A set of exception categories (members of
        constants.EXCEPTION_CATEGORY_ALL_CATEGORIES).

    Returns:
      True if any of the failure_messages match a member in
      exception_categories; else, False.
    """
    for failure in self.failure_messages:
      if failure.exception_category in exception_categories:
        return True

      if (isinstance(failure, failure_message_lib.CompoundFailureMessage) and
          failure.HasExceptionCategories(exception_categories)):
        return True

    return False

  def FindSuspectedChanges(self, changes, build_root, failed_hwtests, sanity):
    """Find and return suspected changes.

    Suspected changes are CLs that probably caused failures and will be
    rejected. This method analyzes every failure message and returns a set of
    changes as suspects.
    1) if a failure message is a PackageBuildFailure, get suspects for the build
    failure. If there're failed packages without assigned suspects, blame all
    changes when sanity is True.
    2) if a failure message is a TEST failure, get suspects for the HWTest
    failure. If there're failed HWTests without assigned suspects, blame all
    changes when sanity is True.
    3) If a failure message is neither PackagebuildFailure nor HWTestFailure,
    we can't explain the failure and so blame all changes when sanity is True.

    It is certainly possible to trick this algorithm: If one developer submits
    a change to libchromeos that breaks the power_manager, and another developer
    submits a change to the power_manager at the same time, only the
    power_manager change will be kicked out. That said, in that situation, the
    libchromeos change will likely be kicked out on the next run when the next
    run fails power_manager but dosen't include any changes from power_manager.

    Args:
      changes: A list of cros_patch.GerritPatch instances.
      build_root: The path to the build root.
      failed_hwtests: A list of name of failed hwtests got from CIDB (see the
        return type of HWTestResultManager.GetFailedHWTestsFromCIDB), or None.
      sanity: The sanity checker builder passed and the tree was open when
        the build started and ended.

    Returns:
      An instance of triage_lib.SuspectChanges.
    """
    suspect_changes = triage_lib.SuspectChanges()
    blame_everything = False
    for failure in self.failure_messages:
      if (failure.exception_type in
          failure_message_lib.PACKAGE_BUILD_FAILURE_TYPES):
        # Find suspects for PackageBuildFailure
        build_suspects, no_assignee_packages = (
            self.FindPackageBuildFailureSuspects(changes, failure))
        suspect_changes.update(
            {x: constants.SUSPECT_REASON_BUILD_FAIL for x in build_suspects})
        blame_everything = blame_everything or no_assignee_packages
      elif failure.exception_category == constants.EXCEPTION_CATEGORY_TEST:
        # Find suspects for HWTestFailure
        hwtest_suspects, no_assignee_hwtests = (
            hwtest_results.HWTestResultManager.FindHWTestFailureSuspects(
                changes, build_root, failed_hwtests))
        suspect_changes.update(
            {x: constants.SUSPECT_REASON_TEST_FAIL for x in hwtest_suspects})
        blame_everything = blame_everything or no_assignee_hwtests
      else:
        # Unknown failures, blame everything
        blame_everything = True

    # Only do broad-brush blaming if the tree is sane.
    if sanity:
      if blame_everything or len(suspect_changes) == 0:
        suspect_changes.update(
            {x: constants.SUSPECT_REASON_UNKNOWN for x in changes})
      else:
        # Never treat changes to overlays as innocent.
        overlay_changes = [x for x in changes if '/overlays/' in x.project]
        suspect_changes.update(
            {x: constants.SUSPECT_REASON_OVERLAY_CHANGE
             for x in overlay_changes})

    return suspect_changes

  def FindPackageBuildFailureSuspects(self, changes, failure):
    """Find suspects for a PackageBuild failure.

    If a change touched a package and that package broke, this change is one of
    the suspects; if multiple changes touched one failed package, all these
    changes will be returned as suspects.

    Args:
      changes: A list of cros_patch.GerritPatch instances.
      failure: An instance of StageFailureMessage(or its sub-class).

    Returns:
      A pair of suspects and no_assignee_packages. suspects is a set of
      cros_patch.GerritPatch instances as suspects. no_assignee_packages is True
      when there're failed packages without assigned suspects; else,
      no_assignee_packages is False.
    """
    suspects = set()
    no_assignee_packages = False
    packages_with_assignee = set()
    failed_packages = failure.GetFailedPackages()
    for package in failed_packages:
      failed_projects = portage_util.FindWorkonProjects([package])
      for change in changes:
        if change.project in failed_projects:
          suspects.add(change)
          packages_with_assignee.add(package)

    if suspects:
      logging.info('Find suspects for BuildPackages failures: %s',
                   cros_patch.GetChangesAsString(suspects))

    packages_without_assignee = set(failed_packages) - packages_with_assignee
    if packages_without_assignee:
      logging.info('Didn\'t find changes to blame for failed packages: %s',
                   list(packages_without_assignee))
      no_assignee_packages = True

    return suspects, no_assignee_packages
