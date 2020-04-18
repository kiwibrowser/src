# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to mount a built image and run tests on it."""

from __future__ import print_function

import os
import unittest

from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import image_test_lib
from chromite.lib import osutils
from chromite.lib import path_util


def ParseArgs(args):
  """Return parsed commandline arguments."""

  parser = commandline.ArgumentParser()
  parser.add_argument('--test_results_root', type='path',
                      help='Directory to store test results')
  parser.add_argument('--board', type=str, help='Board (wolf, beaglebone...)')
  parser.add_argument('image_dir', type='path',
                      help='Image directory (or file) with mount_image.sh and '
                           'umount_image.sh')

  parser.add_argument('-l', '--list', default=False, action='store_true',
                      help='List all the available tests')
  parser.add_argument('tests', nargs='*', metavar='test',
                      help='Specific tests to run (default runs all)')

  opts = parser.parse_args(args)
  opts.Freeze()
  return opts


def FindImage(image_path):
  """Return the path to the image file.

  Args:
    image_path: A path to the image file, or a directory containing the base
      image.

  Returns:
    ImageFileAndMountScripts containing absolute paths to the image,
      the mount and umount invocation commands
  """

  if os.path.isdir(image_path):
    # Assume base image.
    image_file = os.path.join(image_path, constants.BASE_IMAGE_NAME + '.bin')
    if not os.path.exists(image_file):
      raise ValueError('Cannot find base image %s' % image_file)
  elif os.path.isfile(image_path):
    image_file = image_path
  else:
    raise ValueError('%s is neither a directory nor a file' % image_path)

  return image_file


def main(args):
  opts = ParseArgs(args)

  # Build up test suites.
  loader = unittest.TestLoader()
  loader.suiteClass = image_test_lib.ImageTestSuite
  # We use a different prefix here so that unittest DO NOT pick up the
  # image tests automatically because they depend on a proper environment.
  loader.testMethodPrefix = 'Test'
  tests_namespace = 'chromite.cros.test.image_test'
  if opts.tests:
    tests = ['%s.%s' % (tests_namespace, x) for x in opts.tests]
  else:
    tests = (tests_namespace,)
  all_tests = loader.loadTestsFromNames(tests)

  # If they just want to see the lists of tests, show them now.
  if opts.list:
    def _WalkSuite(suite):
      for test in suite:
        if isinstance(test, unittest.BaseTestSuite):
          for result in _WalkSuite(test):
            yield result
        else:
          yield (test.id()[len(tests_namespace) + 1:],
                 test.shortDescription() or '')

    test_list = list(_WalkSuite(all_tests))
    maxlen = max(len(x[0]) for x in test_list)
    for name, desc in test_list:
      print('%-*s  %s' % (maxlen, name, desc))
    return

  # Run them in the image directory.
  runner = image_test_lib.ImageTestRunner()
  runner.SetBoard(opts.board)
  runner.SetResultDir(opts.test_results_root)
  image_file = FindImage(opts.image_dir)
  tmp_in_chroot = path_util.FromChrootPath('/tmp')
  with osutils.TempDir(base_dir=tmp_in_chroot) as temp_dir:
    with osutils.MountImageContext(image_file, temp_dir):
      with osutils.ChdirContext(temp_dir):
        result = runner.run(all_tests)

  if result and not result.wasSuccessful():
    return 1
  return 0
