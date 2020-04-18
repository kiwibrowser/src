#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""git drover: A tool for merging changes to release branches."""

import argparse
import cPickle
import functools
import logging
import os
import re
import shutil
import subprocess
import sys
import tempfile

import git_common


class Error(Exception):
  pass


_PATCH_ERROR_MESSAGE = """Patch failed to apply.

A workdir for this cherry-pick has been created in
  {0}

To continue, resolve the conflicts there and run
  git drover --continue {0}

To abort this cherry-pick run
  git drover --abort {0}
"""


class PatchError(Error):
  """An error indicating that the patch failed to apply."""

  def __init__(self, workdir):
    super(PatchError, self).__init__(_PATCH_ERROR_MESSAGE.format(workdir))


_DEV_NULL_FILE = open(os.devnull, 'w')

if os.name == 'nt':
  # This is a just-good-enough emulation of os.symlink for drover to work on
  # Windows. It uses junctioning of directories (most of the contents of
  # the .git directory), but copies files. Note that we can't use
  # CreateSymbolicLink or CreateHardLink here, as they both require elevation.
  # Creating reparse points is what we want for the directories, but doing so
  # is a relatively messy set of DeviceIoControl work at the API level, so we
  # simply shell to `mklink /j` instead.
  def emulate_symlink_windows(source, link_name):
    if os.path.isdir(source):
      subprocess.check_call(['mklink', '/j',
                             link_name.replace('/', '\\'),
                             source.replace('/', '\\')],
                            shell=True)
    else:
      shutil.copy(source, link_name)
  mk_symlink = emulate_symlink_windows
else:
  mk_symlink = os.symlink


class _Drover(object):

  def __init__(self, branch, revision, parent_repo, dry_run, verbose):
    self._branch = branch
    self._branch_ref = 'refs/remotes/branch-heads/%s' % branch
    self._revision = revision
    self._parent_repo = os.path.abspath(parent_repo)
    self._dry_run = dry_run
    self._workdir = None
    self._branch_name = None
    self._needs_cleanup = True
    self._verbose = verbose
    self._process_options()

  def _process_options(self):
    if self._verbose:
      logging.getLogger().setLevel(logging.DEBUG)


  @classmethod
  def resume(cls, workdir):
    """Continues a cherry-pick that required manual resolution.

    Args:
      workdir: A string containing the path to the workdir used by drover.
    """
    drover = cls._restore_drover(workdir)
    drover._continue()

  @classmethod
  def abort(cls, workdir):
    """Aborts a cherry-pick that required manual resolution.

    Args:
      workdir: A string containing the path to the workdir used by drover.
    """
    drover = cls._restore_drover(workdir)
    drover._cleanup()

  @staticmethod
  def _restore_drover(workdir):
    """Restores a saved drover state contained within a workdir.

    Args:
      workdir: A string containing the path to the workdir used by drover.
    """
    try:
      with open(os.path.join(workdir, '.git', 'drover'), 'rb') as f:
        drover = cPickle.load(f)
        drover._process_options()
        return drover
    except (IOError, cPickle.UnpicklingError):
      raise Error('%r is not git drover workdir' % workdir)

  def _continue(self):
    if os.path.exists(os.path.join(self._workdir, '.git', 'CHERRY_PICK_HEAD')):
      self._run_git_command(
          ['commit', '--no-edit'],
          error_message='All conflicts must be resolved before continuing')

    if self._upload_and_land():
      # Only clean up the workdir on success. The manually resolved cherry-pick
      # can be reused if the user cancels before landing.
      self._cleanup()

  def run(self):
    """Runs this Drover instance.

    Raises:
      Error: An error occurred while attempting to cherry-pick this change.
    """
    try:
      self._run_internal()
    finally:
      self._cleanup()

  def _run_internal(self):
    self._check_inputs()
    if not self._confirm('Going to cherry-pick\n"""\n%s"""\nto %s.' % (
        self._run_git_command(['show', '-s', self._revision]), self._branch)):
      return
    self._create_checkout()
    self._perform_cherry_pick()
    self._upload_and_land()

  def _cleanup(self):
    if not self._needs_cleanup:
      return

    if self._workdir:
      logging.debug('Deleting %s', self._workdir)
      if os.name == 'nt':
        try:
          # Use rmdir to properly handle the junctions we created.
          subprocess.check_call(
              ['rmdir', '/s', '/q', self._workdir], shell=True)
        except subprocess.CalledProcessError:
          logging.error(
              'Failed to delete workdir %r. Please remove it manually.',
              self._workdir)
      else:
        shutil.rmtree(self._workdir)
    self._workdir = None
    if self._branch_name:
      self._run_git_command(['branch', '-D', self._branch_name])

  @staticmethod
  def _confirm(message):
    """Show a confirmation prompt with the given message.

    Returns:
      A bool representing whether the user wishes to continue.
    """
    result = ''
    while result not in ('y', 'n'):
      try:
        result = raw_input('%s Continue (y/n)? ' % message)
      except EOFError:
        result = 'n'
    return result == 'y'

  def _check_inputs(self):
    """Check the input arguments and ensure the parent repo is up to date."""

    if not os.path.isdir(self._parent_repo):
      raise Error('Invalid parent repo path %r' % self._parent_repo)

    self._run_git_command(['--help'], error_message='Unable to run git')
    self._run_git_command(['status'],
                          error_message='%r is not a valid git repo' %
                          os.path.abspath(self._parent_repo))
    self._run_git_command(['fetch', 'origin'],
                          error_message='Failed to fetch origin')
    self._run_git_command(
        ['rev-parse', '%s^{commit}' % self._branch_ref],
        error_message='Branch %s not found' % self._branch_ref)
    self._run_git_command(
        ['rev-parse', '%s^{commit}' % self._revision],
        error_message='Revision "%s" not found' % self._revision)

  FILES_TO_LINK = [
      'refs',
      'logs/refs',
      'info/refs',
      'info/exclude',
      'objects',
      'hooks',
      'packed-refs',
      'remotes',
      'rr-cache',
  ]
  FILES_TO_COPY = ['config', 'HEAD']

  def _create_checkout(self):
    """Creates a checkout to use for cherry-picking.

    This creates a checkout similarly to git-new-workdir. Most of the .git
    directory is shared with the |self._parent_repo| using symlinks. This
    differs from git-new-workdir in that the config is forked instead of shared.
    This is so the new workdir can be a sparse checkout without affecting
    |self._parent_repo|.
    """
    parent_git_dir = os.path.join(self._parent_repo, self._run_git_command(
        ['rev-parse', '--git-dir']).strip())
    self._workdir = tempfile.mkdtemp(prefix='drover_%s_' % self._branch)
    logging.debug('Creating checkout in %s', self._workdir)
    git_dir = os.path.join(self._workdir, '.git')
    git_common.make_workdir_common(parent_git_dir, git_dir, self.FILES_TO_LINK,
                                   self.FILES_TO_COPY, mk_symlink)
    self._run_git_command(['config', 'core.sparsecheckout', 'true'])
    with open(os.path.join(git_dir, 'info', 'sparse-checkout'), 'w') as f:
      f.write('/codereview.settings')

    branch_name = os.path.split(self._workdir)[-1]
    self._run_git_command(['checkout', '-b', branch_name, self._branch_ref])
    self._branch_name = branch_name

  def _perform_cherry_pick(self):
    try:
      self._run_git_command(['cherry-pick', '-x', self._revision],
                            error_message='Patch failed to apply')
    except Error:
      self._prepare_manual_resolve()
      self._save_state()
      self._needs_cleanup = False
      raise PatchError(self._workdir)

  def _save_state(self):
    """Saves the state of this Drover instances to the workdir."""
    with open(os.path.join(self._workdir, '.git', 'drover'), 'wb') as f:
      cPickle.dump(self, f)

  def _prepare_manual_resolve(self):
    """Prepare the workdir for the user to manually resolve the cherry-pick."""
    # Files that have been deleted between branch and cherry-pick will not have
    # their skip-worktree bit set so set it manually for those files to avoid
    # git status incorrectly listing them as unstaged deletes.
    repo_status = self._run_git_command(
        ['-c', 'core.quotePath=false', 'status', '--porcelain']).splitlines()
    extra_files = [f[3:] for f in repo_status if f[:2] == ' D']
    if extra_files:
      self._run_git_command_with_stdin(
          ['update-index', '--skip-worktree', '--stdin'],
          stdin='\n'.join(extra_files) + '\n')

  def _upload_and_land(self):
    if self._dry_run:
      logging.info('--dry_run enabled; not landing.')
      return True

    self._run_git_command(['reset', '--hard'])

    author = self._run_git_command(['log', '-1', '--format=%ae']).strip()
    self._run_git_command(['cl', 'upload', '--send-mail', '--tbrs', author],
                          error_message='Upload failed',
                          interactive=True)

    if not self._confirm('About to land on %s.' % self._branch):
      return False
    self._run_git_command(['cl', 'land', '--bypass-hooks'], interactive=True)
    return True

  def _run_git_command(self, args, error_message=None, interactive=False):
    """Runs a git command.

    Args:
      args: A list of strings containing the args to pass to git.
      error_message: A string containing the error message to report if the
          command fails.
      interactive: A bool containing whether the command requires user
          interaction. If false, the command will be provided with no input and
          the output is captured.

    Returns:
      stdout as a string, or stdout interleaved with stderr if self._verbose

    Raises:
      Error: The command failed to complete successfully.
    """
    cwd = self._workdir if self._workdir else self._parent_repo
    logging.debug('Running git %s (cwd %r)', ' '.join('%s' % arg
                                                      for arg in args), cwd)

    run = subprocess.check_call if interactive else subprocess.check_output

    # Discard stderr unless verbose is enabled.
    stderr = None if self._verbose else _DEV_NULL_FILE

    try:
      return run(['git'] + args, shell=False, cwd=cwd, stderr=stderr)
    except (OSError, subprocess.CalledProcessError) as e:
      if error_message:
        raise Error(error_message)
      else:
        raise Error('Command %r failed: %s' % (' '.join(args), e))

  def _run_git_command_with_stdin(self, args, stdin):
    """Runs a git command with a provided stdin.

    Args:
      args: A list of strings containing the args to pass to git.
      stdin: A string to provide on stdin.

    Returns:
      stdout as a string, or stdout interleaved with stderr if self._verbose

    Raises:
      Error: The command failed to complete successfully.
    """
    cwd = self._workdir if self._workdir else self._parent_repo
    logging.debug('Running git %s (cwd %r)', ' '.join('%s' % arg
                                                      for arg in args), cwd)

    # Discard stderr unless verbose is enabled.
    stderr = None if self._verbose else _DEV_NULL_FILE

    try:
      popen = subprocess.Popen(['git'] + args, shell=False, cwd=cwd,
                               stderr=stderr, stdin=subprocess.PIPE)
      popen.communicate(stdin)
      if popen.returncode != 0:
        raise Error('Command %r failed' % ' '.join(args))
    except OSError as e:
      raise Error('Command %r failed: %s' % (' '.join(args), e))


def cherry_pick_change(branch, revision, parent_repo, dry_run, verbose=False):
  """Cherry-picks a change into a branch.

  Args:
    branch: A string containing the release branch number to which to
        cherry-pick.
    revision: A string containing the revision to cherry-pick. It can be any
        string that git-rev-parse can identify as referring to a single
        revision.
    parent_repo: A string containing the path to the parent repo to use for this
        cherry-pick.
    dry_run: A bool containing whether to stop before uploading the
        cherry-pick cl.
    verbose: A bool containing whether to print verbose logging.

  Raises:
    Error: An error occurred while attempting to cherry-pick |cl| to |branch|.
  """
  drover = _Drover(branch, revision, parent_repo, dry_run, verbose)
  drover.run()


def continue_cherry_pick(workdir):
  """Continues a cherry-pick that required manual resolution.

  Args:
    workdir: A string containing the path to the workdir used by drover.
  """
  _Drover.resume(workdir)


def abort_cherry_pick(workdir):
  """Aborts a cherry-pick that required manual resolution.

  Args:
    workdir: A string containing the path to the workdir used by drover.
  """
  _Drover.abort(workdir)


def main():
  parser = argparse.ArgumentParser(
      description='Cherry-pick a change into a release branch.')
  group = parser.add_mutually_exclusive_group(required=True)
  parser.add_argument(
      '--branch',
      type=str,
      metavar='<branch>',
      help='the name of the branch to which to cherry-pick; e.g. 1234')
  group.add_argument(
      '--cherry-pick',
      type=str,
      metavar='<change>',
      help=('the change to cherry-pick; this can be any string '
            'that unambiguously refers to a revision not involving HEAD'))
  group.add_argument(
      '--continue',
      type=str,
      nargs='?',
      dest='resume',
      const=os.path.abspath('.'),
      metavar='path_to_workdir',
      help='Continue a drover cherry-pick after resolving conflicts')
  group.add_argument('--abort',
                     type=str,
                     nargs='?',
                     const=os.path.abspath('.'),
                     metavar='path_to_workdir',
                     help='Abort a drover cherry-pick')
  parser.add_argument(
      '--parent_checkout',
      type=str,
      default=os.path.abspath('.'),
      metavar='<path_to_parent_checkout>',
      help=('the path to the chromium checkout to use as the source for a '
            'creating git-new-workdir workdir to use for cherry-picking; '
            'if unspecified, the current directory is used'))
  parser.add_argument(
      '--dry-run',
      action='store_true',
      default=False,
      help=("don't actually upload and land; "
            "just check that cherry-picking would succeed"))
  parser.add_argument('-v',
                      '--verbose',
                      action='store_true',
                      default=False,
                      help='show verbose logging')
  options = parser.parse_args()
  try:
    if options.resume:
      _Drover.resume(options.resume)
    elif options.abort:
      _Drover.abort(options.abort)
    else:
      if not options.branch:
        parser.error('argument --branch is required for --cherry-pick')
      cherry_pick_change(options.branch, options.cherry_pick,
                         options.parent_checkout, options.dry_run,
                         options.verbose)
  except Error as e:
    print 'Error:', e.message
    sys.exit(128)


if __name__ == '__main__':
  main()
