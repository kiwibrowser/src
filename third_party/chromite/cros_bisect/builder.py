# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Base class of builders.

It defines methods which builder should support: SetUp(), Build(), Deploy(),
SyncToHead(). Also, it sets up basic data members based on given options:
base_dir, board, and reuse_repo. And repo_dir is derived from base_dir.
"""

from __future__ import print_function

import os

from chromite.cros_bisect import common


class Builder(common.OptionsChecker):
  """Builder base class.

  It sets up basic data members based on given options: base_dir, board, and
  reuse_repo. And repo_dir is derived from base_dir.
  """

  EPILOG = """\
Builder base class.
"""
  # Default repo directory relative to base_dir. Can be overridden.
  DEFAULT_REPO_DIR = 'repo'

  REQUIRED_ARGS = ('base_dir', 'board', 'reuse_repo')

  def __init__(self, options):
    """Constructor.

    Args:
      options: An argparse.Namespace to hold command line arguments. Should
        contain:
        * base_dir: Base directory
        * board: Board name
        * reuse_repo: Reuse repo if available
    """
    super(Builder, self).__init__(options)
    self.options = options
    self.base_dir = options.base_dir
    self.board = options.board
    self.reuse_repo = options.reuse_repo

    # Path to repository. Can be overriden.
    self.repo_dir = os.path.join(self.base_dir, self.DEFAULT_REPO_DIR)

  def SetUp(self):
    """Sets up builder."""

  def Build(self, commit_label):
    """Builds binary.

    Args:
      commit_label: Commit label used for build archive path naming.

    Returns:
      Path to build to deploy. None if the commit fails to build.
    """

  def Deploy(self, remote, build_to_deploy, commit_label):
    """Deploys to DUT (device under test).

    Args:
      remote: DUT to deploy (refer lib.commandline.Device).
      build_to_deploy: Path to build to deploy.
      commit_label: Commit label used for logging.

    Returns:
      True if it deploys successfully. False otherwise.
    """

  def SyncToHead(self, fetch_tags=False):
    """Syncs the repo to origin/master.

    Args:
      fetch_tags: if set, also fetch tags.
    """
