# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Simple log-based classification of build failures."""

from __future__ import print_function

import datetime
import os
import re

from chromite.lib import cros_logging as logging
from chromite.lib import gs


def LogTimestampToDatetime(timestamp):
  """Convert a log timestamp to a datetime.

  Date is assumed to be default of 1900/1/1.

  Args:
    timestamp: string of the form 'HH:MM:SS'

  Returns:
    datetime object.
  """
  return datetime.datetime.strptime(timestamp, '%H:%M:%S')


def WhiteoutImageVersion(test_name):
  """Whiteout the version embedded in a test name

  Args:
    test_name: string including an embedded version number.

  Returns:
    string with version component replaces with '_#.#.#'
  """
  return re.sub(r'_\d+\.\d+\.\d+', '_#.#.#', test_name)


# Patterns used for test root causing.

DEVSERVER_IP_RE = re.compile(r'Staging autotest artifacts for .* on devserver '
                             r'http://(\d+\.\d+\.\d+\.\d+):8082')
NETWORK_RE = re.compile('Returned update_engine error code: ERROR_CODE=37')
SERVICE_FILE_RE = re.compile('The name org.chromium.UpdateEngine '
                             'was not provided by any .service files')
MYSQL_IP_RE = re.compile(r'Can\'t connect to MySQL server on '
                         r'\'(\d+\.\d+\.\d+\.\d+)\'')
AUTOTEST_TIMEOUT_RE = re.compile(
    r'autotest_lib.client.common_lib.error.TimeoutException')


def ClassifyDUTAutoservLog(log_content):
  """Attempts to classify any failures in the DUT's autoserv.DEBUG logs.

  Args:
    log_content: string iterator.

  Returns:
    string of root cause, None otherwise.
  """
  devserver = 'unknown'

  for line in log_content:
    m = DEVSERVER_IP_RE.search(line)
    if m:
      # saving devserver value for other errors below.
      devserver = m.group(1)

    m = NETWORK_RE.search(line)
    if m:
      return 'kOmahaErrorInHTTPResponse: %s' % (devserver)

    m = SERVICE_FILE_RE.search(line)
    if m:
      return 'dot-service'

  return None

def ClassifyDUTTKOParseLog(log_content):
  """Attempts to classify any failures in the TKO parse .parse.log file.

  Args:
    log_content: string iterator.

  Returns:
    string of root cause, None otherwise.
  """
  server = 'unknown'

  for line in log_content:
    m = MYSQL_IP_RE.search(line)
    if m:
      # saving devserver value for other errors below.
      server = m.group(1)

    m = AUTOTEST_TIMEOUT_RE.search(line)
    if m:
      return 'TKOParseTimeout: %s' % (server)

  return None

def ClassifyGSFile(gs_path, handler):
  """Attempts to classify a log file from GS storage.

  Args:
    gs_path: GS path.
    handler: Handler which takes log content and returns a classification.

  Returns:
    result from handler or None if the file cannot be retrieved.
  """
  ctx = gs.GSContext()
  try:
    log_content = ctx.Cat(gs_path)
  except gs.GSNoSuchKey:
    return None
  return handler(log_content.splitlines())

def GetTestRootCause(job_id, dut):
  """Attempts to get the root cause of a test failure.

  Args:
    job_id: Job that failed.
    dut: Device that was running the test.

  Returns:
    string with root cause, or None if the root cause is unknown.
  """
  test_path = ('gs://chromeos-autotest-results/%(job)s-chromeos-test' %
               {'job': job_id})

  # DUT is required to access logs.  If not specified, try and
  # determine it based on contents of test directory.
  ctx = gs.GSContext()
  if dut is None:
    try:
      files = ctx.LS(test_path)
    except gs.GSNoSuchKey:
      return None
    if len(files) == 0:
      return None
    else:
      base_path = files[0]
  else:
    base_path = os.path.join(test_path, dut)

  # Grab autoserv.DEBUG log.
  classification = ClassifyGSFile(os.path.join(base_path,
                                               'debug/autoserv.DEBUG'),
                                  ClassifyDUTAutoservLog)
  if classification:
    return classification

  # Grab .parse.log log.
  classification = ClassifyGSFile(os.path.join(base_path, '.parse.log'),
                                  ClassifyDUTTKOParseLog)
  if classification:
    return classification

  return None


# Patterns used for classifying test failures.

# 05:28:51: INFO: RunCommand: /b/cbuild/internal_master/chromite/third_party/
#    swarming.client/swarming.py run --swarming
# 07:52:52: WARNING: Killing tasks: [<_BackgroundTask(_BackgroundTask-7:7:4,
#     started)>]
TIMESTAMP_PATTERN = r'(\d\d:\d\d:\d\d)'
SWARMING_START_RE = re.compile(TIMESTAMP_PATTERN + ': INFO: RunCommand: ' +
                               '.*/swarming.client/swarming.py run --swarming')
SWARMING_TIMEOUT_RE = re.compile(TIMESTAMP_PATTERN +
                                 ': WARNING: Killing tasks')

FAILED_GS_REMOVAL_1_RE = re.compile(r'^Removing gs:/.*\.\.\.')
FAILED_GS_REMOVAL_2_RE = re.compile(r'^NotFoundException: .* does not exist\.')

# Simple-minded labeling of failures.
SIMPLE_FAILURE_PATTERNS = [(re.compile(pattern), label) for pattern, label in (
    ('MySQL server has gone away', 'MySQL server'),
    ('PayloadTestError: cannot find source payload for testing',
     'missing payload'),
    ('DebugSymbolsUploadException: Failed to upload all symbols',
     'DebugSymbols Upload'),
    ('Failed to install device image', 'image install'),
    ('UnexpectedSignerResultsError', 'signer'),
    ('reached AU retry limit', 'AU limit'),
    ('ERROR:.*writer.Write.*failed', 'delta diff write'),
    ('Timeout reason: This build has reached the timeout '
     'deadline set by the master', 'master timeout'),
    ('FAIL: No JSON object could be decoded', 'JSON decode'),
    ('ERROR: Failed to parse JSON', 'JSON parse'),
    ('FAIL: stage_artifacts timed out', 'artifact staging'),
    ('Command [^a-z]*tar.* returned non-zero exit status 2', 'tar error 2'),
    ('Unhandled run_suite exception: Call is timed out',
     'run_suite call timeout'),
    ('Not enough DUTs', 'DUTs'),
    ('IOError: .Errno 28. No space left on device', 'no space'),
    ('Unhandled DevServerException: All devservers are currently down',
     'DEVSERVERS'),
    ('Devserver portfile does not exist', 'No Devserver portfile'),
    ('Your .*Oauth 2.0 User Account.* credentials are invalid', 'Bad GS Oauth'),
    ('CommandException: No URLs matched', 'Bad URL'),
    ('Reached GCETestStage test run timeout', 'GCETestStage Timeout'),
    (r'No output from .* for \d+ seconds', 'Timeout (No Output Returned)'),
)]

# Test name + outcome pattern.
# For instance:
# login_MultiUserPolicy                   [ PASSED ]
# Spaces in between words are allowed (for instance: Suite job)
TEST_OUTCOME_RE = re.compile(r'(\w[\w\. ]+\w) +\[ ([A-Z]+) \]')

FAILED_TEST_REASON_RE = re.compile('FAIL|ERROR|ABORT')
# TEST_FAILURE_PATTERNS is matched against the entire line which includes
# the test name and the reason.  In cases where additional debugging or
# information is available, a link is provided with details.
TEST_FAILURE_PATTERNS = [(re.compile(pattern), label) for pattern, label in (
    # Tests.
    ('provision', 'https://yaqs.googleplex.com/eng/q/5277276940009472'),
    # Reasons.
    ('ABORT: Failed to install device image.*CODE=37', 'AU37'),
    ('ABORT: Failed to install device image.*CODE=0', 'AU0'),
    ('FAIL: Update server .* not available', 'NoServer'),
    ('ABORT: Timed out, did not run', 'TimeOut'),
    ('ABORT: Host did not return from reboot', 'NoReboot'),
    ('FAIL: Unhandled DevServerException: All devservers.*down', 'NoDevServ'),
    ('ABORT: Failed to trigger an update.*AutoserRunError', 'BlankAUError'),
    ('FAIL: Failed to receive a download started notification', 'AUNoStart'),
    ('FAIL: Saw file system error', 'FSError'),
    ('ABORT: Autotest client terminated unexpectedly: DUT is pingable',
     'AUUnexpect'),
)]
ON_DUT_RE = re.compile(r'on (chromeos\d+-row\d+-rack\d+-host\d+).*')

LOG_LINK_RE = re.compile(
    r'(\w[\w\. ]+\w) +'
    r'http://cautotest/tko/retrieve_logs.cgi\?job=/results/(\d+)-chromeos-test')

AUTOTEST_LOG_LINK_RE = re.compile(
    r'@@@STEP_LINK@\[Test-Logs\]: ([^:]+):.*(FAIL|ABORT).*'
    r'on (chromeos\d+-row\d+-rack\d+-host\d+).*'
    r'http://cautotest/tko/retrieve_logs.cgi\?job=/results/(\d+)-chromeos-test')


def ClassifyTestFailure(log_content):
  """Classifies the logs of a test failure.

  Args:
    log_content: string iterator.

  Returns:
    set of classifications, or None if no classification.
  """
  # Add failures to this set as they are found.
  failure_set = set()
  swarming_start_time = None
  test_runs = {}
  test_jobs = {}

  for line in log_content:
    m = SWARMING_START_RE.search(line)
    if m:
      swarming_start_time = LogTimestampToDatetime(m.group(1))

    m = SWARMING_TIMEOUT_RE.search(line)
    if m and swarming_start_time:
      swarming_end_time = LogTimestampToDatetime(m.group(1))
      elapsed_time = swarming_end_time - swarming_start_time
      # Since these are just times without dates, if the time appears to go
      # backwards, assume that it's rolled over to the next day.
      if elapsed_time.total_seconds() < 0:
        elapsed_time += datetime.timedelta(days=1)
      if elapsed_time > datetime.timedelta(hours=1):
        failure_set.add('swarming timeout')

    # Another multi-line check.
    m = FAILED_GS_REMOVAL_1_RE.search(line)
    if m:
      line = log_content.next()
      m = FAILED_GS_REMOVAL_2_RE.search(line)
      if m:
        failure_set.add('GS removal')

    for failure_pattern, failure_label in SIMPLE_FAILURE_PATTERNS:
      m = failure_pattern.search(line)
      if m:
        failure_set.add(failure_label)

    # Two-line check looking for tests that failed.
    m = TEST_OUTCOME_RE.search(line)
    if m:
      # For each invocation of the test, record its status.  Each test
      # might be invoked multiple times, so need to record the ordering
      # of each run to match it with the logs.
      # TODO: This entire parsing of failed jobs is very weak and fragile.
      # Get the failures from a structured data source or start logging
      # a single line sufficient to trigger the root cause logic.
      autotest_name = m.group(1)
      if autotest_name not in test_runs:
        test_runs[autotest_name] = []
      test_entry = {'status': m.group(2)}
      test_runs[autotest_name].append(test_entry)

      if m.group(2) in ('FAILED', 'ABORT'):
        failure = m.group(1)
        # some test names contain an image version, for instance:
        # autoupdate_EndToEndTest_npo_delta_8803.0.0
        # The version number prevents matching failures and is not
        # useful at this stage so we hide it.
        failure = WhiteoutImageVersion(failure)
        if failure not in ['Suite job']:
          line = log_content.next()

          # Extract and store DUT name if it exists.
          m = ON_DUT_RE.search(line)
          if m:
            test_entry['dut'] = m.group(1)

          m = FAILED_TEST_REASON_RE.search(line)
          # Append a label to the failure name
          if m:
            for error_re, error_label in TEST_FAILURE_PATTERNS:
              m = error_re.search(line)
              if m:
                failure += '{%s}' % error_label
                break
          failure_set.add(failure)

    # Look for links to test logs.
    m = LOG_LINK_RE.search(line)
    if m:
      # Record the ordering of the test logs so they can be reconciled.
      autotest_name = m.group(1)
      autotest_job_id = m.group(2)
      test_jobs.setdefault(autotest_name, []).append(autotest_job_id)

  # Find the root cause for failed test runs.
  root_caused_jobs = set()
  for autotest_name in test_runs:
    for run in xrange(len(test_runs[autotest_name])):
      test_entry = test_runs[autotest_name][run]
      status = test_entry.get('status', '')
      autotest_dut = test_entry.get('dut', None)
      if (status in ('FAILED', 'ABORT') and
          autotest_name in test_jobs and run < len(test_jobs[autotest_name])):
        autotest_job_id = test_jobs[autotest_name][run]
        if autotest_job_id in root_caused_jobs:
          # Avoid processing the same job twice, necessary since the logs
          # are duplicated.
          continue
        root_caused_jobs.add(autotest_job_id)
        root_cause = GetTestRootCause(autotest_job_id, autotest_dut)
        logging.info('root caused job %s dut %s: %s', autotest_job_id,
                     autotest_dut, root_cause)
        if root_cause:
          # Replace existing failure entry with a more specific one
          if autotest_name in failure_set:
            failure_set.discard(autotest_name)
          failure_set.add('%s==%s' % (autotest_name, root_cause))

  if failure_set:
    return failure_set
  return None


# Patterns for classifying timeouts.

TIMEOUT_RE = re.compile('Timeout occurred-')


def IdentifyTimeout(log_content):
  """Classifies the logs of a stage that can contain a timetout.

  Args:
    log_content: string iterator of the stage's logs.

  Returns:
    set of classifications, or None if no classification.
  """
  for line in log_content:
    if TIMEOUT_RE.search(line):
      return set(['timeout'])


# Patterns for classifying archive failures.

BUILD_IMAGE_RE = re.compile('BuildScriptFailure: ./build_image failed .code=1.')

def ClassifyArchiveFailure(log_content):
  """Classifies the logs of an archive failure.

  Args:
    log_content: string iterator of the stage's logs.

  Returns:
    set of classifications, or None if no classification.
  """
  for line in log_content:
    if BUILD_IMAGE_RE.search(line):
      return set(['build_image'])


def GetHandler(stage):
  """Determines a handler to classify a particular type of stage.

  Args:
    stage: name of the stage that failed.

  Returns:
    function to classify logs.
  """
  return {
      'HWTest [sanity]': ClassifyTestFailure,
      'HWTest [bvt-inline]': ClassifyTestFailure,
      'HWTest [bvt-arc]': ClassifyTestFailure,
      'AUTest [au]': ClassifyTestFailure,
      'HWTest [jetstream_cq]': ClassifyTestFailure,
      'Paygen': ClassifyTestFailure,
      'PaygenBuildCanary': ClassifyTestFailure,
      'PaygenTestCanary': ClassifyTestFailure,
      'PaygenBuildDev': ClassifyTestFailure,
      'PaygenTestDev': ClassifyTestFailure,
      'SetupBoard': IdentifyTimeout,
      'BuildPackages': IdentifyTimeout,
      'Archive': ClassifyArchiveFailure,
      'VMTest': ClassifyTestFailure,
      'GCETest': ClassifyTestFailure,
  }.get(stage, None)


def ClassifyFailure(stage, log_content):
  """Classifies the logs of a failed stage.

  Args:
    stage: name of the stage that failed.
    log_content: string iterator of the stage's logs.

  Returns:
    set of classifications, or None if no classification.
  """
  # Ignore steps that failed because substeps failed.
  if stage.startswith('cbuildbot ') or stage in ['steps', 'Failure reason']:
    return None

  # Getting rid of (attempt ...) appended onto labels.
  if stage.startswith('VMTest') or stage.startswith('GCETest'):
    stage = stage.split(' ', 1)[0]

  handler = GetHandler(stage)
  if handler:
    return handler(log_content)
  else:
    logging.debug('Missing handler for stage %s', stage)
    return None

# Extracts block, success, task, notes.
STATUS_LOG_RE = re.compile(
    r'(?P<block>START|END)?\s*(?P<success>GOOD|FAIL)?\s*----\s+'
    r'(?P<task>\S+)\s+timestamp=\d+\s+localtime=\w+ \d+ \d\d:\d\d:\d\d\s*'
    r'(?P<notes>.*)')


def ExtractLabStatusParts(line):
  """Given a status.log lines, extract the parts we care about.

  This means the start/end of a block, success/failure, and what was being done.

  Args:
    line: One line of a status.log log file from the lab.

  Returns:
    (block, result, task, notes), or None
  """
  result = STATUS_LOG_RE.search(line.strip())

  if not result:
    return None

  # Any of these values are allowed to be None.
  return (result.group('block'), result.group('success'),
          result.group('task'), result.group('notes'))


def HandleRepairStatus(line_parts):
  """Given turn line_parts into a set of notes about the repair job.

  Args:
    line_parts: Iterable of ExtractLabStatusParts() results.

  Returns:
    Set of single word strings which describe a given repair job.
  """
  repair_notes = set()

  noop = True  # Did this repair job perform any actual repair?

  for _, result, task, notes in line_parts:
    # If we get no failures, other than the servo connection, we did no repair
    # work.
    if result not in (None, 'GOOD') and task != 'verify.servod':
      noop = False

    # If we ran the repair.usb task, we tried to do a USB based repair.
    if task == 'repair.usb':
      repair_notes.add('USB')

    if (result == 'FAIL' and task == 'verify.ssh' and
        notes.startswith('No answer to ping')):
      repair_notes.add('NetworkWasDown')

  if noop:
    repair_notes.add('NoRepairNeeded')

  return repair_notes


def ClassifyLabJobStatusResult(log_content):
  """Given a repair log, classify the type of repair.

  Args:
    log_content: Iterator yielding the status.log lines.

  Returns:
    String summary of data extracted from log.
  """
  # Parse into parts.
  # Throw away the ones we couldn't parse, since they don't matter.
  line_parts = [ExtractLabStatusParts(l) for l in log_content]
  line_parts = [l for l in line_parts if l]

  # If we have no lines, or the last one isn't the Job END.
  if not line_parts or line_parts[-1][0] != 'END':
    return 'JOB: Unable to parse job status.log.'

  # Extract the final result for the job.
  _block, result, task, _notes = line_parts[-1]

  repair_notes = set()

  if task == 'repair':
    repair_notes |= HandleRepairStatus(line_parts)

  if not repair_notes:
    repair_notes.add('No details extracted.')

  # Example output: repair: SUCCESS: USB
  return '%s: %s: %s' % (task, result, ', '.join(sorted(repair_notes)))
