# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Update the CHROMEOS_LKGM file in a chromium repository."""

from __future__ import print_function

import argparse
import distutils.version
import os

from chromite.cbuildbot import manifest_version
from chromite.lib import chrome_committer
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import osutils


class LKGMNotValid(chrome_committer.CommitError):
  """Raised if the LKGM version is unset or not newer than the current value."""


class LKGMFileNotFound(chrome_committer.CommitError):
  """Raised if the LKGM file is not found."""


class ChromeLKGMCommitter(object):
  """Committer object responsible for obtaining a new LKGM and committing it."""

  _COMMIT_MSG_TEMPLATE = ('LKGM %(version)s for chromeos.'
                          '\n\nBUG=762641')
  # Files needed in a local checkout to successfully update the LKGM. The OWNERS
  # file allows the --tbr-owners mechanism to select an appropriate OWNER to
  # TBR.
  _NEEDED_FILES = [
      constants.PATH_TO_CHROME_CHROMEOS_OWNERS,
      constants.PATH_TO_CHROME_LKGM,
  ]

  def __init__(self, args):
    self._committer = chrome_committer.ChromeCommitter(args)

    # Strip any chrome branch from the lkgm version.
    self._lkgm = manifest_version.VersionInfo(args.lkgm).VersionString()
    self._old_lkgm = None

    if not self._lkgm:
      raise LKGMNotValid('LKGM not provided.')

    logging.info('lkgm=%s', self._lkgm)

  def Run(self):
    self._committer.Cleanup()
    self._committer.Checkout(self._NEEDED_FILES)
    self.UpdateLKGM()
    self.CommitNewLKGM()
    self._committer.Upload()

  def CheckoutChrome(self):
    """Checks out chrome into tmp checkout_dir."""
    self._committer.Checkout(self._NEEDED_FILES)

  @property
  def lkgm_file(self):
    return self._committer.FullPath(constants.PATH_TO_CHROME_LKGM)

  def UpdateLKGM(self):
    """Updates the LKGM file with the new version."""
    lkgm_file = self.lkgm_file
    if not os.path.exists(lkgm_file):
      raise LKGMFileNotFound('%s is an invalid file' % lkgm_file)

    self._old_lkgm = osutils.ReadFile(lkgm_file)

    lv = distutils.version.LooseVersion
    if self._old_lkgm is not None and not lv(self._lkgm) > lv(self._old_lkgm):
      raise LKGMNotValid(
          'LKGM version (%s) is not newer than current version (%s).' %
          (self._lkgm, self._old_lkgm))

    logging.info('Updating LKGM version: %s (was %s),',
                 self._lkgm, self._old_lkgm)
    osutils.WriteFile(lkgm_file, self._lkgm)

  def CommitNewLKGM(self):
    """Commits the new LKGM file using our template commit message."""
    commit_msg = self._COMMIT_MSG_TEMPLATE % dict(version=self._lkgm)
    self._committer.Commit([constants.PATH_TO_CHROME_LKGM], commit_msg)


def GetArgs(argv):
  """Returns a dictionary of parsed args.

  Args:
    argv: raw command line.

  Returns:
    Dictionary of parsed args.
  """
  committer_parser = chrome_committer.ChromeCommitter.GetParser()
  parser = argparse.ArgumentParser(usage=__doc__,
                                   parents=[committer_parser])
  parser.add_argument('--lkgm', required=True,
                      help="LKGM version to update to.")
  return parser.parse_args(argv)

def main(argv):
  ChromeLKGMCommitter(GetArgs(argv)).Run()
  return 0
