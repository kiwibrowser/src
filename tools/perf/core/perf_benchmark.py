# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import sys

from telemetry import benchmark
from telemetry.internal.browser import browser_finder
from telemetry.internal.util import path as path_module


sys.path.append(os.path.join(os.path.dirname(__file__), '..',
                             '..', 'variations'))
import fieldtrial_util  # pylint: disable=import-error

# This function returns a list of two-tuples designed to extend
# browser_options.profile_files_to_copy. On success, it will return two entries:
# 1. The actual indexed ruleset file, which will be placed in a destination
#    directory as specified by the version found in the prefs file.
# 2. A default prefs 'Local State' file, which contains information about the ad
#    tagging ruleset's version.
def GetAdTaggingProfileFiles(chrome_output_directory):
  """Gets ad tagging tuples for browser_options.profile_files_to_copy

  This function looks for input files related to ad tagging, and returns tuples
  indicating where those files should be copied to in the resulting perf
  benchmark profile directory.

  Args:
      chrome_output_directory: path to the output directory for this benchmark
      (e.g. out/Default).

  Returns:
      A list of two-tuples designed to extend profile_files_to_copy in
      BrowserOptions. If no ad tagging related input files could be found,
      returns an empty list.
  """
  if chrome_output_directory is None:
    return []

  ruleset_path = os.path.join(chrome_output_directory, 'gen', 'components',
      'subresource_filter', 'tools','GeneratedRulesetData')
  if not os.path.exists(ruleset_path):
    return []

  local_state_path = os.path.join(
      os.path.dirname(__file__), 'default_local_state.json')
  assert os.path.exists(local_state_path)

  with open(local_state_path, 'r') as f:
    state_json = json.load(f)
    ruleset_version = state_json['subresource_filter']['ruleset_version']

    # The ruleset should reside in:
    # Subresource Filter/Indexed Rules/<iv>/<uv>/Ruleset Data
    # Where iv = indexed version and uv = unindexed version
    ruleset_format_str = '%d' % ruleset_version['format']
    ruleset_dest = os.path.join('Subresource Filter', 'Indexed Rules',
                                ruleset_format_str, ruleset_version['content'],
                                'Ruleset Data')

    return [(ruleset_path, ruleset_dest), (local_state_path, 'Local State')]


class PerfBenchmark(benchmark.Benchmark):
  """ Super class for all benchmarks in src/tools/perf/benchmarks directory.
  All the perf benchmarks must subclass from this one to to make sure that
  the field trial configs are activated for the browser during benchmark runs.
  For more info, see: https://goo.gl/4uvaVM
  """

  def SetExtraBrowserOptions(self, options):
    """ To be overridden by perf benchmarks. """
    pass

  def CustomizeBrowserOptions(self, options):
    # Subclass of PerfBenchmark should override  SetExtraBrowserOptions to add
    # more browser options rather than overriding CustomizeBrowserOptions.
    super(PerfBenchmark, self).CustomizeBrowserOptions(options)

    # Enable taking screen shot on failed pages for all perf benchmarks.
    options.take_screenshot_for_failed_page = True

    # The current field trial config is used for an older build in the case of
    # reference. This is a problem because we are then subjecting older builds
    # to newer configurations that may crash.  To work around this problem,
    # don't add the field trials to reference builds.
    #
    # The same logic applies to the ad filtering ruleset, which could be in a
    # binary format that an older build does not expect.
    if options.browser_type != 'reference':
      variations = self._GetVariationsBrowserArgs(options.finder_options)
      options.AppendExtraBrowserArgs(variations)

      options.profile_files_to_copy.extend(
          GetAdTaggingProfileFiles(self._GetOutDirectoryEstimate(options)))

    self.SetExtraBrowserOptions(options)

  @staticmethod
  def _FixupTargetOS(target_os):
    if target_os == 'darwin':
      return 'mac'
    if target_os.startswith('win'):
      return 'win'
    if target_os.startswith('linux'):
      return 'linux'
    return target_os

  def _GetVariationsBrowserArgs(self, finder_options):
    variations_dir = os.path.join(os.path.dirname(__file__), '..',
                                  '..', '..', 'testing', 'variations')
    possible_browser = browser_finder.FindBrowser(finder_options)
    if not possible_browser:
      return []

    return fieldtrial_util.GenerateArgs(
        os.path.join(variations_dir, 'fieldtrial_testing_config.json'),
        self._FixupTargetOS(possible_browser.target_os))

  def _GetOutDirectoryEstimate(self, options):
    finder_options = options.finder_options
    if finder_options.chromium_output_dir is not None:
      return finder_options.chromium_output_dir

    for path in path_module.GetBuildDirectories(finder_options.chrome_root):
      browser_type = os.path.basename(path).lower()
      if options.browser_type == browser_type and os.path.exists(path):
        return path
    return None

  @staticmethod
  def IsSvelte(possible_browser):
    """Returns whether a possible_browser is on a svelte Android build."""
    if possible_browser.target_os == 'android':
      return possible_browser.platform.IsSvelte()
    return False
