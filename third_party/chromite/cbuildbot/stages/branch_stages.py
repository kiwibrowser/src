# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the branch stages."""

from __future__ import print_function

import os
import re
from xml.etree import ElementTree

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.cbuildbot import manifest_version
from chromite.cbuildbot.stages import generic_stages
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import parallel


site_config = config_lib.GetConfig()


class BranchError(Exception):
  """Raised by branch creation code on error."""


class BranchUtilStage(generic_stages.BuilderStage):
  """Creates, deletes and renames branches, depending on cbuildbot options.

  The two main types of branches are release branches and non-release
  branches.  Release branches have the form 'release-*' - e.g.,
  'release-R29-4319.B'.

  On a very basic level, a branch is created by parsing the manifest of a
  specific version of Chrome OS (e.g., 4319.0.0), and creating the branch
  remotely for each checkout in the manifest at the specified hash.

  Once a branch is created however, the branch component of the version on the
  newly created branch needs to be incremented.  Additionally, in some cases
  the Chrome major version (i.e, R29) and/or the Chrome OS version (i.e.,
  4319.0.0) of the source branch must be incremented
  (see _IncrementVersionOnDiskForSourceBranch docstring).  Finally, the external
  and internal manifests of the new branch need to be fixed up (see
  FixUpManifests docstring).
  """

  COMMIT_MESSAGE = 'Bump %(target)s after branching %(branch)s'

  def __init__(self, builder_run, **kwargs):
    super(BranchUtilStage, self).__init__(builder_run, **kwargs)
    self.skip_remote_push = (self._run.options.skip_remote_push or
                             self._run.options.debug)
    self.branch_name = self._run.options.branch_name
    self.rename_to = self._run.options.rename_to

  def _RunPush(self, checkout, src_ref, dest_ref, force=False):
    """Perform a git push for a checkout.

    Args:
      checkout: A dictionary of checkout manifest attributes.
      src_ref: The source local ref to push to the remote.
      dest_ref: The local remote ref that correspond to destination ref name.
      force: Whether to override non-fastforward checks.
    """
    # Convert local tracking ref to refs/heads/* on a remote:
    # refs/remotes/<remote name>/<branch> to refs/heads/<branch>.
    # If dest_ref is already refs/heads/<branch> it's a noop.
    dest_ref = git.NormalizeRef(git.StripRefs(dest_ref))
    push_to = git.RemoteRef(checkout['push_remote'], dest_ref)
    git.GitPush(checkout['local_path'], src_ref, push_to, force=force,
                skip=self.skip_remote_push)

  def _FetchAndCheckoutTo(self, checkout_dir, remote_ref):
    """Fetch a remote ref and check out to it.

    Args:
      checkout_dir: Path to git repo to operate on.
      remote_ref: A git.RemoteRef object.
    """
    git.RunGit(checkout_dir, ['fetch', remote_ref.remote, remote_ref.ref],
               print_cmd=True)
    git.RunGit(checkout_dir, ['checkout', 'FETCH_HEAD'], print_cmd=True)

  def _GetBranchSuffix(self, manifest, checkout):
    """Return the branch suffix for the given checkout.

    If a given project is checked out to multiple locations, it is necessary
    to append a branch suffix. To be safe, we append branch suffixes for all
    repositories that use a non-standard branch name (e.g., if our default
    revision is "master", then any repository which does not use "master"
    has a non-standard branch name.)

    Args:
      manifest: The associated ManifestCheckout.
      checkout: The associated ProjectCheckout.
    """
    # Get the default and tracking branch.
    suffix = ''
    if len(manifest.FindCheckouts(checkout['name'])) > 1:
      default_branch = git.StripRefs(manifest.default['revision'])
      tracking_branch = git.StripRefs(checkout['tracking_branch'])
      suffix = '-%s' % (tracking_branch,)
      if default_branch != 'master':
        suffix = re.sub('^-%s-' % re.escape(default_branch), '-', suffix)
    return suffix

  def _GetSHA1(self, checkout, branch):
    """Get the SHA1 for the specified |branch| in the specified |checkout|.

    Args:
      checkout: The ProjectCheckout to look in.
      branch: Remote branch to look for.

    Returns:
      If the branch exists, returns the SHA1 of the branch. Otherwise, returns
      the empty string.  If branch is None, return None.
    """
    if branch:
      cmd = ['show-ref', branch]
      result = git.RunGit(checkout['local_path'], cmd, error_code_ok=True)
      if result.returncode == 0:
        # Output looks like:
        # a00733b...30ee40e0c2c1 refs/remotes/cros/test-4980.B
        return result.output.strip().split()[0]

      return ''

  def _CopyBranch(self, src_checkout, src_branch, dst_branch, force=False):
    """Copy the given |src_branch| to |dst_branch|.

    Args:
      src_checkout: The ProjectCheckout to work in.
      src_branch: The remote branch ref to copy from.
      dst_branch: The remote branch ref to copy to.
      force: If True then execute the copy even if dst_branch exists.
    """
    logging.info('Creating new branch "%s" for %s.', dst_branch,
                 src_checkout['name'])
    self._RunPush(src_checkout, src_ref=src_branch, dest_ref=dst_branch,
                  force=force)

  def _DeleteBranch(self, src_checkout, branch):
    """Delete the given |branch| in the given |src_checkout|.

    Args:
      src_checkout: The ProjectCheckout to work in.
      branch: The branch ref to delete.  Must be a remote branch.
    """
    logging.info('Deleting branch "%s" for %s.', branch, src_checkout['name'])
    self._RunPush(src_checkout, src_ref='', dest_ref=branch)

  def _ProcessCheckout(self, src_manifest, src_checkout):
    """Performs per-checkout push operations.

    Args:
      src_manifest: The ManifestCheckout object for the current manifest.
      src_checkout: The ProjectCheckout object to process.
    """
    if not src_checkout.IsBranchableProject():
      # We don't have the ability to push branches to this repository. Just
      # use TOT instead.
      return

    checkout_name = src_checkout['name']
    remote = src_checkout['push_remote']
    src_ref = src_checkout['revision']
    suffix = self._GetBranchSuffix(src_manifest, src_checkout)

    # The source/destination branches depend on options.
    if self.rename_to:
      # Rename flow.  Both src and dst branches exist.
      src_branch = '%s%s' % (self.branch_name, suffix)
      dst_branch = '%s%s' % (self.rename_to, suffix)
    elif self._run.options.delete_branch:
      # Delete flow.  Only dst branch exists.
      src_branch = None
      dst_branch = '%s%s' % (self.branch_name, suffix)
    else:
      # Create flow (default).  Only dst branch exists.  Source
      # for the branch will just be src_ref.
      src_branch = None
      dst_branch = '%s%s' % (self.branch_name, suffix)

    # Normalize branch refs to remote.  We only process remote branches.
    src_branch = git.NormalizeRemoteRef(remote, src_branch)
    dst_branch = git.NormalizeRemoteRef(remote, dst_branch)

    # Determine whether src/dst branches exist now, by getting their sha1s.
    if src_branch:
      src_sha1 = self._GetSHA1(src_checkout, src_branch)
    elif git.IsSHA1(src_ref):
      src_sha1 = src_ref
    dst_sha1 = self._GetSHA1(src_checkout, dst_branch)

    # Complain if the branch already exists, unless that is expected.
    force = self._run.options.force_create or self._run.options.delete_branch
    if dst_sha1 and not force:
      # We are either creating a branch or renaming a branch, and the
      # destination branch unexpectedly exists.  Accept this only if the
      # destination branch is already at the revision we want.
      if src_sha1 != dst_sha1:
        raise BranchError('Checkout %s already contains branch %s.  Run with '
                          '--force-create to overwrite.'
                          % (checkout_name, dst_branch))

      logging.info('Checkout %s already contains branch %s and it already'
                   ' points to revision %s', checkout_name, dst_branch,
                   dst_sha1)

    elif self._run.options.delete_branch:
      # Delete the dst_branch, if it exists.
      if dst_sha1:
        self._DeleteBranch(src_checkout, dst_branch)
      else:
        raise BranchError('Checkout %s does not contain branch %s to delete.'
                          % (checkout_name, dst_branch))

    elif self.rename_to:
      # Copy src_branch to dst_branch, if it exists, then delete src_branch.
      if src_sha1:
        self._CopyBranch(src_checkout, src_branch, dst_branch)
        self._DeleteBranch(src_checkout, src_branch)
      else:
        raise BranchError('Checkout %s does not contain branch %s to rename.'
                          % (checkout_name, src_branch))

    else:
      # Copy src_ref to dst_branch.
      self._CopyBranch(src_checkout, src_ref, dst_branch,
                       force=self._run.options.force_create)

  def _UpdateManifest(self, manifest_path):
    """Rewrite |manifest_path| to point at the right branch.

    Args:
      manifest_path: The path to the manifest file.
    """
    src_manifest = git.ManifestCheckout.Cached(self._build_root,
                                               manifest_path=manifest_path)
    doc = ElementTree.parse(manifest_path)
    root = doc.getroot()

    # Use the local branch ref.
    new_branch_name = self.rename_to if self.rename_to else self.branch_name
    new_branch_name = git.NormalizeRef(new_branch_name)

    logging.info('Updating manifest for %s', new_branch_name)

    default_nodes = root.findall('default')
    for node in default_nodes:
      node.attrib['revision'] = new_branch_name

    for node in root.findall('project'):
      path = node.attrib['path']
      checkout = src_manifest.FindCheckoutFromPath(path)

      if checkout.IsBranchableProject():
        # Point at the new branch.
        node.attrib.pop('revision', None)
        node.attrib.pop('upstream', None)
        suffix = self._GetBranchSuffix(src_manifest, checkout)
        if suffix:
          node.attrib['revision'] = '%s%s' % (new_branch_name, suffix)
          logging.info('Pointing project %s at: %s', node.attrib['name'],
                       node.attrib['revision'])
        elif not default_nodes:
          # If there isn't a default node we have to add the revision directly.
          node.attrib['revision'] = new_branch_name
      else:
        if checkout.IsPinnableProject():
          git_repo = checkout.GetPath(absolute=True)
          repo_head = git.GetGitRepoRevision(git_repo)
          node.attrib['revision'] = repo_head
          logging.info('Pinning project %s at: %s', node.attrib['name'],
                       node.attrib['revision'])
        else:
          logging.info('Updating project %s', node.attrib['name'])
          # We can't branch this repository. Leave it alone.
          node.attrib['revision'] = checkout['revision']
          logging.info('Project %s UNPINNED using: %s', node.attrib['name'],
                       node.attrib['revision'])

        # Can not use the default version of get() here since
        # 'upstream' can be a valid key with a None value.
        upstream = checkout.get('upstream')
        if upstream is not None:
          node.attrib['upstream'] = upstream

    doc.write(manifest_path)
    return [node.attrib['name'] for node in root.findall('include')]

  def _FixUpManifests(self, repo_manifest):
    """Points the checkouts at the new branch in the manifests.

    Within the branch, make sure all manifests with projects that are
    "branchable" are checked out to "refs/heads/<new_branch>".  Do this
    by updating all manifests in the known manifest projects.
    """
    assert not self._run.options.delete_branch, 'Cannot fix a deleted branch.'

    # Use local branch ref.
    branch_ref = git.NormalizeRef(self.branch_name)

    logging.debug('Fixing manifest projects for new branch.')
    for project in site_config.params.MANIFEST_PROJECTS:
      manifest_checkout = repo_manifest.FindCheckout(project)
      manifest_dir = manifest_checkout['local_path']
      push_remote = manifest_checkout['push_remote']

      # Checkout revision can be either a sha1 or a branch ref.
      src_ref = manifest_checkout['revision']
      if not git.IsSHA1(src_ref):
        src_ref = git.NormalizeRemoteRef(push_remote, src_ref)

      git.CreateBranch(
          manifest_dir, manifest_version.PUSH_BRANCH, src_ref)

      # We want to process default.xml and official.xml + their imports.
      pending_manifests = [constants.DEFAULT_MANIFEST,
                           constants.OFFICIAL_MANIFEST]
      processed_manifests = []

      while pending_manifests:
        # Canonicalize the manifest name (resolve dir and symlinks).
        manifest_path = os.path.join(manifest_dir, pending_manifests.pop())
        manifest_path = os.path.realpath(manifest_path)

        # Don't process a manifest more than once.
        if manifest_path in processed_manifests:
          continue

        processed_manifests.append(manifest_path)

        if not os.path.exists(manifest_path):
          logging.info('Manifest not found: %s', manifest_path)
          continue

        logging.debug('Fixing manifest at %s.', manifest_path)
        included_manifests = self._UpdateManifest(manifest_path)
        pending_manifests += included_manifests

      git.RunGit(manifest_dir, ['add', '-A'], print_cmd=True)
      message = 'Fix up manifest after branching %s.' % branch_ref
      git.RunGit(manifest_dir, ['commit', '-m', message], print_cmd=True)
      push_to = git.RemoteRef(push_remote, branch_ref)
      git.GitPush(manifest_dir, manifest_version.PUSH_BRANCH, push_to,
                  skip=self.skip_remote_push)

  def _IncrementVersionOnDisk(self, incr_type, push_to, message):
    """Bumps the version found in chromeos_version.sh on a branch.

    Args:
      incr_type: See docstring for manifest_version.VersionInfo.
      push_to: A git.RemoteRef object.
      message: The message to give the git commit that bumps the version.
    """
    version_info = manifest_version.VersionInfo.from_repo(
        self._build_root, incr_type=incr_type)
    version_info.IncrementVersion()
    version_info.UpdateVersionFile(message,
                                   dry_run=self.skip_remote_push,
                                   push_to=push_to)

  @staticmethod
  def DetermineBranchIncrParams(version_info):
    """Determines the version component to bump for the new branch."""
    # We increment the left-most component that is zero.
    if version_info.branch_build_number != '0':
      if version_info.patch_number != '0':
        raise BranchError('Version %s cannot be branched.' %
                          version_info.VersionString())
      return 'patch', 'patch number'
    else:
      return 'branch', 'branch number'

  @staticmethod
  def DetermineSourceIncrParams(source_name, dest_name):
    """Determines the version component to bump for the original branch."""
    if dest_name.startswith('refs/heads/release-'):
      return 'chrome_branch', 'Chrome version'
    elif source_name == 'refs/heads/master':
      return 'build', 'build number'
    else:
      return 'branch', 'branch build number'

  def _IncrementVersionOnDiskForNewBranch(self, push_remote):
    """Bumps the version found in chromeos_version.sh on the new branch

    When a new branch is created, the branch component of the new branch's
    version needs to bumped.

    For example, say 'stabilize-link' is created from a the 4230.0.0 manifest.
    The new branch's version needs to be bumped to 4230.1.0.

    Args:
      push_remote: a git remote name where the new branch lives.
    """
    # This needs to happen before the source branch version bumping above
    # because we rely on the fact that since our current overlay checkout
    # is what we just pushed to the new branch, we don't need to do another
    # sync.  This also makes it easier to implement skip_remote_push
    # functionality (the new branch doesn't actually get created in
    # skip_remote_push mode).

    # Use local branch ref.
    branch_ref = git.NormalizeRef(self.branch_name)
    push_to = git.RemoteRef(push_remote, branch_ref)
    version_info = manifest_version.VersionInfo(
        version_string=self._run.options.force_version)
    incr_type, incr_target = self.DetermineBranchIncrParams(version_info)
    message = self.COMMIT_MESSAGE % {
        'target': incr_target,
        'branch': branch_ref,
    }
    self._IncrementVersionOnDisk(incr_type, push_to, message)

  def _IncrementVersionOnDiskForSourceBranch(self, overlay_dir, push_remote,
                                             source_branch):
    """Bumps the version found in chromeos_version.sh on the source branch

    The source branch refers to the branch that the manifest used for creating
    the new branch came from.  For release branches, we generally branch from a
    'master' branch manifest.

    To work around crbug.com/213075, for both non-release and release branches,
    we need to bump the Chrome OS version on the source branch if the manifest
    used for branch creation is the latest generated manifest for the source
    branch.

    When we are creating a release branch, the Chrome major version of the
    'master' (source) branch needs to be bumped.  For example, if we branch
    'release-R29-4230.B' from the 4230.0.0 manifest (which is from the 'master'
    branch), the 'master' branch's Chrome major version in chromeos_version.sh
    (which is 29) needs to be bumped to 30.

    Args:
      overlay_dir: Absolute path to the chromiumos overlay repo.
      push_remote: The remote to push to.
      source_branch: The branch that the manifest we are using comes from.
    """
    push_to = git.RemoteRef(push_remote, source_branch)
    self._FetchAndCheckoutTo(overlay_dir, push_to)

    # Use local branch ref.
    branch_ref = git.NormalizeRef(self.branch_name)
    tot_version_info = manifest_version.VersionInfo.from_repo(self._build_root)
    if (branch_ref.startswith('refs/heads/release-') or
        tot_version_info.VersionString() == self._run.options.force_version):
      incr_type, incr_target = self.DetermineSourceIncrParams(
          source_branch, branch_ref)
      message = self.COMMIT_MESSAGE % {
          'target': incr_target,
          'branch': branch_ref,
      }
      try:
        self._IncrementVersionOnDisk(incr_type, push_to, message)
      except cros_build_lib.RunCommandError:
        # There's a chance we are racing against the buildbots for this
        # increment.  We shouldn't quit the script because of this.  Instead, we
        # print a warning.
        self._FetchAndCheckoutTo(overlay_dir, push_to)
        new_version = manifest_version.VersionInfo.from_repo(self._build_root)
        if new_version.VersionString() != tot_version_info.VersionString():
          logging.warning('Version number for branch %s was bumped by another '
                          'bot.', push_to.ref)
        else:
          raise

  def PerformStage(self):
    """Run the branch operation."""
    # Setup and initialize the repo.
    super(BranchUtilStage, self).PerformStage()

    repo_manifest = git.ManifestCheckout.Cached(self._build_root)
    checkouts = repo_manifest.ListCheckouts()

    logging.debug('Processing %d checkouts from manifest in parallel.',
                  len(checkouts))
    args = [[repo_manifest, x] for x in checkouts]
    parallel.RunTasksInProcessPool(self._ProcessCheckout, args, processes=16)

    if not self._run.options.delete_branch:
      self._FixUpManifests(repo_manifest)

    # Increment versions for a new branch.
    if not (self._run.options.delete_branch or self.rename_to):
      overlay_name = 'chromiumos/overlays/chromiumos-overlay'
      overlay_checkout = repo_manifest.FindCheckout(overlay_name)
      overlay_dir = overlay_checkout['local_path']
      push_remote = overlay_checkout['push_remote']
      self._IncrementVersionOnDiskForNewBranch(push_remote)

      source_branch = repo_manifest.default['revision']
      self._IncrementVersionOnDiskForSourceBranch(overlay_dir, push_remote,
                                                  source_branch)
