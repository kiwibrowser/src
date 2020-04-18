# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module that helps to triage Commit Queue failures."""

from __future__ import print_function

import glob
import os
import pprint
import re

from chromite.lib import constants
from chromite.lib import cq_config
from chromite.lib import cros_logging as logging
from chromite.lib import gerrit
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import patch as cros_patch
from chromite.lib import portage_util


def GetRelevantOverlaysForConfig(config, build_root):
  """Returns a list of overlays relevant to |config|.

  Args:
    config: A cbuildbot config name.
    build_root: Path to the build root.

  Returns:
    A set of overlays.
  """
  relevant_overlays = set()
  for board in config.boards:
    overlays = portage_util.FindOverlays(
        constants.BOTH_OVERLAYS, board, build_root)
    relevant_overlays.update(overlays)

  return relevant_overlays


def _GetAffectedImmediateSubdirs(change, git_repo):
  """Gets the set of immediate subdirs affected by |change|.

  Args:
    change: GitRepoPatch to examine.
    git_repo: Path to checkout of git repository.

  Returns:
    A set of absolute paths to modified subdirectories of |git_repo|.
  """
  return set([os.path.join(git_repo, path.split(os.path.sep)[0])
              for path in change.GetDiffStatus(git_repo)])


def GetAffectedOverlays(change, manifest, all_overlays):
  """Get the set of overlays affected by a given change.

  Args:
    change: The GerritPatch instance to look at.
    manifest: A ManifestCheckout instance representing our build directory.
    all_overlays: The set of all valid overlays.

  Returns:
    The set of overlays affected by the specified |change|. If the change
    affected something other than an overlay, return None.
  """
  checkout = change.GetCheckout(manifest, strict=False)
  if checkout:
    git_repo = checkout.GetPath(absolute=True)

    # The whole git repo is an overlay. Return it.
    # Example: src/private-overlays/overlay-x86-zgb-private
    if git_repo in all_overlays:
      return set([git_repo])

    # Get the set of immediate subdirs affected by the change.
    # Example: src/overlays/overlay-x86-zgb
    subdirs = _GetAffectedImmediateSubdirs(change, git_repo)

    # If all of the subdirs are overlays, return them.
    if subdirs.issubset(all_overlays):
      return subdirs


def GetAffectedPackagesForOverlayChange(change, manifest, overlays):
  """Get the set of packages affected by the overlay |change|.

  Args:
    change: The GerritPatch instance that modifies an overlay.
    manifest: A ManifestCheckout instance representing our build directory.
    overlays: List of overlay paths.

  Returns:
    The set of packages affected by the specified |change|. E.g.
    {'chromeos-base/chromite-0.0.1-r1258'}. If the change affects
    something other than packages, return None.
  """
  checkout = change.GetCheckout(manifest, strict=False)
  if checkout:
    git_repo = checkout.GetPath(absolute=True)

  packages = set()
  for path in change.GetDiffStatus(git_repo):
    # Determine if path is in a package directory by walking up
    # directories and see if there is an ebuild in the directory.
    start_path = os.path.join(git_repo, path)
    ebuild_path = osutils.FindInPathParents(
        '*.ebuild', start_path, test_func=glob.glob, end_path=git_repo)
    if ebuild_path:
      # Convert git_repo/../*.ebuild to the real ebuild path.
      ebuild_path = glob.glob(ebuild_path)[0]
      # Double check that the ebuild is two-levels deep in an overlay
      # directory.
      if os.path.sep.join(ebuild_path.split(os.path.sep)[:-3]) in overlays:
        category, pkg_name, _ = portage_util.SplitEbuildPath(ebuild_path)
        packages.add('%s/%s' % (category, pkg_name))
        continue

    # If |change| affects anything other than packages, return None.
    return None

  return packages


def GetTestSubsystemForChange(build_root, change):
  """Get a list of subsystem that a given |change| affects.

  If subsystem is specified in the commit message, use that. Otherwise, look in
  appropriate COMMIT-QUEUE.ini. If subsystem is not specified anywhere,
  'subsystem:default' will be used.

  Based on the subsystems a given |change| affects, the CQ could tell whether a
  failure is potentially caused by this |change|. The CQ could then submit some
  changes in the face of unrelated failures.

  Args:
    build_root: The root of the checkout.
    change: Change to examine, as a PatchQuery object.

  Returns:
    A list of subsystem for the given |change|.
  """
  subsystems = []
  if change.commit_message:
    lines = cros_patch.GetOptionLinesFromCommitMessage(
        change.commit_message, 'subsystem:')
    if lines:
      subsystems = [x for x in re.split("[, ]", ' '.join(lines)) if x]
  if not subsystems:
    cq_config_parser = cq_config.CQConfigParser(build_root, change)
    subsystems = cq_config_parser.GetSubsystems()
  return subsystems if subsystems else ['default']


class CategorizeChanges(object):
  """A collection of methods to help categorize GerritPatch changes.

  This class is mainly used on a build slave to categorize changes
  applied in the build.
  """

  @classmethod
  def ClassifyOverlayChanges(cls, changes, config, build_root, manifest,
                             packages_under_test):
    """Classifies overlay changes in |changes|.

    Args:
      changes: The list or set of GerritPatch instances.
      config: The cbuildbot config.
      build_root: Path to the build root.
      manifest: A ManifestCheckout instance representing our build directory.
      packages_under_test: A list of packages names included in the build
        without version/revision (e.g. ['chromeos-base/chromite']). If None,
        don't try to map overlay changes to packages.

    Returns:
      A (overlay_changes, irrelevant_overlay_changes) tuple; overlay_changes
      is a subset of |changes| that have modified one or more overlays, and
      irrelevant_overlay_changes is a subset of overlay_changes which are
      irrelevant to |config|.
    """
    visible_overlays = set(portage_util.FindOverlays(config.overlays, None,
                                                     build_root))
    # The overlays relevant to this build.
    relevant_overlays = GetRelevantOverlaysForConfig(config, build_root)

    overlay_changes = set()
    irrelevant_overlay_changes = set()
    for change in changes:
      affected_overlays = GetAffectedOverlays(change, manifest,
                                              visible_overlays)
      if affected_overlays is not None:
        # The change modifies an overlay.
        overlay_changes.add(change)
        if not any(x in relevant_overlays for x in affected_overlays):
          # The change touched an irrelevant overlay.
          irrelevant_overlay_changes.add(change)
          continue

        if packages_under_test:
          # If the change modifies packages that are not part of this
          # build, they are considered irrelevant too.
          packages = GetAffectedPackagesForOverlayChange(
              change, manifest, visible_overlays)
          if packages:
            logging.info('%s affects packages %s',
                         cros_patch.GetChangesAsString([change]),
                         ', '.join(packages))
            if not any(x in packages_under_test for x in packages):
              irrelevant_overlay_changes.add(change)

    return overlay_changes, irrelevant_overlay_changes

  @classmethod
  def ClassifyWorkOnChanges(cls, changes, config, build_root,
                            manifest, packages_under_test):
    """Classifies WorkOn package changes in |changes|.

    Args:
      changes: The list or set of GerritPatch instances.
      config: The cbuildbot config.
      build_root: Path to the build root.
      manifest: A ManifestCheckout instance representing our build directory.
      packages_under_test: A list of packages names included in the build.
        (e.g. ['chromeos-base/chromite-0.0.1-r1258']).

    Returns:
      A (workon_changes, irrelevant_workon_changes) tuple; workon_changes
      is a subset of |changes| that have modified workon packages, and
      irrelevant_workon_changes is a subset of workon_changes which are
      irrelevant to |config|.
    """
    workon_changes = set()
    irrelevant_workon_changes = set()

    workon_dict = portage_util.BuildFullWorkonPackageDictionary(
        build_root, config.overlays, manifest)

    pp = pprint.PrettyPrinter(indent=2)
    logging.info('(project, branch) to workon package mapping:\n %s',
                 pp.pformat(workon_dict))
    logging.info('packages under test\n: %s', pp.pformat(packages_under_test))

    for change in changes:
      packages = workon_dict.get((change.project, change.tracking_branch))
      if packages:
        # The CL modifies a workon package.
        workon_changes.add(change)
        if all(x not in packages_under_test for x in packages):
          irrelevant_workon_changes.add(change)

    return workon_changes, irrelevant_workon_changes

  @classmethod
  def _FilterProjectsInManifestByGroup(cls, manifest, groups):
    """Filters projects in |manifest| by |groups|.

    Args:
      manifest: A git.Manifest instance.
      groups: A list of groups to filter.

    Returns:
      A set of (project, branch) tuples where each tuple is asssociated
      with at least one group in |groups|.
    """
    results = set()
    for project, checkout_list in manifest.checkouts_by_name.iteritems():
      for checkout in checkout_list:
        if any(x in checkout['groups'] for x in groups):
          branch = git.StripRefs(checkout['tracking_branch'])
          results.add((project, branch))

    return results

  @classmethod
  def GetChangesToBuildTools(cls, changes, manifest):
    """Returns a changes associated with buildtools projects.

    Args:
      changes: The list or set of GerritPatch instances.
      manifest: A git.Manifest instance.

    Returns:
      A subset of |changes| to projects of "buildtools" group.
    """
    buildtool_set = cls._FilterProjectsInManifestByGroup(
        manifest, ['buildtools'])
    return set([x for x in changes if (x.project, x.tracking_branch)
                in buildtool_set])

  @classmethod
  def GetIrrelevantChanges(cls, changes, config, build_root, manifest,
                           packages_under_test):
    """Determine changes irrelavant to build |config|.

    This method determine a set of changes that are irrelevant to the
    build |config|. The general rule of thumb is that if we are unsure
    whether a change is relevant, consider it relevant.

    Args:
      changes: The list or set of GerritPatch instances.
      config: The cbuildbot config.
      build_root: Path to the build root.
      manifest: A ManifestCheckout instance representing our build directory.
      packages_under_test: A list of packages that were tested in this build.

    Returns:
      A subset of |changes| which are irrelevant to |config|.
    """
    untriaged_changes = set(changes)
    irrelevant_changes = set()

    # Changes that modify projects used in building are always relevant.
    untriaged_changes -= cls.GetChangesToBuildTools(changes, manifest)

    if packages_under_test is not None:
      # Strip the version of the package in packages_under_test.
      cpv_list = [portage_util.SplitCPV(x) for x in packages_under_test]
      packages_under_test = ['%s/%s' % (x.category, x.package) for x in
                             cpv_list]

    # Handles overlay changes.
    # ClassifyOverlayChanges only handles overlays visible to this
    # build. For example, an external build may not be able to view
    # the internal overlays. However, in that case, the internal changes
    # have already been filtered out in CommitQueueSyncStage, and are
    # not included in |changes|.
    overlay_changes, irrelevant_overlay_changes = cls.ClassifyOverlayChanges(
        untriaged_changes, config, build_root, manifest, packages_under_test)
    untriaged_changes -= overlay_changes
    irrelevant_changes |= irrelevant_overlay_changes

    # Handles workon package changes.
    if packages_under_test is not None:
      try:
        workon_changes, irrelevant_workon_changes = cls.ClassifyWorkOnChanges(
            untriaged_changes, config, build_root, manifest,
            packages_under_test)
      except Exception as e:
        # Ignore the exception if we cannot categorize workon
        # changes. We will conservatively assume the changes are
        # relevant.
        logging.warning('Unable to categorize cros workon changes: %s', e)
      else:
        untriaged_changes -= workon_changes
        irrelevant_changes |= irrelevant_workon_changes

    return irrelevant_changes


class CalculateSuspects(object):
  """Diagnose the cause for a given set of failures."""

  @classmethod
  def GetBlamedChanges(cls, changes):
    """Returns the changes that have been manually blamed.

    Args:
      changes: List of GerritPatch changes.

    Returns:
      A list of |changes| that were marked verified: -1 or
      code-review: -2.
    """
    # Load the latest info about whether the changes were vetoed, in case they
    # were vetoed in the middle of a cbuildbot run. That said, be careful not to
    # return info about newer patchsets.
    reloaded_changes = gerrit.GetGerritPatchInfoWithPatchQueries(changes)
    return [x for x, y in zip(changes, reloaded_changes) if y.WasVetoed()]

  @classmethod
  def FindSuspectsForFailures(cls, changes, messages, build_root,
                              failed_hwtests, sanity):
    """Find suspects for the given failure messages and hwtests.

    If messages contain NoneType message and sanity is True, return all changes
    as suspects.

    Args:
      changes: A list of cros_patch.GerritPatch instances.
      messages: A list of build_failure_message.BuildFailureMessage or NoneType
        instances from the failed slaves.
      build_root: The path to the build root.
      failed_hwtests: A list of names of failed hwtests got from CIDB (see the
        return type of HWTestResultManager.GetFailedHWTestsFromCIDB) or a
        NoneType instance.
      sanity: The sanity checker builder passed and the tree was open when
              the build started and ended.

    Returns:
      An instance of SuspectChanges.
    """
    suspect_changes = SuspectChanges()
    for message in messages:
      if message:
        new_suspect_changes = message.FindSuspectedChanges(
            changes, build_root, failed_hwtests, sanity)
        suspect_changes.update(new_suspect_changes)
      elif sanity:
        suspect_changes.update(
            {x: constants.SUSPECT_REASON_UNKNOWN for x in changes})
    return suspect_changes

  @classmethod
  def FilterChangesForInfraFail(cls, changes):
    """Returns a list of changes responsible for infra failures."""
    # Chromite changes could cause infra failures.
    return [x for x in changes if x.project in constants.INFRA_PROJECTS]

  @classmethod
  def _MatchesExceptionCategories(cls, messages, exception_categories,
                                  strict=True):
    """Returns True if all failure messages are in the exception_categories.

    Args:
      messages: A list of build_failure_message.BuildFailureMessage or NoneType
        objects from the failed slaves.
      exception_categories: A set of exception categories (members of
         constants.EXCEPTION_CATEGORY_ALL_CATEGORIES).
      strict: If False, treat NoneType message as a match.

    Returns:
      Returns True if all the messages matches exception_categories (when strict
      is off, None message is considered as a match).
    """
    if not messages:
      return False

    if strict and not all(messages):
      return False

    return (all(x.MatchesExceptionCategories(exception_categories)
                for x in messages if x))

  @classmethod
  def OnlyLabFailures(cls, messages, no_stat):
    """Determine if the cause of build failure was lab failure.

    Args:
      messages: A list of build_failure_message.BuildFailureMessage or NoneType
        objects from the failed slaves.
      no_stat: A list of builders which failed prematurely without reporting
        status.

    Returns:
      True if the build failed purely due to lab failures.
    """
    # If any builder failed prematuely, lab failure was not the only cause.
    return (not no_stat and cls._MatchesExceptionCategories(
        messages, {constants.EXCEPTION_CATEGORY_LAB}))

  @classmethod
  def OnlyInfraFailures(cls, messages, no_stat):
    """Determine if the cause of build failure was infrastructure failure.

    All failures in 'lab' and 'infra' categories are infra failures.

    Args:
      messages: A list of build_failure_message.BuildFailureMessage or NoneType
        objects from the failed slaves.
      no_stat: A list of builders which failed prematurely without reporting
        status.

    Returns:
      True if the build failed purely due to infrastructure failures.
    """
    # "Failed to report status" and "NoneType" messages are considered
    # infra failures.
    return ((not messages and no_stat) or cls._MatchesExceptionCategories(
        messages,
        {constants.EXCEPTION_CATEGORY_INFRA, constants.EXCEPTION_CATEGORY_LAB},
        strict=False))

  @classmethod
  def FindSuspects(cls, changes, messages, infra_fail=False, lab_fail=False,
                   build_root=None, failed_hwtests=None, sanity=True):
    """Find out what changes probably caused our failure.

    1) if there're bad changes to blame, return the bad changes as the suspects;
    2) else if there're only internal lab failures, return an empty suspects;
    3) else if there're only internal infra failures, return infra changes as
    the suspects;
    4) else, find and return suspects by analyzing the failures.

    Args:
      changes: A list of cros_patch.GerritPatch instances to consider.
      messages: A list of build failure messages, of type
        build_failure_message.BuildFailureMessage or of type NoneType.
      infra_fail: The build failed purely due to infrastructure failures.
      lab_fail: The build failed purely due to test lab infrastructure
        failures.
      build_root: The path to the build root.
      failed_hwtests: A list of names of failed hwtests got from CIDB (see the
        return type of HWTestResultManager.GetFailedHWTestsFromCIDB) or a
        NoneType instance.
      sanity: The sanity checker builder passed and the tree was open when
        the build started and ended.

    Returns:
      An instance of SuspectChanges.
    """
    suspect_changes = SuspectChanges()
    bad_changes = cls.GetBlamedChanges(changes)
    if bad_changes:
      # If there are changes that have been set verified=-1 or
      # code-review=-2, these changes are the ONLY suspects of the
      # failed build.
      logging.warning('Detected that some changes have been blamed for '
                      'the build failure. Only these CLs will be rejected: %s',
                      cros_patch.GetChangesAsString(bad_changes))

      suspect_changes.update(
          {x: constants.SUSPECT_REASON_BAD_CHANGE for x in bad_changes})
    elif lab_fail:
      logging.warning('Detected that the build failed purely due to HW '
                      'Test Lab failure(s). Will not reject any changes')
    elif infra_fail:
      # The non-lab infrastructure errors might have been caused
      # by chromite changes.
      logging.warning(
          'Detected that the build failed due to non-lab infrastructure '
          'issue(s). Will only reject chromite changes')
      infra_changes = cls.FilterChangesForInfraFail(changes)
      suspect_changes.update(
          {x: constants.SUSPECT_REASON_INFRA_FAIL for x in infra_changes})
    else:
      suspect_changes = cls.FindSuspectsForFailures(
          changes, messages, build_root, failed_hwtests, sanity)

    return suspect_changes

  @classmethod
  def CanIgnoreFailures(cls, messages, change, build_root,
                        subsys_by_config):
    """Examine whether we can ignore the failures for |change|.

    First, examine the |messages| to see if we are allowed to ignore
    the failures base on the per-repository settings in COMMIT_QUEUE.ini.
    If not, then check whether only the HWTestStage failed, and the failed
    subsystems are unrelated to this change.

    Args:
      messages: A list of build_failure_message.BuildFailureMessage from the
        failed slaves.
      change: A GerritPatch instance to examine.
      build_root: Build root directory.
      subsys_by_config: A dictionary of pass/fail HWTest subsystems indexed
        by the config names.

    Returns:
      A tuple, first element is True/False indicates whether we can ignore the
      failures; second element is the reason to ignore the failures, None if
      failures cannot be ignored.
    """
    # Some repositories may opt to ignore certain stage failures.
    failing_stages = set()
    if any(x.GetFailingStages() is None for x in messages):
      # If there are no tracebacks, that means that the builder
      # did not report its status properly. We don't know what
      # stages failed and cannot safely ignore any stage.
      return (False, None)

    for message in messages:
      failing_stages.update(message.GetFailingStages())

    cq_config_parser = cq_config.CQConfigParser(build_root, change)
    ignored_stages = cq_config_parser.GetStagesToIgnore()
    if ignored_stages and failing_stages.issubset(ignored_stages):
      return (True, constants.STRATEGY_CQ_PARTIAL_IGNORED_STAGES)

    # Among the failed stages, except the stages that can be ignored, only
    # HWTestStage fails, and subsystem logic has been used on all configs
    # failed at HWTest, and failed subsystems of each config are unrelated to
    # the current cl, we submit the cl.
    if not subsys_by_config:
      return (False, None)
    relevant_failing_stages = failing_stages - set(ignored_stages)
    if relevant_failing_stages == set(['HWTest']):
      configs_failed_hwtest = set([m.builder for m in messages
                                   if 'HWTest' in m.GetFailingStages()])
      subsys_result_dicts = ([v for k, v in subsys_by_config.iteritems()
                              if k in configs_failed_hwtest])
      if len(configs_failed_hwtest) != len(subsys_result_dicts):
        logging.warning('There exists malformed config names, which results in '
                        'mismatch between build configs. Cannot decide '
                        'whether change is innocent to failures. Will not '
                        'submit')
        return (False, None)
      # Empty dict indicates subsystem logic not used, make sure subsystem logic
      # is used in all the configs failed at hwtest.
      if all(subsys_result_dicts):
        all_fail_subsys, all_pass_subsys = set(), set()
        for v in subsys_result_dicts:
          all_fail_subsys |= set(v.get('fail_subsystems', []))
          all_pass_subsys |= set(v.get('pass_subsystems', []))
        all_pass_subsys -= all_fail_subsys
        cl_subsys = set(GetTestSubsystemForChange(build_root, change))
        if (not cl_subsys & all_fail_subsys) and (cl_subsys <= all_pass_subsys):
          return (True, constants.STRATEGY_CQ_PARTIAL_SUBSYSTEM)

    return (False, None)

  @classmethod
  def GetFullyVerifiedChanges(cls, changes, changes_by_config, subsys_by_config,
                              passed_in_history_slaves_by_change, failing,
                              inflight, no_stat, messages, build_root):
    """Examines build failures and returns a set of fully verified changes.

    A change is fully verified if all the build configs relevant to
    this change have either passed or failed in a manner that can be
    safely ignored by the change.

    Args:
      changes: A list of GerritPatch instances to examine.
      changes_by_config: A dictionary of relevant changes indexed by the
        config names.
      subsys_by_config: A dictionary of pass/fail HWTest subsystems indexed
        by the config names.
      passed_in_history_slaves_by_change: A dict mapping changes to their
        relevant slaves (build config name strings) which passed in history.
      failing: Names of the builders that failed.
      inflight: Names of the builders that timed out.
      no_stat: Set of builder names of slave builders that had status None.
      messages: A list of build_failure_message.BuildFailureMessage or NoneType
        objects from the failed slaves.
      build_root: Build root directory.

    Returns:
      A dictionary mapping the fully verified changes to their string reasons
      for submission. (Should be None or constant with name STRATEGY_* from
      constants.py.)
    """
    changes = set(changes)
    no_stat = set(no_stat)
    failing = set(failing)
    inflight = set(inflight)

    fully_verified = dict()

    all_tested_changes = set()
    for tested_changes in changes_by_config.itervalues():
      all_tested_changes.update(tested_changes)

    untested_changes = changes - all_tested_changes
    if untested_changes:
      # Some board overlay changes were not tested by CQ at all.
      logging.info('These changes were not tested by any slaves, '
                   'so they will be submitted: %s',
                   cros_patch.GetChangesAsString(untested_changes))
      fully_verified.update({c: constants.STRATEGY_CQ_PARTIAL_NOT_TESTED
                             for c in untested_changes})

    not_completed = set.union(no_stat, inflight)

    for change in all_tested_changes:
      # If each of the relevant configs associated with a change satisifies one
      # of the conditions:
      # 1) passed successfully; OR
      # 2) failed with failures which can be ignored by the change; OR
      # 3) there are builds of the relevant build config passed in history.
      # this change will be considered as fully verified.
      verified = True
      verified_reasons = set()
      relevant_configs = [k for k, v in changes_by_config.iteritems() if
                          change in v]
      passed_in_history_slaves = passed_in_history_slaves_by_change.get(
          change, set())
      logging.info('Checking change %s; relevant configs %s; configs passed in '
                   'history %s.', change.PatchLink(), relevant_configs,
                   list(passed_in_history_slaves))

      for build_config in relevant_configs:
        if build_config in not_completed:
          if build_config in passed_in_history_slaves:
            verified_reasons.add(constants.STRATEGY_CQ_PARTIAL_CQ_HISTORY)
          else:
            logging.info('Failed to verify change %s: relevant build %s isn\'t '
                         'completed in current run and didn\'t pass in history',
                         change.PatchLink(), build_config)
            verified = False
            break
        elif build_config in failing:
          failed_messages = [x for x in messages if x.builder == build_config]
          ignore_result = cls.CanIgnoreFailures(
              failed_messages, change, build_root, subsys_by_config)
          if ignore_result[0]:
            verified_reasons.add(ignore_result[1])
          elif build_config in passed_in_history_slaves:
            verified_reasons.add(constants.STRATEGY_CQ_PARTIAL_CQ_HISTORY)
          else:
            logging.info('Failed to verify change %s: relevant build %s failed '
                         'with not ignorable failures in current run and '
                         'didn\'t pass in history',
                         change.PatchLink(), build_config)
            verified = False
            break
        else:
          verified_reasons.add(constants.STRATEGY_CQ_PARTIAL_BUILDS_PASSED)

      if verified:
        reason = cls._GetVerifiedReason(verified_reasons)
        fully_verified.update({change: reason})
        logging.info('Change %s is verified with reasons %s, choose the final '
                     'reason %s.', change.PatchLink(), list(verified_reasons),
                     reason)

    return fully_verified

  @classmethod
  def _GetVerifiedReason(cls, verified_reasons):
    """Get a reason with the highest prioirty from a set of verified reasons.

    Args:
      verified_reasons: A set of verified reasons (memebers of
          constants.STRATEGY_CQ_PARTIAL_REASONS).

    Returns:
      The reason with the highest prioirty.
    """
    if verified_reasons:
      reasons = list(verified_reasons)
      reason = reasons[0]
      for r in reasons[1:]:
        if (constants.STRATEGY_CQ_PARTIAL_REASONS[r] <
            constants.STRATEGY_CQ_PARTIAL_REASONS[reason]):
          reason = r

      return reason


class SuspectChanges(dict):
  """A dict mapping from suspected changes to their suspect reasons.

  The suspect reason of a change can be updated in the dict only when the change
  isn't in the dict, or the new suspect reason has a higher blame priority.
  """

  def __init__(self, suspect_dict=None):
    """Initialize a SuspectChanges object."""
    if suspect_dict is None:
      suspect_dict = {}
    super(SuspectChanges, self).__init__(suspect_dict)

  def __setitem__(self, key, value):
    """Overwrite __setitem__."""
    if key not in self or value < self[key]:
      super(SuspectChanges, self).__setitem__(key, value)

  def setdefault(self, key, value):
    """Overwrite setdefault."""
    assert value in constants.SUSPECT_REASONS

    if key not in self or value < self[key]:
      self[key] = value
    return self[key]

  def update(self, *args, **kwargs):
    """Overwrite update."""
    if args:
      if len(args) > 1:
        raise TypeError('update expected at most 1 arguments, got %d' %
                        len(args))

      other = dict(args[0])
      for key, value in other.iteritems():
        if key not in self or value < self[key]:
          self[key] = value

      for key, value in kwargs.iteritems():
        if key not in self or value < self[key]:
          self[key] = value
