# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import shutil
import sys
import tempfile

_SRC_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..'))

sys.path.append(os.path.join(_SRC_DIR, 'third_party', 'catapult', 'devil'))
from devil.android import device_utils

sys.path.append(os.path.join(_SRC_DIR, 'third_party', 'catapult', 'telemetry',
    'third_party', 'websocket-client'))
import websocket

import chrome_cache
import common_util
import controller
import devtools_monitor
import device_setup
import loading_trace


# Standard filenames in the sandwich runner's output directory.
ERROR_FILENAME = 'error'
TRACE_FILENAME = 'trace.json'
VIDEO_FILENAME = 'video.mp4'
WPR_LOG_FILENAME = 'wpr.log'

# Memory dump category used to get memory metrics.
MEMORY_DUMP_CATEGORY = 'disabled-by-default-memory-infra'

# Devtools timeout of 1 minute to avoid websocket timeout on slow
# network condition.
_DEVTOOLS_TIMEOUT = 60

# Categories to enable or disable for all traces collected. Disabled categories
# are prefixed with '-'.
_TRACING_CATEGORIES = [
  'blink',
  'blink.net',
  'blink.user_timing',
  'devtools.timeline',
  'java',
  'navigation',
  'toplevel',
  'v8',
  '-cc',  # A lot of unnecessary events are enabled by default in "cc".
]

TTFMP_ADDITIONAL_CATEGORIES = [
  'loading',
  'disabled-by-default-blink.debug.layout',
]

def _CleanArtefactsFromPastRuns(output_directories_path):
  """Cleans artifacts generated from past run in the output directory.

  Args:
    output_directories_path: The output directory path where to clean the
        previous traces.
  """
  for dirname in os.listdir(output_directories_path):
    directory_path = os.path.join(output_directories_path, dirname)
    if not os.path.isdir(directory_path):
      continue
    try:
      int(dirname)
    except ValueError:
      continue
    shutil.rmtree(directory_path)


class CacheOperation(object):
  CLEAR, SAVE, PUSH = range(3)


class SandwichRunnerError(Exception):
  pass


class SandwichRunner(object):
  """Sandwich runner.

  This object is meant to be configured first and then run using the Run()
  method.
  """
  _ATTEMPT_COUNT = 3
  _STOP_DELAY_MULTIPLIER = 2
  _ABORT_RUN_TIMEOUT_SECONDS = 30 * 60

  def __init__(self):
    """Configures a sandwich runner out of the box.

    Public members are meant to be configured before calling Run().
    """
    # Cache operation to do before doing the chrome navigation.
    self.cache_operation = CacheOperation.CLEAR

    # The cache archive's path to save to or push from. Is str or None.
    self.cache_archive_path = None

    # List of additional chrome command line flags.
    self.chrome_args = []

    # Controls whether the WPR server should do script injection.
    self.disable_wpr_script_injection = False

    # Number of times to repeat the url.
    self.repeat = 1

    # Network conditions to emulate. None if no emulation.
    self.network_condition = None

    # Network condition emulator. Can be: browser,wpr
    self.network_emulator = 'browser'

    # Output directory where to save the traces, videos, etc. Is str or None.
    self.output_dir = None

    # URL to navigate to.
    self.url = None

    # Configures whether to record speed-index video.
    self.record_video = False

    # Configures whether to record memory dumps.
    self.record_memory_dumps = False

    # Configures whether to record tracing categories needed for TTFMP.
    self.record_first_meaningful_paint = False

    # Path to the WPR archive to load or save. Is str or None.
    self.wpr_archive_path = None

    # Configures whether the WPR archive should be read or generated.
    self.wpr_record = False

    # The android DeviceUtils to run sandwich on or None to run it locally.
    self.android_device = None

    self._chrome_ctl = None
    self._local_cache_directory_path = None

  def _CleanTraceOutputDirectory(self):
    assert self.output_dir
    if not os.path.isdir(self.output_dir):
      try:
        os.makedirs(self.output_dir)
      except OSError:
        logging.error('Cannot create directory for results: %s',
            self.output_dir)
        raise
    else:
      _CleanArtefactsFromPastRuns(self.output_dir)

  def _GetEmulatorNetworkCondition(self, emulator):
    if self.network_emulator == emulator:
      return self.network_condition
    return None

  def _RunNavigation(self, clear_cache, repeat_id=None):
    """Run a page navigation to the given URL.

    Args:
      clear_cache: Whether if the cache should be cleared before navigation.
      repeat_id: Id of the run in the output directory. If it is None, then no
        trace or video will be saved.
    """
    run_path = None
    if repeat_id is not None:
      run_path = os.path.join(self.output_dir, str(repeat_id))
      if not os.path.isdir(run_path):
        os.makedirs(run_path)
    self._chrome_ctl.SetNetworkEmulation(
        self._GetEmulatorNetworkCondition('browser'))
    categories = _TRACING_CATEGORIES
    if self.record_memory_dumps:
      categories += [MEMORY_DUMP_CATEGORY]
    if self.record_first_meaningful_paint:
      categories += TTFMP_ADDITIONAL_CATEGORIES
    stop_delay_multiplier = 0
    if self.wpr_record or self.cache_operation == CacheOperation.SAVE:
      stop_delay_multiplier = self._STOP_DELAY_MULTIPLIER
    # TODO(gabadie): add a way to avoid recording a trace.
    with common_util.TimeoutScope(
        self._ABORT_RUN_TIMEOUT_SECONDS, 'Sandwich run overdue.'):
      with self._chrome_ctl.Open() as connection:
        if clear_cache:
          connection.ClearCache()

        # Binds all parameters of RecordUrlNavigation() to avoid repetition.
        def RecordTrace():
          return loading_trace.LoadingTrace.RecordUrlNavigation(
              url=self.url,
              connection=connection,
              chrome_metadata=self._chrome_ctl.ChromeMetadata(),
              categories=categories,
              timeout_seconds=_DEVTOOLS_TIMEOUT,
              stop_delay_multiplier=stop_delay_multiplier)

        if run_path is not None and self.record_video:
          device = self._chrome_ctl.GetDevice()
          if device is None:
            raise RuntimeError('Can only record video on a remote device.')
          video_recording_path = os.path.join(run_path, VIDEO_FILENAME)
          with device_setup.RemoteSpeedIndexRecorder(device, connection,
                                                     video_recording_path):
            trace = RecordTrace()
        else:
          trace = RecordTrace()
        for event in trace.request_track.GetEvents():
          if event.failed:
            logging.warning(
                'request to %s failed: %s', event.url, event.error_text)
        if not trace.tracing_track.HasLoadingSucceeded():
          raise SandwichRunnerError('Page load has failed.')
    if run_path is not None:
      trace_path = os.path.join(run_path, TRACE_FILENAME)
      trace.ToJsonFile(trace_path)

  def _RunInRetryLoop(self, repeat_id, perform_dry_run_before):
    """Attempts to run monitoring navigation.

    Args:
      repeat_id: Id of the run in the output directory.
      perform_dry_run_before: Whether it should do a dry run attempt before the
        actual monitoring run.

    Returns:
      Whether the device should be rebooted to continue attempting for that
      given |repeat_id|.
    """
    resume_attempt_id = 0
    if perform_dry_run_before:
      resume_attempt_id = 1
    for attempt_id in xrange(resume_attempt_id, self._ATTEMPT_COUNT):
      try:
        if perform_dry_run_before:
          logging.info('Do sandwich dry run attempt %d', attempt_id)
        else:
          logging.info('Do sandwich run attempt %d', attempt_id)
        self._chrome_ctl.ResetBrowserState()
        clear_cache = False
        if self.cache_operation == CacheOperation.CLEAR:
          clear_cache = True
        elif self.cache_operation == CacheOperation.PUSH:
          self._chrome_ctl.PushBrowserCache(self._local_cache_directory_path)
        elif self.cache_operation == CacheOperation.SAVE:
          clear_cache = repeat_id == 0
        self._RunNavigation(clear_cache=clear_cache, repeat_id=repeat_id)
        if not perform_dry_run_before or attempt_id > resume_attempt_id:
          break
      except controller.ChromeControllerError as error:
        request_reboot = False
        is_intermittent = error.IsIntermittent()
        if (self.android_device and
            attempt_id == 0 and
            error.error_type is websocket.WebSocketConnectionClosedException):
          assert not perform_dry_run_before
          # On Android, the first socket connection closure is likely caused by
          # memory pressure on the device and therefore considered intermittent,
          # and therefore request a reboot of the device to the caller.
          request_reboot = True
          is_intermittent = True
        if is_intermittent and attempt_id + 1 != self._ATTEMPT_COUNT:
          dump_filename = '{}_intermittent_failure'.format(attempt_id)
          dump_path = os.path.join(
              self.output_dir, str(repeat_id), dump_filename)
        else:
          dump_path = os.path.join(self.output_dir, ERROR_FILENAME)
        with open(dump_path, 'w') as dump_output:
          error.Dump(dump_output)
        if not is_intermittent:
          error.RaiseOriginal()
        if request_reboot:
          assert resume_attempt_id is 0
          return True
    else:
      logging.error('Failed to navigate to %s after %d attemps' % \
                    (self.url, self._ATTEMPT_COUNT))
      error.RaiseOriginal()
    return False

  def _RunWithWpr(self, resume_repeat_id, perform_dry_run_before):
    """Opens WPR and attempts to run repeated monitoring navigation.

    Args:
      resume_repeat_id: Id of the run to resume.
      perform_dry_run_before: Whether the repeated run to resume should first do
        a dry run navigation attempt.

    Returns:
      Number of repeat performed. If < self.repeat, then it means that the
        device should be rebooted.
    """
    with self._chrome_ctl.OpenWprHost(self.wpr_archive_path,
        record=self.wpr_record,
        network_condition_name=self._GetEmulatorNetworkCondition('wpr'),
        disable_script_injection=self.disable_wpr_script_injection,
        out_log_path=os.path.join(self.output_dir, WPR_LOG_FILENAME)):
      for repeat_id in xrange(resume_repeat_id, self.repeat):
        reboot_requested = self._RunInRetryLoop(
            repeat_id, perform_dry_run_before)
        if reboot_requested:
          return repeat_id
    return self.repeat

  def _PullCacheFromDevice(self):
    assert self.cache_operation == CacheOperation.SAVE
    assert self.cache_archive_path, 'Need to specify where to save the cache'

    cache_directory_path = self._chrome_ctl.PullBrowserCache()
    chrome_cache.ZipDirectoryContent(
        cache_directory_path, self.cache_archive_path)
    shutil.rmtree(cache_directory_path)

  def Run(self):
    """SandwichRunner main entry point meant to be called once configured."""
    assert self.output_dir is not None
    assert self._chrome_ctl == None
    assert self._local_cache_directory_path == None
    self._CleanTraceOutputDirectory()

    if self.android_device:
      self._chrome_ctl = controller.RemoteChromeController(self.android_device)
    else:
      self._chrome_ctl = controller.LocalChromeController()
    self._chrome_ctl.AddChromeArguments(['--disable-infobars'])
    self._chrome_ctl.AddChromeArguments(self.chrome_args)
    if self.cache_operation == CacheOperation.SAVE:
      self._chrome_ctl.SetSlowDeath()
    try:
      if self.cache_operation == CacheOperation.PUSH:
        assert os.path.isfile(self.cache_archive_path)
        self._local_cache_directory_path = tempfile.mkdtemp(suffix='.cache')
        chrome_cache.UnzipDirectoryContent(
            self.cache_archive_path, self._local_cache_directory_path)
      times_repeated = self._RunWithWpr(0, False)
      if times_repeated < self.repeat:
        self._chrome_ctl.RebootDevice()
        self._RunWithWpr(times_repeated, True)
    finally:
      if self._local_cache_directory_path:
        shutil.rmtree(self._local_cache_directory_path)
        self._local_cache_directory_path = None
    if self.cache_operation == CacheOperation.SAVE:
      self._PullCacheFromDevice()

    self._chrome_ctl = None


def WalkRepeatedRuns(runner_output_dir):
  """Yields unordered (repeat id, path of the repeat directory).

  Args:
    runner_output_dir: Same as for SandwichRunner.output_dir.
  """
  repeated_run_count = 0
  for node_name in os.listdir(runner_output_dir):
    repeat_dir = os.path.join(runner_output_dir, node_name)
    if not os.path.isdir(repeat_dir):
      continue
    try:
      repeat_id = int(node_name)
    except ValueError:
      continue
    yield repeat_id, repeat_dir
    repeated_run_count += 1
  assert repeated_run_count > 0, ('Error: not a sandwich runner output '
                                  'directory: {}').format(runner_output_dir)
