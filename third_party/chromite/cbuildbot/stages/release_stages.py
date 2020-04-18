# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the various stages that a builder runs."""

from __future__ import print_function

import json
import os

from chromite.cbuildbot import commands
from chromite.lib import failures_lib
from chromite.lib import config_lib
from chromite.cbuildbot.stages import artifact_stages
from chromite.cbuildbot.stages import generic_stages
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import timeout_util
from chromite.lib.paygen import gspaths
from chromite.lib.paygen import paygen_build_lib


class InvalidTestConditionException(Exception):
  """Raised when pre-conditions for a test aren't met."""


class SignerTestStage(artifact_stages.ArchivingStage):
  """Run signer related tests."""

  option_name = 'tests'
  config_name = 'signer_tests'

  # If the signer tests take longer than 30 minutes, abort. They usually take
  # five minutes to run.
  SIGNER_TEST_TIMEOUT = 30 * 60

  def PerformStage(self):
    if not self.archive_stage.WaitForRecoveryImage():
      raise InvalidTestConditionException('Missing recovery image.')
    with timeout_util.Timeout(self.SIGNER_TEST_TIMEOUT):
      commands.RunSignerTests(self._build_root, self._current_board)


class SignerResultsTimeout(failures_lib.StepFailure):
  """The signer did not produce any results inside the expected time."""


class SignerFailure(failures_lib.StepFailure):
  """The signer returned an error result."""


class MissingInstructionException(failures_lib.StepFailure):
  """We didn't receive the list of signing instructions PushImage uploaded."""


class MalformedResultsException(failures_lib.StepFailure):
  """The Signer results aren't formatted as we expect."""


class PaygenSigningRequirementsError(failures_lib.StepFailure):
  """Paygen stage can't run if signing failed."""


class PaygenCrostoolsNotAvailableError(failures_lib.StepFailure):
  """Paygen stage can't run if signing failed."""


class PaygenNoPaygenConfigForBoard(failures_lib.StepFailure):
  """Paygen can't run with a release.conf config for the board."""


class SigningStage(generic_stages.BoardSpecificBuilderStage):
  """Stage that waits for image signing.

  This stage waits for values from ArchiveStage (push_image), then waits until
  the signing servers sign the uploaded images.
  """
  option_name = 'paygen'
  config_name = 'paygen'

  # Poll for new results every 30 seconds.
  SIGNING_PERIOD = 30

  # Timeout for the signing process. 2 hours in seconds.
  SIGNING_TIMEOUT = 2 * 60 * 60

  def __init__(self, builder_run, board, **kwargs):
    """Init that accepts the channels argument, if present.

    Args:
      builder_run: See builder_run on ArchivingStage.
      board: See board on ArchivingStage.
    """
    super(SigningStage, self).__init__(builder_run, board, **kwargs)

    # Used to remember partial results between retries.
    self.signing_results = {}

    # Filled in via WaitUntilReady, Of the form:
    #   {'channel': ['gs://instruction_uri1', 'gs://signer_instruction_uri2']}
    self.instruction_urls_per_channel = None

  def _HandleStageException(self, exc_info):
    """Override and don't set status to FAIL but FORGIVEN instead."""
    exc_type, _exc_value, _exc_tb = exc_info

    # Notify stages blocked on us if we error out.
    self.board_runattrs.SetParallel('signed_images_ready', None)

    # Warn so people look at ArchiveStage for the real error.
    if issubclass(exc_type, MissingInstructionException):
      return self._HandleExceptionAsWarning(exc_info)

    return super(SigningStage, self)._HandleStageException(exc_info)

  def _JsonFromUrl(self, gs_ctx, url):
    """Fetch a GS Url, and parse it as Json.

    Args:
      gs_ctx: GS Context.
      url: Url to fetch and parse.

    Returns:
      None if the Url doesn't exist.
      Parsed Json structure if it did.

    Raises:
      MalformedResultsException if it failed to parse.
    """
    try:
      signer_txt = gs_ctx.Cat(url)
    except gs.GSNoSuchKey:
      return None

    try:
      return json.loads(signer_txt)
    except ValueError:
      # We should never see malformed Json, even for intermediate statuses.
      raise MalformedResultsException(signer_txt)

  def _SigningStatusFromJson(self, signer_json):
    """Extract a signing status from a signer result Json DOM.

    Args:
      signer_json: The parsed json status from a signer operation.

    Returns:
      string with a simple status: SIGNER_STATUS_PASSED, SIGNER_STATUS_FAILED,
      etc, or '' if the json doesn't contain a status.
    """
    return (signer_json or {}).get('status', {}).get('status', '')

  def _CheckForResults(self, gs_ctx, instruction_urls_per_channel,
                       channel_notifier=None):
    """timeout_util.WaitForSuccess func to check a list of signer results.

    Args:
      gs_ctx: Google Storage Context.
      instruction_urls_per_channel: Urls of the signer result files
                                    we're expecting.
      channel_notifier: Method to call when a channel is ready or None.

    Returns:
      Number of results not yet collected.
    """
    COMPLETED_STATUS = (constants.SIGNER_STATUS_PASSED,
                        constants.SIGNER_STATUS_FAILED)

    # Assume we are done, then try to prove otherwise.
    results_completed = True

    for channel in instruction_urls_per_channel.keys():
      self.signing_results.setdefault(channel, {})

      if (len(self.signing_results[channel]) ==
          len(instruction_urls_per_channel[channel])):
        continue

      for url in instruction_urls_per_channel[channel]:
        # Convert from instructions URL to instructions result URL.
        url += '.json'

        # We already have a result for this URL.
        if url in self.signing_results[channel]:
          continue

        try:
          signer_json = self._JsonFromUrl(gs_ctx, url)
        except MalformedResultsException as e:
          logging.warning('Received malformed json: %s', e)
          continue

        if self._SigningStatusFromJson(signer_json) in COMPLETED_STATUS:
          # If we find a completed result, remember it.
          self.signing_results[channel][url] = signer_json

      # If we don't have full results for this channel, we aren't done
      # waiting.
      if (len(self.signing_results[channel]) !=
          len(instruction_urls_per_channel[channel])):
        results_completed = False
        continue

      # If we reach here, the channel has just been completed for the first
      # time.

      # If all results passed the channel was successfully signed.
      channel_success = True
      for signer_result in self.signing_results[channel].values():
        if (self._SigningStatusFromJson(signer_result) !=
            constants.SIGNER_STATUS_PASSED):
          channel_success = False

      # If we successfully completed the channel, inform someone.
      if channel_success and channel_notifier:
        channel_notifier(channel)

    return results_completed

  def _WaitForSigningResults(self,
                             instruction_urls_per_channel,
                             channel_notifier=None):
    """Do the work of waiting for signer results and logging them.

    Args:
      instruction_urls_per_channel: push_image data (see _WaitForPushImage).
      channel_notifier: Method to call with channel name when ready or None.

    Raises:
      ValueError: If the signer result isn't valid json.
      RunCommandError: If we are unable to download signer results.
    """
    gs_ctx = gs.GSContext(dry_run=self._run.debug)

    try:
      logging.info('Waiting for signer results.')
      timeout_util.WaitForReturnTrue(
          self._CheckForResults,
          func_args=(gs_ctx, instruction_urls_per_channel, channel_notifier),
          timeout=self.SIGNING_TIMEOUT, period=self.SIGNING_PERIOD)
    except timeout_util.TimeoutError:
      msg = 'Image signing timed out.'
      logging.error(msg)
      logging.PrintBuildbotStepText(msg)
      raise SignerResultsTimeout(msg)

    # Log all signer results, then handle any signing failures.
    failures = []
    for url_results in self.signing_results.values():
      for url, signer_result in url_results.iteritems():
        result_description = os.path.basename(url)
        logging.PrintBuildbotStepText(result_description)
        logging.info('Received results for: %s', result_description)
        logging.info(json.dumps(signer_result, indent=4))

        status = self._SigningStatusFromJson(signer_result)
        if status != constants.SIGNER_STATUS_PASSED:
          failures.append(result_description)
          logging.error('Signing failed for: %s', result_description)

    if failures:
      logging.error('Failure summary:')
      for failure in failures:
        logging.error('  %s', failure)
      raise SignerFailure(', '.join([str(f) for f in failures]))

  def WaitUntilReady(self):
    """Block until push_image data is ready.

    Sets self.instruction_urls_per_channel as described in __init__.

    Returns:
      Boolean that tells if we can run this stage.
    """
    # This call will NEVER time out.
    self.instruction_urls_per_channel = self.board_runattrs.GetParallel(
        'instruction_urls_per_channel', timeout=None)

    # A value of None signals an error in PushImage.
    if self.instruction_urls_per_channel is None:
      # ArchiveStage PushImage failed. Signing won't run at all.
      self.board_runattrs.SetParallel('signed_images_ready', None)
      return False

    return True

  def PerformStage(self):
    """Do the work of generating our release payloads."""
    # Convert to release tools naming for boards.
    board = self._current_board.replace('_', '-')
    version = self._run.attrs.release_tag

    logging.info("Waiting for image signing for: %s, %s", board, version)
    logging.info("GS errors are a normal part of the polling for results.")
    self._WaitForSigningResults(self.instruction_urls_per_channel)

    # Notify stages blocked on us that images are for the given channel list.
    channels = self.instruction_urls_per_channel.keys()
    self.board_runattrs.SetParallel('signed_images_ready', channels)


class PaygenStage(generic_stages.BoardSpecificBuilderStage):
  """Stage that generates release payloads.

  If this stage is created with a 'channels' argument, it can run
  independently. Otherwise, it's dependent on values queued up by
  the SigningStage.
  """
  option_name = 'paygen'
  config_name = 'paygen'

  def __init__(self, builder_run, board, channels=None, **kwargs):
    """Init that accepts the channels argument, if present.

    Args:
      builder_run: See builder_run on ArchivingStage.
      board: See board on ArchivingStage.
      channels: Explicit list of channels to generate payloads for.
                If empty, will instead wait on values from push_image.
                Channels is normally None in release builds, and normally set
                for trybot 'payloads' builds.
    """
    super(PaygenStage, self).__init__(builder_run, board, **kwargs)
    self.channels = channels

  def _HandleStageException(self, exc_info):
    """Override and don't set status to FAIL but FORGIVEN instead."""
    exc_type, _exc_value, _exc_tb = exc_info

    # If Paygen fails to find anything needed in release.conf, treat it
    # as a warning. This is common during new board bring up.
    if issubclass(exc_type, PaygenNoPaygenConfigForBoard):
      return self._HandleExceptionAsWarning(exc_info)

    # If the SigningStage failed, we warn that we didn't run, but don't fail
    # outright. Let SigningStage decide if this should kill the build.
    if issubclass(exc_type, SignerFailure):
      return self._HandleExceptionAsWarning(exc_info)
    return super(PaygenStage, self)._HandleStageException(exc_info)

  def WaitUntilReady(self):
    """Block until signed images are ready.

    Returns:
      Boolean that tells if we can run this stage.
    """
    # If we did got an explicit channel list, there is no need to wait.
    if self.channels is None:
      # Wait for channels from signing stage.
      self.channels = self.board_runattrs.GetParallel(
          'signed_images_ready', timeout=None)

      # If the signing stage errored out for any reason.
      if self.channels is None:
        # SigningStage failed. Payloads can't be generated.
        return False

    return True

  def PerformStage(self):
    """Do the work of generating our release payloads."""
    # Convert to release tools naming for boards.
    board = self._current_board.replace('_', '-')
    version = self._run.attrs.release_tag

    assert version, "We can't generate payloads without a release_tag."
    logging.info("Generating payloads for: %s, %s", board, version)

    # Test to see if the current board has a Paygen configuration. We do
    # this here, not in the sub-process so we don't have to pass back a
    # failure reason.
    try:
      paygen_build_lib.ValidateBoardConfig(board)
    except  paygen_build_lib.BoardNotConfigured:
      raise PaygenNoPaygenConfigForBoard(
          'Golden Eye (%s) has no entry for board %s. Get a TPM to fix.' %
          (paygen_build_lib.PAYGEN_URI, board))

    # Default to False, set to True if it's a canary type build
    skip_duts_check = False
    if config_lib.IsCanaryType(self._run.config.build_type):
      skip_duts_check = True

    with parallel.BackgroundTaskRunner(self._RunPaygenInProcess) as per_channel:
      logging.info("Using channels: %s", self.channels)

      # If we have an explicit list of channels, use it.
      for channel in self.channels:
        per_channel.put((channel, board, version, self._run.debug,
                         self._run.config.paygen_skip_testing,
                         self._run.config.paygen_skip_delta_payloads,
                         skip_duts_check))

  def _RunPaygenInProcess(self, channel, board, version, debug,
                          disable_tests, skip_delta_payloads,
                          skip_duts_check):
    """Runs the PaygenBuild and PaygenTest stage (if applicable)"""
    PaygenBuildStage(self._run, board, channel, version, debug, disable_tests,
                     skip_delta_payloads, skip_duts_check).Run()

class PaygenBuildStage(generic_stages.BoardSpecificBuilderStage):
  """Stage that generates payloads and uploads to Google Storage."""
  def __init__(self, builder_run, board, channel, version, debug,
               skip_testing, skip_delta_payloads, skip_duts_check, **kwargs):
    """Init that accepts the channels argument, if present.

    Args:
      builder_run: See builder_run on ArchiveStage
      board: Board of payloads to generate ('x86-mario', 'x86-alex-he', etc)
      channel: Channel of payloads to generate ('stable', 'beta', etc)
      version: Version of payloads to generate.
      debug: Flag telling if this is a real run, or a test run.
      skip_testing: Do not generate test artifacts or run payload tests.
      skip_delta_payloads: Skip generating delta payloads.
      skip_duts_check: Do not check minimum available DUTs before tests.
    """
    super(PaygenBuildStage, self).__init__(
        builder_run, board, suffix=channel.capitalize(), **kwargs)
    self._run = builder_run
    self.board = board
    self.channel = channel
    self.version = version
    self.debug = debug
    self.skip_testing = skip_testing
    self.skip_delta_payloads = skip_delta_payloads
    self.skip_duts_check = skip_duts_check

  def PerformStage(self):
    """Invoke payload generation. If testing is enabled, schedule tests.

    This method is intended to be safe to invoke inside a process.
    """
    # Convert to release tools naming for channels.
    if not self.channel.endswith('-channel'):
      self.channel += '-channel'

    with osutils.TempDir(sudo_rm=True) as tempdir:
      # Create the definition of the build to generate payloads for.
      build = gspaths.Build(channel=self.channel,
                            board=self.board,
                            version=self.version)

      try:
        # Generate the payloads.
        self._PrintLoudly('Starting %s, %s, %s' % (self.channel, self.version,
                                                   self.board))
        paygen = paygen_build_lib.PaygenBuild(
            build,
            work_dir=tempdir,
            site_config=self._run.site_config,
            dry_run=self.debug,
            skip_delta_payloads=self.skip_delta_payloads,
            skip_duts_check=self.skip_duts_check)

        testdata = paygen.CreatePayloads()

        # Now, schedule the payload tests if desired.
        if not self.skip_testing:
          suite_name, archive_board, archive_build = testdata

          # For unified builds, only test against the specified models.
          if self._run.config.models:
            models = []
            for model in self._run.config.models:
              # 'au' is a test suite generated in ge_build_config.json
              if model.test_suites and 'au' in model.test_suites:
                models.append(model)

            if len(models) > 1:
              stages = [PaygenTestStage(
                  self._run,
                  suite_name,
                  archive_board,
                  model.name,
                  model.lab_board_name,
                  self.channel,
                  archive_build,
                  self.skip_duts_check,
                  self.debug) for model in models]
              steps = [stage.Run for stage in stages]
              parallel.RunParallelSteps(steps)
            elif len(models) == 1:
              PaygenTestStage(
                  self._run,
                  suite_name,
                  archive_board,
                  models[0].name,
                  models[0].lab_board_name,
                  self.channel,
                  archive_build,
                  self.skip_duts_check,
                  self.debug).Run()
          else:
            PaygenTestStage(
                self._run,
                suite_name,
                archive_board,
                None,
                archive_board,
                self.channel,
                archive_build,
                self.skip_duts_check,
                self.debug).Run()



      except (paygen_build_lib.BuildLocked) as e:
        # These errors are normal if it's possible that another builder is
        # processing the same build. (perhaps by a trybot generating payloads on
        # request).
        logging.info('PaygenBuild for %s skipped because: %s', self.channel, e)


class PaygenTestStage(generic_stages.BoardSpecificBuilderStage):
  """Stage that schedules the payload tests."""
  def __init__(
      self,
      builder_run,
      suite_name,
      board,
      model,
      lab_board_name,
      channel,
      build,
      skip_duts_check,
      debug,
      **kwargs):
    """Init that accepts the channels argument, if present.

    Args:
      builder_run: See builder_run on ArchiveStage
      suite_name: See builder_run on ArchiveStage
      board: Board overlay name.
      model: Model that will be tested. ('reef', 'pyro', etc)
      lab_board_name: The actual board label tested against in Autotest
      channel: Channel of payloads to generate ('stable', 'beta', etc)
      build: Version of payloads to generate.
      skip_duts_check: Do not check minimum available DUTs before tests.
      debug: Boolean indicating if this is a test run or a real run.
    """
    self.suite_name = suite_name
    self.board = board
    self.model = model
    self.lab_board_name = lab_board_name

    self.build = build
    self.skip_duts_check = skip_duts_check
    self.debug = debug
    # We don't need the '-channel'suffix.
    if channel.endswith('-channel'):
      channel = channel[0:-len('-channel')]
    suffix = channel.capitalize()
    if model:
      suffix += ' [%s]' % model

    super(PaygenTestStage, self).__init__(
        builder_run, board, suffix=suffix, **kwargs)

  def PerformStage(self):
    """Schedule the tests to run."""
    # Schedule the tests to run and wait for the results.
    paygen_build_lib.ScheduleAutotestTests(self.suite_name,
                                           self.lab_board_name,
                                           self.model,
                                           self.build,
                                           self.skip_duts_check,
                                           self.debug,
                                           job_keyvals=self.GetJobKeyvals())

  def _HandleStageException(self, exc_info):
    """Override and don't set status to FAIL but FORGIVEN instead."""
    exc_type, exc_value, _exc_tb = exc_info

    # If the exception is a TestLabFailure that means we couldn't schedule the
    # test. We don't fail the build for that. We do the CompoundFailure dance,
    # because that's how we'll get failures from background processes returned
    # to us.
    if (issubclass(exc_type, failures_lib.TestLabFailure) or
        (issubclass(exc_type, failures_lib.CompoundFailure) and
         exc_value.MatchesFailureType(failures_lib.TestLabFailure))):
      return self._HandleExceptionAsWarning(exc_info)

    return super(PaygenTestStage, self)._HandleStageException(exc_info)
