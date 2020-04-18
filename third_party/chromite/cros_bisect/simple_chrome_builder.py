# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Builds Chrome on ChromeOS."""

from __future__ import print_function

import os
import shutil

from chromite.cros_bisect import builder
from chromite.cbuildbot import commands
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gclient
from chromite.lib import git
from chromite.lib import osutils


# Chromium repo directory relative to base working directory.
CHROMIUM_DIR = 'chromium'


class SimpleChromeBuilder(builder.Builder):
  """Builds Chrome on ChromeOS."""

  EPILOG = """\
Sets up Chromium repository, syncs to the target commit, and builds Chrome on
ChromeOS (board specified.) Also, it can deploy the result to ChromeOS DUT
(device under test).
"""
  DEFAULT_REPO_DIR = os.path.join(CHROMIUM_DIR, 'src')

  REQUIRED_ARGS = builder.Builder.REQUIRED_ARGS + ('archive_build',
                                                   'reuse_build')

  def __init__(self, options):
    """Constructor.

    Args:
      options: In addition to the flags required by the base class, need to
        specify:
        * chromium_dir: Optional. If specified, use the chromium repo the path
          points to. Otherwise, use base_dir/chromium/src.
        * build_dir: Optional. Store build result to it if specified. Default:
          base_dir/build.
        * archive_build: True to archive build.
        * reuse_build: True to reuse previous build.
    """
    super(SimpleChromeBuilder, self).__init__(options)
    self.reuse_build = options.reuse_build
    self.archive_build = options.archive_build

    if 'chromium_dir' in options and options.chromium_dir:
      self.chromium_dir = options.chromium_dir
      self.repo_dir = os.path.join(self.chromium_dir, 'src')
    else:
      self.chromium_dir = os.path.join(self.base_dir, CHROMIUM_DIR)
      self.repo_dir = os.path.join(self.base_dir, self.DEFAULT_REPO_DIR)

    if 'build_dir' in options and options.build_dir:
      self.archive_base = options.build_dir
    else:
      self.archive_base = os.path.join(self.base_dir, 'build')
    if self.archive_build:
      osutils.SafeMakedirs(self.archive_base)

    self.gclient = osutils.Which('gclient')
    if not self.gclient:
      self.gclient = os.path.join(constants.DEPOT_TOOLS_DIR, 'gclient')

    self.verbose = logging.getLogger().isEnabledFor(logging.DEBUG)
    self.chrome_sdk = commands.ChromeSDK(self.repo_dir, self.board, goma=True,
                                         debug_log=self.verbose)

    # log_output=True: Instead of emitting output to STDOUT, redirect output to
    # logging.info.
    self.log_output_args = {'log_output': True}

  def SetUp(self):
    """Sets up Chromium repo.

    It skips repo set up if reuse_repo is set and repo exists.
    Setup steps:
    1. Make sure chromium_dir doesn't exist.
    2. mkdir chromium_dir and cd to it
    3. Run "fetch --nohooks chromium"
    4. Set up gclient config ('managed' set to False)
    5. In chromium_dir/src, run "git pull origin master"
    6. "gclient sync --nohooks"
    """
    if self.reuse_repo and os.path.exists(self.repo_dir):
      if git.IsGitRepo(self.repo_dir):
        return
      else:
        raise Exception('Chromium repo broken. Please manually remove it: %s' %
                        self.chromium_dir)
    if os.path.exists(self.chromium_dir):
      raise Exception('Chromium repo exists. Please manually remove it: %s' %
                      self.chromium_dir)

    osutils.SafeMakedirs(self.chromium_dir)
    cros_build_lib.RunCommand(['fetch', '--nohooks', 'chromium'],
                              cwd=self.chromium_dir,
                              log_output=True)
    # 'managed' should be set to False. Otherwise, 'gclient sync' will call
    # 'git pull' to ruin bisecting point.
    gclient.WriteConfigFile(self.gclient, self.chromium_dir, True, None,
                            managed=False)

    # Need to perform git pull before gclient sync when managed is set to False.
    git.RunGit(self.repo_dir, ['pull', 'origin', 'master'])
    self.GclientSync(reset=True, nohooks=True)

  def SyncToHead(self, fetch_tags=False):
    """Syncs the repo to origin/master."""
    git.CleanAndCheckoutUpstream(self.repo_dir)
    if fetch_tags:
      git.RunGit(self.repo_dir, ['fetch', '--tags'])

  def GclientSync(self, reset=False, nohooks=False):
    """Runs gclient sync.

    It also sets verbose argument and redirects "gclient sync" output to log.

    Args:
      reset: Reset to pristine version of the source code.
      nohooks: If set, add '--nohooks' argument.

    Returns:
      A CommandResult object.
    """
    return gclient.Sync(self.gclient, self.chromium_dir, reset=reset,
                        nohooks=nohooks, verbose=self.verbose,
                        run_args=self.log_output_args)

  def Build(self, commit_label):
    """Builds a Chromium for CrOS.

    If reuse_build, it first checks if the build exists. If so, uses it.
    Otherwise, it builds CrOS Chrome using SimpleChrome flow:
      http://www.chromium.org/chromium-os/how-tos-and-troubleshooting/
      building-chromium-browser
    and optionally archives the build output.

    Args:
      commit_label: Commit label used for build archive path naming.

    Returns:
      Path to build to deploy. None if it fails to build.
    """
    archive_path = os.path.join(
        self.archive_base, 'out_%s_%s' % (self.board, commit_label),
        'Release')
    # See if archive build available.
    if self.reuse_build:
      if os.path.isdir(archive_path):
        logging.info('Use archive build: %s', archive_path)
        return archive_path
      else:
        logging.info('Archive build not found: %s', archive_path)

    # Build CrOS Chrome.
    logging.info('Building chromium@%s by using SimpleChrome flow',
                 commit_label)
    # Clean build.
    build_out_dir = os.path.join('out_' + self.board, 'Release')
    build_out_full_path = os.path.join(self.repo_dir, build_out_dir)
    osutils.RmDir(build_out_full_path, ignore_missing=True)

    with cros_build_lib.TimedSection() as timer:
      self.GclientSync()
      error_step = None
      result = self.chrome_sdk.Run(
          ['bash', '-c', 'gn gen %s --args="$GN_ARGS"' % build_out_dir],
          run_args=self.log_output_args)
      if result.returncode:
        error_step = 'gn gen'
      else:
        result = self.chrome_sdk.Ninja(
            targets=['chrome', 'chrome_sandbox', 'nacl_helper'],
            run_args=self.log_output_args)
        if result.returncode:
          error_step = 'ninja'
      if error_step:
        logging.error(
            '%s for commit %s failed. returncode %d. stderr %s',
            error_step, commit_label, result.returncode, result.stderr)
        return None

    logging.info('Build successfully. Elapsed time: %s', timer.delta)
    if self.archive_build:
      logging.info('Archiving build from %s to %s', build_out_full_path,
                   archive_path)
      # mkdir parent of archive_path so that shutil.move can rename instead of
      # copying the 30GB build result.
      osutils.SafeMakedirs(os.path.dirname(archive_path))
      shutil.move(build_out_full_path, archive_path)
      return archive_path
    return build_out_dir

  def Deploy(self, remote, build_to_deploy, commit_label):
    """Deploys CrOS Chrome to DUT.

    Args:
      remote: DUT to deploy (refer lib.commandline.Device).
      build_to_deploy: Path to build to deploy.
      commit_label: Commit label used for logging.

    Returns:
      True if it deploys successfully. False otherwise.
    """
    logging.info('Deploying chromium(%s) to DUT: %s', commit_label, remote.raw)
    with cros_build_lib.TimedSection() as timer:
      # --force: for removing rootfs verification.
      command = ['deploy_chrome', '--build-dir', build_to_deploy,
                 '--to', remote.hostname, '--force']
      if remote.port:
        command.extend(['--port', str(remote.port)])
      result = self.chrome_sdk.Run(command, run_args=self.log_output_args)
      if result.returncode:
        logging.error('Deploy failed. returncode %d. stderr %s',
                      result.returncode, result.stderr)
        return False
    logging.info('Deploy successfully. Elapsed time: %s', timer.delta)
    return True
