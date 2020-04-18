#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script runs an automated Cronet performance benchmark.

This script:
1. Sets up "USB reverse tethering" which allow network traffic to flow from
   an Android device connected to the host machine via a USB cable.
2. Starts HTTP and QUIC servers on the host machine.
3. Installs an Android app on the attached Android device and runs it.
4. Collects the results from the app.

Prerequisites:
1. A rooted (i.e. "adb root" succeeds) Android device connected via a USB cable
   to the host machine (i.e. the computer running this script).
2. quic_server has been built for the host machine, e.g. via:
     gn gen out/Release --args="is_debug=false"
     ninja -C out/Release quic_server
3. cronet_perf_test_apk has been built for the Android device, e.g. via:
     ./components/cronet/tools/cr_cronet.py gn -r
     ninja -C out/Release cronet_perf_test_apk
4. If "sudo ufw status" doesn't say "Status: inactive", run "sudo ufw disable".
5. sudo apt-get install lighttpd
6. If the usb0 interface on the host keeps losing it's IPv4 address
   (WaitFor(HasHostAddress) will keep failing), NetworkManager may need to be
   told to leave usb0 alone with these commands:
     sudo bash -c "printf \"\\n[keyfile]\
         \\nunmanaged-devices=interface-name:usb0\\n\" \
         >> /etc/NetworkManager/NetworkManager.conf"
     sudo service network-manager restart

Invocation:
./run.py

Output:
Benchmark timings are output by telemetry to stdout and written to
./results.html

"""

import logging
import json
import optparse
import os
import posixpath
import shutil
import subprocess
import sys
import tempfile
from time import sleep
import urllib

REPOSITORY_ROOT = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..', '..', '..'))

sys.path.append(os.path.join(REPOSITORY_ROOT, 'tools', 'perf'))
from core import path_util
sys.path.append(path_util.GetTelemetryDir())
sys.path.append(os.path.join(REPOSITORY_ROOT, 'build', 'android'))
sys.path.append(os.path.join(
    REPOSITORY_ROOT, 'third_party', 'catapult', 'devil'))

import android_rndis_forwarder
from chrome_telemetry_build import chromium_config
from devil.android import device_utils
from devil.android.sdk import intent
import lighttpd_server
from pylib import constants
from telemetry import android
from telemetry import benchmark
from telemetry import benchmark_runner
from telemetry import story
from telemetry.value import scalar
from telemetry.web_perf import timeline_based_measurement

BUILD_TYPE = 'Release'
BUILD_DIR = os.path.join(REPOSITORY_ROOT, 'out', BUILD_TYPE)
QUIC_SERVER = os.path.join(BUILD_DIR, 'quic_server')
CERT_PATH = os.path.join('net', 'data', 'ssl', 'certificates')
QUIC_CERT_DIR = os.path.join(REPOSITORY_ROOT, CERT_PATH)
QUIC_CERT_HOST = 'test.example.com'
QUIC_CERT_FILENAME = 'quic-chain.pem'
QUIC_CERT = os.path.join(QUIC_CERT_DIR, QUIC_CERT_FILENAME)
QUIC_KEY = os.path.join(QUIC_CERT_DIR, 'quic-leaf-cert.key')
APP_APK = os.path.join(BUILD_DIR, 'apks', 'CronetPerfTest.apk')
APP_PACKAGE = 'org.chromium.net'
APP_ACTIVITY = '.CronetPerfTestActivity'
APP_ACTION = 'android.intent.action.MAIN'
BENCHMARK_CONFIG = {
  # Control various metric recording for further investigation.
  'CAPTURE_NETLOG': False,
  'CAPTURE_TRACE': False,
  'CAPTURE_SAMPLED_TRACE': False,
  # While running Cronet Async API benchmarks, indicate if callbacks should be
  # run on network thread rather than posted back to caller thread.  This allows
  # measuring if thread-hopping overhead is significant.
  'CRONET_ASYNC_USE_NETWORK_THREAD': False,
  # A small resource for device to fetch from host.
  'SMALL_RESOURCE': 'small.html',
  'SMALL_RESOURCE_SIZE': 26,
  # Number of times to fetch SMALL_RESOURCE.
  'SMALL_ITERATIONS': 1000,
  # A large resource for device to fetch from host.
  'LARGE_RESOURCE': 'large.html',
  'LARGE_RESOURCE_SIZE': 10000026,
  # Number of times to fetch LARGE_RESOURCE.
  'LARGE_ITERATIONS': 4,
  # An on-device file containing benchmark timings.  Written by benchmark app.
  'RESULTS_FILE': '/data/data/' + APP_PACKAGE + '/results.txt',
  # An on-device file whose presence indicates benchmark app has terminated.
  'DONE_FILE': '/data/data/' + APP_PACKAGE + '/done.txt',
  # Ports of HTTP and QUIC servers on host.
  'HTTP_PORT': 9000,
  'QUIC_PORT': 9001,
  # Maximum read/write buffer size to use.
  'MAX_BUFFER_SIZE': 16384,
  'HOST': QUIC_CERT_HOST,
  'QUIC_CERT_FILE': QUIC_CERT_FILENAME,
}
# Add benchmark config to global state for easy access.
globals().update(BENCHMARK_CONFIG)
# Pylint doesn't really interpret the file, so it won't find the definitions
# added from BENCHMARK_CONFIG, so suppress the undefined variable warning.
#pylint: disable=undefined-variable

def GetDevice():
  devices = device_utils.DeviceUtils.HealthyDevices()
  assert len(devices) == 1
  return devices[0]


def GetAndroidRndisConfig(device):
  return android_rndis_forwarder.AndroidRndisConfigurator(device)


def GetServersHost(device):
  return GetAndroidRndisConfig(device).host_ip


def GetHttpServerURL(device, resource):
  return 'http://%s:%d/%s' % (GetServersHost(device), HTTP_PORT, resource)


class CronetPerfTestAndroidStory(android.AndroidStory):
  # Android AppStory implementation wrapping CronetPerfTest app.
  # Launches Cronet perf test app and waits for execution to complete
  # by waiting for presence of DONE_FILE.

  def __init__(self, device):
    self._device = device
    device.RemovePath(DONE_FILE, force=True)
    config = BENCHMARK_CONFIG
    config['HOST_IP'] = GetServersHost(device)
    self.url ='http://dummy/?'+urllib.urlencode(config)
    start_intent = intent.Intent(
        package=APP_PACKAGE,
        activity=APP_ACTIVITY,
        action=APP_ACTION,
        # |config| maps from configuration value names to the configured values.
        # |config| is encoded as URL parameter names and values and passed to
        # the Cronet perf test app via the Intent data field.
        data=self.url,
        extras=None,
        category=None)
    super(CronetPerfTestAndroidStory, self).__init__(
        start_intent, name='CronetPerfTest',
        # No reason to wait for app; Run() will wait for results.  By default
        # StartActivity will timeout waiting for CronetPerfTest, so override
        # |is_app_ready_predicate| to not wait.
        is_app_ready_predicate=lambda app: True)

  def Run(self, shared_user_story_state):
    while not self._device.FileExists(DONE_FILE):
      sleep(1.0)


class CronetPerfTestStorySet(story.StorySet):

  def __init__(self, device):
    super(CronetPerfTestStorySet, self).__init__()
    # Create and add Cronet perf test AndroidStory.
    self.AddStory(CronetPerfTestAndroidStory(device))


class CronetPerfTestMeasurement(
    timeline_based_measurement.TimelineBasedMeasurement):
  # For now AndroidStory's SharedAppState works only with
  # TimelineBasedMeasurements, so implement one that just forwards results from
  # Cronet perf test app.

  def __init__(self, device, options):
    super(CronetPerfTestMeasurement, self).__init__(options)
    self._device = device

  def WillRunStory(self, platform):
    # Skip parent implementation which doesn't apply to Cronet perf test app as
    # it is not a browser with a timeline interface.
    pass

  def Measure(self, platform, results):
    # Reads results from |RESULTS_FILE| on target and adds to |results|.
    jsonResults = json.loads(self._device.ReadFile(RESULTS_FILE))
    for test in jsonResults:
      results.AddValue(scalar.ScalarValue(results.current_page, test,
          'ms', jsonResults[test]))

  def DidRunStory(self, platform, results):
    # Skip parent implementation which calls into tracing_controller which this
    # doesn't have.
    pass


class CronetPerfTestBenchmark(benchmark.Benchmark):
  # Benchmark implementation spawning off Cronet perf test measurement and
  # StorySet.
  SUPPORTED_PLATFORMS = [story.expectations.ALL_ANDROID]

  def __init__(self, max_failures=None):
    super(CronetPerfTestBenchmark, self).__init__(max_failures)
    self._device = GetDevice()

  def CreatePageTest(self, options):
    return CronetPerfTestMeasurement(self._device, options)

  def CreateStorySet(self, options):
    return CronetPerfTestStorySet(self._device)


class QuicServer(object):

  def __init__(self, quic_server_doc_root):
    self._process = None
    self._quic_server_doc_root = quic_server_doc_root

  def StartupQuicServer(self, device):
    cmd = [QUIC_SERVER,
           '--quic_response_cache_dir=%s' % self._quic_server_doc_root,
           '--certificate_file=%s' % QUIC_CERT,
           '--key_file=%s' % QUIC_KEY,
           '--port=%d' % QUIC_PORT]
    logging.info("Starting Quic Server: %s", cmd)
    self._process = subprocess.Popen(cmd)
    assert self._process != None
    # Wait for quic_server to start serving.
    waited_s = 0
    while subprocess.call(['lsof', '-i', 'udp:%d' % QUIC_PORT, '-p',
                           '%d' % self._process.pid],
                          stdout=open(os.devnull, 'w')) != 0:
      sleep(0.1)
      waited_s += 0.1
      assert waited_s < 5, "quic_server failed to start after %fs" % waited_s
    # Push certificate to device.
    cert = open(QUIC_CERT, 'r').read()
    device_cert_path = posixpath.join(
        device.GetExternalStoragePath(), 'chromium_tests_root', CERT_PATH)
    device.RunShellCommand(['mkdir', '-p', device_cert_path], check_return=True)
    device.WriteFile(os.path.join(device_cert_path, QUIC_CERT_FILENAME), cert)

  def ShutdownQuicServer(self):
    if self._process:
      self._process.terminate()


def GenerateHttpTestResources():
  http_server_doc_root = tempfile.mkdtemp()
  # Create a small test file to serve.
  small_file_name = os.path.join(http_server_doc_root, SMALL_RESOURCE)
  small_file = open(small_file_name, 'wb')
  small_file.write('<html><body></body></html>');
  small_file.close()
  assert SMALL_RESOURCE_SIZE == os.path.getsize(small_file_name)
  # Create a large (10MB) test file to serve.
  large_file_name = os.path.join(http_server_doc_root, LARGE_RESOURCE)
  large_file = open(large_file_name, 'wb')
  large_file.write('<html><body>');
  for _ in range(0, 1000000):
    large_file.write('1234567890');
  large_file.write('</body></html>');
  large_file.close()
  assert LARGE_RESOURCE_SIZE == os.path.getsize(large_file_name)
  return http_server_doc_root


def GenerateQuicTestResources(device):
  quic_server_doc_root = tempfile.mkdtemp()
  # Use wget to build up fake QUIC in-memory cache dir for serving.
  # quic_server expects the dir/file layout that wget produces.
  for resource in [SMALL_RESOURCE, LARGE_RESOURCE]:
    assert subprocess.Popen(['wget', '-p', '-q', '--save-headers',
                             GetHttpServerURL(device, resource)],
                            cwd=quic_server_doc_root).wait() == 0
  # wget places results in host:port directory.  Adjust for QUIC port.
  os.rename(os.path.join(quic_server_doc_root,
                         "%s:%d" % (GetServersHost(device), HTTP_PORT)),
            os.path.join(quic_server_doc_root,
                         "%s:%d" % (QUIC_CERT_HOST, QUIC_PORT)))
  return quic_server_doc_root


def GenerateLighttpdConfig(config_file, http_server_doc_root, http_server):
  # Must create customized config file to allow overriding the server.bind
  # setting.
  config_file.write('server.document-root = "%s"\n' % http_server_doc_root)
  config_file.write('server.port = %d\n' % HTTP_PORT)
  # These lines are added so lighttpd_server.py's internal test succeeds.
  config_file.write('server.tag = "%s"\n' % http_server.server_tag)
  config_file.write('server.pid-file = "%s"\n' % http_server.pid_file)
  config_file.write('dir-listing.activate = "enable"\n')
  config_file.flush()


def main():
  parser = optparse.OptionParser()
  parser.add_option('--output-format', default='html',
                   help='The output format of the results file.')
  parser.add_option('--output-dir', default=None,
                   help='The directory for the output file. Default value is '
                        'the base directory of this script.')
  options, _ = parser.parse_args()
  constants.SetBuildType(BUILD_TYPE)
  # Install APK
  device = GetDevice()
  device.EnableRoot()
  device.Install(APP_APK)
  # Start USB reverse tethering.
  android_rndis_forwarder.AndroidRndisForwarder(device,
      GetAndroidRndisConfig(device))
  # Start HTTP server.
  http_server_doc_root = GenerateHttpTestResources()
  config_file = tempfile.NamedTemporaryFile()
  http_server = lighttpd_server.LighttpdServer(http_server_doc_root,
      port=HTTP_PORT, base_config_path=config_file.name)
  GenerateLighttpdConfig(config_file, http_server_doc_root, http_server)
  assert http_server.StartupHttpServer()
  config_file.close()
  # Start QUIC server.
  quic_server_doc_root = GenerateQuicTestResources(device)
  quic_server = QuicServer(quic_server_doc_root)
  quic_server.StartupQuicServer(device)
  # Launch Telemetry's benchmark_runner on CronetPerfTestBenchmark.
  # By specifying this file's directory as the benchmark directory, it will
  # allow benchmark_runner to in turn open this file up and find the
  # CronetPerfTestBenchmark class to run the benchmark.
  top_level_dir = os.path.dirname(os.path.realpath(__file__))
  expectations_file = os.path.join(top_level_dir, 'expectations.config')
  runner_config = chromium_config.ChromiumConfig(
      top_level_dir=top_level_dir,
      benchmark_dirs=[top_level_dir],
      expectations_file=expectations_file)
  sys.argv.insert(1, 'run')
  sys.argv.insert(2, 'run.CronetPerfTestBenchmark')
  sys.argv.insert(3, '--browser=any')
  sys.argv.insert(4, '--output-format=' + options.output_format)
  if options.output_dir:
    sys.argv.insert(5, '--output-dir=' + options.output_dir)
  benchmark_runner.main(runner_config)
  # Shutdown.
  quic_server.ShutdownQuicServer()
  shutil.rmtree(quic_server_doc_root)
  http_server.ShutdownHttpServer()
  shutil.rmtree(http_server_doc_root)


if __name__ == '__main__':
  main()
