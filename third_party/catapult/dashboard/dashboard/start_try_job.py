# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""URL endpoint containing server-side functionality for bisect try jobs."""

import difflib
import hashlib
import json
import logging
import pipes
import re

import httplib2

from google.appengine.api import users
from google.appengine.api import app_identity
from google.appengine.ext import ndb

from dashboard import buildbucket_job
from dashboard import can_bisect
from dashboard import list_tests
from dashboard.common import namespaced_stored_object
from dashboard.common import request_handler
from dashboard.common import stored_object
from dashboard.common import utils
from dashboard.models import graph_data
from dashboard.models import try_job
from dashboard.services import buildbucket_service
from dashboard.services import issue_tracker_service


# Path to the perf trybot config file, relative to chromium/src.
_PERF_CONFIG_PATH = 'tools/run-perf-test.cfg'

_PATCH_HEADER = """Index: %(filename)s
diff --git a/%(filename_a)s b/%(filename_b)s
index %(hash_a)s..%(hash_b)s 100644
"""

_BOT_BROWSER_MAP_KEY = 'bot_browser_map'
_INTERNAL_MASTERS_KEY = 'internal_masters'
_BUILDER_TYPES_KEY = 'bisect_builder_types'
_MASTER_TRY_SERVER_MAP_KEY = 'master_try_server_map'
_MASTER_BUILDBUCKET_MAP_KEY = 'master_buildbucket_map'
_NON_TELEMETRY_TEST_COMMANDS = {
    'angle_perftests': [
        './out/Release/angle_perftests',
        '--test-launcher-print-test-stdio=always',
        '--test-launcher-jobs=1',
    ],
    'cc_perftests': [
        './out/Release/cc_perftests',
        '--test-launcher-print-test-stdio=always',
        '--verbose',
    ],
    'idb_perf': [
        './out/Release/performance_ui_tests',
        '--gtest_filter=IndexedDBTest.Perf',
    ],
    'load_library_perf_tests': [
        './out/Release/load_library_perf_tests',
        '--single-process-tests',
    ],
    'media_perftests': [
        './out/Release/media_perftests',
        '--single-process-tests',
    ],
    'performance_browser_tests': [
        './out/Release/performance_browser_tests',
        '--test-launcher-print-test-stdio=always',
        '--enable-gpu',
    ],
    'resource_sizes': [
        'src/build/android/resource_sizes.py',
        '--chromium-output-directory {CHROMIUM_OUTPUT_DIR}',
        '--chartjson',
        '{CHROMIUM_OUTPUT_DIR}',
    ],
    'tracing_perftests': [
        './out/Release/tracing_perftests',
        '--test-launcher-print-test-stdio=always',
        '--verbose',
    ],
}
_NON_TELEMETRY_ANDROID_COMMAND = 'src/build/android/test_runner.py '\
                                 'gtest --release -s %(suite)s --verbose'
_NON_TELEMETRY_ANDROID_SUPPORTED_TESTS = ['cc_perftests', 'tracing_perftests']

_DISABLE_STORY_FILTER_SUITE_LIST = set([
    'octane',  # Has a single story.
])

_DISABLE_STORY_FILTER_STORY_LIST = set([
    'benchmark_duration', # Needs all stories to run to be relevant.
])


class StartBisectHandler(request_handler.RequestHandler):
  """URL endpoint for AJAX requests for bisect config handling.

  Requests are made to this end-point by bisect and trace forms. This handler
  does several different types of things depending on what is given as the
  value of the "step" parameter:
    "prefill-info": Returns JSON with some info to fill into the form.
    "perform-bisect": Triggers a bisect job.
  """

  def post(self):
    """Performs one of several bisect-related actions depending on parameters.

    The only required parameter is "step", which indicates what to do.

    This end-point should always output valid JSON with different contents
    depending on the value of "step".
    """
    user = users.get_current_user()
    if not utils.IsValidSheriffUser():
      message = 'User "%s" not authorized.' % user
      self.response.out.write(json.dumps({'error': message}))
      return

    step = self.request.get('step')

    if step == 'prefill-info':
      result = _PrefillInfo(self.request.get('test_path'))
    elif step == 'perform-bisect':
      result = self._PerformBisectStep(user)
    else:
      result = {'error': 'Invalid parameters.'}

    self.response.write(json.dumps(result))

  def _PerformBisectStep(self, user):
    """Gathers the parameters for a bisect job and triggers the job."""
    bug_id = int(self.request.get('bug_id', -1))
    master_name = self.request.get('master', 'ChromiumPerf')
    internal_only = self.request.get('internal_only') == 'true'
    bisect_bot = self.request.get('bisect_bot')
    use_staging_bot = self.request.get('use_staging_bot') == 'true'

    bisect_config = GetBisectConfig(
        bisect_bot=bisect_bot,
        master_name=master_name,
        suite=self.request.get('suite'),
        metric=self.request.get('metric'),
        good_revision=self.request.get('good_revision'),
        bad_revision=self.request.get('bad_revision'),
        repeat_count=self.request.get('repeat_count', 10),
        max_time_minutes=self.request.get('max_time_minutes', 20),
        bug_id=bug_id,
        story_filter=self.request.get('story_filter'),
        bisect_mode=self.request.get('bisect_mode', 'mean'),
        use_staging_bot=use_staging_bot)

    if 'error' in bisect_config:
      return bisect_config

    config_python_string = 'config = %s\n' % json.dumps(
        bisect_config, sort_keys=True, indent=2, separators=(',', ': '))

    bisect_job = try_job.TryJob(
        bot=bisect_bot,
        config=config_python_string,
        bug_id=bug_id,
        email=user.email(),
        master_name=master_name,
        internal_only=internal_only,
        job_type='bisect')

    alert_keys = self.request.get('alerts')
    alerts = None
    if alert_keys:
      alerts = [ndb.Key(urlsafe=a).get() for a in json.loads(alert_keys)]

    try:
      results = PerformBisect(bisect_job, alerts=alerts)
    except request_handler.InvalidInputError as iie:
      results = {'error': iie.message}
    if 'error' in results and bisect_job.key:
      bisect_job.key.delete()
    return results


def _PrefillInfo(test_path):
  """Pre-fills some best guesses config form based on the test path.

  Args:
    test_path: Test path string.

  Returns:
    A dictionary indicating the result. If successful, this should contain the
    the fields "suite", "email", "all_metrics", and "default_metric". If not
    successful this will contain the field "error".
  """
  if not test_path:
    return {'error': 'No test specified'}

  suite_path = '/'.join(test_path.split('/')[:3])
  suite = utils.TestKey(suite_path).get()
  if not suite:
    return {'error': 'Invalid test %s' % test_path}

  graph_path = '/'.join(test_path.split('/')[:4])
  graph_key = utils.TestKey(graph_path)

  info = {'suite': suite.test_name}
  info['master'] = suite.master_name
  info['internal_only'] = suite.internal_only

  info['all_bots'] = _GetAvailableBisectBots(suite.master_name)
  info['bisect_bot'] = GuessBisectBot(suite.master_name, suite.bot_name)

  user = users.get_current_user()
  if not user:
    return {'error': 'User not logged in.'}

  # Secondary check for bisecting internal only tests.
  if suite.internal_only and not utils.IsInternalUser():
    return {'error': 'Unauthorized access, please use corp account to login.'}

  if users.is_current_user_admin():
    info['is_admin'] = True
  else:
    info['is_admin'] = False

  info['email'] = user.email()

  info['all_metrics'] = []
  metric_keys = list_tests.GetTestDescendants(graph_key, has_rows=True)

  for metric_key in metric_keys:
    metric_path = utils.TestPath(metric_key)
    if metric_path.endswith('/ref') or metric_path.endswith('_ref'):
      continue
    info['all_metrics'].append(GuessMetric(metric_path))
  info['default_metric'] = GuessMetric(test_path)
  info['story_filter'] = GuessStoryFilter(test_path)
  return info


def GetBisectConfig(
    bisect_bot, master_name, suite, metric, good_revision, bad_revision,
    repeat_count, max_time_minutes, bug_id, story_filter=None,
    bisect_mode='mean', use_staging_bot=False):
  """Fills in a JSON response with the filled-in config file.

  Args:
    bisect_bot: Bisect bot name. (This should be either a legacy bisector or a
        recipe-enabled tester).
    master_name: Master name of the test being bisected.
    suite: Test suite name of the test being bisected.
    metric: Bisect bot "metric" parameter, in the form "chart/trace".
    good_revision: Known good revision number.
    bad_revision: Known bad revision number.
    repeat_count: Number of times to repeat the test.
    max_time_minutes: Max time to run the test.
    bug_id: The Chromium issue tracker bug ID.
    bisect_mode: What aspect of the test run to bisect on; possible options are
        "mean", "std_dev", and "return_code".
    use_staging_bot: Specifies if we should redirect to a staging bot.

  Returns:
    A dictionary with the result; if successful, this will contain "config",
    which is a config string; if there's an error, this will contain "error".
  """
  command = GuessCommand(bisect_bot, suite, story_filter=story_filter)
  if not command:
    return {'error': 'Could not guess command for %r.' % suite}

  try:
    repeat_count = int(repeat_count)
    max_time_minutes = int(max_time_minutes)
    bug_id = int(bug_id)
  except ValueError:
    return {'error': 'repeat count, max time and bug_id must be integers.'}

  if not can_bisect.IsValidRevisionForBisect(good_revision):
    return {'error': 'Invalid "good" revision "%s".' % good_revision}
  if not can_bisect.IsValidRevisionForBisect(bad_revision):
    return {'error': 'Invalid "bad" revision "%s".' % bad_revision}

  config_dict = {
      'command': command,
      'good_revision': str(good_revision),
      'bad_revision': str(bad_revision),
      'metric': metric,
      'repeat_count': str(repeat_count),
      'max_time_minutes': str(max_time_minutes),
      'bug_id': str(bug_id),
      'builder_type': _BuilderType(master_name),
      'target_arch': GuessTargetArch(bisect_bot),
      'bisect_mode': bisect_mode,
  }

  # If 'use_staging_bot' was checked, we try to redirect to a staging bot.
  # Do this here since we need to guess parts of the command line using the
  # bot's name.
  if use_staging_bot:
    bisect_bot = _GuessStagingBot(master_name, bisect_bot) or bisect_bot

  config_dict['recipe_tester_name'] = bisect_bot
  return config_dict


def _BuilderType(master_name):
  """Returns the builder_type string to use in the bisect config.

  Args:
    master_name: The test master name.

  Returns:
    A string which indicates where the builds should be obtained from.
  """
  builder_types = namespaced_stored_object.Get(_BUILDER_TYPES_KEY)
  if not builder_types or master_name not in builder_types:
    return 'perf'
  return builder_types[master_name]


def GuessTargetArch(bisect_bot):
  """Returns target architecture for the bisect job."""
  if 'x64' in bisect_bot or 'win64' in bisect_bot:
    return 'x64'
  elif bisect_bot in ['android_nexus9_perf_bisect']:
    return 'arm64'
  else:
    return 'ia32'


def _GetPerfTryConfig(
    bisect_bot, suite, good_revision, bad_revision,
    chrome_trace_filter_string=None, atrace_filter_string=None):
  """Fills in a JSON response with the filled-in config file.

  Args:
    bisect_bot: Bisect bot name.
    suite: Test suite name.
    good_revision: Known good revision number.
    bad_revision: Known bad revision number.
    chrome_trace_filter_string: Argument to telemetry --extra-chrome-categories.
    atrace_filter_string: Argument to telemetry --extra-atrace-categories.

  Returns:
    A dictionary with the result; if successful, this will contain "config",
    which is a config string; if there's an error, this will contain "error".
  """
  command = GuessCommand(
      bisect_bot, suite, chrome_trace_filter_string=chrome_trace_filter_string,
      atrace_filter_string=atrace_filter_string)
  if not command:
    return {'error': 'Only Telemetry is supported at the moment.'}

  if not can_bisect.IsValidRevisionForBisect(good_revision):
    return {'error': 'Invalid "good" revision "%s".' % good_revision}
  if not can_bisect.IsValidRevisionForBisect(bad_revision):
    return {'error': 'Invalid "bad" revision "%s".' % bad_revision}

  config_dict = {
      'command': command,
      'good_revision': str(good_revision),
      'bad_revision': str(bad_revision),
      'repeat_count': '1',
      'max_time_minutes': '60',
  }
  return config_dict


def _GetAvailableBisectBots(master_name):
  """Gets all available bisect bots corresponding to a master name."""
  bisect_bot_map = namespaced_stored_object.Get(can_bisect.BISECT_BOT_MAP_KEY)
  for master, platform_bot_pairs in bisect_bot_map.iteritems():
    if master_name.startswith(master):
      return sorted({bot for _, bot in platform_bot_pairs})
  return []


def GuessBisectBot(master_name, bot_name):
  """Returns a bisect bot name based on |bot_name| (perf_id) string."""
  platform_bot_pairs = []
  bisect_bot_map = namespaced_stored_object.Get(can_bisect.BISECT_BOT_MAP_KEY)
  if bisect_bot_map:
    for master, pairs in bisect_bot_map.iteritems():
      if master_name.startswith(master):
        platform_bot_pairs = pairs
        break

  fallback = 'linux_perf_bisect'
  if not platform_bot_pairs:
    # No bots available.
    logging.error('No bisect bots defined for %s.', master_name)
    return fallback

  bot_name = bot_name.lower()
  for platform, bisect_bot in platform_bot_pairs:
    if platform.lower() in bot_name:
      return bisect_bot

  for _, bisect_bot in platform_bot_pairs:
    if bisect_bot == fallback:
      return fallback

  # Nothing was found; log a warning and return a fall-back name.
  logging.warning('No bisect bot for %s/%s.', master_name, bot_name)
  return platform_bot_pairs[0][1]


def _IsNonTelemetrySuiteName(suite):
  return (suite in _NON_TELEMETRY_TEST_COMMANDS or
          suite.startswith('resource_sizes'))


def GuessCommand(
    bisect_bot, suite, story_filter=None, chrome_trace_filter_string=None,
    atrace_filter_string=None):
  """Returns a command to use in the bisect configuration."""
  if _IsNonTelemetrySuiteName(suite):
    return _GuessCommandNonTelemetry(suite, bisect_bot)
  return _GuessCommandTelemetry(
      suite, bisect_bot, story_filter, chrome_trace_filter_string,
      atrace_filter_string)


def _GuessCommandNonTelemetry(suite, bisect_bot):
  """Returns a command string to use for non-Telemetry tests."""
  if bisect_bot.startswith('android'):
    if suite in _NON_TELEMETRY_ANDROID_SUPPORTED_TESTS:
      return _NON_TELEMETRY_ANDROID_COMMAND % {'suite': suite}
  if suite.startswith('resource_sizes'):
    match = re.match(r'.*\((.*)\)', suite)
    if not match:
      return None
    apk_name = match.group(1)
    command = list(_NON_TELEMETRY_TEST_COMMANDS['resource_sizes'])
    command[-1] += '/apks/' + apk_name
  else:
    command = list(_NON_TELEMETRY_TEST_COMMANDS[suite])

  if command[0].startswith('./out'):
    command[0] = command[0].replace('./', './src/')

  # For Windows x64, the compilation output is put in "out/Release_x64".
  # Note that the legacy bisect script always extracts binaries into Release
  # regardless of platform, so this change is only necessary for recipe bisect.
  if GuessBrowserName(bisect_bot) == 'release_x64':
    command[0] = command[0].replace('/Release/', '/Release_x64/')

  if bisect_bot.startswith('win') or bisect_bot.startswith('staging_win'):
    command[0] = command[0].replace('/', '\\')
    command[0] += '.exe'
  return ' '.join(command)


def _GuessCommandTelemetry(
    suite, bisect_bot, story_filter, chrome_trace_filter_string,
    atrace_filter_string):
  """Returns a command to use given that |suite| is a Telemetry benchmark."""
  command = []

  test_cmd = 'src/tools/perf/run_benchmark'

  # TODO(simonhatch): Workaround for crbug.com/677843
  pageset_repeat = 1
  if ('startup.warm' in suite or
      'start_with_url.warm' in suite):
    pageset_repeat = 5

  command.extend([
      test_cmd,
      '-v',
      '--browser=%s' % GuessBrowserName(bisect_bot),
      '--output-format=chartjson',
      '--upload-results',
      '--pageset-repeat=%d' % pageset_repeat,
      '--also-run-disabled-tests',
  ])
  if story_filter:
    command.append('--story-filter=%s' % pipes.quote(story_filter))

  if chrome_trace_filter_string:
    command.append('--extra-chrome-categories=%s' % chrome_trace_filter_string)

  if atrace_filter_string:
    command.append('--extra-atrace-categories=%s' % atrace_filter_string)

  # Test command might be a little different from the test name on the bots.
  if suite == 'blink_perf':
    test_name = 'blink_perf.all'
  elif suite == 'startup.cold.dirty.blank_page':
    test_name = 'startup.cold.blank_page'
  elif suite == 'startup.warm.dirty.blank_page':
    test_name = 'startup.warm.blank_page'
  else:
    test_name = suite
  command.append(test_name)

  return ' '.join(command)


def GuessBrowserName(bisect_bot):
  """Returns a browser name string for Telemetry to use."""
  default = 'release'
  browser_map = namespaced_stored_object.Get(_BOT_BROWSER_MAP_KEY)
  if not browser_map:
    return default
  for bot_name_prefix, browser_name in browser_map:
    if bisect_bot.startswith(bot_name_prefix):
      return browser_name
  return default


def GuessStoryFilter(test_path):
  """Returns a suitable "story filter" to use in the bisect config.

  Args:
    test_path: The slash-separated test path used by the dashboard.

  Returns:
    A regex pattern that matches the story referred to by the test_path, or
    an empty string if the test_path does not refer to a story and no story
    filter should be used.
  """
  test_path_parts = test_path.split('/')
  suite_name, story_name = test_path_parts[2], test_path_parts[-1]
  if any([
      _IsNonTelemetrySuiteName(suite_name),
      suite_name in _DISABLE_STORY_FILTER_SUITE_LIST,
      suite_name.startswith('media.') and '.html?' not in story_name,
      suite_name.startswith('webrtc.'),
      story_name in _DISABLE_STORY_FILTER_STORY_LIST]):
    return ''
  test_key = utils.TestKey(test_path)
  subtest_keys = list_tests.GetTestDescendants(test_key)
  try:
    subtest_keys.remove(test_key)
  except ValueError:
    pass
  if subtest_keys:  # Stories do not have subtests.
    return ''

  # memory.top_10_mobile runs pairs of "{url}" and "after_{url}" stories to
  # gather foreground and background measurements. Story filters may be used,
  # but we need to strip off the "after_" prefix so that both stories in the
  # pair are always run together.
  # TODO(crbug.com/761014): Remove when benchmark is deprecated.
  if suite_name == 'memory.top_10_mobile' and story_name.startswith('after_'):
    story_name = story_name[len('after_'):]

  # During import, some chars in story names got replaced by "_" so they
  # could be safely included in the test_path. At this point we don't know
  # what the original characters were. Additionally, some special characters
  # and argument quoting are not interpreted correctly, e.g. by bisect
  # scripts (crbug.com/662472). We thus keep only a small set of "safe chars"
  # and replace all others with match-any-character regex dots.
  return re.sub(r'[^a-zA-Z0-9]', '.', story_name)


# TODO(eakuefner): Make bisect work with value-level summaries and delete this.
def GuessMetric(test_path):
  """Returns a "metric" string to use in the bisect config.

  Args:
    test_path: The slash-separated test path used by the dashboard.

  Returns:
    A 2- or 3-part test name, with duplication to accommodate bisect.
  """
  metric_path = '/'.join(test_path.split('/')[3:])
  if metric_path.count('/') == 0:
    metric_path += '/' + metric_path
  return metric_path


def _HasChildTest(test_path):
  key = utils.TestKey(test_path)
  child = graph_data.TestMetadata.query(
      graph_data.TestMetadata.parent_test == key).get()
  return bool(child)


def _CreatePatch(base_config, config_changes, config_path):
  """Takes the base config file and the changes and generates a patch.

  Args:
    base_config: The whole contents of the base config file.
    config_changes: The new config string. This will replace the part of the
        base config file that starts with "config = {" and ends with "}".
    config_path: Path to the config file to use.

  Returns:
    A triple with the patch string, the base md5 checksum, and the "base
    hashes", which normally might contain checksums for multiple files, but
    in our case just contains the base checksum and base filename.
  """
  # Compute git SHA1 hashes for both the original and new config. See:
  # http://git-scm.com/book/en/Git-Internals-Git-Objects#Object-Storage
  base_checksum = hashlib.md5(base_config).hexdigest()
  base_hashes = '%s:%s' % (base_checksum, config_path)
  base_header = 'blob %d\0' % len(base_config)
  base_sha = hashlib.sha1(base_header + base_config).hexdigest()

  # Replace part of the base config to get the new config.
  new_config = (base_config[:base_config.rfind('config')] +
                config_changes +
                base_config[base_config.rfind('}') + 2:])

  # The client sometimes adds extra '\r' chars; remove them.
  new_config = new_config.replace('\r', '')
  new_header = 'blob %d\0' % len(new_config)
  new_sha = hashlib.sha1(new_header + new_config).hexdigest()
  diff = difflib.unified_diff(base_config.split('\n'),
                              new_config.split('\n'),
                              'a/%s' % config_path,
                              'b/%s' % config_path,
                              lineterm='')
  patch_header = _PATCH_HEADER % {
      'filename': config_path,
      'filename_a': config_path,
      'filename_b': config_path,
      'hash_a': base_sha,
      'hash_b': new_sha,
  }
  patch = patch_header + '\n'.join(diff)
  patch = patch.rstrip() + '\n'
  return (patch, base_checksum, base_hashes)


def PerformBisect(bisect_job, alerts=None):
  """Starts the bisect job.

  This creates a patch, uploads it, then tells Rietveld to try the patch.

  Args:
    bisect_job: A TryJob entity.

  Returns:
    A dictionary containing the result; if successful, this dictionary contains
    the field "issue_id" and "issue_url", otherwise it contains "error".

  Raises:
    AssertionError: Bot or config not set as expected.
    request_handler.InvalidInputError: Some property of the bisect job
        is invalid.
  """
  assert bisect_job.bot and bisect_job.config
  if not bisect_job.key:
    bisect_job.put()

  result = _PerformBuildbucketBisect(bisect_job)
  if 'error' in result:
    bisect_job.SetFailed()
    comment = 'Bisect job failed to kick off'
  elif result.get('issue_url'):
    comment = 'Started bisect job %s' % result['issue_url']
  else:
    comment = 'Started bisect job: %s' % result

  if not bisect_job.results_data:
    bisect_job.results_data = {'issue_url': 'N/A', 'issue_id': 'N/A'}
  bisect_job.results_data.update(result)
  bisect_job.put()

  if alerts:
    for a in alerts:
      a.recipe_bisects.append(bisect_job.key)

  if bisect_job.bug_id:
    logging.info('Commenting on bug %s for bisect job', bisect_job.bug_id)
    issue_tracker = issue_tracker_service.IssueTrackerService(
        utils.ServiceAccountHttp())
    issue_tracker.AddBugComment(bisect_job.bug_id, comment, send_email=False)
  return result


def _MakeBuildbucketBisectJob(bisect_job):
  """Creates a bisect job object that the buildbucket service can use.

  Args:
    bisect_job: The entity (try_job.TryJob) off of which to create the
        buildbucket job.

  Returns:
    A buildbucket_job.BisectJob object populated with the necessary attributes
    to pass it to the buildbucket service to start the job.
  """
  config = bisect_job.GetConfigDict()
  if bisect_job.job_type not in ['bisect', 'bisect-fyi']:
    raise request_handler.InvalidInputError(
        'Recipe only supports bisect jobs at this time.')

  # Recipe bisect supports 'perf' and 'return_code' test types only.
  # TODO (prasadv): Update bisect form on dashboard to support test_types.
  test_type = 'perf'
  if config.get('bisect_mode') == 'return_code':
    test_type = config['bisect_mode']

  return buildbucket_job.BisectJob(
      try_job_id=bisect_job.key.id(),
      good_revision=config['good_revision'],
      bad_revision=config['bad_revision'],
      test_command=config['command'],
      metric=config['metric'],
      repeats=config['repeat_count'],
      timeout_minutes=config['max_time_minutes'],
      bug_id=bisect_job.bug_id,
      gs_bucket='chrome-perf',
      recipe_tester_name=config['recipe_tester_name'],
      test_type=test_type,
      required_initial_confidence=config.get('required_initial_confidence')
  )


def _PerformBuildbucketBisect(bisect_job):
  config_dict = bisect_job.GetConfigDict()
  if 'recipe_tester_name' not in config_dict:
    logging.error('"recipe_tester_name" required in bisect jobs '
                  'that use buildbucket. Config: %s', config_dict)
    return {'error': 'No "recipe_tester_name" given.'}

  bucket = _GetTryServerBucket(bisect_job)
  try:
    bisect_job.buildbucket_job_id = buildbucket_service.PutJob(
        _MakeBuildbucketBisectJob(bisect_job), bucket)
    bisect_job.SetStarted()
    hostname = app_identity.get_default_version_hostname()
    job_id = bisect_job.buildbucket_job_id
    issue_url = 'https://%s/buildbucket_job_status/%s' % (hostname, job_id)
    return {
        'issue_id': job_id,
        'issue_url': issue_url,
    }
  except httplib2.HttpLib2Error as e:
    return {
        'error': ('Could not start job because of the following exception: ' +
                  e.message),
    }


def _GetTryServerBucket(bisect_job):
  """Returns the bucket name to be used by buildbucket."""
  master_bucket_map = namespaced_stored_object.Get(_MASTER_BUILDBUCKET_MAP_KEY)
  default = 'master.tryserver.chromium.perf'
  if not master_bucket_map:
    logging.warning(
        'Could not get bucket to be used by buildbucket, using default.')
    return default
  return master_bucket_map.get(bisect_job.master_name, default)

def _GuessStagingBot(master_name, production_bot_name):
  staging_bot_map = stored_object.Get('staging_bot_map') or {
      'ChromiumPerf': [
          ['win', 'staging_win_perf_bisect'],
          ['mac', 'staging_mac_10_10_perf_bisect'],
          ['linux', 'staging_linux_perf_bisect'],
          ['android', 'staging_android_nexus5X_perf_bisect']]
  }
  for infix, staging_bot in staging_bot_map[master_name]:
    if infix in production_bot_name:
      return staging_bot
