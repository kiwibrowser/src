# -*- coding: utf-8 -*-
# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Commits files to the chromium git repository."""

from __future__ import print_function

import os

from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import tree_status


class CommitError(Exception):
  """Raised if the commit failed."""


class ChromeCommitter(object):
  """Committer object responsible for committing a git change."""

  def __init__(self, args):
    self._checkout_dir = args.workdir
    self._dryrun = args.dryrun
    self._git_committer_args = ['-c', 'user.email=%s' % args.user_email,
                                '-c', 'user.name=%s' % args.user_email]
    self._commit_msg = ''

    logging.info('user_email=%s', args.user_email)
    logging.info('checkout_dir=%s', args.workdir)

  def __del__(self):
    self.Cleanup()

  def FullPath(self, file_path):
    """Returns the full path in the source tree given a relative path.

    Args:
      file_path: Path of file.

    Returns:
      Full path rooted in source checkout.
    """
    if os.path.isabs(file_path):
      return file_path
    return os.path.join(self._checkout_dir, file_path)

  def Checkout(self, sparse_checkout):
    """Checks out chrome into tmp checkout dir.

    Args:
      sparse_checkout: List of file paths to fetch.
    """
    assert isinstance(sparse_checkout, list)
    sparse_checkout += ['codereview.settings', 'WATCHLISTS']
    git.ShallowFetch(self._checkout_dir, constants.CHROMIUM_GOB_URL,
                     sparse_checkout=sparse_checkout)
    git.CreateBranch(self._checkout_dir, 'auto-commit-branch',
                     branch_point='origin/master')

  def Commit(self, file_paths, commit_msg):
    """Commits files listed in |file_paths|.

    Args:
      file_paths: List of files to commit.
      commit_msg: Message to use in commit.
    """
    assert file_paths and isinstance(file_paths, list)
    # Make paths absolute and ensure they exist.
    for i, file_path in enumerate(file_paths):
      if not os.path.isabs(file_path):
        file_paths[i] = self.FullPath(file_path)
      if not os.path.exists(file_paths[i]):
        raise CommitError('Invalid path: %s' % file_paths[i])

    self._commit_msg = 'Automated Commit: ' + commit_msg
    try:
      for file_path in file_paths:
        git.AddPath(file_path)
      commit_args = ['commit', '-m', self._commit_msg]
      git.RunGit(self._checkout_dir, self._git_committer_args + commit_args,
                 print_cmd=True, redirect_stderr=True, capture_output=False)
    except cros_build_lib.RunCommandError as e:
      raise CommitError('Could not create git commit: %r' % e)

  def Upload(self):
    """Uploads the change to gerrit."""
    logging.info('Uploading commit.')

    try:
      # Run 'git cl upload' with --bypass-hooks to skip running scripts that are
      # not part of the shallow checkout, -f to skip editing the CL message,
      upload_args = ['cl', 'upload', '-v', '-m', self._commit_msg,
                     '--bypass-hooks', '-f']
      if not self._dryrun:
        # Add the gardener(s) as TBR; fall-back to tbr-owners.
        gardeners = tree_status.GetSheriffEmailAddresses('chrome')
        if gardeners:
          for tbr in gardeners:
            upload_args += ['--tbrs', tbr]
        else:
          upload_args += ['--tbr-owners']
        # Marks CL as ready.
        upload_args += ['--send-mail']
      git.RunGit(self._checkout_dir, self._git_committer_args + upload_args,
                 print_cmd=True, redirect_stderr=True, capture_output=False)

      # Flip the CQ commit bit.
      submit_args = ['cl', 'set-commit', '-v']
      if self._dryrun:
        submit_args += ['--dry-run']
      git.RunGit(self._checkout_dir, submit_args,
                 print_cmd=True, redirect_stderr=True, capture_output=False)
    except cros_build_lib.RunCommandError as e:
      # Log the change for debugging.
      git.RunGit(self._checkout_dir, ['--no-pager', 'log', '--pretty=full'],
                 capture_output=False)
      raise CommitError('Could not submit: %r' % e)

    logging.info('Submitted to CQ.')

  def Cleanup(self):
    """Remove chrome checkout."""
    osutils.RmDir(self._checkout_dir, ignore_missing=True)

  @staticmethod
  def GetParser():
    """Returns parser for ChromeCommitter.

    Returns:
      Dictionary of parsed command line args.
    """
    parser = commandline.ArgumentParser(usage=__doc__, add_help=False)
    parser.add_argument('--dryrun', action='store_true', default=False,
                        help='Don\'t commit changes or send out emails.')
    parser.add_argument('--user_email', required=False,
                        default='chromeos-commit-bot@chromium.org',
                        help='Email address to use when comitting changes.')
    parser.add_argument('--workdir',
                        default=os.path.join(os.getcwd(), 'chrome_src'),
                        help=('Path to a checkout of the chrome src. '
                              'Defaults to PWD/chrome_src'))
    return parser
