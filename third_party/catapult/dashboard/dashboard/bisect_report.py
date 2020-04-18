# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates reports base on bisect result data."""

import copy
import math

from dashboard.common import namespaced_stored_object


_BISECT_HEADER = """
=== BISECT JOB RESULTS ===
<b>%s</b>
"""

_BISECT_TO_RUN = """
To Run This Test
  %(command)s
"""

_BISECT_DEBUG_INFO = """
Debug information about this bisect:
  %(issue_url)s
"""

_BENCHMARK_DOC_URLS = {
    'memory': {
        'benchmarks': [
            'system_health.memory_',
            'memory.top_10_mobile'
        ],
        'url': ('https://chromium.googlesource.com/chromium/src/+/'\
            'master/docs/memory-infra/memory_benchmarks.md'),
    },
    'blink_perf': {
        'benchmarks': [
            'blink_perf.',
        ],
        'url': ('https://chromium.googlesource.com/chromium/src/+/'\
            'master/docs/speed/benchmark_harnesses/blink_perf.md'),
    },
    'webrtc': {
        'benchmarks': [
            'webrtc.',
        ],
        'url': ('https://chromium.googlesource.com/chromium/src/+/'\
            'master/docs/speed/benchmark_harnesses/webrtc_perf.md'),
    },
}

_BENCHMARK_DOC_INFO = """
Please refer to the following doc on diagnosing %(name)s regressions:
  %(url)s
"""

_BISECT_ADDRESSING_DOC_URL = ('http://g.co/ChromePerformanceRegressions')

_BISECT_ADDRESSING_DOC_INFO = """
More information on addressing performance regressions:
  %s
""" % _BISECT_ADDRESSING_DOC_URL

_BISECT_FOOTER = """
For feedback, file a bug with component Speed>Bisection"""

_BISECT_SUSPECTED_COMMIT = """
Suspected Commit
  Author : %(author)s
  Commit : %(cl)s
  Date   : %(cl_date)s
  Subject: %(subject)s
"""

_BISECT_SUSPECTED_RANGE = """
Suspected Commit Range
  %(num)d commits in range
"""

_BISECT_SUSPECTED_RANGE_URL = "  %(url)s%(lkgr)s..%(fkbr)s\n"

_BISECT_SUSPECTED_RANGE_MISMATCH =\
"""  Mismatching LKGR/FKBR depots, unable to provide handy url.
  good_revision: %(lkgr)s
  bad_revision : %(fkbr)s
"""

_BISECT_SUSPECTED_RANGE_UNSUPPORTED =\
    "  Unknown depot, please contact team to have this added.\n"

_BISECT_DETAILS = """
Bisect Details
  Configuration: %(bisect_bot)s
  Benchmark    : %(benchmark)s
  Metric       : %(metric)s
"""

_BISECT_DETAILS_CHANGE =\
    '  Change       : %(change)s | %(good_mean)s -> %(bad_mean)s\n'

_BISECT_WARNING_HEADER =\
    'The following warnings were raised by the bisect job:\n'

_BISECT_WARNING = ' * %s\n'

_REVISION_TABLE_TEMPLATE = """
%(table)s"""

STATUS_REPRO_WITH_CULPRIT = '%(test_type)s found with culprit'

STATUS_REPRO_UNABLE_NARROW =\
    '%(test_type)s found but unable to narrow commit range'

STATUS_REPRO_BUT_UNDECIDED = \
    '%(test_type)s found but unable to continue'

STATUS_NO_REPRO = 'NO %(test_type)s found'

STATUS_NO_VALUES = 'NO %(test_type)s found, tests failed to produce values'

STATUS_FAILED_UNEXPECTED = 'Bisect failed unexpectedly'

STATUS_INCOMPLETE = 'Bisect was unable to run to completion'

STATUS_UNKNOWN = 'Bisect failed for unknown reasons'

STATUS_IN_PROGRESS = 'Bisect is still in progress, results below are incomplete'

STATUS_TYPE_IN_PROGRESS = 'in_progress'

STATUS_TYPE_STARTED = 'started'

MESSAGE_REPRO_BUT_UNDECIDED = """
Bisect was stopped because a commit couldn't be classified as either
good or bad."""

MESSAGE_CONTACT_TEAM = """
Please contact the team (see below) and report the error."""

MESSAGE_FAILED_UNEXPECTED = """
Bisect was aborted with the following:
  %s"""

MESSAGE_INCOMPLETE = """%s

If failures persist contact the team (see below) and report the error."""

MESSAGE_FAILURE_REASON = "Error: %(failure_reason)s"

MESSAGE_RERUN = """
Please try rerunning the bisect.
"""

MESSAGE_RERUN_FROM_PARTIAL_RESULTS = """
The bisect was able to narrow the range, you can try running with:
  good_revision: %(lkgr)s
  bad_revision : %(fkbr)s"""

MESSAGE_REPRO_BUILD_FAILURES = """
Build failures prevented the bisect from narrowing the range further."""

_NON_TELEMETRY_TEST_COMMANDS = {
    'angle_perftests': 'angle_perftests',
    'cc_perftests': 'cc_perftests',
    'idb_perf': 'performance_ui_tests',
    'load_library_perf_tests': 'load_library_perf_tests',
    'media_perftests': 'media_perftests',
    'performance_browser_tests': 'performance_browser_tests',
    'resource_sizes': 'resource_sizes.py',
}

_REPOSITORIES_KEY = 'repositories'


def _GuessBenchmarkFromRunCommand(run_command):
  if 'run_benchmark' in run_command:
    return run_command.split()[-1]
  for k, v in _NON_TELEMETRY_TEST_COMMANDS.iteritems():
    if v in run_command:
      return k
  return '???'


def _WasCommitTested(commit):
  return commit.get('failed') or commit.get(
      'n_observations', len(commit.get('values', [])))


def _GenerateReport(results_data):
  revision_data = results_data.get('revision_data', [])

  lkgr_index = -1
  fkbr_index = -1
  lkgr = {}
  fkbr = {}
  for i in xrange(len(revision_data)):
    r = revision_data[i]
    if r.get('result') == 'good':
      lkgr_index = i
      lkgr = revision_data[i]
    if r.get('result') == 'bad':
      fkbr_index = i
      fkbr = revision_data[i]
      break

  test_type = 'Perf regression'
  if results_data.get('test_type') == 'return_code':
    test_type = 'Test failure'

  # Generally bisects end a few ways:
  # 1 - Success, found a culprit
  # 2 - Unexpected failure, bisect aborts suddenly with an exception
  # 3 - Interrupted, bisect didn't finish and we only got partial results
  # 4 - Semi-success, found a range but couldn't narrow further
  message = STATUS_UNKNOWN
  message_details = ''

  # 1 - Easiest case, bisect named a culprit.
  if results_data.get('culprit_data'):
    message = STATUS_REPRO_WITH_CULPRIT

  # 2 - Unexpected failure in the recipe, could be a master restart, exception
  # thrown, etc.
  if message == STATUS_UNKNOWN:
    aborted_reason = results_data.get('aborted_reason', '')
    if aborted_reason:
      # TODO(simonhatch); Ideally the recipe would only set the "aborted"
      # field on an unexpected failure. We have to wait until the dashbaord
      # changes are live to remove that, so for now we'll just filter those.
      if ('The metric values for the initial' in aborted_reason or
          'Bisect failed to reproduce the regression' in aborted_reason):
        message = STATUS_NO_REPRO
      elif ('No values were found while testing' in aborted_reason or
            'Test runs failed to produce output' in aborted_reason):
        message = STATUS_NO_VALUES
      elif 'Bisect cannot identify a culprit' in aborted_reason:
        message = STATUS_REPRO_BUT_UNDECIDED
        message_details = MESSAGE_REPRO_BUT_UNDECIDED
      else:
        message = STATUS_FAILED_UNEXPECTED
        message_details = MESSAGE_FAILED_UNEXPECTED % aborted_reason

  # 3 - Incomplete bisects, try to print out a useful narrowed range and ask
  # them to try rerunning.
  if message == STATUS_UNKNOWN:
    if (results_data.get('status') == STATUS_TYPE_STARTED or
        results_data.get('status') == STATUS_TYPE_IN_PROGRESS):
      # Try to provide some useful info on where to restart the bisect from
      rerun_info = MESSAGE_RERUN
      if lkgr_index > 0 or fkbr_index < (len(revision_data) - 1):
        if lkgr.get('depot_name') == fkbr.get('depot_name'):
          rerun_info = MESSAGE_RERUN_FROM_PARTIAL_RESULTS % {
              'lkgr': lkgr.get('commit_hash'),
              'fkbr': fkbr.get('commit_hash'),
          }
          if results_data.get('failure_reason'):
            failure_reason = MESSAGE_FAILURE_REASON % results_data
            rerun_info = '\n%s\n%s' % (failure_reason, rerun_info)
      if results_data.get('status') == STATUS_TYPE_STARTED:
        message = STATUS_INCOMPLETE
        message_details = MESSAGE_INCOMPLETE % rerun_info
      else:
        message = STATUS_IN_PROGRESS
        message_details = rerun_info

  # 4 - Semi-successful in that they were able to run the tests, but failed to
  # either repro the regression or narrow it to a single commit.
  if message == STATUS_UNKNOWN:
    if revision_data:
      commits = revision_data[lkgr_index+1:fkbr_index-1]
      if lkgr_index == 0 and fkbr_index == len(revision_data) - 1:
        # The bisect never got past the initial testing.
        if (lkgr.get('n_observations') == 0 and
            fkbr.get('n_observations') == 0):
          message = STATUS_NO_VALUES
        else:
          if all([_WasCommitTested(c) for c in commits]):
            message = STATUS_REPRO_UNABLE_NARROW
          else:
            message = STATUS_NO_REPRO
      else:
        message = STATUS_REPRO_UNABLE_NARROW

      if message == STATUS_REPRO_UNABLE_NARROW:
        if all([c.get('failed') for c in commits]):
          message_details = MESSAGE_REPRO_BUILD_FAILURES

  # No idea what happened, ask them to file a bug.
  if message == STATUS_UNKNOWN:
    message_details = MESSAGE_CONTACT_TEAM

  # Start constructing the full output.
  result = ''
  result += _BISECT_HEADER % (message % {'test_type': test_type})
  if message_details:
    result += '%s\n\n' % message_details

  warnings = results_data.get('warnings')
  if warnings:
    result += _BISECT_WARNING_HEADER
    for w in warnings:
      result += _BISECT_WARNING % w
    result += '\n'

  results_data['benchmark'] = _GuessBenchmarkFromRunCommand(
      results_data.get('command'))

  # Print out the suspect commit info
  if results_data.get('culprit_data'):
    result += _BISECT_SUSPECTED_COMMIT % results_data.get('culprit_data')
  result += _BISECT_DETAILS % results_data

  if results_data.get('test_type') == 'perf':
    results_data['good_mean'] = None
    results_data['bad_mean'] = None
    for r in results_data.get('revision_data', []):
      if r.get('commit_hash') == results_data.get('good_revision'):
        results_data['good_mean'] = r.get('mean_value')
      if r.get('commit_hash') == results_data.get('bad_revision'):
        results_data['bad_mean'] = r.get('mean_value')
    if results_data['good_mean'] and results_data['bad_mean']:
      result += _BISECT_DETAILS_CHANGE % results_data

  # If we're unable to narrow for whatever reason, try to print out a link to
  # a log containing all entries in the suspected range.
  if message == STATUS_REPRO_UNABLE_NARROW:
    depot_name = lkgr.get('depot_name')

    repositories = namespaced_stored_object.Get(_REPOSITORIES_KEY)
    depot_url = repositories.get(depot_name, {}).get('repository_url')
    result += _BISECT_SUSPECTED_RANGE % {'num': fkbr_index - lkgr_index}
    if depot_url and lkgr.get('depot_name') == fkbr.get('depot_name'):
      git_url = depot_url + '/+log/'
      result += _BISECT_SUSPECTED_RANGE_URL % {
          'url': git_url,
          'lkgr': lkgr.get('commit_hash'),
          'fkbr': fkbr.get('commit_hash')}
    elif not depot_url:
      result += _BISECT_SUSPECTED_RANGE_UNSUPPORTED
    else:
      result += _BISECT_SUSPECTED_RANGE_MISMATCH % {
          'lkgr': '%s@%s' % (
              lkgr.get('depot_name'), lkgr.get('commit_hash')),
          'fkbr': '%s@%s' % (
              fkbr.get('depot_name'), fkbr.get('commit_hash'))}
    result += '\n'

  # Print out a nice table of all the tested commits.
  if results_data.get('revision_data'):
    result += _RevisionTable(results_data)

  # Print out common footer stuff for all bisects, info like the command line,
  # and how to contact the team.
  result += '\n'

  # TODO(eakuefner): Replace this with a generic property in TestMetadata
  # when data pipe is available.
  # github:3690: Include doc url for benchmarks if available.
  for benchmark, benchmark_details in _BENCHMARK_DOC_URLS.iteritems():
    benchmark_names = benchmark_details['benchmarks']
    if any(results_data['benchmark'].startswith(b) for b in benchmark_names):
      result += _BENCHMARK_DOC_INFO % {
          'name': benchmark, 'url': benchmark_details['url']}

  result += _BISECT_TO_RUN % results_data
  result += _BISECT_ADDRESSING_DOC_INFO
  result += _BISECT_DEBUG_INFO % results_data
  result += '\n'
  result += _BISECT_FOOTER
  result = result.encode('utf-8')

  return result


def GetReport(try_job_entity, in_progress=False):
  """Generates a report for bisect results.

  This was ported from recipe_modules/auto_bisect/bisect_results.py.

  Args:
    try_job_entity: A TryJob entity.

  Returns:
    Bisect report string.
  """
  results_data = copy.deepcopy(try_job_entity.results_data)
  if not results_data:
    return ''

  # This is an in-progress bisect, and we want it to display a message
  # indicating so.
  if in_progress:
    results_data['status'] = STATUS_TYPE_IN_PROGRESS

  if try_job_entity.bug_id > 0:
    results_data['_tryjob_id'] = try_job_entity.key.id()

  return  _GenerateReport(results_data)


def _MakeLegacyRevisionString(r):
  result = 'chromium@' + str(r.get('commit_pos', 'unknown'))
  if r.get('depot_name', 'chromium') != 'chromium':
    result += ',%s@%s' % (r['depot_name'], r.get('deps_revision', 'unknown'))
  return result


def _RevisionTable(results_data):
  is_return_code = results_data.get('test_type') == 'return_code'
  culprit_commit_hash = None
  if 'culprit_data' in results_data and results_data['culprit_data']:
    culprit_commit_hash = results_data['culprit_data']['cl']

  # Only display some rows depending on whether they're part of the failure or
  # regression.
  last_good = 0
  first_bad = len(results_data['revision_data'])
  for i in xrange(len(results_data['revision_data'])):
    r = results_data['revision_data'][i]
    if r['result'] == 'good':
      last_good = i
    if r['result'] == 'bad':
      first_bad = i
      break

  revision_rows = []
  for i in xrange(len(results_data['revision_data'])):
    r = results_data['revision_data'][i]

    number_of_observations = r.get(
        'n_observations', len(r.get('values', [])) or None)
    result = None
    if not r.get('failed') and number_of_observations:
      result = [
          r.get('revision_string', _MakeLegacyRevisionString(r)),
          '%s +- %s' % (
              _FormatNumber(r['mean_value']),
              _FormatNumber(r['std_dev'])),
          _FormatNumber(number_of_observations),
          r['result'],
          '<--' if r['commit_hash'] == culprit_commit_hash  else '',
      ]
    elif r.get('failed'):
      # Outside the culprit range we don't care about displaying build failures.
      if i > last_good and i < first_bad:
        if first_bad - last_good > 10:
          if i == last_good + 1 or i == first_bad - 1:
            result = [
                r.get('revision_string', _MakeLegacyRevisionString(r)),
                '---',
                '---',
                'build failure',
                '',
            ]
          elif i == last_good + 2:
            # Inside the culprit range, if there were more than 10 failures,
            # just mention they all failed.
            result = [
                '---',
                '---',
                '---',
                'too many build failures to list',
                '',
            ]
        else:
          result = [
              r.get('revision_string', _MakeLegacyRevisionString(r)),
              '---',
              '---',
              'build failure',
              '',
          ]
    if result:
      revision_rows.append(result)

  revision_rows = [map(str, r) for r in revision_rows if r]
  if not revision_rows:
    return ''

  headers_row = [[
      'Revision',
      'Result' if not is_return_code else 'Exit Code',
      'N',
      '',
      '',
  ]]
  all_rows = headers_row + revision_rows
  return _REVISION_TABLE_TEMPLATE % {'table': _PrettyTable(all_rows)}


def _FormatNumber(x):
  if x is None:
    return 'N/A'
  if isinstance(x, int) or x == 0:
    return str(x)

  if x >= 10**5:
    # It's a little awkward to round 123456789.987 to 123457000.0,
    # so just make it 123456790.
    return str(int(round(x)))
  # Round to 6 significant figures.
  return str(round(x, 5-int(math.floor(math.log10(abs(x))))))


def _PrettyTable(data):
  column_lengths = [max(map(len, c)) for c in zip(*data)]
  formatted_rows = []
  for row in data:
    formatted_elements = []
    for element_length, element in zip(column_lengths, row):
      formatted_elements.append(element.ljust(element_length))
    formatted_rows.append('      '.join(formatted_elements).strip())
  return '\n'.join(formatted_rows)
