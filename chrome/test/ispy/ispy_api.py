# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
from distutils.version import LooseVersion
from PIL import Image

from common import cloud_bucket
from common import ispy_utils


class ISpyApi(object):
  """The public API for interacting with ISpy."""

  def __init__(self, cloud_bucket):
    """Initializes the utility class.

    Args:
      cloud_bucket: a BaseCloudBucket in which to the version file,
          expectations and results are to be stored.
    """
    self._cloud_bucket = cloud_bucket
    self._ispy = ispy_utils.ISpyUtils(self._cloud_bucket)
    self._rebaselineable_cache = {}

  def UpdateExpectationVersion(self, chrome_version, version_file):
    """Updates the most recent expectation version to the Chrome version.

    Should be called after generating a new set of expectations.

    Args:
      chrome_version: the chrome version as a string of the form "31.0.123.4".
      version_file: path to the version file in the cloud bucket. The version
          file contains a json list of ordered Chrome versions for which
          expectations exist.
    """
    insert_pos = 0
    expectation_versions = []
    try:
      expectation_versions = self._GetExpectationVersionList(version_file)
      if expectation_versions:
        try:
          version = self._GetExpectationVersion(
              chrome_version, expectation_versions)
          if version == chrome_version:
            return
          insert_pos = expectation_versions.index(version)
        except:
          insert_pos = len(expectation_versions)
    except cloud_bucket.FileNotFoundError:
      pass
    expectation_versions.insert(insert_pos, chrome_version)
    logging.info('Updating expectation version...')
    self._cloud_bucket.UploadFile(
        version_file, json.dumps(expectation_versions),
        'application/json')

  def _GetExpectationVersion(self, chrome_version, expectation_versions):
    """Returns the expectation version for the given Chrome version.

    Args:
      chrome_version: the chrome version as a string of the form "31.0.123.4".
      expectation_versions: Ordered list of Chrome versions for which
        expectations exist, as stored in the version file.

    Returns:
      Expectation version string.
    """
    # Find the closest version that is not greater than the chrome version.
    for version in expectation_versions:
      if LooseVersion(version) <= LooseVersion(chrome_version):
        return version
    raise Exception('No expectation exists for Chrome %s' % chrome_version)

  def _GetExpectationVersionList(self, version_file):
    """Gets the list of expectation versions from google storage.

    Args:
      version_file: path to the version file in the cloud bucket. The version
          file contains a json list of ordered Chrome versions for which
          expectations exist.

    Returns:
      Ordered list of Chrome versions.
    """
    try:
      return json.loads(self._cloud_bucket.DownloadFile(version_file))
    except:
      return []

  def _GetExpectationNameWithVersion(self, device_type, expectation,
                                     chrome_version, version_file):
    """Get the expectation to be used with the current Chrome version.

    Args:
      device_type: string identifier for the device type.
      expectation: name for the expectation to generate.
      chrome_version: the chrome version as a string of the form "31.0.123.4".

    Returns:
      Version as an integer.
    """
    version = self._GetExpectationVersion(
        chrome_version, self._GetExpectationVersionList(version_file))
    return self._CreateExpectationName(device_type, expectation, version)

  def _CreateExpectationName(self, device_type, expectation, version):
    """Create the full expectation name from the expectation and version.

    Args:
      device_type: string identifier for the device type, example: mako
      expectation: base name for the expectation, example: google.com
      version: expectation version, example: 31.0.23.1

    Returns:
      Full expectation name as a string, example: mako:google.com(31.0.23.1)
    """
    return '%s:%s(%s)' % (device_type, expectation, version)

  def GenerateExpectation(self, device_type, expectation, chrome_version,
                          version_file, screenshots):
    """Create an expectation for I-Spy.

    Args:
      device_type: string identifier for the device type.
      expectation: name for the expectation to generate.
      chrome_version: the chrome version as a string of the form "31.0.123.4".
      screenshots: a list of similar PIL.Images.
    """
    # https://code.google.com/p/chromedriver/issues/detail?id=463
    expectation_with_version = self._CreateExpectationName(
        device_type, expectation, chrome_version)
    if self._ispy.ExpectationExists(expectation_with_version):
      logging.warning(
          'I-Spy expectation \'%s\' already exists, overwriting.',
          expectation_with_version)
    logging.info('Generating I-Spy expectation...')
    self._ispy.GenerateExpectation(expectation_with_version, screenshots)

  def PerformComparison(self, test_run, device_type, expectation,
                        chrome_version, version_file, screenshot):
    """Compare a screenshot with the given expectation in I-Spy.

    Args:
      test_run: name for the test run.
      device_type: string identifier for the device type.
      expectation: name for the expectation to compare against.
      chrome_version: the chrome version as a string of the form "31.0.123.4".
      screenshot: a PIL.Image to compare.
    """
    # https://code.google.com/p/chromedriver/issues/detail?id=463
    logging.info('Performing I-Spy comparison...')
    self._ispy.PerformComparison(
        test_run,
        self._GetExpectationNameWithVersion(
            device_type, expectation, chrome_version, version_file),
        screenshot)

  def CanRebaselineToTestRun(self, test_run):
    """Returns whether the test run has associated expectations.

    Returns:
      True if RebaselineToTestRun() can be called for this test run.
    """
    if test_run in self._rebaselineable_cache:
      return True
    return self._cloud_bucket.FileExists(
        ispy_utils.GetTestRunPath(test_run, 'rebaseline.txt'))

  def RebaselineToTestRun(self, test_run):
    """Update the version file to use expectations associated with |test_run|.

    Args:
      test_run: The name of the test run to rebaseline.
    """
    rebaseline_path = ispy_utils.GetTestRunPath(test_run, 'rebaseline.txt')
    rebaseline_attrib = json.loads(
        self._cloud_bucket.DownloadFile(rebaseline_path))
    self.UpdateExpectationVersion(
        rebaseline_attrib['version'], rebaseline_attrib['version_file'])
    self._cloud_bucket.RemoveFile(rebaseline_path)

  def _SetTestRunRebaselineable(self, test_run, chrome_version, version_file):
    """Writes a JSON file containing the data needed to rebaseline.

    Args:
      test_run: The name of the test run to add the rebaseline file to.
      chrome_version: the chrome version that can be rebaselined to (must have
        associated Expectations).
      version_file: the path of the version file associated with the test run.
    """
    self._rebaselineable_cache[test_run] = True
    self._cloud_bucket.UploadFile(
        ispy_utils.GetTestRunPath(test_run, 'rebaseline.txt'),
        json.dumps({
            'version': chrome_version,
            'version_file': version_file}),
        'application/json')

  def PerformComparisonAndPrepareExpectation(self, test_run, device_type,
                                             expectation, chrome_version,
                                             version_file, screenshots):
    """Perform comparison and generate an expectation that can used later.

    The test run web UI will have a button to set the Expectations generated for
    this version as the expectation for comparison with later versions.

    Args:
      test_run: The name of the test run to add the rebaseline file to.
      device_type: string identifier for the device type.
      chrome_version: the chrome version that can be rebaselined to (must have
        associated Expectations).
      version_file: the path of the version file associated with the test run.
      screenshot: a list of similar PIL.Images.
    """
    if not self.CanRebaselineToTestRun(test_run):
      self._SetTestRunRebaselineable(test_run, chrome_version, version_file)
    expectation_with_version = self._CreateExpectationName(
        device_type, expectation, chrome_version)
    self._ispy.GenerateExpectation(expectation_with_version, screenshots)
    self._ispy.PerformComparison(
        test_run,
        self._GetExpectationNameWithVersion(
            device_type, expectation, chrome_version, version_file),
        screenshots[-1])

