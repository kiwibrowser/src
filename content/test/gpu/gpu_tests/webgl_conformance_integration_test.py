# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import sys

from gpu_tests import gpu_integration_test
from gpu_tests import path_util
from gpu_tests import webgl_conformance_expectations
from gpu_tests import webgl2_conformance_expectations
from gpu_tests import webgl_test_util

conformance_harness_script = r"""
  var testHarness = {};
  testHarness._allTestSucceeded = true;
  testHarness._messages = '';
  testHarness._failures = 0;
  testHarness._finished = false;
  testHarness._originalLog = window.console.log;

  testHarness.log = function(msg) {
    testHarness._messages += msg + "\n";
    testHarness._originalLog.apply(window.console, [msg]);
  }

  testHarness.reportResults = function(url, success, msg) {
    testHarness._allTestSucceeded = testHarness._allTestSucceeded && !!success;
    if(!success) {
      testHarness._failures++;
      if(msg) {
        testHarness.log(msg);
      }
    }
  };
  testHarness.notifyFinished = function(url) {
    testHarness._finished = true;
  };
  testHarness.navigateToPage = function(src) {
    var testFrame = document.getElementById("test-frame");
    testFrame.src = src;
  };

  window.webglTestHarness = testHarness;
  window.parent.webglTestHarness = testHarness;
  window.console.log = testHarness.log;
  window.onerror = function(message, url, line) {
    testHarness.reportResults(null, false, message);
    testHarness.notifyFinished(null);
  };
  window.quietMode = function() { return true; }
"""

extension_harness_additional_script = r"""
  window.onload = function() { window._loaded = true; }
"""

def _GenerateTestNameFromTestPath(test_path):
  return ('WebglConformance.%s' %
          test_path.replace('/', '_').replace('-', '_').
          replace('\\', '_').rpartition('.')[0].replace('.', '_'))

def _CompareVersion(version1, version2):
  ver_num1 = [int(x) for x in version1.split('.')]
  ver_num2 = [int(x) for x in version2.split('.')]
  size = min(len(ver_num1), len(ver_num2))
  return cmp(ver_num1[0:size], ver_num2[0:size])


class WebGLConformanceIntegrationTest(gpu_integration_test.GpuIntegrationTest):

  _webgl_version = None
  _is_asan = False

  @classmethod
  def Name(cls):
    return 'webgl_conformance'

  @classmethod
  def AddCommandlineArgs(cls, parser):
    parser.add_option('--webgl-conformance-version',
        help='Version of the WebGL conformance tests to run.',
        default='1.0.4')
    parser.add_option('--webgl2-only',
        help='Whether we include webgl 1 tests if version is 2.0.0 or above.',
        default='false')
    parser.add_option('--is-asan',
        help='Indicates whether currently running an ASAN build',
        action='store_true')

  @classmethod
  def GenerateGpuTests(cls, options):
    #
    # Conformance tests
    #
    test_paths = cls._ParseTests(
        '00_test_list.txt',
        options.webgl_conformance_version,
        (options.webgl2_only == 'true'),
        None)
    cls._webgl_version = [
        int(x) for x in options.webgl_conformance_version.split('.')][0]
    cls._is_asan = options.is_asan
    for test_path in test_paths:
      # generated test name cannot contain '.'
      name = _GenerateTestNameFromTestPath(test_path).replace(
          '.', '_')
      yield (name,
             os.path.join(webgl_test_util.conformance_relpath, test_path),
             ('_RunConformanceTest'))

    #
    # Extension tests
    #
    extension_tests = cls._GetExtensionList()
    # Coverage test.
    yield('WebglExtension_TestCoverage',
          os.path.join(webgl_test_util.extensions_relpath,
                       'webgl_extension_test.html'),
          ('_RunExtensionCoverageTest',
           extension_tests,
           cls._webgl_version))
    # Individual extension tests.
    for extension in extension_tests:
      yield('WebglExtension_%s' % extension,
            os.path.join(webgl_test_util.extensions_relpath,
                         'webgl_extension_test.html'),
            ('_RunExtensionTest',
             extension,
             cls._webgl_version))

  @classmethod
  def _GetExtensionList(cls):
    if cls._webgl_version == 1:
      return [
        'ANGLE_instanced_arrays',
        'EXT_blend_minmax',
        'EXT_color_buffer_half_float',
        'EXT_disjoint_timer_query',
        'EXT_frag_depth',
        'EXT_shader_texture_lod',
        'EXT_sRGB',
        'EXT_texture_filter_anisotropic',
        'OES_element_index_uint',
        'OES_standard_derivatives',
        'OES_texture_float',
        'OES_texture_float_linear',
        'OES_texture_half_float',
        'OES_texture_half_float_linear',
        'OES_vertex_array_object',
        'WEBGL_color_buffer_float',
        'WEBGL_compressed_texture_astc',
        'WEBGL_compressed_texture_etc1',
        'WEBGL_compressed_texture_pvrtc',
        'WEBGL_compressed_texture_s3tc',
        'WEBGL_compressed_texture_s3tc_srgb',
        'WEBGL_debug_renderer_info',
        'WEBGL_debug_shaders',
        'WEBGL_depth_texture',
        'WEBGL_draw_buffers',
        'WEBGL_lose_context',
      ]
    else:
      return [
        'EXT_color_buffer_float',
        'EXT_disjoint_timer_query_webgl2',
        'EXT_texture_filter_anisotropic',
        'OES_texture_float_linear',
        'WEBGL_compressed_texture_astc',
        'WEBGL_compressed_texture_etc',
        'WEBGL_compressed_texture_etc1',
        'WEBGL_compressed_texture_pvrtc',
        'WEBGL_compressed_texture_s3tc',
        'WEBGL_compressed_texture_s3tc_srgb',
        'WEBGL_debug_renderer_info',
        'WEBGL_debug_shaders',
        'WEBGL_lose_context',
      ]

  def RunActualGpuTest(self, test_path, *args):
    # This indirection allows these tests to trampoline through
    # _RunGpuTest.
    test_name = args[0]
    getattr(self, test_name)(test_path, *args[1:])

  def _NavigateTo(self, test_path, harness_script):
    url = self.UrlOfStaticFilePath(test_path)
    self.tab.Navigate(url, script_to_evaluate_on_commit=harness_script)

  def _CheckTestCompletion(self):
    self.tab.action_runner.WaitForJavaScriptCondition(
        'webglTestHarness._finished', timeout=self._GetTestTimeout())
    if not self._DidWebGLTestSucceed(self.tab):
      self.fail(self._WebGLTestMessages(self.tab))

  def _RunConformanceTest(self, test_path, *args):
    self._NavigateTo(test_path, conformance_harness_script)
    self._CheckTestCompletion()


  def _GetExtensionHarnessScript(self):
    return conformance_harness_script + extension_harness_additional_script

  def _RunExtensionCoverageTest(self, test_path, *args):
    self._NavigateTo(test_path, self._GetExtensionHarnessScript())
    self.tab.action_runner.WaitForJavaScriptCondition(
        'window._loaded', timeout=self._GetTestTimeout())
    extension_list = args[0]
    webgl_version = args[1]
    context_type = "webgl2" if webgl_version == 2 else "webgl"
    extension_list_string = "["
    for extension in extension_list:
      extension_list_string = extension_list_string + extension + ", "
    extension_list_string = extension_list_string + "]"
    self.tab.action_runner.EvaluateJavaScript(
        'checkSupportedExtensions({{ extensions_string }}, {{context_type}})',
        extensions_string=extension_list_string, context_type=context_type)
    self._CheckTestCompletion()

  def _RunExtensionTest(self, test_path, *args):
    self._NavigateTo(test_path, self._GetExtensionHarnessScript())
    self.tab.action_runner.WaitForJavaScriptCondition(
        'window._loaded', timeout=self._GetTestTimeout())
    extension = args[0]
    webgl_version = args[1]
    context_type = "webgl2" if webgl_version == 2 else "webgl"
    self.tab.action_runner.EvaluateJavaScript(
      'checkExtension({{ extension }}, {{ context_type }})',
      extension=extension, context_type=context_type)
    self._CheckTestCompletion()

  def _GetTestTimeout(self):
    timeout = 300
    if self._is_asan:
      # Asan runs much slower and needs a longer timeout
      timeout *= 2
    return timeout

  @classmethod
  def SetupWebGLBrowserArgs(cls, browser_args):
    # --test-type=gpu is used only to suppress the "Google API Keys are missing"
    # infobar, which causes flakiness in tests.
    browser_args += [
      '--autoplay-policy=no-user-gesture-required',
      '--disable-domain-blocking-for-3d-apis',
      '--disable-gpu-process-crash-limit',
      '--test-type=gpu',
      '--enable-experimental-web-platform-features',
      # Try disabling the GPU watchdog to see if this affects the
      # intermittent GPU process hangs that have been seen on the
      # waterfall. crbug.com/596622 crbug.com/609252
      '--disable-gpu-watchdog',
      # TODO(http://crbug.com/832952): Remove this when WebXR spec is more
      # stable and setCompatibleXRDevice is part of the conformance test.
      '--disable-blink-features=WebXR'
    ]
    # Note that the overriding of the default --js-flags probably
    # won't interact well with RestartBrowserIfNecessaryWithArgs, but
    # we don't use that in this test.
    browser_options = cls._finder_options.browser_options
    builtin_js_flags = '--js-flags=--expose-gc'
    found_js_flags = False
    user_js_flags = ''
    if browser_options.extra_browser_args:
      for o in browser_options.extra_browser_args:
        if o.startswith('--js-flags'):
          found_js_flags = True
          user_js_flags = o
          break
    if found_js_flags:
      logging.warning('Overriding built-in JavaScript flags:')
      logging.warning(' Original flags: ' + builtin_js_flags)
      logging.warning(' New flags: ' + user_js_flags)
    else:
      browser_args += [builtin_js_flags]
    cls.CustomizeBrowserArgs(browser_args)

  @classmethod
  def _CreateExpectations(cls):
    assert cls._webgl_version == 1 or cls._webgl_version == 2
    clz = None
    if cls._webgl_version == 1:
      clz = webgl_conformance_expectations.WebGLConformanceExpectations
    else:
      clz = webgl2_conformance_expectations.WebGL2ConformanceExpectations
    return clz(is_asan=cls._is_asan)

  @classmethod
  def SetUpProcess(cls):
    super(WebGLConformanceIntegrationTest, cls).SetUpProcess()
    cls.SetupWebGLBrowserArgs([])
    cls.StartBrowser()
    # By setting multiple server directories, the root of the server
    # implicitly becomes the common base directory, i.e., the Chromium
    # src dir, and all URLs have to be specified relative to that.
    cls.SetStaticServerDirs([
      os.path.join(path_util.GetChromiumSrcDir(),
                   webgl_test_util.conformance_relpath),
      os.path.join(path_util.GetChromiumSrcDir(),
                   webgl_test_util.extensions_relpath)])

  # Helper functions.

  @staticmethod
  def _DidWebGLTestSucceed(tab):
    return tab.EvaluateJavaScript('webglTestHarness._allTestSucceeded')

  @staticmethod
  def _WebGLTestMessages(tab):
    return tab.EvaluateJavaScript('webglTestHarness._messages')

  @classmethod
  def _ParseTests(cls, path, version, webgl2_only, folder_min_version):
    test_paths = []
    current_dir = os.path.dirname(path)
    full_path = os.path.normpath(os.path.join(webgl_test_util.conformance_path,
                                              path))
    webgl_version = int(version.split('.')[0])

    if not os.path.exists(full_path):
      raise Exception('The WebGL conformance test path specified ' +
        'does not exist: ' + full_path)

    with open(full_path, 'r') as f:
      for line in f:
        line = line.strip()

        if not line:
          continue

        if line.startswith('//') or line.startswith('#'):
          continue

        line_tokens = line.split(' ')
        test_name = line_tokens[-1]

        i = 0
        min_version = None
        max_version = None
        while i < len(line_tokens):
          token = line_tokens[i]
          if token == '--min-version':
            i += 1
            min_version = line_tokens[i]
          elif token == '--max-version':
            i += 1
            max_version = line_tokens[i]
          i += 1

        min_version_to_compare = min_version or folder_min_version

        if (min_version_to_compare and
            _CompareVersion(version, min_version_to_compare) < 0):
          continue

        if max_version and _CompareVersion(version, max_version) > 0:
          continue

        if (webgl2_only and not '.txt' in test_name and
            (not min_version_to_compare or
             not min_version_to_compare.startswith('2'))):
          continue

        if '.txt' in test_name:
          include_path = os.path.join(current_dir, test_name)
          # We only check min-version >= 2.0.0 for the top level list.
          test_paths += cls._ParseTests(
              include_path, version, webgl2_only, min_version_to_compare)
        else:
          test = os.path.join(current_dir, test_name)
          if webgl_version > 1:
            test += '?webglVersion=' + str(webgl_version)
          test_paths.append(test)

    return test_paths


def load_tests(loader, tests, pattern):
  del loader, tests, pattern  # Unused.
  return gpu_integration_test.LoadAllTestsInModule(sys.modules[__name__])
