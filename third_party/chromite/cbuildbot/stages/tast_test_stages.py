# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing stages for running Tast integration tests.

This module contains cbuildbot test stages that run Tast integration tests. See
https://chromium.googlesource.com/chromiumos/platform/tast/ for more details.
"""

from __future__ import print_function

import json
import os
import shutil

from chromite.cbuildbot import commands
from chromite.cbuildbot.stages import generic_stages
from chromite.lib import cgroups
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import failures_lib
from chromite.lib import osutils
from chromite.lib import timeout_util


# Error format string used when tast exits with a non-zero exit code.
FAILURE_EXIT_CODE = '** Tests failed with code %d **'

# Error format string used when one or more tests fail.
FAILURE_TESTS_FAILED = '** %d test(s) failed **'

# Error format string used when results dir is missing or empty.
FAILURE_NO_RESULTS = '** No results in %s **'

# Error format string used when results file is unreadable.
FAILURE_BAD_RESULTS = '** Failed to read results from %s: %s **'

# Prefix for results download link.
RESULTS_LINK_PREFIX = 'results: '

# Prefix for results link for flaky tests.
FLAKY_PREFIX = 'flaky: '

# Name of JSON file containing individual tests' results written by the tast
# command to the results dir.
RESULTS_FILENAME = 'results.json'

# Names of properties in test objects from results JSON files.
RESULTS_NAME_KEY = 'name'
RESULTS_ERRORS_KEY = 'errors'
RESULTS_ATTR_KEY = 'attr'

# Attribute used to label flaky tests.
RESULTS_FLAKY_ATTR = 'flaky'

# Directory written within main results dir by tast command containing per-test
# results.
RESULTS_TESTS_DIR = 'tests'


def _CopyResultsDir(src, dest):
  """Copies a results dir to a new directory for archiving.

  Args:
    src: String source path.
    dest: String destination path (presumably under
          generic_stages.ArchivingStageMixin.archive_path). Must not exist
          already.

  Raises:
    OSError if dest already exists or the copy fails.
  """
  # Skip symlinks since gsutil chokes on broken ones (and just duplicates
  # symlinked files, in any case).
  def GetSymlinks(dirname, files):
    return [x for x in files if os.path.islink(os.path.join(dirname, x))]
  shutil.copytree(src, dest, ignore=GetSymlinks)


class TastVMTestStage(generic_stages.BoardSpecificBuilderStage,
                      generic_stages.ArchivingStageMixin):
  """Runs Tast integration tests in a virtual machine."""

  # Time allotted to cros_run_tast_vm_test to clean up (i.e. shut down the
  # VM) after receiving SIGTERM. After this, SIGKILL is sent.
  CLEANUP_TIMEOUT_SEC = 30 * 60

  # Path within src/scripts to start/stop VM and run tests.
  SCRIPT_PATH = 'bin/cros_run_tast_vm_test'

  # These magic attributes can be used to turn off the stage via the build
  # config. See generic_stages.BuilderStage.
  option_name = 'tests'
  config_name = 'tast_vm_tests'

  def PerformStage(self):
    """Performs the stage. Overridden from generic_stages.BuilderStage."""

    # CreateTestRoot creates a results directory and returns its path relative
    # to the chroot.
    chroot_results_dir = commands.CreateTestRoot(self._build_root)

    try:
      got_exception = False
      try:
        self._RunAllSuites(self._run.config.tast_vm_tests, chroot_results_dir)
      except Exception:
        # sys.exc_info() returns (None, None, None) in the finally block, so we
        # need to record the fact that we already have an error here.
        got_exception = True
        raise
      finally:
        self._ProcessAndArchiveResults(
            self._MakeChrootPathAbsolute(chroot_results_dir),
            [t.suite_name for t in self._run.config.tast_vm_tests],
            got_exception)
    except Exception:
      logging.exception('Tast VM tests failed')
      raise

  def _MakeChrootPathAbsolute(self, path):
    """Appends the supplied path to the chroot's path.

    Args:
      path: String containing path (either relative or absolute) to be rooted
            in chroot.

    Returns:
      String containing chroot suffixed by path.
    """
    # When os.path.join encounters an absolute path, it throws away everything
    # it's alredy seen.
    return os.path.join(self._build_root, constants.DEFAULT_CHROOT_DIR,
                        path.lstrip('/'))

  def _RunAllSuites(self, suites, base_chroot_results_dir):
    """Runs multiple test suites sequentially.

    Args:
      suites: List of TestVMTestConfig objects describing suites to run.
      base_chroot_results_dir: Base results directory relative to chroot.

    Raises:
      failures_lib.TestFailure if an internal error is encountered.
    """
    with cgroups.SimpleContainChildren('TastVMTest'):
      for suite in suites:
        logging.info('Running Tast VM test suite %s (%s)',
                     suite.suite_name, (' '.join(suite.test_exprs)))
        # We apparently always prefix reasons with spaces because timeout_util
        # appends them directly to error messages.
        reason = ' Reached TastVMTestStage test run timeout.'
        with timeout_util.Timeout(suite.timeout, reason_message=reason):
          self._RunSuite(suite.test_exprs,
                         os.path.join(base_chroot_results_dir,
                                      suite.suite_name))

  def _RunSuite(self, test_exprs, suite_chroot_results_dir):
    """Runs a collection of tests.

    Args:
      test_exprs: List of string expressions describing which tests to run; this
                  is passed directly to the 'tast run' command. See
                  https://goo.gl/UPNEgT for info about test expressions.
      suite_chroot_results_dir: String containing path of directory where the
                                tast command should store test results,
                                relative to chroot.

    Raises:
      failures_lib.TestFailure if an internal error is encountered.
    """
    image = os.path.join(self.GetImageDirSymlink(), constants.TEST_IMAGE_BIN)
    ssh_key = os.path.join(self.GetImageDirSymlink(),
                           constants.TEST_KEY_PRIVATE)
    cmd = [TastVMTestStage.SCRIPT_PATH,
           '--board=' + self._current_board,
           '--image_path=' + image,
           '--ssh_private_key=' + ssh_key,
           '--no_graphics',
           '--results_dir=' + suite_chroot_results_dir,
          ]
    cmd += test_exprs

    result = cros_build_lib.RunCommand(
        cmd, cwd=os.path.join(self._build_root, 'src/scripts'),
        error_code_ok=True, kill_timeout=TastVMTestStage.CLEANUP_TIMEOUT_SEC)
    if result.returncode:
      raise failures_lib.TestFailure(FAILURE_EXIT_CODE % result.returncode)

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure,
                               exclude_exceptions=[failures_lib.TestFailure])
  def _ProcessAndArchiveResults(self, abs_results_dir, suite_names,
                                already_have_error):
    """Processes and archives test results.

    Args:
      abs_results_dir: Absolute path to directory containing test results.
      suite_names: List of string test suite names.
      already_have_error: Boolean for whether testing has already failed.

    Raises:
      failures_lib.TestFailure if one or more tests failed or results were
        unavailable. Suppressed if already_have_error is True.
    """
    if not os.path.isdir(abs_results_dir) or not os.listdir(abs_results_dir):
      raise failures_lib.TestFailure(FAILURE_NO_RESULTS % abs_results_dir)

    archive_base = constants.TAST_VM_TEST_RESULTS % {'attempt': self._attempt}
    _CopyResultsDir(abs_results_dir,
                    os.path.join(self.archive_path, archive_base))

    # TODO(crbug.com/770562): Collect stack traces once the tast executable is
    # symbolizing and collecting them (see VMTestStage._ArchiveTestResults).

    # Now archive the results to Cloud Storage.
    logging.info('Uploading artifacts to Cloud Storage...')
    self.UploadArtifact(archive_base, archive=False, strict=False)
    self.PrintDownloadLink(archive_base, RESULTS_LINK_PREFIX)

    try:
      self._ProcessResultsFile(abs_results_dir, archive_base, suite_names)
    except Exception as e:
      # Don't raise a new exception if testing already failed.
      if already_have_error:
        logging.exception('Got error while archiving or processing results')
      else:
        raise e
    finally:
      osutils.RmDir(abs_results_dir, ignore_missing=True, sudo=True)

  def _ProcessResultsFile(self, abs_results_dir, url_base, suite_names):
    """Parses the results file and prints links to failed tests.

    Args:
      abs_results_dir: Absolute path to directory containing test results.
      url_base: Relative path within the archive dir where results are stored.
      suite_names: List of string test suite names.

    Raises:
      failures_lib.TestFailure if one or more tests failed or results were
        missing or unreadable.
    """
    num_failed = 0

    for suite_name in sorted(suite_names):
      results_path = os.path.join(abs_results_dir, suite_name, RESULTS_FILENAME)

      # The results file contains an array with objects representing tests.
      # Each object should contain the test name in a 'name' attribute and a
      # list of errors in an 'error' attribute.
      try:
        with open(results_path, 'r') as f:
          for test in json.load(f):
            if test[RESULTS_ERRORS_KEY]:
              flaky = RESULTS_FLAKY_ATTR in test.get(RESULTS_ATTR_KEY, [])

              name = test[RESULTS_NAME_KEY]
              test_url = os.path.join(
                  url_base, suite_name, RESULTS_TESTS_DIR, name)
              desc = FLAKY_PREFIX + name if flaky else name
              self.PrintDownloadLink(test_url, text_to_display=desc)

              # Ignore the failure if the test was marked flaky.
              if not flaky:
                num_failed += 1
      except Exception as e:
        raise failures_lib.TestFailure(FAILURE_BAD_RESULTS %
                                       (results_path, str(e)))

    if num_failed > 0:
      logging.error('%d test(s) failed', num_failed)
      raise failures_lib.TestFailure(FAILURE_TESTS_FAILED % num_failed)
