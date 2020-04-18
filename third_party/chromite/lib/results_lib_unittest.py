# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the stage results."""

from __future__ import print_function

import mock
import os
import signal
import StringIO
import time

from chromite.lib import constants
from chromite.lib import config_lib_unittest
from chromite.lib import failures_lib
from chromite.lib import results_lib
from chromite.cbuildbot import cbuildbot_run
from chromite.cbuildbot.builders import simple_builders
from chromite.cbuildbot.stages import generic_stages
from chromite.cbuildbot.stages import sync_stages
from chromite.lib import cidb
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import cros_test_lib
from chromite.lib import fake_cidb
from chromite.lib import parallel


class PassStage(generic_stages.BuilderStage):
  """PassStage always works"""


class Pass2Stage(generic_stages.BuilderStage):
  """Pass2Stage always works"""


class FailStage(generic_stages.BuilderStage):
  """FailStage always throws an exception"""

  FAIL_EXCEPTION = failures_lib.StepFailure("Fail stage needs to fail.")

  def PerformStage(self):
    """Throw the exception to make us fail."""
    raise self.FAIL_EXCEPTION


class SkipStage(generic_stages.BuilderStage):
  """SkipStage is skipped."""
  config_name = 'signer_tests'


class SneakyFailStage(generic_stages.BuilderStage):
  """SneakyFailStage exits with an error."""

  def PerformStage(self):
    """Exit without reporting back."""
    # pylint: disable=protected-access
    os._exit(1)


class SuicideStage(generic_stages.BuilderStage):
  """SuicideStage kills itself with kill -9."""

  def PerformStage(self):
    """Exit without reporting back."""
    os.kill(os.getpid(), signal.SIGKILL)


class SetAttrStage(generic_stages.BuilderStage):
  """Stage that sets requested run attribute to a value."""

  DEFAULT_ATTR = 'unittest_value'
  VALUE = 'HereTakeThis'

  def __init__(self, builder_run, delay=2, attr=DEFAULT_ATTR, *args, **kwargs):
    super(SetAttrStage, self).__init__(builder_run, *args, **kwargs)
    self.delay = delay
    self.attr = attr

  def PerformStage(self):
    """Wait self.delay seconds then set requested run attribute."""
    time.sleep(self.delay)
    self._run.attrs.SetParallel(self.attr, self.VALUE)

  def QueueableException(self):
    return cbuildbot_run.ParallelAttributeError(self.attr)


class GetAttrStage(generic_stages.BuilderStage):
  """Stage that accesses requested run attribute and confirms value."""

  DEFAULT_ATTR = 'unittest_value'

  def __init__(self, builder_run, tester=None, timeout=5, attr=DEFAULT_ATTR,
               *args, **kwargs):
    super(GetAttrStage, self).__init__(builder_run, *args, **kwargs)
    self.tester = tester
    self.timeout = timeout
    self.attr = attr

  def PerformStage(self):
    """Wait for attrs.test value to show up."""
    assert not self._run.attrs.HasParallel(self.attr)
    value = self._run.attrs.GetParallel(self.attr, self.timeout)
    if self.tester:
      self.tester(value)

  def QueueableException(self):
    return cbuildbot_run.ParallelAttributeError(self.attr)

  def TimeoutException(self):
    return cbuildbot_run.AttrTimeoutError(self.attr)


class BuildStagesResultsTest(cros_test_lib.TestCase):
  """Tests for stage results and reporting."""

  def setUp(self):
    # Always stub RunCommmand out as we use it in every method.
    self._bot_id = 'amd64-generic-paladin'
    site_config = config_lib_unittest.MockSiteConfig()
    build_config = site_config[self._bot_id]
    self.build_root = '/fake_root'
    # This test compares log output from the stages, so turn on buildbot
    # logging.
    logging.EnableBuildbotMarkers()

    self.db = fake_cidb.FakeCIDBConnection()
    cidb.CIDBConnectionFactory.SetupMockCidb(self.db)

    # Create a class to hold
    class Options(object):
      """Dummy class to hold option values."""

    options = Options()
    options.archive_base = 'gs://dontcare'
    options.buildroot = self.build_root
    options.debug = False
    options.prebuilts = False
    options.clobber = False
    options.nosdk = False
    options.remote_trybot = False
    options.latest_toolchain = False
    options.buildnumber = 1234
    options.android_rev = None
    options.chrome_rev = None
    options.branch = 'dontcare'
    options.chrome_root = False

    self._manager = parallel.Manager()
    self._manager.__enter__()

    self._run = cbuildbot_run.BuilderRun(
        options, site_config, build_config, self._manager)

    results_lib.Results.Clear()

  def tearDown(self):
    # Mimic exiting with statement for self._manager.
    if hasattr(self, '_manager') and self._manager is not None:
      self._manager.__exit__(None, None, None)

    cidb.CIDBConnectionFactory.SetupMockCidb()

  def _runStages(self):
    """Run a couple of stages so we can capture the results"""
    # Run two pass stages, and one fail stage.
    PassStage(self._run).Run()
    Pass2Stage(self._run).Run()
    self.assertRaises(
        failures_lib.StepFailure,
        FailStage(self._run).Run)

  def _verifyRunResults(self, expectedResults, max_time=2.0):
    actualResults = results_lib.Results.Get()

    # Break out the asserts to be per item to make debugging easier
    self.assertEqual(len(expectedResults), len(actualResults))
    for i in xrange(len(expectedResults)):
      entry = actualResults[i]
      xname, xresult = expectedResults[i]

      if entry.result not in results_lib.Results.NON_FAILURE_TYPES:
        self.assertTrue(isinstance(entry.result, BaseException))
        if isinstance(entry.result, failures_lib.StepFailure):
          self.assertEqual(str(entry.result), entry.description)

      self.assertTrue(entry.time >= 0 and entry.time < max_time)
      self.assertEqual(xname, entry.name)
      self.assertEqual(type(xresult), type(entry.result))
      self.assertEqual(repr(xresult), repr(entry.result))

  def _PassString(self):
    record = results_lib.Result('Pass', results_lib.Results.SUCCESS, 'None',
                                'Pass', '', '0')
    return results_lib.Results.SPLIT_TOKEN.join(record) + '\n'

  def testRunStages(self):
    """Run some stages and verify the captured results"""

    self.assertEqual(results_lib.Results.Get(), [])

    self._runStages()

    # Verify that the results are what we expect.
    expectedResults = [
        ('Pass', results_lib.Results.SUCCESS),
        ('Pass2', results_lib.Results.SUCCESS),
        ('Fail', FailStage.FAIL_EXCEPTION),
    ]
    self._verifyRunResults(expectedResults)

  def testSuccessTest(self):
    """Run some stages and verify the captured results"""

    results_lib.Results.Record('Pass', results_lib.Results.SUCCESS)

    self.assertTrue(results_lib.Results.BuildSucceededSoFar())

    results_lib.Results.Record('Fail', FailStage.FAIL_EXCEPTION, time=1)

    self.assertFalse(results_lib.Results.BuildSucceededSoFar())

    results_lib.Results.Record('Pass2', results_lib.Results.SUCCESS)

    self.assertFalse(results_lib.Results.BuildSucceededSoFar())

  def testSuccessTestWithDB(self):
    """Test BuildSucceededSoFar with DB instance"""
    build_id = 'dummy_build_id'

    results_lib.Results.Record('stage1', results_lib.Results.SUCCESS)
    results_lib.Results.Record('stage2', results_lib.Results.SKIPPED)

    self.db.InsertBuildStage(build_id, 'stage1',
                             status=constants.BUILDER_STATUS_PASSED)
    self.db.InsertBuildStage(build_id, 'stage2',
                             status=constants.BUILDER_STATUS_SKIPPED)
    self.db.InsertBuildStage(build_id, 'stage3',
                             status=constants.BUILDER_STATUS_FORGIVEN)
    self.db.InsertBuildStage(build_id, 'stage4',
                             status=constants.BUILDER_STATUS_PLANNED)

    self.assertTrue(results_lib.Results.BuildSucceededSoFar(
        self.db, build_id))

    self.db.InsertBuildStage(build_id, 'stage5',
                             status=constants.BUILDER_STATUS_INFLIGHT)

    self.assertTrue(results_lib.Results.BuildSucceededSoFar(
        self.db, build_id, 'stage5'))

    self.db.InsertBuildStage(build_id, 'stage6',
                             status=constants.BUILDER_STATUS_FAILED)

    self.assertFalse(results_lib.Results.BuildSucceededSoFar(
        self.db, build_id, 'stage5'))

  def _TestParallelStages(self, stage_objs):
    builder = simple_builders.SimpleBuilder(self._run)
    error = None
    # pylint: disable=protected-access
    with mock.patch.multiple(parallel._BackgroundTask, PRINT_INTERVAL=0.01):
      try:
        builder._RunParallelStages(stage_objs)
      except parallel.BackgroundFailure as ex:
        error = ex

    return error

  def testParallelStages(self):
    stage_objs = [stage(self._run) for stage in
                  (PassStage, SneakyFailStage, FailStage, SuicideStage,
                   Pass2Stage)]
    error = self._TestParallelStages(stage_objs)
    self.assertTrue(error)
    expectedResults = [
        ('Pass', results_lib.Results.SUCCESS),
        ('Fail', FailStage.FAIL_EXCEPTION),
        ('Pass2', results_lib.Results.SUCCESS),
        ('SneakyFail', error),
        ('Suicide', error),
    ]
    self._verifyRunResults(expectedResults)

  def testParallelStageCommunicationOK(self):
    """Test run attr communication betweeen parallel stages."""
    def assert_test(value):
      self.assertEqual(value, SetAttrStage.VALUE,
                       'Expected value %r to be passed between stages, but'
                       ' got %r.' % (SetAttrStage.VALUE, value))
    stage_objs = [
        SetAttrStage(self._run),
        GetAttrStage(self._run, assert_test, timeout=30),
        GetAttrStage(self._run, assert_test, timeout=30),
    ]
    error = self._TestParallelStages(stage_objs)
    self.assertFalse(error)
    expectedResults = [
        ('SetAttr', results_lib.Results.SUCCESS),
        ('GetAttr', results_lib.Results.SUCCESS),
        ('GetAttr', results_lib.Results.SUCCESS),
    ]
    self._verifyRunResults(expectedResults, max_time=90.0)

    # Make sure run attribute propagated up to the top, too.
    value = self._run.attrs.GetParallel('unittest_value')
    self.assertEqual(SetAttrStage.VALUE, value)

  def testParallelStageCommunicationTimeout(self):
    """Test run attr communication between parallel stages that times out."""
    def assert_test(value):
      self.assertEqual(value, SetAttrStage.VALUE,
                       'Expected value %r to be passed between stages, but'
                       ' got %r.' % (SetAttrStage.VALUE, value))
    stage_objs = [SetAttrStage(self._run, delay=11),
                  GetAttrStage(self._run, assert_test, timeout=1),
                 ]
    error = self._TestParallelStages(stage_objs)
    self.assertTrue(error)
    expectedResults = [
        ('SetAttr', results_lib.Results.SUCCESS),
        ('GetAttr', stage_objs[1].TimeoutException()),
    ]
    self._verifyRunResults(expectedResults, max_time=12.0)

  def testParallelStageCommunicationNotQueueable(self):
    """Test setting non-queueable run attr in parallel stage."""
    stage_objs = [SetAttrStage(self._run, attr='release_tag'),
                  GetAttrStage(self._run, timeout=2),
                 ]
    error = self._TestParallelStages(stage_objs)
    self.assertTrue(error)
    expectedResults = [
        ('SetAttr', stage_objs[0].QueueableException()),
        ('GetAttr', stage_objs[1].TimeoutException()),
    ]
    self._verifyRunResults(expectedResults, max_time=12.0)

  def testStagesReportSuccess(self):
    """Tests Stage reporting."""

    sync_stages.ManifestVersionedSyncStage.manifest_manager = None

    # Store off a known set of results and generate a report
    results_lib.Results.Record('Sync', results_lib.Results.SUCCESS, time=1)
    results_lib.Results.Record('Build', results_lib.Results.SUCCESS, time=2)
    results_lib.Results.Record('Test', FailStage.FAIL_EXCEPTION, time=3)
    results_lib.Results.Record('SignerTests', results_lib.Results.SKIPPED)
    result = cros_build_lib.CommandResult(cmd=['/bin/false', '/nosuchdir'],
                                          returncode=2)
    results_lib.Results.Record(
        'Archive',
        cros_build_lib.RunCommandError(
            'Command "/bin/false /nosuchdir" failed.\n',
            result), time=4)

    results = StringIO.StringIO()

    results_lib.Results.Report(results)

    expectedResults = (
        "************************************************************\n"
        "** Stage Results\n"
        "************************************************************\n"
        "** PASS Sync (0:00:01)\n"
        "************************************************************\n"
        "** PASS Build (0:00:02)\n"
        "************************************************************\n"
        "** FAIL Test (0:00:03) with StepFailure\n"
        "************************************************************\n"
        "** FAIL Archive (0:00:04) in /bin/false\n"
        "************************************************************\n"
    )

    expectedLines = expectedResults.split('\n')
    actualLines = results.getvalue().split('\n')

    # Break out the asserts to be per item to make debugging easier
    for i in xrange(min(len(actualLines), len(expectedLines))):
      self.assertEqual(expectedLines[i], actualLines[i])
    self.assertEqual(len(expectedLines), len(actualLines))

  def testStagesReportError(self):
    """Tests Stage reporting with exceptions."""

    sync_stages.ManifestVersionedSyncStage.manifest_manager = None

    # Store off a known set of results and generate a report
    results_lib.Results.Record('Sync', results_lib.Results.SUCCESS, time=1)
    results_lib.Results.Record('Build', results_lib.Results.SUCCESS, time=2)
    results_lib.Results.Record('Test', FailStage.FAIL_EXCEPTION,
                               'failException Msg\nLine 2', time=3)
    result = cros_build_lib.CommandResult(cmd=['/bin/false', '/nosuchdir'],
                                          returncode=2)
    results_lib.Results.Record(
        'Archive',
        cros_build_lib.RunCommandError(
            'Command "/bin/false /nosuchdir" failed.\n',
            result),
        'FailRunCommand msg', time=4)

    results = StringIO.StringIO()

    results_lib.Results.Report(results)

    expectedResults = (
        "************************************************************\n"
        "** Stage Results\n"
        "************************************************************\n"
        "** PASS Sync (0:00:01)\n"
        "************************************************************\n"
        "** PASS Build (0:00:02)\n"
        "************************************************************\n"
        "** FAIL Test (0:00:03) with StepFailure\n"
        "************************************************************\n"
        "** FAIL Archive (0:00:04) in /bin/false\n"
        "************************************************************\n"
        "\n"
        "Failed in stage Test:\n"
        "\n"
        "failException Msg\n"
        "Line 2\n"
        "\n"
        "Failed in stage Archive:\n"
        "\n"
        "FailRunCommand msg\n"
    )

    expectedLines = expectedResults.split('\n')
    actualLines = results.getvalue().split('\n')

    # Break out the asserts to be per item to make debugging easier
    for i in xrange(min(len(actualLines), len(expectedLines))):
      self.assertEqual(expectedLines[i], actualLines[i])
    self.assertEqual(len(expectedLines), len(actualLines))

  def testStagesReportReleaseTag(self):
    """Tests Release Tag entry in stages report."""

    current_version = "release_tag_string"
    # Store off a known set of results and generate a report
    results_lib.Results.Record('Pass', results_lib.Results.SUCCESS, time=1)

    results = StringIO.StringIO()

    results_lib.Results.Report(results, current_version)

    expectedResults = (
        "************************************************************\n"
        "** RELEASE VERSION: release_tag_string\n"
        "************************************************************\n"
        "** Stage Results\n"
        "************************************************************\n"
        "** PASS Pass (0:00:01)\n"
        "************************************************************\n"
    )

    expectedLines = expectedResults.split('\n')
    actualLines = results.getvalue().split('\n')

    # Break out the asserts to be per item to make debugging easier
    for i in xrange(len(expectedLines)):
      self.assertEqual(expectedLines[i], actualLines[i])
    self.assertEqual(len(expectedLines), len(actualLines))

  def testSaveCompletedStages(self):
    """Tests that we can save out completed stages."""

    # Run this again to make sure we have the expected results stored
    results_lib.Results.Record('Pass', results_lib.Results.SUCCESS)
    results_lib.Results.Record('Fail', FailStage.FAIL_EXCEPTION)
    results_lib.Results.Record('Pass2', results_lib.Results.SUCCESS)

    saveFile = StringIO.StringIO()
    results_lib.Results.SaveCompletedStages(saveFile)
    self.assertEqual(saveFile.getvalue(), self._PassString())

  def testRestoreCompletedStages(self):
    """Tests that we can read in completed stages."""

    results_lib.Results.RestoreCompletedStages(
        StringIO.StringIO(self._PassString()))

    previous = results_lib.Results.GetPrevious()
    self.assertEqual(previous.keys(), ['Pass'])

  def testRunAfterRestore(self):
    """Tests that we skip previously completed stages."""

    # Fake results_lib.Results.RestoreCompletedStages
    results_lib.Results.RestoreCompletedStages(
        StringIO.StringIO(self._PassString()))

    self._runStages()

    # Verify that the results are what we expect.
    expectedResults = [
        ('Pass', results_lib.Results.SUCCESS),
        ('Pass2', results_lib.Results.SUCCESS),
        ('Fail', FailStage.FAIL_EXCEPTION),
    ]
    self._verifyRunResults(expectedResults)

  def testFailedButForgiven(self):
    """Tests that warnings are flagged as such."""
    results_lib.Results.Record('Warn', results_lib.Results.FORGIVEN, time=1)
    results = StringIO.StringIO()
    results_lib.Results.Report(results)
    self.assertTrue('@@@STEP_WARNINGS@@@' in results.getvalue())
