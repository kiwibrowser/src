# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys

from gpu_tests import gpu_integration_test
from gpu_tests import path_util
from gpu_tests import pixel_test_pages
from gpu_tests import trace_test_expectations

from telemetry.timeline import model as model_module
from telemetry.timeline import tracing_config

TOPLEVEL_GL_CATEGORY = 'gpu_toplevel'
TOPLEVEL_SERVICE_CATEGORY = 'disabled-by-default-gpu.service'
TOPLEVEL_DEVICE_CATEGORY = 'disabled-by-default-gpu.device'
TOPLEVEL_CATEGORIES = [TOPLEVEL_SERVICE_CATEGORY, TOPLEVEL_DEVICE_CATEGORY]

gpu_relative_path = "content/test/data/gpu/"

data_paths = [os.path.join(
                  path_util.GetChromiumSrcDir(), gpu_relative_path),
              os.path.join(
                  path_util.GetChromiumSrcDir(), 'media', 'test', 'data')]

test_harness_script = r"""
  var domAutomationController = {};

  domAutomationController._finished = false;

  domAutomationController.send = function(msg) {
    // Issue a read pixel to synchronize the gpu process to ensure
    // the asynchronous category enabling is finished.
    var temp_canvas = document.createElement("canvas")
    temp_canvas.width = 1;
    temp_canvas.height = 1;
    var temp_gl = temp_canvas.getContext("experimental-webgl") ||
                  temp_canvas.getContext("webgl");
    if (temp_gl) {
      temp_gl.clear(temp_gl.COLOR_BUFFER_BIT);
      var id = new Uint8Array(4);
      temp_gl.readPixels(0, 0, 1, 1, temp_gl.RGBA, temp_gl.UNSIGNED_BYTE, id);
    } else {
      console.log('Failed to get WebGL context.');
    }

    domAutomationController._finished = true;
  }

  window.domAutomationController = domAutomationController;
"""

class TraceIntegrationTest(gpu_integration_test.GpuIntegrationTest):
  """Tests GPU traces are plumbed through properly.

  Also tests that GPU Device traces show up on devices that support them."""

  @classmethod
  def Name(cls):
    return 'trace_test'

  @classmethod
  def GenerateGpuTests(cls, options):
    # Include the device level trace tests, even though they're
    # currently skipped on all platforms, to give a hint that they
    # should perhaps be enabled in the future.
    for p in pixel_test_pages.DefaultPages('TraceTest'):
      yield (p.name, gpu_relative_path + p.url, (TOPLEVEL_SERVICE_CATEGORY))
    for p in pixel_test_pages.DefaultPages('DeviceTraceTest'):
      yield (p.name, gpu_relative_path + p.url, (TOPLEVEL_DEVICE_CATEGORY))

  def RunActualGpuTest(self, test_path, *args):
    # The version of this test in the old GPU test harness restarted
    # the browser after each test, so continue to do that to match its
    # behavior.
    self._RestartBrowser('Restarting browser to ensure clean traces')

    # Set up tracing.
    config = tracing_config.TracingConfig()
    config.chrome_trace_config.category_filter.AddExcludedCategory('*')
    for cat in TOPLEVEL_CATEGORIES:
      config.chrome_trace_config.category_filter.AddDisabledByDefault(cat)
    config.enable_chrome_trace = True
    tab = self.tab
    tab.browser.platform.tracing_controller.StartTracing(config, 60)

    # Perform page navigation.
    url = self.UrlOfStaticFilePath(test_path)
    tab.Navigate(url, script_to_evaluate_on_commit=test_harness_script)
    tab.action_runner.WaitForJavaScriptCondition(
      'domAutomationController._finished', timeout=30)

    # Stop tracing.
    timeline_data = tab.browser.platform.tracing_controller.StopTracing()[0]

    # Evaluate success.
    timeline_model = model_module.TimelineModel(timeline_data)
    category_name = args[0]
    event_iter = timeline_model.IterAllEvents(
        event_type_predicate=model_module.IsSliceOrAsyncSlice)
    for event in event_iter:
      if (event.args.get('gl_category', None) == TOPLEVEL_GL_CATEGORY and
          event.category == category_name):
        print 'Found event with category name ' + category_name
        break
    else:
      self.fail(self._FormatException(category_name))

  def _FormatException(self, category):
    return 'Trace markers for GPU category were not found: %s' % category

  @classmethod
  def _CreateExpectations(cls):
    return trace_test_expectations.TraceTestExpectations()

  @classmethod
  def SetUpProcess(cls):
    super(TraceIntegrationTest, cls).SetUpProcess()
    path_util.SetupTelemetryPaths()
    cls.CustomizeBrowserArgs([
      '--enable-logging',
      '--enable-experimental-web-platform-features'])
    cls.StartBrowser()
    cls.SetStaticServerDirs(data_paths)

def load_tests(loader, tests, pattern):
  del loader, tests, pattern  # Unused.
  return gpu_integration_test.LoadAllTestsInModule(sys.modules[__name__])
