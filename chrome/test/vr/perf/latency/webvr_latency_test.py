# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import motopho_thread as mt
import robot_arm as ra

import json
import glob
import logging
import numpy
import os
import re
import subprocess
import sys
import time


MOTOPHO_THREAD_TIMEOUT = 15
MOTOPHO_THREAD_TERMINATION_TIMEOUT = 2
MOTOPHO_THREAD_RETRIES = 4
DEFAULT_URLS = [
    # URLs that render 3D scenes in addition to the Motopho patch.
    # Heavy CPU load, moderate GPU load.
    'https://webvr.info/samples/test-slow-render.html?'
    'latencyPatch=1\&canvasClickPresents=1\&'
    'heavyGpu=1\&workTime=20\&cubeCount=8\&cubeScale=0.4',
    # No additional CPU load, very light GPU load.
    'https://webvr.info/samples/test-slow-render.html?'
    'latencyPatch=1\&canvasClickPresents=1',
    # Increased render scale
    'https://webvr.info/samples/test-slow-render.html?'
    'latencyPatch=1\&canvasClickPresents=1\&'
    'renderScale=1.5',
    # Default render scale, increased load
    'https://webvr.info/samples/test-slow-render.html?'
    'latencyPatch=1\&canvasClickPresents=1\&'
    'renderScale=1\&heavyGpu=1\&cubeScale=0.3\&workTime=10',
]


def GetTtyDevices(tty_pattern, vendor_ids):
  """Finds all devices connected to tty that match a pattern and device id.

  If a serial device is connected to the computer via USB, this function
  will check all tty devices that match tty_pattern, and return the ones
  that have vendor identification number in the list vendor_ids.

  Args:
    tty_pattern: The search pattern, such as r'ttyACM\d+'.
    vendor_ids: The list of 16-bit USB vendor ids, such as [0x2a03].

  Returns:
    A list of strings of tty devices, for example ['ttyACM0'].
  """
  product_string = 'PRODUCT='
  sys_class_dir = '/sys/class/tty/'

  tty_devices = glob.glob(sys_class_dir + '*')

  matcher = re.compile('.*' + tty_pattern)
  tty_matches = [x for x in tty_devices if matcher.search(x)]
  tty_matches = [x[len(sys_class_dir):] for x in tty_matches]

  found_devices = []
  for match in tty_matches:
    class_filename = sys_class_dir + match + '/device/uevent'
    with open(class_filename, 'r') as uevent_file:
      # Look for the desired product id in the uevent text.
      for line in uevent_file:
        if product_string in line:
          ids = line[len(product_string):].split('/')
          ids = [int(x, 16) for x in ids]

          for desired_id in vendor_ids:
            if desired_id in ids:
              found_devices.append(match)

  return found_devices


class WebVrLatencyTest(object):
  """Base class for all WebVR latency tests.

  This handles the platform-independent _Run and _SaveResultsToFile functions.
  Platform-specific setup and teardown should be somehow handled by classes
  that inherit from this one.
  """
  def __init__(self, args):
    super(WebVrLatencyTest, self).__init__(args)
    self._num_samples = args.num_samples
    self._test_urls = args.urls or DEFAULT_URLS
    assert (self._num_samples > 0),'Number of samples must be greater than 0'
    self._test_results = {}
    self._test_name = 'vr_perf.motopho_latency'

    # Connect to the Arduino that drives the servos.
    devices = GetTtyDevices(r'ttyACM\d+', [0x2a03, 0x2341])
    assert (len(devices) == 1),'Found %d devices, expected 1' % len(devices)
    self.robot_arm = ra.RobotArm(devices[0])

  def _Run(self, url):
    """Run the latency test.

    Handles the actual latency measurement, which is identical across
    different platforms, as well as result storing.
    """
    # Motopho scripts use relative paths, so switch to the Motopho directory.
    os.chdir(self._args.motopho_path)

    # Set up the thread that runs the Motopho script.
    motopho_thread = mt.MotophoThread()
    motopho_thread.start()

    # Run multiple times so we can get an average and standard deviation.
    num_retries = 0
    samples_obtained = 0
    while samples_obtained < self._num_samples:
      self.robot_arm.ResetPosition()
      # Start the Motopho script.
      motopho_thread.StartIteration()
      # Let the Motopho be stationary so the script can calculate the bias.
      time.sleep(3)
      motopho_thread.BlockNextIteration()
      # Move so we can measure latency.
      self.robot_arm.StartMotophoMovement()
      if not motopho_thread.WaitForIterationEnd(MOTOPHO_THREAD_TIMEOUT):
        # TODO(bsheedy): Look into ways to prevent Motopho from not sending any
        # data until unplugged and replugged into the machine after a reboot.
        logging.error('Motopho thread timeout, '
                      'Motopho may need to be replugged.')

      self.robot_arm.StopAllMovement()

      if motopho_thread.failed_iteration:
        num_retries += 1
        if num_retries > MOTOPHO_THREAD_RETRIES:
          self._ReportSummaryResult(False, url)
          # Raising an exception with another thread still alive causes the
          # test to hang until the swarming timeout is hit, so kill the thread
          # before raising.
          motopho_thread.Terminate()
          motopho_thread.join(MOTOPHO_THREAD_TERMINATION_TIMEOUT)
          raise RuntimeError(
              'Motopho thread failed more than %d times, aborting' % (
                  MOTOPHO_THREAD_RETRIES))
        logging.warning('Motopho thread failed, retrying iteration')
      else:
        samples_obtained += 1
      time.sleep(1)
    self._ReportSummaryResult(True, url)
    self._StoreResults(motopho_thread.latencies, motopho_thread.correlations,
                       url)
    # Leaving old threads around shouldn't cause issues, but clean up just in
    # case
    motopho_thread.Terminate()
    motopho_thread.join(MOTOPHO_THREAD_TERMINATION_TIMEOUT)
    if motopho_thread.isAlive():
      logging.warning('Motopho thread failed to terminate.')

  def _ReportSummaryResult(self, passed, url):
    """Stores pass/fail results for the summary output JSON file.

    Args:
      passed: Boolean, whether the test passed or not
      url: The URL that was being tested
    """
    self._results_summary[url] = {
        'actual': 'PASS' if passed else 'FAIL',
        'expected': 'PASS',
        'is_unexpected': not passed,
    }

  def _StoreResults(self, latencies, correlations, url):
    """Temporarily stores the results of a test.

    Stores the given results in memory to be later retrieved and written to
    a file in _SaveResultsToFile once all tests are done. Also logs the raw
    data and its average/standard deviation.
    """
    avg_latency = sum(latencies) / len(latencies)
    std_latency = numpy.std(latencies)
    avg_correlation = sum(correlations) / len(correlations)
    std_correlation = numpy.std(correlations)
    logging.info('\nURL: %s\n'
                 'Raw latencies: %s\nRaw correlations: %s\n'
                 'Avg latency: %f +/- %f\nAvg correlation: %f +/- %f',
                 url, str(latencies), str(correlations), avg_latency,
                 std_latency, avg_correlation, std_correlation)

    self._test_results[url] = {
        'correlations': correlations,
        'std_correlation': std_correlation,
        'latencies': latencies,
        'std_latency': std_latency,
    }

  def _SaveResultsToFile(self):
    outpath = None
    if (hasattr(self._args, 'isolated_script_test_perf_output') and
        self._args.isolated_script_test_perf_output):
      outpath = self._args.isolated_script_test_perf_output
    elif (hasattr(self._args, 'isolated_script_test_chartjson_output') and
        self._args.isolated_script_test_chartjson_output):
      outpath = self._args.isolated_script_test_chartjson_output
    else:
      logging.warning('No output file set, not saving results to file')
      return

    charts = {
        'correlation': {
            'summary': {
                'improvement_direction': 'up',
                'name': 'correlation',
                'std': 0.0,
                'type': 'list_of_scalar_values',
                'units': '',
                'values': [],
            }
        },
        'latency': {
            'summary': {
                'improvement_direction': 'down',
                'name': 'latency',
                'std': 0.0,
                'type': 'list_of_scalar_values',
                'units': 'ms',
                'values': [],
            }
        }
    }
    for url, results in self._test_results.iteritems():
      charts['correlation'][url] = {
          'improvement_direction': 'up',
          'name': 'correlation',
          'std': results['std_correlation'],
          'type': 'list_of_scalar_values',
          'units': '',
          'values': results['correlations'],
      }

      charts['correlation']['summary']['values'].extend(
          results['correlations'])

      charts['latency'][url] = {
          'improvement_direction': 'down',
          'name': 'latency',
          'std': results['std_latency'],
          'type': 'list_of_scalar_values',
          'units': 'ms',
          'values': results['latencies'],
      }

      charts['latency']['summary']['values'].extend(results['latencies'])

    results = {
      'format_version': '1.0',
      'benchmark_name': self._test_name,
      'benchmark_description': 'Measures the motion-to-photon latency of WebVR',
      'charts': charts,
    }

    with file(outpath, 'w') as outfile:
      json.dump(results, outfile)
