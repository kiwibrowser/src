# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Gclient-specific SCM-specific operations."""

from __future__ import print_function

import collections
import contextlib
import errno
import json
import logging
import os
import posixpath
import re
import sys
import tempfile
import threading
import traceback
import urlparse

import download_from_google_storage
import gclient_utils
import git_cache
import scm
import shutil
import subprocess2


THIS_FILE_PATH = os.path.abspath(__file__)

GSUTIL_DEFAULT_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), 'gsutil.py')


class NoUsableRevError(gclient_utils.Error):
  """Raised if requested revision isn't found in checkout."""


class DiffFiltererWrapper(object):
  """Simple base class which tracks which file is being diffed and
  replaces instances of its file name in the original and
  working copy lines of the git diff output."""
  index_string = None
  original_prefix = "--- "
  working_prefix = "+++ "

  def __init__(self, relpath, print_func):
    # Note that we always use '/' as the path separator to be
    # consistent with cygwin-style output on Windows
    self._relpath = relpath.replace("\\", "/")
    self._current_file = None
    self._print_func = print_func

  def SetCurrentFile(self, current_file):
    self._current_file = current_file

  @property
  def _replacement_file(self):
    return posixpath.join(self._relpath, self._current_file)

  def _Replace(self, line):
    return line.replace(self._current_file, self._replacement_file)

  def Filter(self, line):
    if (line.startswith(self.index_string)):
      self.SetCurrentFile(line[len(self.index_string):])
      line = self._Replace(line)
    else:
      if (line.startswith(self.original_prefix) or
          line.startswith(self.working_prefix)):
        line = self._Replace(line)
    self._print_func(line)


class GitDiffFilterer(DiffFiltererWrapper):
  index_string = "diff --git "

  def SetCurrentFile(self, current_file):
    # Get filename by parsing "a/<filename> b/<filename>"
    self._current_file = current_file[:(len(current_file)/2)][2:]

  def _Replace(self, line):
    return re.sub("[a|b]/" + self._current_file, self._replacement_file, line)


# SCMWrapper base class

class SCMWrapper(object):
  """Add necessary glue between all the supported SCM.

  This is the abstraction layer to bind to different SCM.
  """

  def __init__(self, url=None, root_dir=None, relpath=None, out_fh=None,
               out_cb=None, print_outbuf=False):
    self.url = url
    self._root_dir = root_dir
    if self._root_dir:
      self._root_dir = self._root_dir.replace('/', os.sep)
    self.relpath = relpath
    if self.relpath:
      self.relpath = self.relpath.replace('/', os.sep)
    if self.relpath and self._root_dir:
      self.checkout_path = os.path.join(self._root_dir, self.relpath)
    if out_fh is None:
      out_fh = sys.stdout
    self.out_fh = out_fh
    self.out_cb = out_cb
    self.print_outbuf = print_outbuf

  def Print(self, *args, **kwargs):
    kwargs.setdefault('file', self.out_fh)
    if kwargs.pop('timestamp', True):
      self.out_fh.write('[%s] ' % gclient_utils.Elapsed())
    print(*args, **kwargs)

  def RunCommand(self, command, options, args, file_list=None):
    commands = ['update', 'updatesingle', 'revert',
                'revinfo', 'status', 'diff', 'pack', 'runhooks']

    if not command in commands:
      raise gclient_utils.Error('Unknown command %s' % command)

    if not command in dir(self):
      raise gclient_utils.Error('Command %s not implemented in %s wrapper' % (
          command, self.__class__.__name__))

    return getattr(self, command)(options, args, file_list)

  @staticmethod
  def _get_first_remote_url(checkout_path):
    log = scm.GIT.Capture(
        ['config', '--local', '--get-regexp', r'remote.*.url'],
        cwd=checkout_path)
    # Get the second token of the first line of the log.
    return log.splitlines()[0].split(' ', 1)[1]

  def GetCacheMirror(self):
    if (getattr(self, 'cache_dir', None)):
      url, _ = gclient_utils.SplitUrlRevision(self.url)
      return git_cache.Mirror(url)
    return None

  def GetActualRemoteURL(self, options):
    """Attempt to determine the remote URL for this SCMWrapper."""
    # Git
    if os.path.exists(os.path.join(self.checkout_path, '.git')):
      actual_remote_url = self._get_first_remote_url(self.checkout_path)

      mirror = self.GetCacheMirror()
      # If the cache is used, obtain the actual remote URL from there.
      if (mirror and mirror.exists() and
          mirror.mirror_path.replace('\\', '/') ==
          actual_remote_url.replace('\\', '/')):
        actual_remote_url = self._get_first_remote_url(mirror.mirror_path)
      return actual_remote_url
    return None

  def DoesRemoteURLMatch(self, options):
    """Determine whether the remote URL of this checkout is the expected URL."""
    if not os.path.exists(self.checkout_path):
      # A checkout which doesn't exist can't be broken.
      return True

    actual_remote_url = self.GetActualRemoteURL(options)
    if actual_remote_url:
      return (gclient_utils.SplitUrlRevision(actual_remote_url)[0].rstrip('/')
              == gclient_utils.SplitUrlRevision(self.url)[0].rstrip('/'))
    else:
      # This may occur if the self.checkout_path exists but does not contain a
      # valid git checkout.
      return False

  def _DeleteOrMove(self, force):
    """Delete the checkout directory or move it out of the way.

    Args:
        force: bool; if True, delete the directory. Otherwise, just move it.
    """
    if force and os.environ.get('CHROME_HEADLESS') == '1':
      self.Print('_____ Conflicting directory found in %s. Removing.'
                 % self.checkout_path)
      gclient_utils.AddWarning('Conflicting directory %s deleted.'
                               % self.checkout_path)
      gclient_utils.rmtree(self.checkout_path)
    else:
      bad_scm_dir = os.path.join(self._root_dir, '_bad_scm',
                                 os.path.dirname(self.relpath))

      try:
        os.makedirs(bad_scm_dir)
      except OSError as e:
        if e.errno != errno.EEXIST:
          raise

      dest_path = tempfile.mkdtemp(
          prefix=os.path.basename(self.relpath),
          dir=bad_scm_dir)
      self.Print('_____ Conflicting directory found in %s. Moving to %s.'
                 % (self.checkout_path, dest_path))
      gclient_utils.AddWarning('Conflicting directory %s moved to %s.'
                               % (self.checkout_path, dest_path))
      shutil.move(self.checkout_path, dest_path)


class GitWrapper(SCMWrapper):
  """Wrapper for Git"""
  name = 'git'
  remote = 'origin'

  cache_dir = None

  def __init__(self, url=None, *args, **kwargs):
    """Removes 'git+' fake prefix from git URL."""
    if url.startswith('git+http://') or url.startswith('git+https://'):
      url = url[4:]
    SCMWrapper.__init__(self, url, *args, **kwargs)
    filter_kwargs = { 'time_throttle': 1, 'out_fh': self.out_fh }
    if self.out_cb:
      filter_kwargs['predicate'] = self.out_cb
    self.filter = gclient_utils.GitFilter(**filter_kwargs)

  @staticmethod
  def BinaryExists():
    """Returns true if the command exists."""
    try:
      # We assume git is newer than 1.7.  See: crbug.com/114483
      result, version = scm.GIT.AssertVersion('1.7')
      if not result:
        raise gclient_utils.Error('Git version is older than 1.7: %s' % version)
      return result
    except OSError:
      return False

  def GetCheckoutRoot(self):
    return scm.GIT.GetCheckoutRoot(self.checkout_path)

  def GetRevisionDate(self, _revision):
    """Returns the given revision's date in ISO-8601 format (which contains the
    time zone)."""
    # TODO(floitsch): get the time-stamp of the given revision and not just the
    # time-stamp of the currently checked out revision.
    return self._Capture(['log', '-n', '1', '--format=%ai'])

  def _GetDiffFilenames(self, base):
    """Returns the names of files modified since base."""
    return self._Capture(
      # Filter to remove base if it is None.
      filter(bool, ['-c', 'core.quotePath=false', 'diff', '--name-only', base])
    ).split()

  def diff(self, options, _args, _file_list):
    _, revision = gclient_utils.SplitUrlRevision(self.url)
    if not revision:
      revision = 'refs/remotes/%s/master' % self.remote
    self._Run(['-c', 'core.quotePath=false', 'diff', revision], options)

  def pack(self, _options, _args, _file_list):
    """Generates a patch file which can be applied to the root of the
    repository.

    The patch file is generated from a diff of the merge base of HEAD and
    its upstream branch.
    """
    try:
      merge_base = [self._Capture(['merge-base', 'HEAD', self.remote])]
    except subprocess2.CalledProcessError:
      merge_base = []
    gclient_utils.CheckCallAndFilter(
        ['git', 'diff'] + merge_base,
        cwd=self.checkout_path,
        filter_fn=GitDiffFilterer(self.relpath, print_func=self.Print).Filter)

  def _Scrub(self, target, options):
    """Scrubs out all changes in the local repo, back to the state of target."""
    quiet = []
    if not options.verbose:
      quiet = ['--quiet']
    self._Run(['reset', '--hard', target] + quiet, options)
    if options.force and options.delete_unversioned_trees:
      # where `target` is a commit that contains both upper and lower case
      # versions of the same file on a case insensitive filesystem, we are
      # actually in a broken state here. The index will have both 'a' and 'A',
      # but only one of them will exist on the disk. To progress, we delete
      # everything that status thinks is modified.
      output = self._Capture([
          '-c', 'core.quotePath=false', 'status', '--porcelain'], strip=False)
      for line in output.splitlines():
        # --porcelain (v1) looks like:
        # XY filename
        try:
          filename = line[3:]
          self.Print('_____ Deleting residual after reset: %r.' % filename)
          gclient_utils.rm_file_or_tree(
            os.path.join(self.checkout_path, filename))
        except OSError:
          pass

  def _FetchAndReset(self, revision, file_list, options):
    """Equivalent to git fetch; git reset."""
    self._UpdateBranchHeads(options, fetch=False)

    self._Fetch(options, prune=True, quiet=options.verbose)
    self._Scrub(revision, options)
    if file_list is not None:
      files = self._Capture(
          ['-c', 'core.quotePath=false', 'ls-files']).splitlines()
      file_list.extend([os.path.join(self.checkout_path, f) for f in files])

  def _DisableHooks(self):
    hook_dir = os.path.join(self.checkout_path, '.git', 'hooks')
    if not os.path.isdir(hook_dir):
      return
    for f in os.listdir(hook_dir):
      if not f.endswith('.sample') and not f.endswith('.disabled'):
        disabled_hook_path = os.path.join(hook_dir, f + '.disabled')
        if os.path.exists(disabled_hook_path):
          os.remove(disabled_hook_path)
        os.rename(os.path.join(hook_dir, f), disabled_hook_path)

  def _maybe_break_locks(self, options):
    """This removes all .lock files from this repo's .git directory, if the
    user passed the --break_repo_locks command line flag.

    In particular, this will cleanup index.lock files, as well as ref lock
    files.
    """
    if options.break_repo_locks:
      git_dir = os.path.join(self.checkout_path, '.git')
      for path, _, filenames in os.walk(git_dir):
        for filename in filenames:
          if filename.endswith('.lock'):
            to_break = os.path.join(path, filename)
            self.Print('breaking lock: %s' % (to_break,))
            try:
              os.remove(to_break)
            except OSError as ex:
              self.Print('FAILED to break lock: %s: %s' % (to_break, ex))
              raise

  def apply_patch_ref(self, patch_repo, patch_ref, options, file_list):
    base_rev = self._Capture(['rev-parse', 'HEAD'])
    self.Print('===Applying patch ref===')
    self.Print('Repo is %r @ %r, ref is %r, root is %r' % (
        patch_repo, patch_ref, base_rev, self.checkout_path))
    self._Capture(['reset', '--hard'])
    self._Capture(['fetch', patch_repo, patch_ref])
    if file_list is not None:
      file_list.extend(self._GetDiffFilenames('FETCH_HEAD'))
    self._Capture(['checkout', 'FETCH_HEAD'])

    if options.rebase_patch_ref:
      try:
        # TODO(ehmaldonado): Look into cherry-picking to avoid an expensive
        # checkout + rebase.
        self._Capture(['rebase', base_rev])
      except subprocess2.CalledProcessError as e:
        self.Print('Failed to apply %r @ %r to %r at %r' % (
                patch_repo, patch_ref, base_rev, self.checkout_path))
        self.Print('git returned non-zero exit status %s:\n%s' % (
            e.returncode, e.stderr))
        self._Capture(['rebase', '--abort'])
        raise
    if options.reset_patch_ref:
      self._Capture(['reset', '--soft', base_rev])

  def update(self, options, args, file_list):
    """Runs git to update or transparently checkout the working copy.

    All updated files will be appended to file_list.

    Raises:
      Error: if can't get URL for relative path.
    """
    if args:
      raise gclient_utils.Error("Unsupported argument(s): %s" % ",".join(args))

    self._CheckMinVersion("1.6.6")

    # If a dependency is not pinned, track the default remote branch.
    default_rev = 'refs/remotes/%s/master' % self.remote
    url, deps_revision = gclient_utils.SplitUrlRevision(self.url)
    revision = deps_revision
    managed = True
    if options.revision:
      # Override the revision number.
      revision = str(options.revision)
    if revision == 'unmanaged':
      # Check again for a revision in case an initial ref was specified
      # in the url, for example bla.git@refs/heads/custombranch
      revision = deps_revision
      managed = False
    if not revision:
      revision = default_rev

    if managed:
      self._DisableHooks()

    printed_path = False
    verbose = []
    if options.verbose:
      self.Print('_____ %s at %s' % (self.relpath, revision), timestamp=False)
      verbose = ['--verbose']
      printed_path = True

    remote_ref = scm.GIT.RefToRemoteRef(revision, self.remote)
    if remote_ref:
      # Rewrite remote refs to their local equivalents.
      revision = ''.join(remote_ref)
      rev_type = "branch"
    elif revision.startswith('refs/'):
      # Local branch? We probably don't want to support, since DEPS should
      # always specify branches as they are in the upstream repo.
      rev_type = "branch"
    else:
      # hash is also a tag, only make a distinction at checkout
      rev_type = "hash"

    mirror = self._GetMirror(url, options)
    if mirror:
      url = mirror.mirror_path

    # If we are going to introduce a new project, there is a possibility that
    # we are syncing back to a state where the project was originally a
    # sub-project rolled by DEPS (realistic case: crossing the Blink merge point
    # syncing backwards, when Blink was a DEPS entry and not part of src.git).
    # In such case, we might have a backup of the former .git folder, which can
    # be used to avoid re-fetching the entire repo again (useful for bisects).
    backup_dir = self.GetGitBackupDirPath()
    target_dir = os.path.join(self.checkout_path, '.git')
    if os.path.exists(backup_dir) and not os.path.exists(target_dir):
      gclient_utils.safe_makedirs(self.checkout_path)
      os.rename(backup_dir, target_dir)
      # Reset to a clean state
      self._Scrub('HEAD', options)

    if (not os.path.exists(self.checkout_path) or
        (os.path.isdir(self.checkout_path) and
         not os.path.exists(os.path.join(self.checkout_path, '.git')))):
      if mirror:
        self._UpdateMirrorIfNotContains(mirror, options, rev_type, revision)
      try:
        self._Clone(revision, url, options)
      except subprocess2.CalledProcessError:
        self._DeleteOrMove(options.force)
        self._Clone(revision, url, options)
      if file_list is not None:
        files = self._Capture(
          ['-c', 'core.quotePath=false', 'ls-files']).splitlines()
        file_list.extend([os.path.join(self.checkout_path, f) for f in files])
      if not verbose:
        # Make the output a little prettier. It's nice to have some whitespace
        # between projects when cloning.
        self.Print('')
      return self._Capture(['rev-parse', '--verify', 'HEAD'])

    if not managed:
      self._UpdateBranchHeads(options, fetch=False)
      self.Print('________ unmanaged solution; skipping %s' % self.relpath)
      return self._Capture(['rev-parse', '--verify', 'HEAD'])

    self._maybe_break_locks(options)

    if mirror:
      self._UpdateMirrorIfNotContains(mirror, options, rev_type, revision)

    # See if the url has changed (the unittests use git://foo for the url, let
    # that through).
    current_url = self._Capture(['config', 'remote.%s.url' % self.remote])
    return_early = False
    # TODO(maruel): Delete url != 'git://foo' since it's just to make the
    # unit test pass. (and update the comment above)
    # Skip url auto-correction if remote.origin.gclient-auto-fix-url is set.
    # This allows devs to use experimental repos which have a different url
    # but whose branch(s) are the same as official repos.
    if (current_url.rstrip('/') != url.rstrip('/') and
        url != 'git://foo' and
        subprocess2.capture(
            ['git', 'config', 'remote.%s.gclient-auto-fix-url' % self.remote],
            cwd=self.checkout_path).strip() != 'False'):
      self.Print('_____ switching %s to a new upstream' % self.relpath)
      if not (options.force or options.reset):
        # Make sure it's clean
        self._CheckClean(revision)
      # Switch over to the new upstream
      self._Run(['remote', 'set-url', self.remote, url], options)
      if mirror:
        with open(os.path.join(
            self.checkout_path, '.git', 'objects', 'info', 'alternates'),
            'w') as fh:
          fh.write(os.path.join(url, 'objects'))
      self._EnsureValidHeadObjectOrCheckout(revision, options, url)
      self._FetchAndReset(revision, file_list, options)

      return_early = True
    else:
      self._EnsureValidHeadObjectOrCheckout(revision, options, url)

    if return_early:
      return self._Capture(['rev-parse', '--verify', 'HEAD'])

    cur_branch = self._GetCurrentBranch()

    # Cases:
    # 0) HEAD is detached. Probably from our initial clone.
    #   - make sure HEAD is contained by a named ref, then update.
    # Cases 1-4. HEAD is a branch.
    # 1) current branch is not tracking a remote branch
    #   - try to rebase onto the new hash or branch
    # 2) current branch is tracking a remote branch with local committed
    #    changes, but the DEPS file switched to point to a hash
    #   - rebase those changes on top of the hash
    # 3) current branch is tracking a remote branch w/or w/out changes, and
    #    no DEPS switch
    #   - see if we can FF, if not, prompt the user for rebase, merge, or stop
    # 4) current branch is tracking a remote branch, but DEPS switches to a
    #    different remote branch, and
    #   a) current branch has no local changes, and --force:
    #      - checkout new branch
    #   b) current branch has local changes, and --force and --reset:
    #      - checkout new branch
    #   c) otherwise exit

    # GetUpstreamBranch returns something like 'refs/remotes/origin/master' for
    # a tracking branch
    # or 'master' if not a tracking branch (it's based on a specific rev/hash)
    # or it returns None if it couldn't find an upstream
    if cur_branch is None:
      upstream_branch = None
      current_type = "detached"
      logging.debug("Detached HEAD")
    else:
      upstream_branch = scm.GIT.GetUpstreamBranch(self.checkout_path)
      if not upstream_branch or not upstream_branch.startswith('refs/remotes'):
        current_type = "hash"
        logging.debug("Current branch is not tracking an upstream (remote)"
                      " branch.")
      elif upstream_branch.startswith('refs/remotes'):
        current_type = "branch"
      else:
        raise gclient_utils.Error('Invalid Upstream: %s' % upstream_branch)

    if not scm.GIT.IsValidRevision(self.checkout_path, revision, sha_only=True):
      # Update the remotes first so we have all the refs.
      remote_output = scm.GIT.Capture(['remote'] + verbose + ['update'],
              cwd=self.checkout_path)
      if verbose:
        self.Print(remote_output)

    self._UpdateBranchHeads(options, fetch=True)

    revision = self._AutoFetchRef(options, revision)

    # This is a big hammer, debatable if it should even be here...
    if options.force or options.reset:
      target = 'HEAD'
      if options.upstream and upstream_branch:
        target = upstream_branch
      self._Scrub(target, options)

    if current_type == 'detached':
      # case 0
      # We just did a Scrub, this is as clean as it's going to get. In
      # particular if HEAD is a commit that contains two versions of the same
      # file on a case-insensitive filesystem (e.g. 'a' and 'A'), there's no way
      # to actually "Clean" the checkout; that commit is uncheckoutable on this
      # system. The best we can do is carry forward to the checkout step.
      if not (options.force or options.reset):
        self._CheckClean(revision)
      self._CheckDetachedHead(revision, options)
      if self._Capture(['rev-list', '-n', '1', 'HEAD']) == revision:
        self.Print('Up-to-date; skipping checkout.')
      else:
        # 'git checkout' may need to overwrite existing untracked files. Allow
        # it only when nuclear options are enabled.
        self._Checkout(
            options,
            revision,
            force=(options.force and options.delete_unversioned_trees),
            quiet=True,
        )
      if not printed_path:
        self.Print('_____ %s at %s' % (self.relpath, revision), timestamp=False)
    elif current_type == 'hash':
      # case 1
      # Can't find a merge-base since we don't know our upstream. That makes
      # this command VERY likely to produce a rebase failure. For now we
      # assume origin is our upstream since that's what the old behavior was.
      upstream_branch = self.remote
      if options.revision or deps_revision:
        upstream_branch = revision
      self._AttemptRebase(upstream_branch, file_list, options,
                          printed_path=printed_path, merge=options.merge)
      printed_path = True
    elif rev_type == 'hash':
      # case 2
      self._AttemptRebase(upstream_branch, file_list, options,
                          newbase=revision, printed_path=printed_path,
                          merge=options.merge)
      printed_path = True
    elif remote_ref and ''.join(remote_ref) != upstream_branch:
      # case 4
      new_base = ''.join(remote_ref)
      if not printed_path:
        self.Print('_____ %s at %s' % (self.relpath, revision), timestamp=False)
      switch_error = ("Could not switch upstream branch from %s to %s\n"
                     % (upstream_branch, new_base) +
                     "Please use --force or merge or rebase manually:\n" +
                     "cd %s; git rebase %s\n" % (self.checkout_path, new_base) +
                     "OR git checkout -b <some new branch> %s" % new_base)
      force_switch = False
      if options.force:
        try:
          self._CheckClean(revision)
          # case 4a
          force_switch = True
        except gclient_utils.Error as e:
          if options.reset:
            # case 4b
            force_switch = True
          else:
            switch_error = '%s\n%s' % (e.message, switch_error)
      if force_switch:
        self.Print("Switching upstream branch from %s to %s" %
                   (upstream_branch, new_base))
        switch_branch = 'gclient_' + remote_ref[1]
        self._Capture(['branch', '-f', switch_branch, new_base])
        self._Checkout(options, switch_branch, force=True, quiet=True)
      else:
        # case 4c
        raise gclient_utils.Error(switch_error)
    else:
      # case 3 - the default case
      rebase_files = self._GetDiffFilenames(upstream_branch)
      if verbose:
        self.Print('Trying fast-forward merge to branch : %s' % upstream_branch)
      try:
        merge_args = ['merge']
        if options.merge:
          merge_args.append('--ff')
        else:
          merge_args.append('--ff-only')
        merge_args.append(upstream_branch)
        merge_output = self._Capture(merge_args)
      except subprocess2.CalledProcessError as e:
        rebase_files = []
        if re.match('fatal: Not possible to fast-forward, aborting.', e.stderr):
          if not printed_path:
            self.Print('_____ %s at %s' % (self.relpath, revision),
                       timestamp=False)
            printed_path = True
          while True:
            if not options.auto_rebase:
              try:
                action = self._AskForData(
                    'Cannot %s, attempt to rebase? '
                    '(y)es / (q)uit / (s)kip : ' %
                        ('merge' if options.merge else 'fast-forward merge'),
                    options)
              except ValueError:
                raise gclient_utils.Error('Invalid Character')
            if options.auto_rebase or re.match(r'yes|y', action, re.I):
              self._AttemptRebase(upstream_branch, rebase_files, options,
                                  printed_path=printed_path, merge=False)
              printed_path = True
              break
            elif re.match(r'quit|q', action, re.I):
              raise gclient_utils.Error("Can't fast-forward, please merge or "
                                        "rebase manually.\n"
                                        "cd %s && git " % self.checkout_path
                                        + "rebase %s" % upstream_branch)
            elif re.match(r'skip|s', action, re.I):
              self.Print('Skipping %s' % self.relpath)
              return
            else:
              self.Print('Input not recognized')
        elif re.match("error: Your local changes to '.*' would be "
                      "overwritten by merge.  Aborting.\nPlease, commit your "
                      "changes or stash them before you can merge.\n",
                      e.stderr):
          if not printed_path:
            self.Print('_____ %s at %s' % (self.relpath, revision),
                       timestamp=False)
            printed_path = True
          raise gclient_utils.Error(e.stderr)
        else:
          # Some other problem happened with the merge
          logging.error("Error during fast-forward merge in %s!" % self.relpath)
          self.Print(e.stderr)
          raise
      else:
        # Fast-forward merge was successful
        if not re.match('Already up-to-date.', merge_output) or verbose:
          if not printed_path:
            self.Print('_____ %s at %s' % (self.relpath, revision),
                       timestamp=False)
            printed_path = True
          self.Print(merge_output.strip())
          if not verbose:
            # Make the output a little prettier. It's nice to have some
            # whitespace between projects when syncing.
            self.Print('')

      if file_list is not None:
        file_list.extend(
            [os.path.join(self.checkout_path, f) for f in rebase_files])

    # If the rebase generated a conflict, abort and ask user to fix
    if self._IsRebasing():
      raise gclient_utils.Error('\n____ %s at %s\n'
                                '\nConflict while rebasing this branch.\n'
                                'Fix the conflict and run gclient again.\n'
                                'See man git-rebase for details.\n'
                                % (self.relpath, revision))

    if verbose:
      self.Print('Checked out revision %s' % self.revinfo(options, (), None),
                 timestamp=False)

    # If --reset and --delete_unversioned_trees are specified, remove any
    # untracked directories.
    if options.reset and options.delete_unversioned_trees:
      # GIT.CaptureStatus() uses 'dit diff' to compare to a specific SHA1 (the
      # merge-base by default), so doesn't include untracked files. So we use
      # 'git ls-files --directory --others --exclude-standard' here directly.
      paths = scm.GIT.Capture(
          ['-c', 'core.quotePath=false', 'ls-files',
           '--directory', '--others', '--exclude-standard'],
          self.checkout_path)
      for path in (p for p in paths.splitlines() if p.endswith('/')):
        full_path = os.path.join(self.checkout_path, path)
        if not os.path.islink(full_path):
          self.Print('_____ removing unversioned directory %s' % path)
          gclient_utils.rmtree(full_path)

    return self._Capture(['rev-parse', '--verify', 'HEAD'])

  def revert(self, options, _args, file_list):
    """Reverts local modifications.

    All reverted files will be appended to file_list.
    """
    if not os.path.isdir(self.checkout_path):
      # revert won't work if the directory doesn't exist. It needs to
      # checkout instead.
      self.Print('_____ %s is missing, synching instead' % self.relpath)
      # Don't reuse the args.
      return self.update(options, [], file_list)

    default_rev = "refs/heads/master"
    if options.upstream:
      if self._GetCurrentBranch():
        upstream_branch = scm.GIT.GetUpstreamBranch(self.checkout_path)
        default_rev = upstream_branch or default_rev
    _, deps_revision = gclient_utils.SplitUrlRevision(self.url)
    if not deps_revision:
      deps_revision = default_rev
    if deps_revision.startswith('refs/heads/'):
      deps_revision = deps_revision.replace('refs/heads/', self.remote + '/')
    try:
      deps_revision = self.GetUsableRev(deps_revision, options)
    except NoUsableRevError as e:
      # If the DEPS entry's url and hash changed, try to update the origin.
      # See also http://crbug.com/520067.
      logging.warn(
          'Couldn\'t find usable revision, will retrying to update instead: %s',
          e.message)
      return self.update(options, [], file_list)

    if file_list is not None:
      files = self._GetDiffFilenames(deps_revision)

    self._Scrub(deps_revision, options)
    self._Run(['clean', '-f', '-d'], options)

    if file_list is not None:
      file_list.extend([os.path.join(self.checkout_path, f) for f in files])

  def revinfo(self, _options, _args, _file_list):
    """Returns revision"""
    return self._Capture(['rev-parse', 'HEAD'])

  def runhooks(self, options, args, file_list):
    self.status(options, args, file_list)

  def status(self, options, _args, file_list):
    """Display status information."""
    if not os.path.isdir(self.checkout_path):
      self.Print('________ couldn\'t run status in %s:\n'
                 'The directory does not exist.' % self.checkout_path)
    else:
      try:
        merge_base = [self._Capture(['merge-base', 'HEAD', self.remote])]
      except subprocess2.CalledProcessError:
        merge_base = []
      self._Run(
          ['-c', 'core.quotePath=false', 'diff', '--name-status'] + merge_base,
          options, stdout=self.out_fh, always=options.verbose)
      if file_list is not None:
        files = self._GetDiffFilenames(merge_base[0] if merge_base else None)
        file_list.extend([os.path.join(self.checkout_path, f) for f in files])

  def GetUsableRev(self, rev, options):
    """Finds a useful revision for this repository."""
    sha1 = None
    if not os.path.isdir(self.checkout_path):
      raise NoUsableRevError(
          'This is not a git repo, so we cannot get a usable rev.')

    if scm.GIT.IsValidRevision(cwd=self.checkout_path, rev=rev):
      sha1 = rev
    else:
      # May exist in origin, but we don't have it yet, so fetch and look
      # again.
      self._Fetch(options)
      if scm.GIT.IsValidRevision(cwd=self.checkout_path, rev=rev):
        sha1 = rev

    if not sha1:
      raise NoUsableRevError(
          'Hash %s does not appear to be a valid hash in this repo.' % rev)

    return sha1

  def GetGitBackupDirPath(self):
    """Returns the path where the .git folder for the current project can be
    staged/restored. Use case: subproject moved from DEPS <-> outer project."""
    return os.path.join(self._root_dir,
                        'old_' + self.relpath.replace(os.sep, '_')) + '.git'

  def _GetMirror(self, url, options):
    """Get a git_cache.Mirror object for the argument url."""
    if not git_cache.Mirror.GetCachePath():
      return None
    mirror_kwargs = {
        'print_func': self.filter,
        'refs': []
    }
    if hasattr(options, 'with_branch_heads') and options.with_branch_heads:
      mirror_kwargs['refs'].append('refs/branch-heads/*')
    if hasattr(options, 'with_tags') and options.with_tags:
      mirror_kwargs['refs'].append('refs/tags/*')
    return git_cache.Mirror(url, **mirror_kwargs)

  def _UpdateMirrorIfNotContains(self, mirror, options, rev_type, revision):
    """Update a git mirror by fetching the latest commits from the remote,
    unless mirror already contains revision whose type is sha1 hash.
    """
    if rev_type == 'hash' and mirror.contains_revision(revision):
      if options.verbose:
        self.Print('skipping mirror update, it has rev=%s already' % revision,
                   timestamp=False)
      return

    if getattr(options, 'shallow', False):
      # HACK(hinoka): These repositories should be super shallow.
      if 'flash' in mirror.url:
        depth = 10
      else:
        depth = 10000
    else:
      depth = None
    mirror.populate(verbose=options.verbose,
                    bootstrap=not getattr(options, 'no_bootstrap', False),
                    depth=depth,
                    ignore_lock=getattr(options, 'ignore_locks', False),
                    lock_timeout=getattr(options, 'lock_timeout', 0))
    mirror.unlock()

  def _Clone(self, revision, url, options):
    """Clone a git repository from the given URL.

    Once we've cloned the repo, we checkout a working branch if the specified
    revision is a branch head. If it is a tag or a specific commit, then we
    leave HEAD detached as it makes future updates simpler -- in this case the
    user should first create a new branch or switch to an existing branch before
    making changes in the repo."""
    if not options.verbose:
      # git clone doesn't seem to insert a newline properly before printing
      # to stdout
      self.Print('')
    cfg = gclient_utils.DefaultIndexPackConfig(url)
    clone_cmd = cfg + ['clone', '--no-checkout', '--progress']
    if self.cache_dir:
      clone_cmd.append('--shared')
    if options.verbose:
      clone_cmd.append('--verbose')
    clone_cmd.append(url)
    # If the parent directory does not exist, Git clone on Windows will not
    # create it, so we need to do it manually.
    parent_dir = os.path.dirname(self.checkout_path)
    gclient_utils.safe_makedirs(parent_dir)

    template_dir = None
    if hasattr(options, 'no_history') and options.no_history:
      if gclient_utils.IsGitSha(revision):
        # In the case of a subproject, the pinned sha is not necessarily the
        # head of the remote branch (so we can't just use --depth=N). Instead,
        # we tell git to fetch all the remote objects from SHA..HEAD by means of
        # a template git dir which has a 'shallow' file pointing to the sha.
        template_dir = tempfile.mkdtemp(
            prefix='_gclient_gittmp_%s' % os.path.basename(self.checkout_path),
            dir=parent_dir)
        self._Run(['init', '--bare', template_dir], options, cwd=self._root_dir)
        with open(os.path.join(template_dir, 'shallow'), 'w') as template_file:
          template_file.write(revision)
        clone_cmd.append('--template=' + template_dir)
      else:
        # Otherwise, we're just interested in the HEAD. Just use --depth.
        clone_cmd.append('--depth=1')

    tmp_dir = tempfile.mkdtemp(
        prefix='_gclient_%s_' % os.path.basename(self.checkout_path),
        dir=parent_dir)
    try:
      clone_cmd.append(tmp_dir)
      if self.print_outbuf:
        print_stdout = True
        stdout = gclient_utils.WriteToStdout(self.out_fh)
      else:
        print_stdout = False
        stdout = self.out_fh
      self._Run(clone_cmd, options, cwd=self._root_dir, retry=True,
                print_stdout=print_stdout, stdout=stdout)
      gclient_utils.safe_makedirs(self.checkout_path)
      gclient_utils.safe_rename(os.path.join(tmp_dir, '.git'),
                                os.path.join(self.checkout_path, '.git'))
    except:
      traceback.print_exc(file=self.out_fh)
      raise
    finally:
      if os.listdir(tmp_dir):
        self.Print('_____ removing non-empty tmp dir %s' % tmp_dir)
      gclient_utils.rmtree(tmp_dir)
      if template_dir:
        gclient_utils.rmtree(template_dir)
    self._UpdateBranchHeads(options, fetch=True)
    revision = self._AutoFetchRef(options, revision)
    remote_ref = scm.GIT.RefToRemoteRef(revision, self.remote)
    self._Checkout(options, ''.join(remote_ref or revision), quiet=True)
    if self._GetCurrentBranch() is None:
      # Squelch git's very verbose detached HEAD warning and use our own
      self.Print(
        ('Checked out %s to a detached HEAD. Before making any commits\n'
         'in this repo, you should use \'git checkout <branch>\' to switch to\n'
         'an existing branch or use \'git checkout %s -b <branch>\' to\n'
         'create a new branch for your work.') % (revision, self.remote))

  def _AskForData(self, prompt, options):
    if options.jobs > 1:
      self.Print(prompt)
      raise gclient_utils.Error("Background task requires input. Rerun "
                                "gclient with --jobs=1 so that\n"
                                "interaction is possible.")
    try:
      return raw_input(prompt)
    except KeyboardInterrupt:
      # Hide the exception.
      sys.exit(1)


  def _AttemptRebase(self, upstream, files, options, newbase=None,
                     branch=None, printed_path=False, merge=False):
    """Attempt to rebase onto either upstream or, if specified, newbase."""
    if files is not None:
      files.extend(self._GetDiffFilenames(upstream))
    revision = upstream
    if newbase:
      revision = newbase
    action = 'merge' if merge else 'rebase'
    if not printed_path:
      self.Print('_____ %s : Attempting %s onto %s...' % (
          self.relpath, action, revision))
      printed_path = True
    else:
      self.Print('Attempting %s onto %s...' % (action, revision))

    if merge:
      merge_output = self._Capture(['merge', revision])
      if options.verbose:
        self.Print(merge_output)
      return

    # Build the rebase command here using the args
    # git rebase [options] [--onto <newbase>] <upstream> [<branch>]
    rebase_cmd = ['rebase']
    if options.verbose:
      rebase_cmd.append('--verbose')
    if newbase:
      rebase_cmd.extend(['--onto', newbase])
    rebase_cmd.append(upstream)
    if branch:
      rebase_cmd.append(branch)

    try:
      rebase_output = scm.GIT.Capture(rebase_cmd, cwd=self.checkout_path)
    except subprocess2.CalledProcessError, e:
      if (re.match(r'cannot rebase: you have unstaged changes', e.stderr) or
          re.match(r'cannot rebase: your index contains uncommitted changes',
                   e.stderr)):
        while True:
          rebase_action = self._AskForData(
              'Cannot rebase because of unstaged changes.\n'
              '\'git reset --hard HEAD\' ?\n'
              'WARNING: destroys any uncommitted work in your current branch!'
              ' (y)es / (q)uit / (s)how : ', options)
          if re.match(r'yes|y', rebase_action, re.I):
            self._Scrub('HEAD', options)
            # Should this be recursive?
            rebase_output = scm.GIT.Capture(rebase_cmd, cwd=self.checkout_path)
            break
          elif re.match(r'quit|q', rebase_action, re.I):
            raise gclient_utils.Error("Please merge or rebase manually\n"
                                      "cd %s && git " % self.checkout_path
                                      + "%s" % ' '.join(rebase_cmd))
          elif re.match(r'show|s', rebase_action, re.I):
            self.Print('%s' % e.stderr.strip())
            continue
          else:
            gclient_utils.Error("Input not recognized")
            continue
      elif re.search(r'^CONFLICT', e.stdout, re.M):
        raise gclient_utils.Error("Conflict while rebasing this branch.\n"
                                  "Fix the conflict and run gclient again.\n"
                                  "See 'man git-rebase' for details.\n")
      else:
        self.Print(e.stdout.strip())
        self.Print('Rebase produced error output:\n%s' % e.stderr.strip())
        raise gclient_utils.Error("Unrecognized error, please merge or rebase "
                                  "manually.\ncd %s && git " %
                                  self.checkout_path
                                  + "%s" % ' '.join(rebase_cmd))

    self.Print(rebase_output.strip())
    if not options.verbose:
      # Make the output a little prettier. It's nice to have some
      # whitespace between projects when syncing.
      self.Print('')

  @staticmethod
  def _CheckMinVersion(min_version):
    (ok, current_version) = scm.GIT.AssertVersion(min_version)
    if not ok:
      raise gclient_utils.Error('git version %s < minimum required %s' %
                                (current_version, min_version))

  def _EnsureValidHeadObjectOrCheckout(self, revision, options, url):
    # Special case handling if all 3 conditions are met:
    #   * the mirros have recently changed, but deps destination remains same,
    #   * the git histories of mirrors are conflicting.
    #   * git cache is used
    # This manifests itself in current checkout having invalid HEAD commit on
    # most git operations. Since git cache is used, just deleted the .git
    # folder, and re-create it by cloning.
    try:
      self._Capture(['rev-list', '-n', '1', 'HEAD'])
    except subprocess2.CalledProcessError as e:
      if ('fatal: bad object HEAD' in e.stderr
          and self.cache_dir and self.cache_dir in url):
        self.Print((
          'Likely due to DEPS change with git cache_dir, '
          'the current commit points to no longer existing object.\n'
          '%s' % e)
        )
        self._DeleteOrMove(options.force)
        self._Clone(revision, url, options)
      else:
        raise

  def _IsRebasing(self):
    # Check for any of REBASE-i/REBASE-m/REBASE/AM. Unfortunately git doesn't
    # have a plumbing command to determine whether a rebase is in progress, so
    # for now emualate (more-or-less) git-rebase.sh / git-completion.bash
    g = os.path.join(self.checkout_path, '.git')
    return (
      os.path.isdir(os.path.join(g, "rebase-merge")) or
      os.path.isdir(os.path.join(g, "rebase-apply")))

  def _CheckClean(self, revision, fixup=False):
    lockfile = os.path.join(self.checkout_path, ".git", "index.lock")
    if os.path.exists(lockfile):
      raise gclient_utils.Error(
        '\n____ %s at %s\n'
        '\tYour repo is locked, possibly due to a concurrent git process.\n'
        '\tIf no git executable is running, then clean up %r and try again.\n'
        % (self.relpath, revision, lockfile))

    # Make sure the tree is clean; see git-rebase.sh for reference
    try:
      scm.GIT.Capture(['update-index', '--ignore-submodules', '--refresh'],
                      cwd=self.checkout_path)
    except subprocess2.CalledProcessError:
      raise gclient_utils.Error('\n____ %s at %s\n'
                                '\tYou have unstaged changes.\n'
                                '\tPlease commit, stash, or reset.\n'
                                  % (self.relpath, revision))
    try:
      scm.GIT.Capture(['diff-index', '--cached', '--name-status', '-r',
                       '--ignore-submodules', 'HEAD', '--'],
                       cwd=self.checkout_path)
    except subprocess2.CalledProcessError:
      raise gclient_utils.Error('\n____ %s at %s\n'
                                '\tYour index contains uncommitted changes\n'
                                '\tPlease commit, stash, or reset.\n'
                                  % (self.relpath, revision))

  def _CheckDetachedHead(self, revision, _options):
    # HEAD is detached. Make sure it is safe to move away from (i.e., it is
    # reference by a commit). If not, error out -- most likely a rebase is
    # in progress, try to detect so we can give a better error.
    try:
      scm.GIT.Capture(['name-rev', '--no-undefined', 'HEAD'],
          cwd=self.checkout_path)
    except subprocess2.CalledProcessError:
      # Commit is not contained by any rev. See if the user is rebasing:
      if self._IsRebasing():
        # Punt to the user
        raise gclient_utils.Error('\n____ %s at %s\n'
                                  '\tAlready in a conflict, i.e. (no branch).\n'
                                  '\tFix the conflict and run gclient again.\n'
                                  '\tOr to abort run:\n\t\tgit-rebase --abort\n'
                                  '\tSee man git-rebase for details.\n'
                                   % (self.relpath, revision))
      # Let's just save off the commit so we can proceed.
      name = ('saved-by-gclient-' +
              self._Capture(['rev-parse', '--short', 'HEAD']))
      self._Capture(['branch', '-f', name])
      self.Print('_____ found an unreferenced commit and saved it as \'%s\'' %
          name)

  def _GetCurrentBranch(self):
    # Returns name of current branch or None for detached HEAD
    branch = self._Capture(['rev-parse', '--abbrev-ref=strict', 'HEAD'])
    if branch == 'HEAD':
      return None
    return branch

  def _Capture(self, args, **kwargs):
    kwargs.setdefault('cwd', self.checkout_path)
    kwargs.setdefault('stderr', subprocess2.PIPE)
    strip = kwargs.pop('strip', True)
    env = scm.GIT.ApplyEnvVars(kwargs)
    ret = subprocess2.check_output(['git'] + args, env=env, **kwargs)
    if strip:
      ret = ret.strip()
    return ret

  def _Checkout(self, options, ref, force=False, quiet=None):
    """Performs a 'git-checkout' operation.

    Args:
      options: The configured option set
      ref: (str) The branch/commit to checkout
      quiet: (bool/None) Whether or not the checkout shoud pass '--quiet'; if
          'None', the behavior is inferred from 'options.verbose'.
    Returns: (str) The output of the checkout operation
    """
    if quiet is None:
      quiet = (not options.verbose)
    checkout_args = ['checkout']
    if force:
      checkout_args.append('--force')
    if quiet:
      checkout_args.append('--quiet')
    checkout_args.append(ref)
    return self._Capture(checkout_args)

  def _Fetch(self, options, remote=None, prune=False, quiet=False,
             refspec=None):
    cfg = gclient_utils.DefaultIndexPackConfig(self.url)
    fetch_cmd =  cfg + [
        'fetch',
        remote or self.remote,
    ]
    if refspec:
      fetch_cmd.append(refspec)

    if prune:
      fetch_cmd.append('--prune')
    if options.verbose:
      fetch_cmd.append('--verbose')
    elif quiet:
      fetch_cmd.append('--quiet')
    self._Run(fetch_cmd, options, show_header=options.verbose, retry=True)

    # Return the revision that was fetched; this will be stored in 'FETCH_HEAD'
    return self._Capture(['rev-parse', '--verify', 'FETCH_HEAD'])

  def _UpdateBranchHeads(self, options, fetch=False):
    """Adds, and optionally fetches, "branch-heads" and "tags" refspecs
    if requested."""
    need_fetch = fetch
    if hasattr(options, 'with_branch_heads') and options.with_branch_heads:
      config_cmd = ['config', 'remote.%s.fetch' % self.remote,
                    '+refs/branch-heads/*:refs/remotes/branch-heads/*',
                    '^\\+refs/branch-heads/\\*:.*$']
      self._Run(config_cmd, options)
      need_fetch = True
    if hasattr(options, 'with_tags') and options.with_tags:
      config_cmd = ['config', 'remote.%s.fetch' % self.remote,
                    '+refs/tags/*:refs/tags/*',
                    '^\\+refs/tags/\\*:.*$']
      self._Run(config_cmd, options)
      need_fetch = True
    if fetch and need_fetch:
      self._Fetch(options, prune=options.force)

  def _AutoFetchRef(self, options, revision):
    """Attempts to fetch |revision| if not available in local repo.

    Returns possibly updated revision."""
    try:
      self._Capture(['rev-parse', revision])
    except subprocess2.CalledProcessError:
      self._Fetch(options, refspec=revision)
      revision = self._Capture(['rev-parse', 'FETCH_HEAD'])
    return revision

  def _Run(self, args, options, show_header=True, **kwargs):
    # Disable 'unused options' warning | pylint: disable=unused-argument
    kwargs.setdefault('cwd', self.checkout_path)
    kwargs.setdefault('stdout', self.out_fh)
    kwargs['filter_fn'] = self.filter
    kwargs.setdefault('print_stdout', False)
    env = scm.GIT.ApplyEnvVars(kwargs)
    cmd = ['git'] + args
    if show_header:
      gclient_utils.CheckCallAndFilterAndHeader(cmd, env=env, **kwargs)
    else:
      gclient_utils.CheckCallAndFilter(cmd, env=env, **kwargs)


class CipdPackage(object):
  """A representation of a single CIPD package."""

  def __init__(self, name, version, authority_for_subdir):
    self._authority_for_subdir = authority_for_subdir
    self._name = name
    self._version = version

  @property
  def authority_for_subdir(self):
    """Whether this package has authority to act on behalf of its subdir.

    Some operations should only be performed once per subdirectory. A package
    that has authority for its subdirectory is the only package that should
    perform such operations.

    Returns:
      bool; whether this package has subdir authority.
    """
    return self._authority_for_subdir

  @property
  def name(self):
    return self._name

  @property
  def version(self):
    return self._version


class CipdRoot(object):
  """A representation of a single CIPD root."""
  def __init__(self, root_dir, service_url):
    self._all_packages = set()
    self._mutator_lock = threading.Lock()
    self._packages_by_subdir = collections.defaultdict(list)
    self._root_dir = root_dir
    self._service_url = service_url

  def add_package(self, subdir, package, version):
    """Adds a package to this CIPD root.

    As far as clients are concerned, this grants both root and subdir authority
    to packages arbitrarily. (The implementation grants root authority to the
    first package added and subdir authority to the first package added for that
    subdir, but clients should not depend on or expect that behavior.)

    Args:
      subdir: str; relative path to where the package should be installed from
        the cipd root directory.
      package: str; the cipd package name.
      version: str; the cipd package version.
    Returns:
      CipdPackage; the package that was created and added to this root.
    """
    with self._mutator_lock:
      cipd_package = CipdPackage(
          package, version,
          not self._packages_by_subdir[subdir])
      self._all_packages.add(cipd_package)
      self._packages_by_subdir[subdir].append(cipd_package)
      return cipd_package

  def packages(self, subdir):
    """Get the list of configured packages for the given subdir."""
    return list(self._packages_by_subdir[subdir])

  def clobber(self):
    """Remove the .cipd directory.

    This is useful for forcing ensure to redownload and reinitialize all
    packages.
    """
    with self._mutator_lock:
      cipd_cache_dir = os.path.join(self.root_dir, '.cipd')
      try:
        gclient_utils.rmtree(os.path.join(cipd_cache_dir))
      except OSError:
        if os.path.exists(cipd_cache_dir):
          raise

  @contextlib.contextmanager
  def _create_ensure_file(self):
    try:
      ensure_file = None
      with tempfile.NamedTemporaryFile(
          suffix='.ensure', delete=False) as ensure_file:
        for subdir, packages in sorted(self._packages_by_subdir.iteritems()):
          ensure_file.write('@Subdir %s\n' % subdir)
          for package in packages:
            ensure_file.write('%s %s\n' % (package.name, package.version))
          ensure_file.write('\n')
      yield ensure_file.name
    finally:
      if ensure_file is not None and os.path.exists(ensure_file.name):
        os.remove(ensure_file.name)

  def ensure(self):
    """Run `cipd ensure`."""
    with self._mutator_lock:
      with self._create_ensure_file() as ensure_file:
        cmd = [
            'cipd', 'ensure',
            '-log-level', 'error',
            '-root', self.root_dir,
            '-ensure-file', ensure_file,
        ]
        gclient_utils.CheckCallAndFilterAndHeader(cmd)

  def run(self, command):
    if command == 'update':
      self.ensure()
    elif command == 'revert':
      self.clobber()
      self.ensure()

  def created_package(self, package):
    """Checks whether this root created the given package.

    Args:
      package: CipdPackage; the package to check.
    Returns:
      bool; whether this root created the given package.
    """
    return package in self._all_packages

  @property
  def root_dir(self):
    return self._root_dir

  @property
  def service_url(self):
    return self._service_url


class CipdWrapper(SCMWrapper):
  """Wrapper for CIPD.

  Currently only supports chrome-infra-packages.appspot.com.
  """
  name = 'cipd'

  def __init__(self, url=None, root_dir=None, relpath=None, out_fh=None,
               out_cb=None, root=None, package=None):
    super(CipdWrapper, self).__init__(
        url=url, root_dir=root_dir, relpath=relpath, out_fh=out_fh,
        out_cb=out_cb)
    assert root.created_package(package)
    self._package = package
    self._root = root

  #override
  def GetCacheMirror(self):
    return None

  #override
  def GetActualRemoteURL(self, options):
    return self._root.service_url

  #override
  def DoesRemoteURLMatch(self, options):
    del options
    return True

  def revert(self, options, args, file_list):
    """Does nothing.

    CIPD packages should be reverted at the root by running
    `CipdRoot.run('revert')`.
    """
    pass

  def diff(self, options, args, file_list):
    """CIPD has no notion of diffing."""
    pass

  def pack(self, options, args, file_list):
    """CIPD has no notion of diffing."""
    pass

  def revinfo(self, options, args, file_list):
    """Grab the instance ID."""
    try:
      tmpdir = tempfile.mkdtemp()
      describe_json_path = os.path.join(tmpdir, 'describe.json')
      cmd = [
          'cipd', 'describe',
          self._package.name,
          '-log-level', 'error',
          '-version', self._package.version,
          '-json-output', describe_json_path
      ]
      gclient_utils.CheckCallAndFilter(
          cmd, filter_fn=lambda _line: None, print_stdout=False)
      with open(describe_json_path) as f:
        describe_json = json.load(f)
      return describe_json.get('result', {}).get('pin', {}).get('instance_id')
    finally:
      gclient_utils.rmtree(tmpdir)

  def status(self, options, args, file_list):
    pass

  def update(self, options, args, file_list):
    """Does nothing.

    CIPD packages should be updated at the root by running
    `CipdRoot.run('update')`.
    """
    pass
