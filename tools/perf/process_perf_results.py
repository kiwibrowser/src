#!/usr/bin/env vpython
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import os
import shutil
import sys
import tempfile
import uuid

from core import oauth_api
from core import path_util
from core import upload_results_to_perf_dashboard
from core import results_merger
from os import listdir
from os.path import isfile, join, basename

path_util.AddAndroidPylibToPath()

try:
  from pylib.utils import logdog_helper
except ImportError:
  pass


RESULTS_URL = 'https://chromeperf.appspot.com'

# Until we are migrated to LUCI, we will be utilizing a hard
# coded master name based on what is passed in in the build properties.
# See crbug.com/801289 for more details.
MACHINE_GROUP_JSON_FILE = os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'perf', 'core',
      'perf_dashboard_machine_group_mapping.json')

def _GetMachineGroup(build_properties):
  machine_group = None
  if build_properties.get('perf_dashboard_machine_group', False):
    # Once luci migration is complete this will exist as a property
    # in the build properties
    machine_group =  build_properties['perf_dashboard_machine_group']
  else:
    mastername_mapping = {}
    with open(MACHINE_GROUP_JSON_FILE) as fp:
      mastername_mapping = json.load(fp)
      legacy_mastername = build_properties['mastername']
      if mastername_mapping.get(legacy_mastername):
        machine_group = mastername_mapping[legacy_mastername]
  if not machine_group:
    raise ValueError(
        'Must set perf_dashboard_machine_group or have a valid '
        'mapping in '
        'src/tools/perf/core/perf_dashboard_machine_group_mapping.json'
        'See bit.ly/perf-dashboard-machine-group for more details')
  return machine_group


def _upload_perf_results(json_to_upload, name, configuration_name,
    build_properties, oauth_file, tmp_dir, output_json_file):
  """Upload the contents of result JSON(s) to the perf dashboard."""
  args = [
      '--tmp-dir', tmp_dir,
      '--buildername', build_properties['buildername'],
      '--buildnumber', build_properties['buildnumber'],
      '--name', name,
      '--configuration-name', configuration_name,
      '--results-file', json_to_upload,
      '--results-url', RESULTS_URL,
      '--got-revision-cp', build_properties['got_revision_cp'],
      '--got-v8-revision', build_properties['got_v8_revision'],
      '--got-webrtc-revision', build_properties['got_webrtc_revision'],
      '--oauth-token-file', oauth_file,
      '--output-json-file', output_json_file,
      '--perf-dashboard-machine-group', _GetMachineGroup(build_properties)
  ]
  if build_properties.get('git_revision'):
    args.append('--git-revision')
    args.append(build_properties['git_revision'])
  if _is_histogram(json_to_upload):
    args.append('--send-as-histograms')

  return upload_results_to_perf_dashboard.main(args)


def _is_histogram(json_file):
  with open(json_file) as f:
    data = json.load(f)
    return isinstance(data, list)
  return False


def _merge_json_output(output_json, jsons_to_merge, extra_links):
  """Merges the contents of one or more results JSONs.

  Args:
    output_json: A path to a JSON file to which the merged results should be
      written.
    jsons_to_merge: A list of JSON files that should be merged.
    extra_links: a (key, value) map in which keys are the human-readable strings
      which describe the data, and value is logdog url that contain the data.
  """
  merged_results = results_merger.merge_test_results(jsons_to_merge)

  # Only append the perf results links if present
  if extra_links:
    merged_results['links'] = extra_links

  with open(output_json, 'w') as f:
    json.dump(merged_results, f)

  return 0


def _handle_perf_json_test_results(
    benchmark_directory_list, test_results_list):
  benchmark_enabled_map = {}
  for directory in benchmark_directory_list:
    # Obtain the test name we are running
    benchmark_name = _get_benchmark_name(directory)
    is_ref = '.reference' in benchmark_name
    enabled = True
    with open(join(directory, 'test_results.json')) as json_data:
      json_results = json.load(json_data)
      if not json_results:
        # Output is null meaning the test didn't produce any results.
        # Want to output an error and continue loading the rest of the
        # test results.
        print 'No results produced for %s, skipping upload' % directory
        continue
      if json_results.get('version') == 3:
        # Non-telemetry tests don't have written json results but
        # if they are executing then they are enabled and will generate
        # chartjson results.
        if not bool(json_results.get('tests')):
          enabled = False
      if not is_ref:
        # We don't need to upload reference build data to the
        # flakiness dashboard since we don't monitor the ref build
        test_results_list.append(json_results)
    if not enabled:
      # We don't upload disabled benchmarks or tests that are run
      # as a smoke test
      print 'Benchmark %s disabled' % benchmark_name
    benchmark_enabled_map[benchmark_name] = enabled

  return benchmark_enabled_map


def _generate_unique_logdog_filename(name_prefix):
  return name_prefix + '_' + str(uuid.uuid4())


def _handle_perf_logs(benchmark_directory_list, extra_links):
  """ Upload benchmark logs to logdog and add a page entry for them. """

  benchmark_logs_links = {}

  for directory in benchmark_directory_list:
    # Obtain the test name we are running
    benchmark_name = _get_benchmark_name(directory)
    with open(join(directory, 'benchmark_log.txt')) as f:
      uploaded_link = logdog_helper.text(
          name=_generate_unique_logdog_filename(benchmark_name),
          data=f.read())
      benchmark_logs_links[benchmark_name] = uploaded_link

  logdog_file_name = _generate_unique_logdog_filename('Benchmarks_Logs')
  logdog_stream = logdog_helper.text(
      logdog_file_name, json.dumps(benchmark_logs_links, sort_keys=True,
                                   indent=4, separators=(',', ': ')))
  extra_links['Benchmarks logs'] = logdog_stream



def _get_benchmark_name(directory):
  return basename(directory).replace(" benchmark", "")


def _process_perf_results(output_json, configuration_name,
                          service_account_file,
                          build_properties, task_output_dir,
                          smoke_test_mode):
  """Process perf results.

  Consists of merging the json-test-format output, uploading the perf test
  output (chartjson and histogram), and store the benchmark logs in logdog.

  Each directory in the task_output_dir represents one benchmark
  that was run. Within this directory, there is a subdirectory with the name
  of the benchmark that was run. In that subdirectory, there is a
  perftest-output.json file containing the performance results in histogram
  or dashboard json format and an output.json file containing the json test
  results for the benchmark.
  """
  return_code = 0
  directory_list = [
      f for f in listdir(task_output_dir)
      if not isfile(join(task_output_dir, f))
  ]
  benchmark_directory_list = []
  for directory in directory_list:
    benchmark_directory_list += [
      join(task_output_dir, directory, f)
      for f in listdir(join(task_output_dir, directory))
    ]

  test_results_list = []

  build_properties = json.loads(build_properties)
  if not configuration_name:
    # we are deprecating perf-id crbug.com/817823
    configuration_name = build_properties['buildername']

  extra_links = {}

  # First, upload all the benchmark logs to logdog and add a page entry for
  # those links in extra_links.
  _handle_perf_logs(benchmark_directory_list, extra_links)

  # Then try to obtain the list of json test results to merge
  # and determine the status of each benchmark.
  benchmark_enabled_map = _handle_perf_json_test_results(
      benchmark_directory_list, test_results_list)

  if not smoke_test_mode:
    return_code = _handle_perf_results(
        benchmark_enabled_map, benchmark_directory_list,
        configuration_name, build_properties, service_account_file, extra_links)

  # Finally, merge all test results json, add the extra links and write out to
  # output location
  _merge_json_output(output_json, test_results_list, extra_links)
  return return_code


def _handle_perf_results(
    benchmark_enabled_map, benchmark_directory_list, configuration_name,
    build_properties, service_account_file, extra_links):
  """
    Upload perf results to the perf dashboard.

    This method also upload the perf results to logdog and augment it to
    |extra_links|.

    Returns:
      0 if this upload to perf dashboard succesfully, 1 otherwise.
  """
  tmpfile_dir = tempfile.mkdtemp('resultscache')
  try:
    # Upload all eligible benchmarks to the perf dashboard
    logdog_dict = {}
    logdog_stream = None
    logdog_label = 'Results Dashboard'
    upload_fail = False
    with oauth_api.with_access_token(service_account_file) as oauth_file:
      for directory in benchmark_directory_list:
        benchmark_name = _get_benchmark_name(directory)
        if not benchmark_enabled_map[benchmark_name]:
          continue
        print 'Uploading perf results from %s benchmark' % benchmark_name
        upload_fail = _upload_and_write_perf_data_to_logfile(
            benchmark_name, directory, configuration_name, build_properties,
            oauth_file, tmpfile_dir, logdog_dict,
            ('.reference' in benchmark_name))

    logdog_file_name = _generate_unique_logdog_filename('Results_Dashboard_')
    logdog_stream = logdog_helper.text(logdog_file_name,
        json.dumps(logdog_dict, sort_keys=True,
            indent=4, separators=(',', ': ')))
    if upload_fail:
      logdog_label += ' Upload Failure'
    extra_links[logdog_label] = logdog_stream
    if upload_fail:
      return 1
    return 0
  finally:
    shutil.rmtree(tmpfile_dir)


def _upload_and_write_perf_data_to_logfile(benchmark_name, directory,
    configuration_name, build_properties, oauth_file,
    tmpfile_dir, logdog_dict, is_ref):
  upload_failure = False
  # logdog file to write perf results to
  output_json_file = logdog_helper.open_text(benchmark_name)

  # upload results and write perf results to logdog file
  upload_failure = _upload_perf_results(
    join(directory, 'perf_results.json'),
    benchmark_name, configuration_name, build_properties,
    oauth_file, tmpfile_dir, output_json_file)

  output_json_file.close()

  base_benchmark_name = benchmark_name.replace('.reference', '')

  if base_benchmark_name not in logdog_dict:
    logdog_dict[base_benchmark_name] = {}

  # add links for the perf results and the dashboard url to
  # the logs section of buildbot
  if is_ref:
    logdog_dict[base_benchmark_name]['perf_results_ref'] = \
        output_json_file.get_viewer_url()
  else:
    if upload_failure:
      logdog_dict[base_benchmark_name]['dashboard_url'] = \
          'upload failed'
    else:
      logdog_dict[base_benchmark_name]['dashboard_url'] = \
          upload_results_to_perf_dashboard.GetDashboardUrl(
              benchmark_name,
              configuration_name, RESULTS_URL,
              build_properties['got_revision_cp'],
              _GetMachineGroup(build_properties))
    logdog_dict[base_benchmark_name]['perf_results'] = \
        output_json_file.get_viewer_url()

  return upload_failure


def main():
  """ See collect_task.collect_task for more on the merge script API. """
  parser = argparse.ArgumentParser()
  # configuration-name (previously perf-id) is the name of bot the tests run on
  # For example, buildbot-test is the name of the obbs_fyi bot
  # configuration-name and results-url are set in the json file which is going
  # away tools/perf/core/chromium.perf.fyi.extras.json
  parser.add_argument('--configuration-name', help=argparse.SUPPRESS)
  parser.add_argument('--service-account-file', help=argparse.SUPPRESS)

  parser.add_argument('--build-properties', help=argparse.SUPPRESS)
  parser.add_argument('--summary-json', help=argparse.SUPPRESS)
  parser.add_argument('--task-output-dir', help=argparse.SUPPRESS)
  parser.add_argument('-o', '--output-json', required=True,
                      help=argparse.SUPPRESS)
  parser.add_argument('json_files', nargs='*', help=argparse.SUPPRESS)
  parser.add_argument('--smoke-test-mode', action='store_true',
                      help='This test should be run in smoke test mode'
                      ' meaning it does not upload to the perf dashboard')

  args = parser.parse_args()

  if not args.service_account_file and not args.smoke_test_mode:
    raise Exception(
        'Service account file must be specificed for dashboard upload')

  return _process_perf_results(
      args.output_json, args.configuration_name,
      args.service_account_file,
      args.build_properties, args.task_output_dir,
      args.smoke_test_mode)


if __name__ == '__main__':
  sys.exit(main())
