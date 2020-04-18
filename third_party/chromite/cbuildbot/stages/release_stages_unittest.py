# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for build stages."""

from __future__ import print_function

import mock

from chromite.cbuildbot import cbuildbot_unittest
from chromite.cbuildbot.stages import generic_stages_unittest
from chromite.cbuildbot.stages import release_stages
from chromite.lib import config_lib
from chromite.lib import failures_lib
from chromite.lib import parallel
from chromite.lib import results_lib
from chromite.lib import timeout_util

from chromite.cbuildbot.stages.generic_stages_unittest import patch

from chromite.lib.paygen import gspaths
from chromite.lib.paygen import paygen_build_lib


# pylint: disable=protected-access


class SigningStageTest(generic_stages_unittest.AbstractStageTestCase,
                       cbuildbot_unittest.SimpleBuilderTestCase):
  """Test the SigningStage."""

  RELEASE_TAG = '0.0.1'

  SIGNER_RESULT = """
    { "status": { "status": "passed" }, "board": "link",
    "keyset": "link-mp-v4", "type": "recovery", "channel": "stable" }
    """

  INSNS_URLS_PER_CHANNEL = {
      'chan1': ['chan1_uri1', 'chan1_uri2'],
      'chan2': ['chan2_uri1'],
  }

  def setUp(self):
    self._Prepare()

  def ConstructStage(self):
    return release_stages.SigningStage(self._run, self._current_board)

  def testWaitForPushImageSuccess(self):
    """Test waiting for input from PushImage."""
    stage = self.ConstructStage()
    stage.board_runattrs.SetParallel(
        'instruction_urls_per_channel', self.INSNS_URLS_PER_CHANNEL)

    self.assertEqual(stage.WaitUntilReady(), True)
    self.assertEqual(stage.instruction_urls_per_channel,
                     self.INSNS_URLS_PER_CHANNEL)

  def testWaitForPushImageError(self):
    """Test WaitForPushImageError with an error output from pushimage."""
    stage = self.ConstructStage()
    stage.board_runattrs.SetParallel(
        'instruction_urls_per_channel', None)

    self.assertEqual(stage.WaitUntilReady(), False)

  def testWaitForSigningResultsSuccess(self):
    """Test that _WaitForSigningResults works when signing works."""
    results = ['chan1_uri1.json', 'chan1_uri2.json', 'chan2_uri1.json']

    with patch(release_stages.gs, 'GSContext') as mock_gs_ctx_init:
      mock_gs_ctx = mock_gs_ctx_init.return_value
      mock_gs_ctx.Cat.return_value = self.SIGNER_RESULT
      notifier = mock.Mock()

      stage = self.ConstructStage()
      stage._WaitForSigningResults(self.INSNS_URLS_PER_CHANNEL, notifier)

      self.assertEqual(notifier.mock_calls,
                       [mock.call('chan1'),
                        mock.call('chan2')])

      for result in results:
        mock_gs_ctx.Cat.assert_any_call(result)

  def testWaitForSigningResultsSuccessNoNotifier(self):
    """Test that _WaitForSigningResults works when signing works."""
    results = ['chan1_uri1.json', 'chan1_uri2.json', 'chan2_uri1.json']

    with patch(release_stages.gs, 'GSContext') as mock_gs_ctx_init:
      mock_gs_ctx = mock_gs_ctx_init.return_value
      mock_gs_ctx.Cat.return_value = self.SIGNER_RESULT

      stage = self.ConstructStage()
      stage._WaitForSigningResults(self.INSNS_URLS_PER_CHANNEL, None)

      for result in results:
        mock_gs_ctx.Cat.assert_any_call(result)

  def testWaitForSigningResultsSuccessNothingSigned(self):
    """Test _WaitForSigningResults when there are no signed images."""
    with patch(release_stages.gs, 'GSContext') as mock_gs_ctx_init:
      mock_gs_ctx = mock_gs_ctx_init.return_value
      mock_gs_ctx.Cat.return_value = self.SIGNER_RESULT
      notifier = mock.Mock()

      stage = self.ConstructStage()
      stage._WaitForSigningResults({}, notifier)

      self.assertEqual(notifier.mock_calls, [])
      self.assertEqual(mock_gs_ctx.Cat.mock_calls, [])

  def testWaitForSigningResultsFailure(self):
    """Test _WaitForSigningResults when the signers report an error."""
    with patch(release_stages.gs, 'GSContext') as mock_gs_ctx_init:
      mock_gs_ctx = mock_gs_ctx_init.return_value
      mock_gs_ctx.Cat.return_value = """
          { "status": { "status": "failed" }, "board": "link",
            "keyset": "link-mp-v4", "type": "recovery", "channel": "stable" }
          """
      notifier = mock.Mock()

      stage = self.ConstructStage()

      self.assertRaisesStringifyable(
          release_stages.SignerFailure,
          stage._WaitForSigningResults,
          {'chan1': ['chan1_uri1']}, notifier)

      # Ensure we didn't notify anyone of success.
      self.assertEqual(notifier.mock_calls, [])
      self.assertEqual(mock_gs_ctx.Cat.mock_calls,
                       [mock.call('chan1_uri1.json')])

  def testWaitForSigningResultsTimeout(self):
    """Test that _WaitForSigningResults reports timeouts correctly."""
    with patch(release_stages.timeout_util, 'WaitForSuccess') as mock_wait:
      mock_wait.side_effect = timeout_util.TimeoutError
      notifier = mock.Mock()

      stage = self.ConstructStage()

      self.assertRaises(release_stages.SignerResultsTimeout,
                        stage._WaitForSigningResults,
                        {'chan1': ['chan1_uri1']}, notifier)

      self.assertEqual(notifier.mock_calls, [])

  def testCheckForResultsSuccess(self):
    """Test that _CheckForResults works when signing works."""
    with patch(release_stages.gs, 'GSContext') as mock_gs_ctx_init:
      mock_gs_ctx = mock_gs_ctx_init.return_value
      mock_gs_ctx.Cat.return_value = self.SIGNER_RESULT
      notifier = mock.Mock()

      stage = self.ConstructStage()
      self.assertTrue(
          stage._CheckForResults(mock_gs_ctx,
                                 self.INSNS_URLS_PER_CHANNEL,
                                 notifier))
      self.assertEqual(notifier.mock_calls,
                       [mock.call('chan1'), mock.call('chan2')])

  def testCheckForResultsSuccessNoChannels(self):
    """Test that _CheckForResults works when there is nothing to check for."""
    with patch(release_stages.gs, 'GSContext') as mock_gs_ctx_init:
      mock_gs_ctx = mock_gs_ctx_init.return_value
      notifier = mock.Mock()

      stage = self.ConstructStage()

      # Ensure we find that we are ready if there are no channels to look for.
      self.assertTrue(stage._CheckForResults(mock_gs_ctx, {}, notifier))

      # Ensure we didn't contact GS while checking for no channels.
      self.assertFalse(mock_gs_ctx.Cat.called)
      self.assertEqual(notifier.mock_calls, [])

  def testCheckForResultsPartialComplete(self):
    """Verify _CheckForResults handles partial signing results."""
    def catChan2Success(url):
      if url.startswith('chan2'):
        return self.SIGNER_RESULT
      else:
        raise release_stages.gs.GSNoSuchKey()

    with patch(release_stages.gs, 'GSContext') as mock_gs_ctx_init:
      mock_gs_ctx = mock_gs_ctx_init.return_value
      mock_gs_ctx.Cat.side_effect = catChan2Success
      notifier = mock.Mock()

      stage = self.ConstructStage()
      self.assertFalse(
          stage._CheckForResults(mock_gs_ctx,
                                 self.INSNS_URLS_PER_CHANNEL,
                                 notifier))
      self.assertEqual(stage.signing_results, {
          'chan1': {},
          'chan2': {
              'chan2_uri1.json': {
                  'board': 'link',
                  'channel': 'stable',
                  'keyset': 'link-mp-v4',
                  'status': {'status': 'passed'},
                  'type': 'recovery'
              }
          }
      })
      self.assertEqual(notifier.mock_calls, [mock.call('chan2')])

  def testCheckForResultsUnexpectedJson(self):
    """Verify _CheckForResults handles unexpected Json values."""
    with patch(release_stages.gs, 'GSContext') as mock_gs_ctx_init:
      mock_gs_ctx = mock_gs_ctx_init.return_value
      mock_gs_ctx.Cat.return_value = '{}'
      notifier = mock.Mock()

      stage = self.ConstructStage()
      self.assertFalse(
          stage._CheckForResults(mock_gs_ctx,
                                 self.INSNS_URLS_PER_CHANNEL,
                                 notifier))
      self.assertEqual(stage.signing_results, {
          'chan1': {}, 'chan2': {}
      })
      self.assertEqual(notifier.mock_calls, [])

  def testCheckForResultsMalformedJson(self):
    """Verify _CheckForResults handles unexpected Json values."""
    with patch(release_stages.gs, 'GSContext') as mock_gs_ctx_init:
      mock_gs_ctx = mock_gs_ctx_init.return_value
      mock_gs_ctx.Cat.return_value = '{'
      notifier = mock.Mock()

      stage = self.ConstructStage()
      self.assertFalse(
          stage._CheckForResults(mock_gs_ctx,
                                 self.INSNS_URLS_PER_CHANNEL,
                                 notifier))
      self.assertEqual(stage.signing_results, {
          'chan1': {}, 'chan2': {}
      })
      self.assertEqual(notifier.mock_calls, [])

  def testCheckForResultsNoResult(self):
    """Verify _CheckForResults handles missing signer results."""
    with patch(release_stages.gs, 'GSContext') as mock_gs_ctx_init:
      mock_gs_ctx = mock_gs_ctx_init.return_value
      mock_gs_ctx.Cat.side_effect = release_stages.gs.GSNoSuchKey
      notifier = mock.Mock()

      stage = self.ConstructStage()
      self.assertFalse(
          stage._CheckForResults(mock_gs_ctx,
                                 self.INSNS_URLS_PER_CHANNEL,
                                 notifier))
      self.assertEqual(stage.signing_results, {
          'chan1': {}, 'chan2': {}
      })
      self.assertEqual(notifier.mock_calls, [])

  def testCheckForResultsFailed(self):
    """Verify _CheckForResults handles missing signer results."""
    with patch(release_stages.gs, 'GSContext') as mock_gs_ctx_init:
      mock_gs_ctx = mock_gs_ctx_init.return_value
      mock_gs_ctx.Cat.side_effect = release_stages.gs.GSNoSuchKey
      notifier = mock.Mock()

      stage = self.ConstructStage()
      self.assertFalse(
          stage._CheckForResults(mock_gs_ctx,
                                 self.INSNS_URLS_PER_CHANNEL,
                                 notifier))
      self.assertEqual(stage.signing_results, {
          'chan1': {}, 'chan2': {}
      })
      self.assertEqual(notifier.mock_calls, [])

  def testPerformStageSuccess(self):
    """Test that SigningStage works when signing works."""
    stage = self.ConstructStage()
    stage.instruction_urls_per_channel = self.INSNS_URLS_PER_CHANNEL
    self.PatchObject(stage, '_WaitForSigningResults')

    stage.PerformStage()

    # Verify that we send the right notifications.
    result = stage.board_runattrs.GetParallel('signed_images_ready', timeout=0)
    self.assertEqual(result, ['chan1', 'chan2'])


class PaygenStageTest(generic_stages_unittest.AbstractStageTestCase,
                      cbuildbot_unittest.SimpleBuilderTestCase):
  """Test the PaygenStage Stage."""

  # We use a variant board to make sure the '_' is translated to '-'.
  BOT_ID = 'x86-alex_he-release'
  RELEASE_TAG = '0.0.1'

  def setUp(self):
    self._Prepare()

    # This method fetches a file from GS, mock it out.
    self.validateMock = self.PatchObject(
        paygen_build_lib, 'ValidateBoardConfig')

    instanceMock = mock.MagicMock()
    self.paygenBuildMock = self.PatchObject(paygen_build_lib, 'PaygenBuild',
                                            return_value=instanceMock)

    instanceMock.CreatePayloads.side_effect = iter([(
        'foo-suite-name',
        'foo-archive-board',
        'foo-archive-build',
    )])

  # pylint: disable=arguments-differ
  def ConstructStage(self, channels=None):
    return release_stages.PaygenStage(self._run, self._current_board,
                                      channels=channels)

  def testWaitUntilReadSigning(self):
    """Test that PaygenStage works when signing works."""
    stage = self.ConstructStage()
    stage.board_runattrs.SetParallel('signed_images_ready',
                                     ['stable', 'beta'])

    self.assertEqual(stage.WaitUntilReady(), True)
    self.assertEqual(stage.channels, ['stable', 'beta'])

  def testWaitUntilReadSigningFailure(self):
    """Test that PaygenStage works when signing works."""
    stage = self.ConstructStage()
    stage.board_runattrs.SetParallel('signed_images_ready', None)

    self.assertEqual(stage.WaitUntilReady(), False)

  def testWaitUntilReadSigningEmpty(self):
    """Test that PaygenStage works when signing works."""
    stage = self.ConstructStage()
    stage.board_runattrs.SetParallel('signed_images_ready', [])

    self.assertEqual(stage.WaitUntilReady(), True)
    self.assertEqual(stage.channels, [])

  def testPerformStageSuccess(self):
    """Test that PaygenStage works when signing works."""
    with patch(release_stages.parallel, 'BackgroundTaskRunner') as background:
      queue = background().__enter__()

      stage = self.ConstructStage(channels=['stable', 'beta'])

      stage.PerformStage()

      # Verify that we validate with the board name in release name space.
      self.assertEqual(
          self.validateMock.call_args_list,
          [mock.call('x86-alex-he')])

      # Verify that we queue up work
      self.assertEqual(
          queue.put.call_args_list,
          [mock.call(('stable', 'x86-alex-he', '0.0.1',
                      False, False, False, True)),
           mock.call(('beta', 'x86-alex-he', '0.0.1',
                      False, False, False, True))])

  def testPerformStageNoChannels(self):
    """Test that PaygenStage works when signing works."""
    with patch(release_stages.parallel, 'BackgroundTaskRunner') as background:
      queue = background().__enter__()

      stage = self.ConstructStage(channels=[])

      stage.PerformStage()

      # Verify that we queue up work
      self.assertEqual(queue.put.call_args_list, [])

  def testPerformStageTrybot(self):
    """Test the PerformStage alternate behavior for trybot runs."""
    with patch(release_stages.parallel, 'BackgroundTaskRunner') as background:
      queue = background().__enter__()

      # The stage is constructed differently for trybots, so don't use
      # ConstructStage.
      stage = self.ConstructStage(channels=['foo', 'bar'])
      stage.PerformStage()

      # Notice that we didn't put anything in _wait_for_channel_signing, but
      # still got results right away.
      self.assertEqual(
          queue.put.call_args_list,
          [mock.call(('foo', 'x86-alex-he', '0.0.1',
                      False, False, False, True)),
           mock.call(('bar', 'x86-alex-he', '0.0.1',
                      False, False, False, True))])

  def testPerformStageUnknownBoard(self):
    """Test that PaygenStage exits when an unknown board is specified."""
    self._current_board = 'unknown-board-name'

    # Setup a board validation failure.
    badBoardException = paygen_build_lib.BoardNotConfigured(self._current_board)
    self.validateMock.side_effect = badBoardException

    stage = self.ConstructStage()

    with self.assertRaises(release_stages.PaygenNoPaygenConfigForBoard):
      stage.PerformStage()

  def testRunPaygenInProcess(self):
    """Test that _RunPaygenInProcess works in the simple case."""
    # Have to patch and verify that the PaygenTestStage is created.
    stage = self.ConstructStage()

    with patch(paygen_build_lib, 'ScheduleAutotestTests') as sched_tests:
      # Call the method under test.
      stage._RunPaygenInProcess('foo', 'foo-board', 'foo-version',
                                True, False, False, skip_duts_check=False)
      # Ensure that PaygenTestStage is created and schedules the test suite
      # with the correct arguments.
      sched_tests.assert_called_once_with(
          'foo-suite-name', 'foo-archive-board', None, 'foo-archive-build',
          False, True, job_keyvals=mock.ANY)

    # Ensure arguments are properly converted and passed along.
    self.paygenBuildMock.assert_called_with(
        gspaths.Build(
            version='foo-version',
            board='foo-board',
            channel='foo-channel'),
        work_dir=mock.ANY,
        site_config=stage._run.site_config,
        dry_run=True,
        skip_delta_payloads=False,
        skip_duts_check=False)

  def testRunPaygenInProcessComplex(self):
    """Test that _RunPaygenInProcess with arguments that are more unusual."""
    # Call the method under test.
    # Use release tools channel naming, and a board name including a variant.
    stage = self.ConstructStage()
    stage._RunPaygenInProcess('foo-channel', 'foo-board-variant',
                              'foo-version', True, True, True,
                              skip_duts_check=False)

    # Ensure arguments are properly converted and passed along.
    self.paygenBuildMock.assert_called_with(
        gspaths.Build(version='foo-version',
                      board='foo-board-variant',
                      channel='foo-channel'),
        dry_run=True,
        work_dir=mock.ANY,
        site_config=stage._run.site_config,
        skip_delta_payloads=True,
        skip_duts_check=False)

  def testRunPaygenInProcessWithUnifiedBuild(self):
    self._run.config.models = [config_lib.ModelTestConfig('model1', 'model1'),
                               config_lib.ModelTestConfig(
                                   'model2', 'board', ['au'])]

    # Have to patch and verify that the PaygenTestStage is created.
    stage = self.ConstructStage()

    with patch(paygen_build_lib, 'ScheduleAutotestTests') as sched_tests:
      # Call the method under test.
      stage._RunPaygenInProcess('foo', 'foo-board', 'foo-version',
                                True, False, False, skip_duts_check=False)
      # Ensure that the first model from the unified build was selected
      # as the platform to be tested
      sched_tests.assert_called_once_with(
          'foo-suite-name', 'board', 'model2', 'foo-archive-build',
          False, True, job_keyvals=mock.ANY)

  def testRunPaygenInParallelWithUnifiedBuild(self):
    self._run.config.models = [
        config_lib.ModelTestConfig('model1', 'model1', ['au']),
        config_lib.ModelTestConfig('model2', 'model1', ['au'])]

    # Have to patch and verify that the PaygenTestStage is created.
    stage = self.ConstructStage()

    with patch(parallel, 'RunParallelSteps') as parallel_tests:
      stage._RunPaygenInProcess('foo', 'foo-board', 'foo-version',
                                True, False, False, skip_duts_check=False)
      parallel_tests.assert_called_once_with([mock.ANY, mock.ANY])


class PaygenBuildStageTest(generic_stages_unittest.AbstractStageTestCase,
                           cbuildbot_unittest.SimpleBuilderTestCase):
  """Test the PaygenBuild stage."""

  # We use a variant board to make sure the '_' is translated to '-'.
  BOT_ID = 'x86-alex_he-release'
  RELEASE_TAG = '0.0.1'

  def setUp(self):
    self._Prepare()

  # pylint: disable=arguments-differ
  def ConstructStage(self):
    return release_stages.PaygenBuildStage(
        self._run,
        board=self._current_board,
        channel='foochan',
        version='foo-version',
        debug=True,
        skip_testing=False,
        skip_delta_payloads=False,
        skip_duts_check=False)

  def testStageName(self):
    """See if the stage name is correctly formed."""
    stage = self.ConstructStage()
    self.assertEqual(stage.name, 'PaygenBuildFoochan')

class PaygenTestStageTest(generic_stages_unittest.AbstractStageTestCase,
                          cbuildbot_unittest.SimpleBuilderTestCase):
  """Test the PaygenTestStage stage."""

  # We use a variant board to make sure the '_' is translated to '-'.
  BOT_ID = 'x86-alex_he-release'
  RELEASE_TAG = '0.0.1'

  def setUp(self):
    self._Prepare()

  # pylint: disable=arguments-differ
  def ConstructStage(self):
    return release_stages.PaygenTestStage(
        builder_run=self._run,
        suite_name='foo-test-suite',
        board=self._current_board,
        model=self._current_board,
        lab_board_name=self._current_board,
        # The PaygenBuild stage will add the '-channel' suffix to the channel
        # when converting to release tools naming.
        channel='foochan-channel',
        build='foo-version',
        skip_duts_check=False,
        debug=True)

  def testStageName(self):
    """See if the stage name is correctly formed."""
    stage = self.ConstructStage()
    self.assertEqual(stage.name, 'PaygenTestFoochan [x86-alex_he]')

  def testPerformStageTestLabFail(self):
    """Test that exception from RunHWTestSuite are properly handled."""
    with patch(paygen_build_lib, 'ScheduleAutotestTests') as sched_tests:
      sched_tests.side_effect = failures_lib.TestLabFailure

      stage = self.ConstructStage()

      with patch(stage, '_HandleExceptionAsWarning') as warning_handler:
        warning_handler.return_value = (results_lib.Results.FORGIVEN,
                                        'description',
                                        0)

        stage.Run()

        # This proves the exception was turned into a warning.
        self.assertTrue(warning_handler.called)
