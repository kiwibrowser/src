# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module that contains unittests for triage_lib module."""

from __future__ import print_function

import json
import mock

from chromite.lib import build_failure_message
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cq_config
from chromite.lib import cros_test_lib
from chromite.lib import failure_message_lib_unittest
from chromite.lib import gerrit
from chromite.lib import patch as cros_patch
from chromite.lib import patch_unittest
from chromite.lib import portage_util
from chromite.lib import triage_lib


site_config = config_lib.GetConfig()
failure_msg_helper = failure_message_lib_unittest.FailureMessageHelper()


class GetTestSubsystemForChangeTests(cros_test_lib.MockTestCase):
  """Tests for GetTestSubsystemForChange."""

  def setUp(self):
    self.PatchObject(cq_config.CQConfigParser, 'GetCommonConfigFileForChange')
    self._patch_factory = patch_unittest.MockPatchFactory()

  def testGetSubsystemFromValidCommitMessage(self):
    """Test whether we can get subsystem from commit message."""
    change = self._patch_factory.MockPatch(
        commit_message='First line\nThird line\nsubsystem: network audio\n'
                       'subsystem: wifi')
    self.PatchObject(cq_config.CQConfigParser, 'GetOption',
                     return_value='power light')
    result = triage_lib.GetTestSubsystemForChange('foo/build/root', change)
    self.assertEqual(['network', 'audio', 'wifi'], result)

  def testGetSubsystemFromInvalidCommitMessage(self):
    """Test get subsystem from config file when commit message not have it."""
    change = self._patch_factory.MockPatch(
        commit_message='First line\nThird line\n')
    self.PatchObject(cq_config.CQConfigParser, 'GetOption',
                     return_value='power light')
    result = triage_lib.GetTestSubsystemForChange('foo/build/root', change)
    self.assertEqual(['power', 'light'], result)

  def testGetDefaultSubsystem(self):
    """Test if we can get default subsystem when subsystem is not specified."""
    change = self._patch_factory.MockPatch(
        commit_message='First line\nThird line\n')
    self.PatchObject(cq_config.CQConfigParser, 'GetOption',
                     return_value=None)
    result = triage_lib.GetTestSubsystemForChange('foo/build/root', change)
    self.assertEqual(['default'], result)


class MessageHelper(object):
  """Helper class to create failure messages for tests."""

  @staticmethod
  def GetFailedMessage(failure_messages, stage='Build', internal=False,
                       bot='daisy_spring-paladin'):
    """Returns a build_failure_message.BuildFailureMessage object."""
    return build_failure_message.BuildFailureMessage(
        'Stage %s failed' % stage, failure_messages, internal,
        'failure reason string', bot)

  @staticmethod
  def GetGeneralFailure(stage='Build'):
    return failure_msg_helper.GetStageFailureMessage(stage_name=stage)

  @staticmethod
  def GetTestLabFailure(stage='Build'):
    return failure_msg_helper.GetStageFailureMessage(
        exception_type='TestLabFailure',
        exception_category=constants.EXCEPTION_CATEGORY_LAB,
        stage_name=stage)

  @staticmethod
  def GetInfraFailure(stage='Build'):
    return failure_msg_helper.GetStageFailureMessage(
        exception_type='InfrastructureFailure',
        exception_category=constants.EXCEPTION_CATEGORY_INFRA,
        stage_name=stage)

  @staticmethod
  def GetPackageStageBuildFailure(extra_info=None, stage='Build'):
    return failure_msg_helper.GetPackageBuildFailureMessage(
        extra_info=extra_info,
        stage_name=stage)

# pylint: disable=protected-access
class TestFindSuspects(cros_test_lib.MockTestCase):
  """Tests CalculateSuspects."""

  def setUp(self):
    overlay = 'chromiumos/overlays/chromiumos-overlay'
    self._patch_factory = patch_unittest.MockPatchFactory()
    self.overlay_patch = self._patch_factory.GetPatches(project=overlay)
    chromite = 'chromiumos/chromite'
    self.chromite_patch = self._patch_factory.GetPatches(project=chromite)
    self.power_manager = 'chromiumos/platform2/power_manager'
    self.power_manager_pkg = 'chromeos-base/power_manager'
    self.power_manager_patch = self._patch_factory.GetPatches(
        project=self.power_manager)
    self.kernel = 'chromiumos/third_party/kernel/foo'
    self.kernel_pkg = 'sys-kernel/chromeos-kernel-foo'
    self.kernel_patch = self._patch_factory.GetPatches(project=self.kernel)
    self.secret = 'chromeos/secret'
    self.secret_patch = self._patch_factory.GetPatches(
        project=self.secret, remote=site_config.params.INTERNAL_REMOTE)
    self.PatchObject(cros_patch.GitRepoPatch, 'GetCheckout')
    self.PatchObject(cros_patch.GitRepoPatch, 'GetDiffStatus')
    self.PatchObject(gerrit, 'GetGerritPatchInfoWithPatchQueries',
                     side_effect=lambda x: x)
    self.changes = [self.overlay_patch, self.chromite_patch,
                    self.power_manager_patch, self.kernel_patch,
                    self.secret_patch]

  @staticmethod
  def _GetBuildFailure(pkg):
    """Create a PackageBuildFailure for the specified |pkg|.

    Args:
      pkg: Package that failed to build.
    """
    extra_info_dict = {'shortname': './build_image',
                       'failed_packages': [pkg]}
    extra_info = json.dumps(extra_info_dict)
    return MessageHelper.GetPackageStageBuildFailure(extra_info=extra_info)

  def _AssertSuspects(self, patches, suspects, pkgs=(), exceptions=(),
                      internal=False, infra_fail=False, lab_fail=False,
                      sanity=True):
    """Run _FindSuspects and verify its output.

    Args:
      patches: List of patches to look at.
      suspects: Expected list of suspects returned by _FindSuspects.
      pkgs: List of packages that failed with exceptions in the build.
      exceptions: List of other failure messages (instances of
        failure_message_lib.StageFailureMessage) that occurred during the build.
      internal: Whether the failures occurred on an internal bot.
      infra_fail: Whether the build failed due to infrastructure issues.
      lab_fail: Whether the build failed due to lab infrastructure issues.
      sanity: The sanity checker builder passed and the tree was open when
              the build started.
    """
    all_exceptions = list(exceptions) + [self._GetBuildFailure(x) for x in pkgs]
    message = MessageHelper.GetFailedMessage(all_exceptions, internal=internal)
    results = triage_lib.CalculateSuspects.FindSuspects(
        patches, [message], lab_fail=lab_fail, infra_fail=infra_fail,
        sanity=sanity)
    self.assertItemsEqual(suspects, results.keys())

  def testFailSameProject(self):
    """Patches to the package that failed should be marked as failing."""
    suspects = [self.kernel_patch]
    patches = suspects + [self.power_manager_patch, self.secret_patch]
    with self.PatchObject(portage_util, 'FindWorkonProjects',
                          return_value=self.kernel):
      self._AssertSuspects(patches, suspects, [self.kernel_pkg])
      self._AssertSuspects(patches, suspects, [self.kernel_pkg], sanity=False)

  def testFailSameProjectPlusOverlay(self):
    """Patches to the overlay should be marked as failing."""
    suspects = [self.overlay_patch, self.kernel_patch]
    patches = suspects + [self.power_manager_patch, self.secret_patch]
    with self.PatchObject(portage_util, 'FindWorkonProjects',
                          return_value=self.kernel):
      self._AssertSuspects(patches, suspects, [self.kernel_pkg])
      self._AssertSuspects(patches, [self.kernel_patch], [self.kernel_pkg],
                           sanity=False)

  def testFailUnknownPackage(self):
    """If no patches changed the package, all patches should fail."""
    changes = [self.overlay_patch, self.power_manager_patch, self.secret_patch]
    self._AssertSuspects(changes, changes, [self.kernel_pkg])
    self._AssertSuspects(changes, [], [self.kernel_pkg], sanity=False)

  def testFailUnknownException(self):
    """An unknown exception should cause all patches to fail."""
    changes = [self.kernel_patch, self.power_manager_patch, self.secret_patch]
    self._AssertSuspects(changes, changes,
                         exceptions=[MessageHelper.GetGeneralFailure()])
    self._AssertSuspects(changes, [],
                         exceptions=[MessageHelper.GetGeneralFailure()],
                         sanity=False)

  def testFailUnknownInternalException(self):
    """An unknown exception should cause all patches to fail."""
    suspects = [self.kernel_patch, self.power_manager_patch, self.secret_patch]
    self._AssertSuspects(
        suspects, suspects, exceptions=[MessageHelper.GetGeneralFailure()],
        internal=True)
    self._AssertSuspects(
        suspects, [], exceptions=[MessageHelper.GetGeneralFailure()],
        internal=True, sanity=False)

  def testFailUnknownCombo(self):
    """Unknown exceptions should cause all patches to fail.

    Even if there are also build failures that we can explain.
    """
    suspects = [self.kernel_patch, self.power_manager_patch, self.secret_patch]
    with self.PatchObject(portage_util, 'FindWorkonProjects',
                          return_value=self.kernel):
      self._AssertSuspects(suspects, suspects, [self.kernel_pkg],
                           [MessageHelper.GetGeneralFailure()])
      self._AssertSuspects(suspects, [self.kernel_patch], [self.kernel_pkg],
                           [MessageHelper.GetGeneralFailure()], sanity=False)

  def testFailNone(self):
    """If a message is just 'None', it should cause all patches to fail."""
    patches = [self.kernel_patch, self.power_manager_patch, self.secret_patch]
    results = triage_lib.CalculateSuspects.FindSuspects(patches, [None])
    self.assertItemsEqual(results.keys(), patches)

    results = triage_lib.CalculateSuspects.FindSuspects(
        patches, [None], sanity=False)
    self.assertItemsEqual(results.keys(), [])

  def testFailNoExceptions(self):
    """If there are no exceptions, all patches should be failed."""
    suspects = [self.kernel_patch, self.power_manager_patch, self.secret_patch]
    self._AssertSuspects(suspects, suspects)
    self._AssertSuspects(suspects, [], sanity=False)

  def testLabFail(self):
    """If there are only lab failures, no suspect is chosen."""
    suspects = []
    changes = [self.kernel_patch, self.power_manager_patch]
    self._AssertSuspects(changes, suspects, lab_fail=True, infra_fail=True)
    self._AssertSuspects(changes, suspects, lab_fail=True, infra_fail=True,
                         sanity=False)

  def testInfraFail(self):
    """If there are only non-lab infra failures, pick chromite changes."""
    suspects = [self.chromite_patch]
    changes = [self.kernel_patch, self.power_manager_patch] + suspects
    self._AssertSuspects(changes, suspects, lab_fail=False, infra_fail=True)
    self._AssertSuspects(changes, suspects, lab_fail=False, infra_fail=True,
                         sanity=False)

  def testManualBlame(self):
    """If there are changes that were manually blamed, pick those changes."""
    approvals1 = [{'type': 'VRIF', 'value': '-1', 'grantedOn': 1391733002},
                  {'type': 'CRVW', 'value': '2', 'grantedOn': 1391733002},
                  {'type': 'COMR', 'value': '1', 'grantedOn': 1391733002},]
    approvals2 = [{'type': 'VRIF', 'value': '1', 'grantedOn': 1391733002},
                  {'type': 'CRVW', 'value': '-2', 'grantedOn': 1391733002},
                  {'type': 'COMR', 'value': '1', 'grantedOn': 1391733002},]
    suspects = [self._patch_factory.MockPatch(approvals=approvals1),
                self._patch_factory.MockPatch(approvals=approvals2)]
    changes = [self.kernel_patch, self.chromite_patch] + suspects
    self._AssertSuspects(changes, suspects, lab_fail=False, infra_fail=False)
    self._AssertSuspects(changes, suspects, lab_fail=True, infra_fail=False)
    self._AssertSuspects(changes, suspects, lab_fail=True, infra_fail=True)
    self._AssertSuspects(changes, suspects, lab_fail=False, infra_fail=True)
    self._AssertSuspects(changes, suspects, lab_fail=False, infra_fail=False,
                         sanity=False)
    self._AssertSuspects(changes, suspects, lab_fail=True, infra_fail=False,
                         sanity=False)
    self._AssertSuspects(changes, suspects, lab_fail=True, infra_fail=True,
                         sanity=False)
    self._AssertSuspects(changes, suspects, lab_fail=False, infra_fail=True,
                         sanity=False)

  def _GetMessages(self, lab_fail=0, infra_fail=0, other_fail=0):
    """Returns a list ofbuild_failure_message.BuildFailureMessage objects."""
    messages = []
    messages.extend(
        [MessageHelper.GetFailedMessage([MessageHelper.GetTestLabFailure()])
         for _ in range(lab_fail)])
    messages.extend(
        [MessageHelper.GetFailedMessage([MessageHelper.GetInfraFailure()])
         for _ in range(infra_fail)])
    messages.extend(
        [MessageHelper.GetFailedMessage([MessageHelper.GetGeneralFailure()])
         for _ in range(other_fail)])

    return messages

  def testMatchesExceptionCategories(self):
    """Test MatchesExceptionCategories."""
    messages = self._GetMessages(lab_fail=1, infra_fail=1)
    messages.append(None)
    self.assertFalse(
        triage_lib.CalculateSuspects._MatchesExceptionCategories(
            messages, [constants.EXCEPTION_CATEGORY_LAB]))
    self.assertFalse(
        triage_lib.CalculateSuspects._MatchesExceptionCategories(
            messages, [constants.EXCEPTION_CATEGORY_LAB], strict=False))
    self.assertTrue(
        triage_lib.CalculateSuspects._MatchesExceptionCategories(
            messages,
            {constants.EXCEPTION_CATEGORY_INFRA,
             constants.EXCEPTION_CATEGORY_LAB},
            strict=False))

  def testMatchesExceptionCategoriesWithEmptyMessages(self):
    """Test MatchesExceptionCategoriesWithEmptyMessages."""
    messages = self._GetMessages()
    self.assertFalse(
        triage_lib.CalculateSuspects._MatchesExceptionCategories(
            messages, {constants.EXCEPTION_CATEGORY_LAB}))

  def testOnlyLabFailures(self):
    """Tests the OnlyLabFailures function."""
    messages = self._GetMessages(lab_fail=2)
    no_stat = []
    self.assertTrue(
        triage_lib.CalculateSuspects.OnlyLabFailures(messages, no_stat))

    no_stat = ['foo', 'bar']
    # Some builders did not start. This is not a lab failure.
    self.assertFalse(
        triage_lib.CalculateSuspects.OnlyLabFailures(messages, no_stat))

    messages = self._GetMessages(lab_fail=1, infra_fail=1)
    no_stat = []
    # Non-lab infrastructure failures are present.
    self.assertFalse(
        triage_lib.CalculateSuspects.OnlyLabFailures(messages, no_stat))

  def testOnlyInfraFailures(self):
    """Tests the OnlyInfraFailures function."""
    messages = self._GetMessages(infra_fail=2)
    no_stat = []
    self.assertTrue(
        triage_lib.CalculateSuspects.OnlyInfraFailures(messages, no_stat))

    messages = self._GetMessages(lab_fail=2)
    no_stat = []
    # Lab failures are infrastructure failures.
    self.assertTrue(
        triage_lib.CalculateSuspects.OnlyInfraFailures(messages, no_stat))

    messages = self._GetMessages(lab_fail=1, infra_fail=1)
    no_stat = []
    # Lab failures are infrastructure failures.
    self.assertTrue(
        triage_lib.CalculateSuspects.OnlyInfraFailures(messages, no_stat))

    messages = self._GetMessages(other_fail=1, infra_fail=1)
    no_stat = []
    self.assertFalse(
        triage_lib.CalculateSuspects.OnlyInfraFailures(messages, no_stat))

    no_stat = ['orange']
    messages = []
    # 'Builders failed to report statuses' belong to infrastructure failures.
    self.assertTrue(
        triage_lib.CalculateSuspects.OnlyInfraFailures(messages, no_stat))

  def testFindSuspectsForFailuresWithMessages(self):
    """Test FindSuspectsForFailures with not None messages."""
    build_root = mock.Mock()
    failed_hwtests = mock.Mock()
    messages = []
    for _ in range(0, 3):
      m = mock.Mock()
      m.FindSuspectedChanges.return_value = triage_lib.SuspectChanges({
          self.changes[0]: constants.SUSPECT_REASON_UNKNOWN})
      messages.append(m)

    suspects = triage_lib.CalculateSuspects.FindSuspectsForFailures(
        self.changes, messages, build_root, failed_hwtests, False)
    self.assertItemsEqual(suspects.keys(), self.changes[0:1])

    suspects = triage_lib.CalculateSuspects.FindSuspectsForFailures(
        self.changes, messages, build_root, failed_hwtests, True)
    self.assertItemsEqual(suspects.keys(), self.changes[0:1])

    for index in range(0, 3):
      messages[index].FindSuspectedChanges.called_once_with(
          self.changes, build_root, failed_hwtests, True)
      messages[index].FindSuspectedChanges.called_once_with(
          self.changes, build_root, failed_hwtests, False)

  def testFindSuspectsForFailuresWithNoneMessage(self):
    """Test FindSuspectsForFailuresWith None message."""
    build_root = mock.Mock()
    failed_hwtests = mock.Mock()
    messages = [None]

    suspects = triage_lib.CalculateSuspects.FindSuspectsForFailures(
        self.changes, messages, build_root, failed_hwtests, False)
    self.assertItemsEqual(suspects.keys(), set())

    suspects = triage_lib.CalculateSuspects.FindSuspectsForFailures(
        self.changes, messages, build_root, failed_hwtests, True)
    self.assertItemsEqual(suspects.keys(), self.changes)


class TestGetFullyVerifiedChanges(cros_test_lib.MockTestCase):
  """Tests GetFullyVerifiedChanges() and related functions."""

  def setUp(self):
    self.build_root = '/foo/build/root'
    self._patch_factory = patch_unittest.MockPatchFactory()
    self.changes = self._patch_factory.GetPatches(how_many=5)

  def testChangesNoAllTested(self):
    """Tests that those changes are fully verified."""
    no_stat = failing = messages = []
    inflight = ['foo-paladin']
    changes_by_config = {'foo-paladin': []}
    subsys_by_config = None

    verified_results = triage_lib.CalculateSuspects.GetFullyVerifiedChanges(
        self.changes, changes_by_config, subsys_by_config, {}, failing,
        inflight, no_stat, messages, self.build_root)
    verified_changes = set(verified_results.keys())
    self.assertEquals(verified_changes, set(self.changes))

  def testChangesOnNotCompletedBuilds(self):
    """Test changes on not completed builds."""
    failing = messages = []
    inflight = ['foo-paladin']
    no_stat = ['puppy-paladin']
    changes_by_config = {'foo-paladin': set(self.changes[:2]),
                         'bar-paladin': set(self.changes),
                         'puppy-paladin': set(self.changes[-2:])}
    subsys_by_config = None

    verified_results = triage_lib.CalculateSuspects.GetFullyVerifiedChanges(
        self.changes, changes_by_config, subsys_by_config, {}, failing,
        inflight, no_stat, messages, self.build_root)
    verified_changes = set(verified_results.keys())
    self.assertEquals(verified_changes, set(self.changes[2:-2]))

  def testChangesOnNotCompletedBuildsWithCQHistory(self):
    """Tests changes on not completed builds with builds passed in history."""
    failing = messages = []
    inflight = ['foo-paladin']
    no_stat = ['puppy-paladin']
    changes_by_config = {'foo-paladin': set(self.changes[:2]),
                         'bar-paladin': set(self.changes),
                         'puppy-paladin': set(self.changes[-2:])}
    subsys_by_config = None
    passed_slave_by_change = {
        self.changes[1]: {'foo-paladin'},
        self.changes[3]: {'puppy-paladin'}
    }

    verified_results = triage_lib.CalculateSuspects.GetFullyVerifiedChanges(
        self.changes, changes_by_config, subsys_by_config,
        passed_slave_by_change, failing, inflight, no_stat, messages,
        self.build_root)
    verified_changes = set(verified_results.keys())
    self.assertEquals(verified_changes, set(self.changes[1:-1]))

  def testChangesNotVerifiedOnFailures(self):
    """Tests that changes are not verified if failures cannot be ignored."""
    messages = no_stat = inflight = []
    failing = ['cub-paladin']
    changes_by_config = {'bar-paladin': set(self.changes),
                         'cub-paladin': set(self.changes[:2])}
    subsys_by_config = None

    self.PatchObject(
        triage_lib.CalculateSuspects, 'CanIgnoreFailures',
        return_value=(False, None))
    verified_results = triage_lib.CalculateSuspects.GetFullyVerifiedChanges(
        self.changes, changes_by_config, subsys_by_config, {}, failing,
        inflight, no_stat, messages, self.build_root)
    verified_changes = set(verified_results.keys())
    self.assertEquals(verified_changes, set(self.changes[2:]))

  def testChangesNotVerifiedOnFailuresWithCQHistory(self):
    """Tests on not ignorable faiulres with CQ history."""
    messages = no_stat = inflight = []
    failing = ['cub-paladin']
    changes_by_config = {'bar-paladin': set(self.changes),
                         'cub-paladin': set(self.changes[:2])}
    subsys_by_config = None
    passed_slave_by_change = {
        self.changes[1]: {'cub-paladin'},
    }

    self.PatchObject(
        triage_lib.CalculateSuspects, 'CanIgnoreFailures',
        return_value=(False, None))
    verified_results = triage_lib.CalculateSuspects.GetFullyVerifiedChanges(
        self.changes, changes_by_config, subsys_by_config,
        passed_slave_by_change, failing, inflight, no_stat, messages,
        self.build_root)
    verified_changes = set(verified_results.keys())
    self.assertEquals(verified_changes, set(self.changes[1:]))

  def testChangesVerifiedWhenFailuresCanBeIgnored(self):
    """Tests that changes are verified if failures can be ignored."""
    messages = no_stat = inflight = []
    failing = ['cub-paladin']
    changes_by_config = {'bar-paladin': set(self.changes),
                         'cub-paladin': set(self.changes[:2])}
    subsys_by_config = None

    self.PatchObject(
        triage_lib.CalculateSuspects, 'CanIgnoreFailures',
        return_value=(True, constants.STRATEGY_CQ_PARTIAL_IGNORED_STAGES))
    verified_results = triage_lib.CalculateSuspects.GetFullyVerifiedChanges(
        self.changes, changes_by_config, subsys_by_config, {}, failing,
        inflight, no_stat, messages, self.build_root)
    verified_changes = set(verified_results.keys())
    self.assertEquals(verified_changes, set(self.changes))

  def testCanIgnoreFailures(self):
    """Tests CanIgnoreFailures()."""
    # pylint: disable=protected-access
    change = self.changes[0]
    messages = [
        MessageHelper.GetFailedMessage(
            [MessageHelper.GetGeneralFailure(stage='HWTest')], stage='HWTest'),
        MessageHelper.GetFailedMessage(
            [MessageHelper.GetGeneralFailure(stage='VMTest')], stage='VMTest')]
    subsys_by_config = None
    self.PatchObject(cq_config.CQConfigParser, 'GetCommonConfigFileForChange')
    m = self.PatchObject(cq_config.CQConfigParser, 'GetStagesToIgnore')

    m.return_value = ('HWTest',)
    self.assertEqual(triage_lib.CalculateSuspects.CanIgnoreFailures(
        messages, change, self.build_root, subsys_by_config), (False, None))

    m.return_value = ('HWTest', 'VMTest', 'Foo')
    self.assertEqual(triage_lib.CalculateSuspects.CanIgnoreFailures(
        messages, change, self.build_root, subsys_by_config),
                     (True, constants.STRATEGY_CQ_PARTIAL_IGNORED_STAGES))

    m.return_value = None
    self.assertEqual(triage_lib.CalculateSuspects.CanIgnoreFailures(
        messages, change, self.build_root, subsys_by_config), (False, None))

  def testCanIgnoreFailuresWithSubsystemLogic(self):
    """Tests CanIgnoreFailures with subsystem logic."""
    # pylint: disable=protected-access
    change = self.changes[0]
    messages = [
        MessageHelper.GetFailedMessage(
            [MessageHelper.GetGeneralFailure(stage='HWTest')], stage='HWTest',
            bot='foo-paladin'),
        MessageHelper.GetFailedMessage(
            [MessageHelper.GetGeneralFailure(stage='VMTest')], stage='VMTest',
            bot='foo-paladin'),
        MessageHelper.GetFailedMessage(
            [MessageHelper.GetGeneralFailure(stage='HWTest')], stage='HWTest',
            bot='cub-paladin')]
    self.PatchObject(cq_config.CQConfigParser, 'GetCommonConfigFileForChange')
    m = self.PatchObject(cq_config.CQConfigParser, 'GetStagesToIgnore')
    m.return_value = ('VMTest', )
    cl_subsys = self.PatchObject(triage_lib, 'GetTestSubsystemForChange')
    cl_subsys.return_value = ['A']

    # Test not all configs failed at HWTest run the subsystem logic.
    subsys_by_config = {'foo-paladin': {'pass_subsystems': ['A', 'B'],
                                        'fail_subsystems': ['C']},
                        'cub-paladin': {}}
    self.assertEqual(triage_lib.CalculateSuspects.CanIgnoreFailures(
        messages, change, self.build_root, subsys_by_config), (False, None))
    # Test all configs failed at HWTest run the subsystem logic.
    subsys_by_config = {'foo-paladin': {'pass_subsystems': ['A', 'B'],
                                        'fail_subsystems': ['C']},
                        'cub-paladin': {'pass_subsystems': ['A'],
                                        'fail_subsystems': ['B']}}
    result = triage_lib.CalculateSuspects.CanIgnoreFailures(
        messages, change, self.build_root, subsys_by_config)
    self.assertEqual(result, (True, constants.STRATEGY_CQ_PARTIAL_SUBSYSTEM))

    subsys_by_config = {'foo-paladin': {'pass_subsystems': ['A', 'B'],
                                        'fail_subsystems': ['C']},
                        'cub-paladin': {'pass_subsystems': ['D'],
                                        'fail_subsystems': ['A']}}
    self.assertEqual(triage_lib.CalculateSuspects.CanIgnoreFailures(
        messages, change, self.build_root, subsys_by_config), (False, None))

  # pylint: disable=protected-access
  def testGetVerifiedReason(self):
    """Test _GetVerifiedReason."""
    verified_reasons = set()
    self.assertEqual(
        triage_lib.CalculateSuspects._GetVerifiedReason(verified_reasons), None)

    verified_reasons = {constants.STRATEGY_CQ_PARTIAL_BUILDS_PASSED,
                        constants.STRATEGY_CQ_PARTIAL_SUBSYSTEM,
                        constants.STRATEGY_CQ_PARTIAL_IGNORED_STAGES}
    self.assertEqual(
        triage_lib.CalculateSuspects._GetVerifiedReason(verified_reasons),
        constants.STRATEGY_CQ_PARTIAL_IGNORED_STAGES)


class SuspectChangesTest(cros_test_lib.MockTestCase):
  """Tests for SuspectChanges."""

  def setUp(self):
    self._patch_factory = patch_unittest.MockPatchFactory()
    self.patches = self._patch_factory.GetPatches(how_many=3)

  def _CreateSuspectChanges(self, suspect_dict=None):
    return triage_lib.SuspectChanges(suspect_dict)

  def testSetitem(self):
    """Test __setitem__."""
    suspects = self._CreateSuspectChanges()

    suspects[self.patches[0]] = constants.SUSPECT_REASON_BAD_CHANGE
    suspects[self.patches[1]] = constants.SUSPECT_REASON_UNKNOWN

    self.assertEqual(len(suspects), 2)
    self.assertEqual(suspects[self.patches[0]],
                     constants.SUSPECT_REASON_BAD_CHANGE)
    self.assertEqual(suspects[self.patches[1]],
                     constants.SUSPECT_REASON_UNKNOWN)

    suspects[self.patches[0]] = constants.SUSPECT_REASON_UNKNOWN
    suspects[self.patches[1]] = constants.SUSPECT_REASON_BUILD_FAIL

    self.assertEqual(len(suspects), 2)
    self.assertEqual(suspects[self.patches[0]],
                     constants.SUSPECT_REASON_BAD_CHANGE)
    self.assertEqual(suspects[self.patches[1]],
                     constants.SUSPECT_REASON_BUILD_FAIL)

    suspects[self.patches[2]] = constants.SUSPECT_REASON_OVERLAY_CHANGE
    self.assertEqual(len(suspects), 3)
    self.assertEqual(suspects[self.patches[2]],
                     constants.SUSPECT_REASON_OVERLAY_CHANGE)

  def testSetdefault(self):
    """Test setdefault."""
    suspects = self._CreateSuspectChanges()
    self.assertRaises(Exception, suspects.setdefault, self.patches[0], None)

    suspects.setdefault(self.patches[0], constants.SUSPECT_REASON_BAD_CHANGE)
    suspects.setdefault(self.patches[1], constants.SUSPECT_REASON_UNKNOWN)

    self.assertEqual(len(suspects), 2)
    self.assertEqual(suspects[self.patches[0]],
                     constants.SUSPECT_REASON_BAD_CHANGE)
    self.assertEqual(suspects[self.patches[1]],
                     constants.SUSPECT_REASON_UNKNOWN)

    suspects.setdefault(self.patches[0], constants.SUSPECT_REASON_UNKNOWN)
    suspects.setdefault(self.patches[1], constants.SUSPECT_REASON_BUILD_FAIL)

    self.assertEqual(len(suspects), 2)
    self.assertEqual(suspects[self.patches[0]],
                     constants.SUSPECT_REASON_BAD_CHANGE)
    self.assertEqual(suspects[self.patches[1]],
                     constants.SUSPECT_REASON_BUILD_FAIL)

    suspects.setdefault(self.patches[2],
                        constants.SUSPECT_REASON_OVERLAY_CHANGE)
    self.assertEqual(len(suspects), 3)
    self.assertEqual(suspects[self.patches[2]],
                     constants.SUSPECT_REASON_OVERLAY_CHANGE)

  def testUpdate(self):
    """Test update."""
    suspects = self._CreateSuspectChanges({
        self.patches[0]: constants.SUSPECT_REASON_BAD_CHANGE,
        self.patches[1]: constants.SUSPECT_REASON_UNKNOWN})
    suspects.update({
        self.patches[0]: constants.SUSPECT_REASON_UNKNOWN,
        self.patches[1]: constants.SUSPECT_REASON_BUILD_FAIL})
    expected = self._CreateSuspectChanges({
        self.patches[0]: constants.SUSPECT_REASON_BAD_CHANGE,
        self.patches[1]: constants.SUSPECT_REASON_BUILD_FAIL})

    self.assertEqual(suspects, expected)
