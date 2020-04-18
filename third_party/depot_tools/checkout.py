# coding=utf8
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Manages a project checkout.

Includes support only for git.
"""

import fnmatch
import logging
import os
import re
import shutil
import subprocess
import sys
import tempfile

# The configparser module was renamed in Python 3.
try:
  import configparser
except ImportError:
  import ConfigParser as configparser

import patch
import scm
import subprocess2


if sys.platform in ('cygwin', 'win32'):
  # Disable timeouts on Windows since we can't have shells with timeouts.
  GLOBAL_TIMEOUT = None
  FETCH_TIMEOUT = None
else:
  # Default timeout of 15 minutes.
  GLOBAL_TIMEOUT = 15*60
  # Use a larger timeout for checkout since it can be a genuinely slower
  # operation.
  FETCH_TIMEOUT = 30*60


def get_code_review_setting(path, key,
    codereview_settings_file='codereview.settings'):
  """Parses codereview.settings and return the value for the key if present.

  Don't cache the values in case the file is changed."""
  # TODO(maruel): Do not duplicate code.
  settings = {}
  try:
    settings_file = open(os.path.join(path, codereview_settings_file), 'r')
    try:
      for line in settings_file.readlines():
        if not line or line.startswith('#'):
          continue
        if not ':' in line:
          # Invalid file.
          return None
        k, v = line.split(':', 1)
        settings[k.strip()] = v.strip()
    finally:
      settings_file.close()
  except IOError:
    return None
  return settings.get(key, None)


def align_stdout(stdout):
  """Returns the aligned output of multiple stdouts."""
  output = ''
  for item in stdout:
    item = item.strip()
    if not item:
      continue
    output += ''.join('  %s\n' % line for line in item.splitlines())
  return output


class PatchApplicationFailed(Exception):
  """Patch failed to be applied."""
  def __init__(self, errors, verbose):
    super(PatchApplicationFailed, self).__init__(errors, verbose)
    self.errors = errors
    self.verbose = verbose

  def __str__(self):
    out = []
    for e in self.errors:
      p, status = e
      if p and p.filename:
        out.append('Failed to apply patch for %s:' % p.filename)
      if status:
        out.append(status)
      if p and self.verbose:
        out.append('Patch: %s' % p.dump())
    return '\n'.join(out)


class CheckoutBase(object):
  # Set to None to have verbose output.
  VOID = subprocess2.VOID

  def __init__(self, root_dir, project_name, post_processors):
    """
    Args:
      post_processor: list of lambda(checkout, patches) to call on each of the
                      modified files.
    """
    super(CheckoutBase, self).__init__()
    self.root_dir = root_dir
    self.project_name = project_name
    if self.project_name is None:
      self.project_path = self.root_dir
    else:
      self.project_path = os.path.join(self.root_dir, self.project_name)
    # Only used for logging purposes.
    self._last_seen_revision = None
    self.post_processors = post_processors
    assert self.root_dir
    assert self.project_path
    assert os.path.isabs(self.project_path)

  def get_settings(self, key):
    return get_code_review_setting(self.project_path, key)

  def prepare(self, revision):
    """Checks out a clean copy of the tree and removes any local modification.

    This function shouldn't throw unless the remote repository is inaccessible,
    there is no free disk space or hard issues like that.

    Args:
      revision: The revision it should sync to, SCM specific.
    """
    raise NotImplementedError()

  def apply_patch(self, patches, post_processors=None, verbose=False):
    """Applies a patch and returns the list of modified files.

    This function should throw patch.UnsupportedPatchFormat or
    PatchApplicationFailed when relevant.

    Args:
      patches: patch.PatchSet object.
    """
    raise NotImplementedError()

  def commit(self, commit_message, user):
    """Commits the patch upstream, while impersonating 'user'."""
    raise NotImplementedError()

  def revisions(self, rev1, rev2):
    """Returns the count of revisions from rev1 to rev2, e.g. len(]rev1, rev2]).

    If rev2 is None, it means 'HEAD'.

    Returns None if there is no link between the two.
    """
    raise NotImplementedError()


class GitCheckout(CheckoutBase):
  """Manages a git checkout."""
  def __init__(self, root_dir, project_name, remote_branch, git_url,
      commit_user, post_processors=None):
    super(GitCheckout, self).__init__(root_dir, project_name, post_processors)
    self.git_url = git_url
    self.commit_user = commit_user
    self.remote_branch = remote_branch
    # The working branch where patches will be applied. It will track the
    # remote branch.
    self.working_branch = 'working_branch'
    # There is no reason to not hardcode origin.
    self.remote = 'origin'
    # There is no reason to not hardcode master.
    self.master_branch = 'master'

  def prepare(self, revision):
    """Resets the git repository in a clean state.

    Checks it out if not present and deletes the working branch.
    """
    assert self.remote_branch
    assert self.git_url

    if not os.path.isdir(self.project_path):
      # Clone the repo if the directory is not present.
      logging.info(
          'Checking out %s in %s', self.project_name, self.project_path)
      self._check_call_git(
          ['clone', self.git_url, '-b', self.remote_branch, self.project_path],
          cwd=None, timeout=FETCH_TIMEOUT)
    else:
      # Throw away all uncommitted changes in the existing checkout.
      self._check_call_git(['checkout', self.remote_branch])
      self._check_call_git(
          ['reset', '--hard', '--quiet',
           '%s/%s' % (self.remote, self.remote_branch)])

    if revision:
      try:
        # Look if the commit hash already exist. If so, we can skip a
        # 'git fetch' call.
        revision = self._check_output_git(['rev-parse', revision]).rstrip()
      except subprocess.CalledProcessError:
        self._check_call_git(
            ['fetch', self.remote, self.remote_branch, '--quiet'])
        revision = self._check_output_git(['rev-parse', revision]).rstrip()
      self._check_call_git(['checkout', '--force', '--quiet', revision])
    else:
      branches, active = self._branches()
      if active != self.master_branch:
        self._check_call_git(
            ['checkout', '--force', '--quiet', self.master_branch])
      self._sync_remote_branch()

      if self.working_branch in branches:
        self._call_git(['branch', '-D', self.working_branch])
    return self._get_head_commit_hash()

  def _sync_remote_branch(self):
    """Syncs the remote branch."""
    # We do a 'git pull origin master:refs/remotes/origin/master' instead of
    # 'git pull origin master' because from the manpage for git-pull:
    #   A parameter <ref> without a colon is equivalent to <ref>: when
    #   pulling/fetching, so it merges <ref> into the current branch without
    #   storing the remote branch anywhere locally.
    remote_tracked_path = 'refs/remotes/%s/%s' % (
        self.remote, self.remote_branch)
    self._check_call_git(
        ['pull', self.remote,
         '%s:%s' % (self.remote_branch, remote_tracked_path),
         '--quiet'])

  def _get_head_commit_hash(self):
    """Gets the current revision (in unicode) from the local branch."""
    return unicode(self._check_output_git(['rev-parse', 'HEAD']).strip())

  def apply_patch(self, patches, post_processors=None, verbose=False):
    """Applies a patch on 'working_branch' and switches to it.

    The changes remain staged on the current branch.
    """
    post_processors = post_processors or self.post_processors or []
    # It this throws, the checkout is corrupted. Maybe worth deleting it and
    # trying again?
    if self.remote_branch:
      self._check_call_git(
          ['checkout', '-b', self.working_branch, '-t', self.remote_branch,
           '--quiet'])

    errors = []
    for index, p in enumerate(patches):
      stdout = []
      try:
        filepath = os.path.join(self.project_path, p.filename)
        if p.is_delete:
          if (not os.path.exists(filepath) and
              any(p1.source_filename == p.filename for p1 in patches[0:index])):
            # The file was already deleted if a prior patch with file rename
            # was already processed because 'git apply' did it for us.
            pass
          else:
            stdout.append(self._check_output_git(['rm', p.filename]))
            assert(not os.path.exists(filepath))
            stdout.append('Deleted.')
        else:
          dirname = os.path.dirname(p.filename)
          full_dir = os.path.join(self.project_path, dirname)
          if dirname and not os.path.isdir(full_dir):
            os.makedirs(full_dir)
            stdout.append('Created missing directory %s.' % dirname)
          if p.is_binary:
            content = p.get()
            with open(filepath, 'wb') as f:
              f.write(content)
            stdout.append('Added binary file %d bytes' % len(content))
            cmd = ['add', p.filename]
            if verbose:
              cmd.append('--verbose')
            stdout.append(self._check_output_git(cmd))
          else:
            # No need to do anything special with p.is_new or if not
            # p.diff_hunks. git apply manages all that already.
            cmd = ['apply', '--index', '-3', '-p%s' % p.patchlevel]
            if verbose:
              cmd.append('--verbose')
            stdout.append(self._check_output_git(cmd, stdin=p.get(True)))
        for post in post_processors:
          post(self, p)
        if verbose:
          print p.filename
          print align_stdout(stdout)
      except OSError, e:
        errors.append((p, '%s%s' % (align_stdout(stdout), e)))
      except subprocess.CalledProcessError, e:
        errors.append((p,
            'While running %s;\n%s%s' % (
              ' '.join(e.cmd),
              align_stdout(stdout),
              align_stdout([getattr(e, 'stdout', '')]))))
    if errors:
      raise PatchApplicationFailed(errors, verbose)
    found_files = self._check_output_git(
        ['-c', 'core.quotePath=false', 'diff', '--ignore-submodules',
         '--name-only', '--staged']).splitlines(False)
    if sorted(patches.filenames) != sorted(found_files):
      extra_files = sorted(set(found_files) - set(patches.filenames))
      unpatched_files = sorted(set(patches.filenames) - set(found_files))
      if extra_files:
        print 'Found extra files: %r' % (extra_files,)
      if unpatched_files:
        print 'Found unpatched files: %r' % (unpatched_files,)


  def commit(self, commit_message, user):
    """Commits, updates the commit message and pushes."""
    # TODO(hinoka): CQ no longer uses this, I think its deprecated.
    #               Delete this.
    assert self.commit_user
    assert isinstance(commit_message, unicode)
    current_branch = self._check_output_git(
        ['rev-parse', '--abbrev-ref', 'HEAD']).strip()
    assert current_branch == self.working_branch

    commit_cmd = ['commit', '-m', commit_message]
    if user and user != self.commit_user:
      # We do not have the first or last name of the user, grab the username
      # from the email and call it the original author's name.
      # TODO(rmistry): Do not need the below if user is already in
      #                "Name <email>" format.
      name = user.split('@')[0]
      commit_cmd.extend(['--author', '%s <%s>' % (name, user)])
    self._check_call_git(commit_cmd)

    # Push to the remote repository.
    self._check_call_git(
        ['push', 'origin', '%s:%s' % (self.working_branch, self.remote_branch),
         '--quiet'])
    # Get the revision after the push.
    revision = self._get_head_commit_hash()
    # Switch back to the remote_branch and sync it.
    self._check_call_git(['checkout', self.remote_branch])
    self._sync_remote_branch()
    # Delete the working branch since we are done with it.
    self._check_call_git(['branch', '-D', self.working_branch])

    return revision

  def _check_call_git(self, args, **kwargs):
    kwargs.setdefault('cwd', self.project_path)
    kwargs.setdefault('stdout', self.VOID)
    kwargs.setdefault('timeout', GLOBAL_TIMEOUT)
    return subprocess2.check_call_out(['git'] + args, **kwargs)

  def _call_git(self, args, **kwargs):
    """Like check_call but doesn't throw on failure."""
    kwargs.setdefault('cwd', self.project_path)
    kwargs.setdefault('stdout', self.VOID)
    kwargs.setdefault('timeout', GLOBAL_TIMEOUT)
    return subprocess2.call(['git'] + args, **kwargs)

  def _check_output_git(self, args, **kwargs):
    kwargs.setdefault('cwd', self.project_path)
    kwargs.setdefault('timeout', GLOBAL_TIMEOUT)
    return subprocess2.check_output(
        ['git'] + args, stderr=subprocess2.STDOUT, **kwargs)

  def _branches(self):
    """Returns the list of branches and the active one."""
    out = self._check_output_git(['branch']).splitlines(False)
    branches = [l[2:] for l in out]
    active = None
    for l in out:
      if l.startswith('*'):
        active = l[2:]
        break
    return branches, active

  def revisions(self, rev1, rev2):
    """Returns the number of actual commits between both hash."""
    self._fetch_remote()

    rev2 = rev2 or '%s/%s' % (self.remote, self.remote_branch)
    # Revision range is ]rev1, rev2] and ordering matters.
    try:
      out = self._check_output_git(
          ['log', '--format="%H"' , '%s..%s' % (rev1, rev2)])
    except subprocess.CalledProcessError:
      return None
    return len(out.splitlines())

  def _fetch_remote(self):
    """Fetches the remote without rebasing."""
    # git fetch is always verbose even with -q, so redirect its output.
    self._check_output_git(['fetch', self.remote, self.remote_branch],
                           timeout=FETCH_TIMEOUT)


class ReadOnlyCheckout(object):
  """Converts a checkout into a read-only one."""
  def __init__(self, checkout, post_processors=None):
    super(ReadOnlyCheckout, self).__init__()
    self.checkout = checkout
    self.post_processors = (post_processors or []) + (
        self.checkout.post_processors or [])

  def prepare(self, revision):
    return self.checkout.prepare(revision)

  def get_settings(self, key):
    return self.checkout.get_settings(key)

  def apply_patch(self, patches, post_processors=None, verbose=False):
    return self.checkout.apply_patch(
        patches, post_processors or self.post_processors, verbose)

  def commit(self, message, user):  # pylint: disable=no-self-use
    logging.info('Would have committed for %s with message: %s' % (
        user, message))
    return 'FAKE'

  def revisions(self, rev1, rev2):
    return self.checkout.revisions(rev1, rev2)

  @property
  def project_name(self):
    return self.checkout.project_name

  @property
  def project_path(self):
    return self.checkout.project_path
