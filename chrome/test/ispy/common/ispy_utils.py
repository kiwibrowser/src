# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Internal utilities for managing I-Spy test results in Google Cloud Storage.

See the ispy.ispy_api module for the external API.
"""

import collections
import itertools
import json
import os
import sys

import image_tools


_INVALID_EXPECTATION_CHARS = ['/', '\\', ' ', '"', '\'']


def IsValidExpectationName(expectation_name):
  return not any(c in _INVALID_EXPECTATION_CHARS for c in expectation_name)


def GetExpectationPath(expectation, file_name=''):
  """Get the path to a test file in the given test run and expectation.

  Args:
    expectation: name of the expectation.
    file_name: name of the file.

  Returns:
    the path as a string relative to the bucket.
  """
  return 'expectations/%s/%s' % (expectation, file_name)


def GetFailurePath(test_run, expectation, file_name=''):
  """Get the path to a failure file in the given test run and test.

  Args:
    test_run: name of the test run.
    expectation: name of the expectation.
    file_name: name of the file.

  Returns:
    the path as a string relative to the bucket.
  """
  return GetTestRunPath(test_run, '%s/%s' % (expectation, file_name))


def GetTestRunPath(test_run, file_name=''):
  """Get the path to a the given test run.

  Args:
    test_run: name of the test run.
    file_name: name of the file.

  Returns:
    the path as a string relative to the bucket.
  """
  return 'failures/%s/%s' % (test_run, file_name)


class ISpyUtils(object):
  """Utility functions for working with an I-Spy google storage bucket."""

  def __init__(self, cloud_bucket):
    """Initialize with a cloud bucket instance to supply GS functionality.

    Args:
      cloud_bucket: An object implementing the cloud_bucket.BaseCloudBucket
        interface.
    """
    self.cloud_bucket = cloud_bucket

  def UploadImage(self, full_path, image):
    """Uploads an image to a location in GS.

    Args:
      full_path: the path to the file in GS including the file extension.
      image: a RGB PIL.Image to be uploaded.
    """
    self.cloud_bucket.UploadFile(
        full_path, image_tools.EncodePNG(image), 'image/png')

  def DownloadImage(self, full_path):
    """Downloads an image from a location in GS.

    Args:
      full_path: the path to the file in GS including the file extension.

    Returns:
      The downloaded RGB PIL.Image.

    Raises:
      cloud_bucket.NotFoundError: if the path to the image is not valid.
    """
    return image_tools.DecodePNG(self.cloud_bucket.DownloadFile(full_path))

  def UpdateImage(self, full_path, image):
    """Updates an existing image in GS, preserving permissions and metadata.

    Args:
      full_path: the path to the file in GS including the file extension.
      image: a RGB PIL.Image.
    """
    self.cloud_bucket.UpdateFile(full_path, image_tools.EncodePNG(image))

  def GenerateExpectation(self, expectation, images):
    """Creates and uploads an expectation to GS from a set of images and name.

    This method generates a mask from the uploaded images, then
      uploads the mask and first of the images to GS as a expectation.

    Args:
      expectation: name for this expectation, any existing expectation with the
        name will be replaced.
      images: a list of RGB encoded PIL.Images

    Raises:
      ValueError: if the expectation name is invalid.
    """
    if not IsValidExpectationName(expectation):
      raise ValueError("Expectation name contains an illegal character: %s." %
                       str(_INVALID_EXPECTATION_CHARS))

    mask = image_tools.InflateMask(image_tools.CreateMask(images), 7)
    self.UploadImage(
        GetExpectationPath(expectation, 'expected.png'), images[0])
    self.UploadImage(GetExpectationPath(expectation, 'mask.png'), mask)

  def PerformComparison(self, test_run, expectation, actual):
    """Runs an image comparison, and uploads discrepancies to GS.

    Args:
      test_run: the name of the test_run.
      expectation: the name of the expectation to use for comparison.
      actual: an RGB-encoded PIL.Image that is the actual result.

    Raises:
      cloud_bucket.NotFoundError: if the given expectation is not found.
      ValueError: if the expectation name is invalid.
    """
    if not IsValidExpectationName(expectation):
      raise ValueError("Expectation name contains an illegal character: %s." %
                       str(_INVALID_EXPECTATION_CHARS))

    expectation_tuple = self.GetExpectation(expectation)
    if not image_tools.SameImage(
        actual, expectation_tuple.expected, mask=expectation_tuple.mask):
      self.UploadImage(
          GetFailurePath(test_run, expectation, 'actual.png'), actual)
      diff, diff_pxls = image_tools.VisualizeImageDifferences(
          expectation_tuple.expected, actual, mask=expectation_tuple.mask)
      self.UploadImage(GetFailurePath(test_run, expectation, 'diff.png'), diff)
      self.cloud_bucket.UploadFile(
          GetFailurePath(test_run, expectation, 'info.txt'),
          json.dumps({
            'different_pixels': diff_pxls,
            'fraction_different':
                diff_pxls / float(actual.size[0] * actual.size[1])}),
          'application/json')

  def GetExpectation(self, expectation):
    """Returns the given expectation from GS.

    Args:
      expectation: the name of the expectation to get.

    Returns:
      A named tuple: 'Expectation', containing two images: expected and mask.

    Raises:
      cloud_bucket.NotFoundError: if the test is not found in GS.
    """
    Expectation = collections.namedtuple('Expectation', ['expected', 'mask'])
    return Expectation(self.DownloadImage(GetExpectationPath(expectation,
                                                             'expected.png')),
                       self.DownloadImage(GetExpectationPath(expectation,
                                                             'mask.png')))

  def ExpectationExists(self, expectation):
    """Returns whether the given expectation exists in GS.

    Args:
      expectation: the name of the expectation to check.

    Returns:
      A boolean indicating whether the test exists.
    """
    expected_image_exists = self.cloud_bucket.FileExists(
        GetExpectationPath(expectation, 'expected.png'))
    mask_image_exists = self.cloud_bucket.FileExists(
        GetExpectationPath(expectation, 'mask.png'))
    return expected_image_exists and mask_image_exists

  def FailureExists(self, test_run, expectation):
    """Returns whether a failure for the expectation exists for the given run.

    Args:
      test_run: the name of the test_run.
      expectation: the name of the expectation that failed.

    Returns:
      A boolean indicating whether the failure exists.
    """
    actual_image_exists = self.cloud_bucket.FileExists(
        GetFailurePath(test_run, expectation, 'actual.png'))
    test_exists = self.ExpectationExists(expectation)
    info_exists = self.cloud_bucket.FileExists(
        GetFailurePath(test_run, expectation, 'info.txt'))
    return test_exists and actual_image_exists and info_exists

  def RemoveExpectation(self, expectation):
    """Removes an expectation and all associated failures with that test.

    Args:
      expectation: the name of the expectation to remove.
    """
    test_paths = self.cloud_bucket.GetAllPaths(
        GetExpectationPath(expectation))
    for path in test_paths:
      self.cloud_bucket.RemoveFile(path)

  def GenerateExpectationPinkOut(self, expectation, images, pint_out, rgb):
    """Uploads an ispy-test to GS with the pink_out workaround.

    Args:
      expectation: the name of the expectation to be uploaded.
      images: a json encoded list of base64 encoded png images.
      pink_out: an image.
      RGB: a json list representing the RGB values of a color to mask out.

    Raises:
      ValueError: if expectation name is invalid.
    """
    if not IsValidExpectationName(expectation):
      raise ValueError("Expectation name contains an illegal character: %s." %
                       str(_INVALID_EXPECTATION_CHARS))

    # convert the pink_out into a mask
    black = (0, 0, 0, 255)
    white = (255, 255, 255, 255)
    pink_out.putdata(
        [black if px == (rgb[0], rgb[1], rgb[2], 255) else white
         for px in pink_out.getdata()])
    mask = image_tools.CreateMask(images)
    mask = image_tools.InflateMask(image_tools.CreateMask(images), 7)
    combined_mask = image_tools.AddMasks([mask, pink_out])
    self.UploadImage(GetExpectationPath(expectation, 'expected.png'), images[0])
    self.UploadImage(GetExpectationPath(expectation, 'mask.png'), combined_mask)

  def RemoveFailure(self, test_run, expectation):
    """Removes a failure from GS.

    Args:
      test_run: the name of the test_run.
      expectation: the expectation on which the failure to be removed occured.
    """
    failure_paths = self.cloud_bucket.GetAllPaths(
        GetFailurePath(test_run, expectation))
    for path in failure_paths:
      self.cloud_bucket.RemoveFile(path)

  def GetFailure(self, test_run, expectation):
    """Returns a given test failure's expected, diff, and actual images.

    Args:
      test_run: the name of the test_run.
      expectation: the name of the expectation the result corresponds to.

    Returns:
      A named tuple: Failure containing three images: expected, diff, and
        actual.

    Raises:
      cloud_bucket.NotFoundError: if the result is not found in GS.
    """
    expected = self.DownloadImage(
        GetExpectationPath(expectation, 'expected.png'))
    actual = self.DownloadImage(
        GetFailurePath(test_run, expectation, 'actual.png'))
    diff = self.DownloadImage(
        GetFailurePath(test_run, expectation, 'diff.png'))
    info = json.loads(self.cloud_bucket.DownloadFile(
        GetFailurePath(test_run, expectation, 'info.txt')))
    Failure = collections.namedtuple(
        'Failure', ['expected', 'diff', 'actual', 'info'])
    return Failure(expected, diff, actual, info)

  def GetAllPaths(self, prefix, max_keys=None, marker=None, delimiter=None):
    """Gets urls to all files in GS whose path starts with a given prefix.

    Args:
      prefix: the prefix to filter files in GS by.
      max_keys: Integer. Specifies the maximum number of objects returned
      marker: String. Only objects whose fullpath starts lexicographically
        after marker (exclusively) will be returned
      delimiter: String. Turns on directory mode, specifies characters
        to be used as directory separators

    Returns:
      a list containing urls to all objects that started with
         the prefix.
    """
    return self.cloud_bucket.GetAllPaths(
        prefix, max_keys=max_keys, marker=marker, delimiter=delimiter)
