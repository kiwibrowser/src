# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Repository module to handle different types of repositories."""

from __future__ import print_function

import glob
import multiprocessing
import os
import re
import shutil
import time
import urlparse

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import locking
from chromite.lib import metrics
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import path_util
from chromite.lib import retry_util
from chromite.lib import rewrite_git_alternates


site_config = config_lib.GetConfig()


# File that marks a buildroot as being used by a trybot
_TRYBOT_MARKER = '.trybot'

# Default sleep time(second) between retries
DEFAULT_SLEEP_TIME = 5

# Retry limit for 'repo init'
REPO_INIT_RETRY_LIMIT = 2

SELFUPDATE_WARNING = r'Skipped upgrade to unverified version'

SELFUPDATE_WARNING_RE = re.compile(SELFUPDATE_WARNING, re.IGNORECASE)

class SrcCheckOutException(Exception):
  """Exception gets thrown for failure to sync sources"""


def IsARepoRoot(directory):
  """Returns True if directory is the root of a repo checkout."""
  return os.path.exists(os.path.join(directory, '.repo'))


def IsInternalRepoCheckout(root):
  """Returns whether root houses an internal 'repo' checkout."""
  manifest_dir = os.path.join(root, '.repo', 'manifests')
  manifest_url = git.RunGit(
      manifest_dir, ['config', 'remote.origin.url']).output.strip()
  return (os.path.splitext(os.path.basename(manifest_url))[0] ==
          os.path.splitext(os.path.basename(
              site_config.params.MANIFEST_INT_URL))[0])


def _IsLocalPath(url):
  """Returns whether the url is a local path.

  Args:
    url: The url string to parse.

  Returns:
    True if the url actually refers to a local path (with prefix
      'file://' or '/'); else, False.
  """
  o = urlparse.urlparse(url)
  return o.scheme in ('file', '')


def CloneGitRepo(working_dir, repo_url, reference=None, bare=False,
                 mirror=False, depth=None, branch=None, single_branch=False):
  """Clone given git repo

  Args:
    working_dir: location where it should be cloned to
    repo_url: git repo to clone
    reference: If given, pathway to a git repository to access git objects
      from.  Note that the reference must exist as long as the newly created
      repo is to be usable.
    bare: Clone a bare checkout.
    mirror: Clone a mirror checkout.
    depth: If given, do a shallow clone limiting the objects pulled to just
      that # of revs of history.  This option is mutually exclusive to
      reference.
    branch: If given, clone the given branch from the parent repository.
    single_branch: Clone only one the requested branch.
  """
  osutils.SafeMakedirs(working_dir)
  cmd = ['clone', repo_url, working_dir]
  if reference:
    if depth:
      raise ValueError("reference and depth are mutually exclusive "
                       "options; please pick one or the other.")
    cmd += ['--reference', reference]
  if bare:
    cmd += ['--bare']
  if mirror:
    cmd += ['--mirror']
  if depth:
    cmd += ['--depth', str(int(depth))]
  if branch:
    cmd += ['--branch', branch]
  if single_branch:
    cmd += ['--single-branch']
  git.RunGit(working_dir, cmd, print_cmd=True)


def CloneWorkingRepo(dest, url, reference, branch=None, single_branch=False):
  """Clone a git repository with an existing local copy as a reference.

  Also copy the hooks into the new repository.

  Args:
    dest: The directory to clone int.
    url: The URL of the repository to clone.
    reference: Local checkout to draw objects from.
    branch: The branch to clone.
    single_branch: Clone only one the requested branch.
  """
  CloneGitRepo(dest, url, reference=reference,
               single_branch=single_branch, branch=branch)
  for name in glob.glob(os.path.join(reference, '.git', 'hooks', '*')):
    newname = os.path.join(dest, '.git', 'hooks', os.path.basename(name))
    shutil.copyfile(name, newname)
    shutil.copystat(name, newname)

def UpdateGitRepo(working_dir, repo_url, **kwargs):
  """Update the given git repo, blowing away any local changes.

  If the repo does not exist, clone it from scratch.

  Args:
    working_dir: location where it should be cloned to
    repo_url: git repo to clone
    **kwargs: See CloneGitRepo.
  """
  assert not kwargs.get('bare'), 'Bare checkouts are not supported'
  if git.IsGitRepo(working_dir):
    try:
      git.CleanAndCheckoutUpstream(working_dir)
    except cros_build_lib.RunCommandError:
      logging.warning('Could not update %s', working_dir, exc_info=True)
      shutil.rmtree(working_dir)
      CloneGitRepo(working_dir, repo_url, **kwargs)
  else:
    CloneGitRepo(working_dir, repo_url, **kwargs)


def GetTrybotMarkerPath(buildroot):
  """Get path to trybot marker file given the buildroot."""
  return os.path.join(buildroot, _TRYBOT_MARKER)


def CreateTrybotMarker(buildroot):
  """Create the file that identifies a buildroot as being used by a trybot."""
  osutils.WriteFile(GetTrybotMarkerPath(buildroot), '')


def ClearBuildRoot(buildroot, preserve_paths=()):
  """Remove and recreate the buildroot while preserving the trybot marker."""
  trybot_root = os.path.exists(GetTrybotMarkerPath(buildroot))
  if os.path.exists(buildroot):
    cmd = ['find', buildroot, '-mindepth', '1', '-maxdepth', '1']

    ignores = []
    for path in preserve_paths:
      if ignores:
        ignores.append('-a')
      ignores += ['!', '-name', path]
    cmd.extend(ignores)

    cmd += ['-exec', 'rm', '-rf', '{}', '+']
    cros_build_lib.SudoRunCommand(cmd)
  else:
    os.makedirs(buildroot)
  if trybot_root:
    CreateTrybotMarker(buildroot)


def PrepManifestForRepo(git_repo, manifest):
  """Use this to store a local manifest in a git repo suitable for repo.

  The repo tool can only fetch manifests from git repositories. So, to use
  a local manifest file as the basis for a checkout, it must be checked into
  a local git repository.

  Common Usage:
    manifest = CreateOrFetchWondrousManifest()
    with osutils.TempDir() as manifest_git_dir:
      PrepManifestForRepo(manifest_git_dir, manifest)
      repo = RepoRepository(manifest_git_dir, repo_dir)
      repo.Sync()

  Args:
    git_repo: Path at which to create the git repository (directory created, if
              needed). If a tempdir, then cleanup is owned by the caller.
    manifest: Path to existing manifest file to copy into the new git
              repository.
  """
  if not git.IsGitRepo(git_repo):
    git.Init(git_repo)

  new_manifest = os.path.join(git_repo, constants.DEFAULT_MANIFEST)

  shutil.copyfile(manifest, new_manifest)
  git.AddPath(new_manifest)
  message = 'Local repository holding: %s' % manifest

  # Commit new manifest. allow_empty in case it's the same as last manifest.
  git.Commit(git_repo, message, allow_empty=True)


class RepoRepository(object):
  """A Class that encapsulates a repo repository."""
  # If a repo hasn't been used in the last 5 runs, wipe it.
  LRU_THRESHOLD = 5

  def __init__(self, manifest_repo_url, directory, branch=None,
               referenced_repo=None, manifest=constants.DEFAULT_MANIFEST,
               depth=None, repo_url=site_config.params.REPO_URL,
               repo_branch=None, groups=None, repo_cmd='repo',
               preserve_paths=(), git_cache_dir=None):
    """Initialize.

    Args:
      manifest_repo_url: URL to fetch repo manifest from.
      directory: local path where to checkout the repository.
      branch: Branch to check out the manifest at.
      referenced_repo: Repository to reference for git objects, if possible.
      manifest: Which manifest.xml within the branch to use.  Effectively
        default.xml if not given.
      depth: Mutually exclusive option to referenced_repo; this limits the
        checkout to a max commit history of the given integer.
      repo_url: URL to fetch repo tool from.
      repo_branch: Branch to check out the repo tool at.
      groups: Only sync projects that match this filter.
      repo_cmd: Name of repo_cmd to use.
      preserve_paths: paths need to be preserved in repo clean
        in case we want to clean and retry repo sync.
      git_cache_dir: If specified, use --cache-dir=git_cache_dir in repo sync.
    """
    self.manifest_repo_url = manifest_repo_url
    self.repo_url = repo_url
    self.repo_branch = repo_branch
    self.directory = directory
    self.branch = branch
    self.groups = groups
    self.repo_cmd = repo_cmd
    self.preserve_paths = preserve_paths

    # It's perfectly acceptable to pass in a reference pathway that isn't
    # usable.  Detect it, and suppress the setting so that any depth
    # settings aren't disabled due to it.
    if referenced_repo is not None:
      if depth is not None:
        raise ValueError("referenced_repo and depth are mutually exclusive "
                         "options; please pick one or the other.")
      if git_cache_dir is not None:
        raise ValueError("referenced_repo and git_cache_dir are mutually "
                         "exclusive options; please pick one or the other.")
      if not IsARepoRoot(referenced_repo):
        referenced_repo = None

    self.git_cache_dir = git_cache_dir
    self._referenced_repo = referenced_repo
    self._manifest = manifest

    # Use by Intialize to avoid updating more than once.
    self._repo_update_needed = True
    self._depth = int(depth) if depth is not None else None

  def _SwitchToLocalManifest(self, local_manifest):
    """Reinitializes the repository if the manifest has changed."""
    logging.debug('Moving to manifest defined by %s', local_manifest)
    # TODO: use upstream repo's manifest logic when we bump repo version.
    manifest_path = self.GetRelativePath('.repo/manifest.xml')
    os.unlink(manifest_path)
    shutil.copyfile(local_manifest, manifest_path)

  def _RepoSelfupdate(self):
    """Execute repo selfupdate command.

    'repo selfupdate' would clean up the .repo/repo dir on certain exceptions
    and warnings, it must be followed by the 'repo init' command, which would
    recover .repo/repo in this circumstance.
    """
    cmd = [self.repo_cmd, 'selfupdate']
    failed_to_selfupdate = False
    try:
      cmd_result = cros_build_lib.RunCommand(
          cmd, cwd=self.directory, capture_output=True, log_output=True)

      if (cmd_result.error is not None and
          SELFUPDATE_WARNING_RE.search(cmd_result.error)):
        logging.warning('Unable to selfupdate because of warning "%s"',
                        SELFUPDATE_WARNING)
        failed_to_selfupdate = True
    except cros_build_lib.RunCommandError as e:
      logging.warning('repo selfupdate failed with exception: %s', e)
      failed_to_selfupdate = True

    if failed_to_selfupdate:
      metrics.Counter(constants.MON_REPO_SELFUPDATE_FAILURE_COUNT).increment()
      logging.warning('Failed to selfupdate repo, cleaning .repo/repo in %s',
                      self.directory)
      osutils.RmDir(os.path.join(self.directory, '.repo', 'repo'),
                    ignore_missing=True)

  def _CleanUpRepoManifest(self, directory):
    """Clean up the manifest and repo dirs under the '.repo' dir.

    Args:
      directory: The directory where stores repo and manifest dirs.
    """
    paths = [os.path.join(directory, '.repo', x) for x in
             ('manifest.xml', 'manifests.git', 'manifests', 'repo')]
    cros_build_lib.SudoRunCommand(['rm', '-rf'] + paths)

  def _RepoInit(self, *args, **kwargs):
    """Run 'repo init' and clean up repo manifest on init failures.

    Args:
      args: args to pass to cros_build_lib.RunCommand.
      kwargs: kwargs to pass to cros_build_lib.RunCommand.
    """
    try:
      kwargs.setdefault('cwd', self.directory)
      kwargs.setdefault('input', '\n\ny\n')
      cros_build_lib.RunCommand(*args, **kwargs)
    except cros_build_lib.RunCommandError as e:
      logging.warning("Wiping %r due to `repo init` failures.", self.directory)
      self._CleanUpRepoManifest(self.directory)
      raise e

  def BuildRootGitCleanup(self, prune_all=False):
    """Put buildroot onto manifest branch. Delete branches created on last run.

    Args:
      prune_all: If True, prune all loose objects regardless of gc.pruneExpire.

    Raises:
      A variety of exceptions if the buildroot is missing/corrupt.
    """
    lock_path = os.path.join(self.directory, '.clean_lock')
    deleted_objdirs = multiprocessing.Event()

    def RunCleanupCommands(project, cwd):
      with locking.FileLock(lock_path, verbose=False).read_lock() as lock:
        # Calculate where the git repository is stored.
        relpath = os.path.relpath(cwd, self.directory)
        projects_dir = os.path.join(self.directory, '.repo', 'projects')
        project_objects_dir = os.path.join(
            self.directory, '.repo', 'project-objects')
        repo_git_store = '%s.git' % os.path.join(projects_dir, relpath)
        repo_obj_store = '%s.git' % os.path.join(project_objects_dir, project)

        try:
          if os.path.isdir(cwd):
            git.CleanAndDetachHead(cwd)

          if os.path.isdir(repo_git_store):
            git.GarbageCollection(repo_git_store, prune_all=prune_all)
        except cros_build_lib.RunCommandError as e:
          result = e.result
          logging.PrintBuildbotStepWarnings()
          logging.warning('\n%s', result.error)

          # If there's no repository corruption, just delete the index.
          corrupted = git.IsGitRepositoryCorrupted(repo_git_store)
          lock.write_lock()
          logging.warning('Deleting %s because %s failed', cwd, result.cmd)
          osutils.RmDir(cwd, ignore_missing=True, sudo=True)
          if corrupted:
            # Looks like the object dir is corrupted. Delete it.
            deleted_objdirs.set()
            for store in (repo_git_store, repo_obj_store):
              logging.warning('Deleting %s as well', store)
              osutils.RmDir(store, ignore_missing=True)

        # TODO: Make the deletions below smarter. Look to see what exists,
        # instead of just deleting things we think might be there.

        # Delete all branches created by cbuildbot.
        if os.path.isdir(repo_git_store):
          cmd = ['branch', '-D'] + list(constants.CREATED_BRANCHES)
          # Ignore errors, since we delete branches without checking existence.
          git.RunGit(repo_git_store, cmd, error_code_ok=True)

        if os.path.isdir(cwd):
          # Above we deleted refs/heads/<branch> for each created branch, now we
          # need to delete the bare ref <branch> if it was created somehow.
          for ref in constants.CREATED_BRANCHES:
            # Ignore errors, since we delete branches without checking
            # existence.
            git.RunGit(cwd, ['update-ref', '-d', ref], error_code_ok=True)


    # Cleanup all of the directories.
    dirs = [[attrs['name'], os.path.join(self.directory, attrs['path'])]
            for attrs in
            git.ManifestCheckout.Cached(self.directory).ListCheckouts()]
    parallel.RunTasksInProcessPool(RunCleanupCommands, dirs)

    # repo shares git object directories amongst multiple project paths. If the
    # first pass deleted an object dir for a project path, then other
    # repositories (project paths) of that same project may now be broken. Do a
    # second pass to clean them up as well.
    if deleted_objdirs.is_set():
      parallel.RunTasksInProcessPool(RunCleanupCommands, dirs)

  def AssertNotNested(self):
    """Assert that the current repository isn't inside another repository.

    Since repo detects it's root by looking for .repo, it can't support having
    one repo inside another.
    """
    if not IsARepoRoot(self.directory):
      repo_root = git.FindRepoDir(self.directory)
      if repo_root:
        raise ValueError('%s is nested inside a repo at %s.' %
                         (self.directory, repo_root))

  def Initialize(self, local_manifest=None, manifest_repo_url=None,
                 extra_args=()):
    """Initializes a repository.  Optionally forces a local manifest.

    Args:
      local_manifest: The absolute path to a custom manifest to use.  This will
                      replace .repo/manifest.xml.
      manifest_repo_url: A new value for manifest_repo_url.
      extra_args: Extra args to pass to 'repo init'
    """
    self.AssertNotNested()

    if manifest_repo_url:
      self.manifest_repo_url = manifest_repo_url

    # Do a sanity check on the repo; if it exists and we can't pull a
    # manifest from it, we know it's fairly screwed up and needs a fresh
    # rebuild.
    if os.path.exists(os.path.join(self.directory, '.repo', 'manifest.xml')):
      cmd = [self.repo_cmd, 'manifest']
      try:
        cros_build_lib.RunCommand(cmd, cwd=self.directory, capture_output=True)
      except cros_build_lib.RunCommandError:
        metrics.Counter(constants.MON_REPO_MANIFEST_FAILURE_COUNT).increment()
        logging.warning("Wiping %r due to `repo manifest` failure",
                        self.directory)
        self._CleanUpRepoManifest(self.directory)
        self._repo_update_needed = False

    # Wipe local_manifest.xml if it exists- it can interfere w/ things in
    # bad ways (duplicate projects, etc); we control this repository, thus
    # we can destroy it.
    osutils.SafeUnlink(os.path.join(self.directory, 'local_manifest.xml'))

    # Force a repo update the first time we initialize an old repo checkout.
    # Don't update if there is nothing to update.
    if self._repo_update_needed:
      if IsARepoRoot(self.directory):
        self._RepoSelfupdate()
      self._repo_update_needed = False

    # Use our own repo, in case android.kernel.org (the default location) is
    # down.
    init_cmd = [self.repo_cmd, 'init',
                '--repo-url', self.repo_url,
                '--manifest-url', self.manifest_repo_url]
    if self._referenced_repo:
      init_cmd.extend(['--reference', self._referenced_repo])
    if self._manifest:
      init_cmd.extend(['--manifest-name', self._manifest])
    if self._depth is not None:
      init_cmd.extend(['--depth', str(self._depth)])
    init_cmd.extend(extra_args)
    # Handle branch / manifest options.
    if self.branch:
      init_cmd.extend(['--manifest-branch', self.branch])
    if self.repo_branch:
      init_cmd.extend(['--repo-branch', self.repo_branch])
    if self.groups:
      init_cmd.extend(['--groups', self.groups])

    def _StatusCallback(attempt, _):
      if attempt:
        metrics.Counter(constants.MON_REPO_INIT_RETRY_COUNT).increment(
            fields={'manifest_url': self.manifest_repo_url})

    retry_util.RetryCommand(self._RepoInit,
                            REPO_INIT_RETRY_LIMIT,
                            init_cmd,
                            sleep=DEFAULT_SLEEP_TIME,
                            backoff_factor=2,
                            log_retries=True,
                            status_callback=_StatusCallback)

    if local_manifest and local_manifest != self._manifest:
      self._SwitchToLocalManifest(local_manifest)

  @property
  def _ManifestConfig(self):
    return os.path.join(self.directory, '.repo', 'manifests.git', 'config')

  def _EnsureMirroring(self, post_sync=False):
    """Ensure git is usable from w/in the chroot if --references is enabled

    repo init --references hardcodes the abspath to parent; this pathway
    however isn't usable from the chroot (it doesn't exist).  As such the
    pathway is rewritten to use relative pathways pointing at the root of
    the repo, which via I84988630 enter_chroot sets up a helper bind mount
    allowing git/repo to access the actual referenced repo.

    This has to be invoked prior to a repo sync of the target trybot to
    fix any pathways that may have been broken by the parent repo moving
    on disk, and needs to be invoked after the sync has completed to rewrite
    any new project's abspath to relative.
    """
    if not self._referenced_repo:
      return

    proj_root = os.path.join(self.directory, '.repo', 'project-objects')
    if not os.path.exists(proj_root):
      # Not yet synced, nothing to be done.
      return

    rewrite_git_alternates.RebuildRepoCheckout(self.directory,
                                               self._referenced_repo)

    if post_sync:
      chroot_path = os.path.join(self._referenced_repo, '.repo', 'chroot',
                                 'external')
      chroot_path = path_util.ToChrootPath(chroot_path)
      rewrite_git_alternates.RebuildRepoCheckout(
          self.directory, self._referenced_repo, chroot_path)

    # Finally, force the git config marker that enter_chroot looks for
    # to know when to do bind mounting trickery; this normally will exist,
    # but if we're converting a pre-existing repo checkout, it's possible
    # that it was invoked w/out the reference arg.  Note this must be
    # an absolute path to the source repo- enter_chroot uses that to know
    # what to bind mount into the chroot.
    cmd = ['config', '--file', self._ManifestConfig, 'repo.reference',
           self._referenced_repo]
    git.RunGit('.', cmd)

  def _ForceSyncSupported(self):
    """Detect whether --force-sync is supported

    When repo changes its internal object layout, it'll refuse to sync unless
    this option is specified.
    """
    result = cros_build_lib.RunCommand([self.repo_cmd, 'sync', '--help'],
                                       capture_output=True, cwd=self.directory)
    return '--force-sync' in result.output

  def _CleanUpAndRunCommand(self, *args, **kwargs):
    """Clean up repository and run command.

    This is only called in repo network Sync retries.
    """
    self.BuildRootGitCleanup()
    local_manifest = kwargs.pop('local_manifest', None)
    # Always re-initialize to the current branch.
    self.Initialize(local_manifest)
    # Fix existing broken mirroring configurations.
    self._EnsureMirroring()

    fields = {'manifest_repo': self.manifest_repo_url}
    metrics.Counter(constants.MON_REPO_SYNC_RETRY_COUNT).increment(
        fields=fields)

    cros_build_lib.RunCommand(*args, **kwargs)

  def Sync(self, local_manifest=None, jobs=None, all_branches=True,
           network_only=False, detach=False):
    """Sync/update the source.  Changes manifest if specified.

    Args:
      local_manifest: If true, checks out source to manifest.  DEFAULT_MANIFEST
        may be used to set it back to the default manifest.
      jobs: May be set to override the default sync parallelism defined by
        the manifest.
      all_branches: If False, a repo sync -c is performed; this saves on
        sync'ing via grabbing only what is needed for the manifest specified
        branch. Defaults to True. TODO(davidjames): Set the default back to
        False once we've fixed http://crbug.com/368722 .
      network_only: If true, perform only the network half of the sync; skip
        the checkout.  Primarily of use to validate a manifest (although
        if the manifest has bad copyfile statements, via skipping checkout
        the broken copyfile tag won't be spotted), or of use when the
        invoking code is fine w/ operating on bare repos, ie .repo/projects/*.
      detach: If true, throw away all local changes, even if on tracking
        branches.
    """
    try:
      # Always re-initialize to the current branch.
      self.Initialize(local_manifest)
      # Fix existing broken mirroring configurations.
      self._EnsureMirroring()

      cmd = [self.repo_cmd, '--time', 'sync']
      if self._ForceSyncSupported():
        cmd += ['--force-sync']
      if jobs:
        cmd += ['--jobs', str(jobs)]
      if not all_branches or self._depth is not None:
        # Note that this option can break kernel checkouts. crbug.com/464536
        cmd.append('-c')
      if self.git_cache_dir is not None:
        cmd.append('--cache-dir=%s' % self.git_cache_dir)
      # Do the network half of the sync; retry as necessary to get the content.
      try:
        if not _IsLocalPath(self.manifest_repo_url):
          fields = {'manifest_repo': self.manifest_repo_url}
          metrics.Counter(constants.MON_REPO_SYNC_COUNT).increment(
              fields=fields)

        cros_build_lib.RunCommand(cmd + ['-n'], cwd=self.directory)
      except cros_build_lib.RunCommandError:
        if constants.SYNC_RETRIES > 0:
          # Retry on clean up and repo sync commands,
          # decrement max_retry for this command
          logging.warning('cmd %s failed, clean up repository and retry sync.'
                          % cmd)
          time.sleep(DEFAULT_SLEEP_TIME)
          retry_util.RetryCommand(self._CleanUpAndRunCommand,
                                  constants.SYNC_RETRIES - 1,
                                  cmd + ['-n'],
                                  cwd=self.directory,
                                  local_manifest=local_manifest,
                                  sleep=DEFAULT_SLEEP_TIME,
                                  backoff_factor=2,
                                  log_retries=True)
        else:
          # No need to retry
          raise

      if network_only:
        return

      if detach:
        cmd.append('--detach')

      # Do the local sync; note that there is a couple of corner cases where
      # the new manifest cannot transition from the old checkout cleanly-
      # primarily involving git submodules.  Thus we intercept, and do
      # a forced wipe, then a retry.
      try:
        cros_build_lib.RunCommand(cmd + ['-l'], cwd=self.directory)
      except cros_build_lib.RunCommandError:
        manifest = git.ManifestCheckout.Cached(self.directory)
        targets = set(project['path'].split('/', 1)[0]
                      for project in manifest.ListCheckouts())
        if not targets:
          # No directories to wipe, thus nothing we can fix.
          raise

        cros_build_lib.SudoRunCommand(['rm', '-rf'] + sorted(targets),
                                      cwd=self.directory)

        # Retry the sync now; if it fails, let the exception propagate.
        cros_build_lib.RunCommand(cmd + ['-l'], cwd=self.directory)

      # We do a second run to fix any new repositories created by repo to
      # use relative object pathways.  Note that cros_sdk also triggers the
      # same cleanup- we however kick it erring on the side of caution.
      self._EnsureMirroring(True)
      self._DoCleanup()

    except cros_build_lib.RunCommandError as e:
      err_msg = e.Stringify()
      logging.error(err_msg)
      raise SrcCheckOutException(err_msg)

  def _DoCleanup(self):
    """Wipe unused repositories."""

    # Find all projects, even if they're not in the manifest.  Note the find
    # trickery this is done to keep it as fast as possible.
    repo_path = os.path.join(self.directory, '.repo', 'projects')
    current = set(cros_build_lib.RunCommand(
        ['find', repo_path, '-type', 'd', '-name', '*.git', '-printf', '%P\n',
         '-a', '!', '-wholename', '*.git/*', '-prune'],
        print_cmd=False, capture_output=True).output.splitlines())
    data = {}.fromkeys(current, 0)

    path = os.path.join(self.directory, '.repo', 'project.lru')
    if os.path.exists(path):
      existing = [x.strip().split(None, 1)
                  for x in osutils.ReadFile(path).splitlines()]
      data.update((k, int(v)) for k, v in existing if k in current)

    # Increment it all...
    data.update((k, v + 1) for k, v in data.iteritems())
    # Zero out what is now used.
    checkouts = git.ManifestCheckout.Cached(self.directory).ListCheckouts()
    data.update(('%s.git' % x['path'], 0) for x in checkouts)

    # Finally... wipe anything that's greater than our threshold.
    wipes = [k for k, v in data.iteritems() if v > self.LRU_THRESHOLD]
    if wipes:
      cros_build_lib.SudoRunCommand(
          ['rm', '-rf'] + [os.path.join(repo_path, proj) for proj in wipes])
      map(data.pop, wipes)

    osutils.WriteFile(path, "\n".join('%s %i' % x for x in data.iteritems()))

  def GetRelativePath(self, path):
    """Returns full path including source directory of path in repo."""
    return os.path.join(self.directory, path)

  def ExportManifest(self, mark_revision=False, revisions=True):
    """Export the revision locked manifest

    Args:
      mark_revision: If True, then the sha1 of manifest.git is recorded
        into the resultant manifest tag as a version attribute.
        Specifically, if manifests.git is at 1234, <manifest> becomes
        <manifest revision="1234">.
      revisions: If True, then rewrite all branches/tags into a specific
        sha1 revision.  If False, don't.

    Returns:
      The manifest as a string.
    """
    cmd = [self.repo_cmd, 'manifest', '-o', '-']
    if revisions:
      cmd += ['-r']
    output = cros_build_lib.RunCommand(
        cmd, cwd=self.directory, print_cmd=False, capture_output=True,
        extra_env={'PAGER':'cat'}).output

    if not mark_revision:
      return output
    modified = git.RunGit(os.path.join(self.directory, '.repo/manifests'),
                          ['rev-list', '-n1', 'HEAD'])
    assert modified.output
    return output.replace("<manifest>", '<manifest revision="%s">' %
                          modified.output.strip())

  def IsManifestDifferent(self, other_manifest):
    """Checks whether this manifest is different than another.

    May blacklists certain repos as part of the diff.

    Args:
      other_manifest: Second manifest file to compare against.

    Returns:
      True: If the manifests are different
      False: If the manifests are same
    """
    logging.debug('Calling IsManifestDifferent against %s', other_manifest)

    black_list = ['="chromium/']
    blacklist_pattern = re.compile(r'|'.join(black_list))
    manifest_revision_pattern = re.compile(r'<manifest revision="[a-f0-9]+">',
                                           re.I)

    current = self.ExportManifest()
    with open(other_manifest, 'r') as manifest2_fh:
      for (line1, line2) in zip(current.splitlines(), manifest2_fh):
        line1 = line1.strip()
        line2 = line2.strip()
        if blacklist_pattern.search(line1):
          logging.debug('%s ignored %s', line1, line2)
          continue

        if line1 != line2:
          logging.debug('Current and other manifest differ.')
          logging.debug('current: "%s"', line1)
          logging.debug('other  : "%s"', line2)

          # Ignore revision differences on the manifest line. The revision of
          # the manifest.git repo is uninteresting when determining if the
          # current manifest describes the same sources as the other manifest.
          if manifest_revision_pattern.search(line2):
            logging.debug('Ignoring difference in manifest revision.')
            continue

          return True

      return False
