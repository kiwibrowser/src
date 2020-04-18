# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A library to generate and store the manifests for cros builders to use."""

from __future__ import print_function

import codecs
import os
import re
import tempfile
from xml.dom import minidom
from xml.parsers import expat

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.cbuildbot import manifest_version
from chromite.cbuildbot import repository
from chromite.cbuildbot import trybot_patch_pool
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import osutils


site_config = config_lib.GetConfig()


# Paladin constants for manifest names.
PALADIN_COMMIT_ELEMENT = 'pending_commit'

ANDROID_ELEMENT = 'android'
ANDROID_VERSION_ATTR = 'version'
CHROME_ELEMENT = 'chrome'
CHROME_VERSION_ATTR = 'version'
LKGM_ELEMENT = 'lkgm'
LKGM_VERSION_ATTR = 'version'


class PromoteCandidateException(Exception):
  """Exception thrown for failure to promote manifest candidate."""


class _LKGMCandidateInfo(manifest_version.VersionInfo):
  """Class to encapsualte the chrome os lkgm candidate info

  You can instantiate this class in two ways.
  1)using a version file, specifically chromeos_version.sh,
  which contains the version information.
  2) just passing in the 4 version components (major, minor, sp, patch and
    revision number),
  Args:
      You can instantiate this class in two ways.
  1)using a version file, specifically chromeos_version.sh,
  which contains the version information.
  2) passing in a string with the 3 version components + revision e.g. 41.0.0-r1
  Args:
    version_string: Optional 3 component version string to parse.  Contains:
        build_number: release build number.
        branch_build_number: current build number on a branch.
        patch_number: patch number.
        revision_number: version revision
    chrome_branch: If version_string specified, specify chrome_branch i.e. 13.
    version_file: version file location.
  """
  LKGM_RE = r'(\d+\.\d+\.\d+)(?:-rc(\d+))?'

  def __init__(self, version_string=None, chrome_branch=None, incr_type=None,
               version_file=None):
    self.revision_number = 1
    if version_string:
      match = re.search(self.LKGM_RE, version_string)
      assert match, 'LKGM did not re %s' % self.LKGM_RE
      super(_LKGMCandidateInfo, self).__init__(match.group(1), chrome_branch,
                                               incr_type=incr_type)
      if match.group(2):
        self.revision_number = int(match.group(2))

    else:
      super(_LKGMCandidateInfo, self).__init__(version_file=version_file,
                                               incr_type=incr_type)

  def VersionString(self):
    """returns the full version string of the lkgm candidate"""
    return '%s.%s.%s-rc%s' % (self.build_number, self.branch_build_number,
                              self.patch_number, self.revision_number)

  def VersionComponents(self):
    """Return an array of ints of the version fields for comparing."""
    return map(int, [self.build_number, self.branch_build_number,
                     self.patch_number, self.revision_number])

  def IncrementVersion(self):
    """Increments the version by incrementing the revision #."""
    self.revision_number += 1
    return self.VersionString()

  def UpdateVersionFile(self, *args, **kwargs):
    """Update the version file on disk.

    For LKGMCandidateInfo there is no version file so this function is a no-op.
    """


class LKGMManager(manifest_version.BuildSpecsManager):
  """A Class to manage lkgm candidates and their states.

  Vars:
    lkgm_subdir:  Subdirectory within manifest repo to store candidates.
  """
  # Sub-directories for LKGM and Chrome LKGM's.
  LKGM_SUBDIR = 'LKGM-candidates'
  CHROME_PFQ_SUBDIR = 'chrome-LKGM-candidates'
  ANDROID_PFQ_SUBDIR = 'android-LKGM-candidates'
  COMMIT_QUEUE_SUBDIR = 'paladin'
  TOOLCHAIN_SUBDIR = 'toolchain'

  def __init__(self, source_repo, manifest_repo, build_names, build_type,
               incr_type, force, branch, manifest=constants.DEFAULT_MANIFEST,
               dry_run=True, lkgm_path_rel=constants.LKGM_MANIFEST,
               config=None, metadata=None, buildbucket_client=None):
    """Initialize an LKGM Manager.

    Args:
      source_repo: Repository object for the source code.
      manifest_repo: Manifest repository for manifest versions/buildspecs.
      build_names: Identifiers for the build. Must match config_lib
          entries. If multiple identifiers are provided, the first item in the
          list must be an identifier for the group.
      build_type: Type of build.  Must be a pfq type.
      incr_type: How we should increment this version - build|branch|patch
      force: Create a new manifest even if there are no changes.
      branch: Branch this builder is running on.
      manifest: Manifest to use for checkout. E.g. 'full' or 'buildtools'.
      dry_run: Whether we actually commit changes we make or not.
      master: Whether we are the master builder.
      lkgm_path_rel: Path to the LKGM symlink, relative to manifest dir.
      config: Instance of config_lib.BuildConfig. Config dict of this builder.
      metadata: Instance of metadata_lib.CBuildbotMetadata. Metadata of this
                builder.
      buildbucket_client: Instance of buildbucket_lib.buildbucket_client.
    """
    super(LKGMManager, self).__init__(
        source_repo=source_repo, manifest_repo=manifest_repo,
        manifest=manifest, build_names=build_names, incr_type=incr_type,
        force=force, branch=branch, dry_run=dry_run,
        config=config, metadata=metadata,
        buildbucket_client=buildbucket_client)

    self.lkgm_path = os.path.join(self.manifest_dir, lkgm_path_rel)
    self.compare_versions_fn = _LKGMCandidateInfo.VersionCompare
    self.build_type = build_type
    # Chrome PFQ and PFQ's exist at the same time and version separately so they
    # must have separate subdirs in the manifest-versions repository.
    if self.build_type == constants.CHROME_PFQ_TYPE:
      self.rel_working_dir = self.CHROME_PFQ_SUBDIR
    elif self.build_type == constants.ANDROID_PFQ_TYPE:
      self.rel_working_dir = self.ANDROID_PFQ_SUBDIR
    elif self.build_type == constants.TOOLCHAIN_TYPE:
      self.rel_working_dir = self.TOOLCHAIN_SUBDIR
    elif config_lib.IsCQType(self.build_type):
      self.rel_working_dir = self.COMMIT_QUEUE_SUBDIR
    else:
      assert config_lib.IsPFQType(self.build_type)
      self.rel_working_dir = self.LKGM_SUBDIR

  def GetCurrentVersionInfo(self):
    """Returns the lkgm version info from the version file."""
    version_info = super(LKGMManager, self).GetCurrentVersionInfo()
    return _LKGMCandidateInfo(version_info.VersionString(),
                              chrome_branch=version_info.chrome_branch,
                              incr_type=self.incr_type)

  def _WriteXml(self, dom_instance, file_path):
    """Wrapper function to write xml encoded in a proper way.

    Args:
      dom_instance: A DOM document instance contains contents to be written.
      file_path: Path to the file to write into.
    """
    with codecs.open(file_path, 'w+', 'utf-8') as f:
      dom_instance.writexml(f)

  def _AddLKGMToManifest(self, manifest):
    """Write the last known good version string to the manifest.

    Args:
      manifest: Path to the manifest.
    """
    # Get the last known good version string.
    try:
      lkgm_filename = os.path.basename(os.readlink(self.lkgm_path))
      lkgm_version, _ = os.path.splitext(lkgm_filename)
    except OSError:
      return

    # Write the last known good version string to the manifest.
    try:
      manifest_dom = minidom.parse(manifest)
    except expat.ExpatError:
      logging.error('Got parsing error for: %s', manifest)
      logging.error('Bad XML:\n%s', osutils.ReadFile(manifest))
      raise

    lkgm_element = manifest_dom.createElement(LKGM_ELEMENT)
    lkgm_element.setAttribute(LKGM_VERSION_ATTR, lkgm_version)
    manifest_dom.documentElement.appendChild(lkgm_element)
    self._WriteXml(manifest_dom, manifest)

  def _AddAndroidVersionToManifest(self, manifest, android_version):
    """Adds the Android element with version |android_version| to |manifest|.

    The manifest file should contain the Android version to build for
    PFQ slaves.

    Args:
      manifest: Path to the manifest
      android_version: A string representing the version of Android
    """
    manifest_dom = minidom.parse(manifest)
    android = manifest_dom.createElement(ANDROID_ELEMENT)
    android.setAttribute(ANDROID_VERSION_ATTR, android_version)
    manifest_dom.documentElement.appendChild(android)
    self._WriteXml(manifest_dom, manifest)

  def _AddChromeVersionToManifest(self, manifest, chrome_version):
    """Adds the chrome element with version |chrome_version| to |manifest|.

    The manifest file should contain the Chrome version to build for
    PFQ slaves.

    Args:
      manifest: Path to the manifest
      chrome_version: A string representing the version of Chrome
        (e.g. 35.0.1863.0).
    """
    manifest_dom = minidom.parse(manifest)
    chrome = manifest_dom.createElement(CHROME_ELEMENT)
    chrome.setAttribute(CHROME_VERSION_ATTR, chrome_version)
    manifest_dom.documentElement.appendChild(chrome)
    self._WriteXml(manifest_dom, manifest)

  def _AddPatchesToManifest(self, manifest, validation_pool):
    """Adds list of |patches| to given |manifest|.

    The manifest should have sufficient information for the slave
    builders to fetch the patches from Gerrit and to print the CL link
    (see cros_patch.GerritFetchOnlyPatch).

    Args:
      manifest: Path to the manifest.
      validation_pool: Validation pool to apply to the manifest.
    """
    manifest_dom = minidom.parse(manifest)
    for patch in validation_pool.applied:
      pending_commit = manifest_dom.createElement(PALADIN_COMMIT_ELEMENT)
      attr_dict = patch.GetAttributeDict()
      for k, v in attr_dict.iteritems():
        pending_commit.setAttribute(k, v)

      try:
        # A patch with unicode can be added to manifest,
        # but not with invalid tokens.
        pending_commit_xml = pending_commit.toxml(encoding='utf-8')
        minidom.parseString(pending_commit_xml)
        manifest_dom.documentElement.appendChild(pending_commit)
      except expat.ExpatError:
        logging.error('Got parsing error for: %s', pending_commit_xml)
        msg = ('Failed to apply your change. Please check if there are '
               'invalid tokens in your commit messages.')
        validation_pool.SendNotification(patch, msg)
        validation_pool.RemoveReady(patch)

    self._WriteXml(manifest_dom, manifest)

  def _AdjustRepoCheckoutToLocalManifest(self, manifest_path):
    """Re-checkout repository based on patched internal manifest repository.

    This method clones the current state of 'manifest-internal' into a temp
    location, and then re-sync's the current repository to that manifest. This
    is intended to allow sync'ing with test manifest CLs included.

    It does NOT clean up afterwards.

    Args:
      manifest_path: Directory containing the already patched manifest.
        Normally SOURCE_ROOT/manifest or SOURCE_ROOT/manifest-internal.
    """
    tmp_manifest_repo = tempfile.mkdtemp(prefix='patched_manifest')

    logging.info('Cloning manifest repository from %s to %s.',
                 manifest_path, tmp_manifest_repo)

    repository.CloneGitRepo(tmp_manifest_repo, manifest_path)
    git.CreateBranch(tmp_manifest_repo, self.cros_source.branch or 'master')

    logging.info('Switching to local patched manifest repository:')
    logging.info('TMPDIR: %s', tmp_manifest_repo)
    logging.info('        %s', os.listdir(tmp_manifest_repo))

    self.cros_source.Initialize(manifest_repo_url=tmp_manifest_repo)
    self.cros_source.Sync(detach=True)

  def CreateNewCandidate(self, validation_pool=None,
                         android_version=None,
                         chrome_version=None,
                         retries=manifest_version.NUM_RETRIES,
                         build_id=None):
    """Creates, syncs to, and returns the next candidate manifest.

    Args:
      validation_pool: Validation pool to apply to the manifest before
        publishing.
      android_version: The Android version to write in the manifest. Defaults
        to None, in which case no version is written.
      chrome_version: The Chrome version to write in the manifest. Defaults
        to None, in which case no version is written.
      retries: Number of retries for updating the status. Defaults to
        manifest_version.NUM_RETRIES.
      build_id: Optional integer cidb id of the build that is creating
                this candidate.

    Raises:
      GenerateBuildSpecException in case of failure to generate a buildspec
    """
    self.CheckoutSourceCode()

    # Refresh manifest logic from manifest_versions repository to grab the
    # LKGM to generate the blamelist.
    version_info = self.GetCurrentVersionInfo()
    self.RefreshManifestCheckout()
    self.InitializeManifestVariables(version_info)

    has_chump_cls = self.GenerateBlameListSinceLKGM()

    # Throw away CLs that might not be used this run.
    if validation_pool:
      validation_pool.has_chump_cls = has_chump_cls
      validation_pool.FilterChangesForThrottledTree()

      # Apply any manifest CLs (internal or exteral).
      validation_pool.ApplyPoolIntoRepo(
          filter_fn=trybot_patch_pool.ManifestFilter)

      manifest_dir = os.path.join(
          validation_pool.build_root, 'manifest-internal')

      if not os.path.exists(manifest_dir):
        # Fall back to external manifest directory.
        manifest_dir = os.path.join(
            validation_pool.build_root, 'manifest')

      # This is only needed if there were internal manifest changes, but we
      # always run it to make sure this logic works.
      self._AdjustRepoCheckoutToLocalManifest(manifest_dir)

    new_manifest = self.CreateManifest()

    # For Android PFQ, add the version of Android to use.
    if android_version:
      self._AddAndroidVersionToManifest(new_manifest, android_version)

    # For Chrome PFQ, add the version of Chrome to use.
    if chrome_version:
      self._AddChromeVersionToManifest(new_manifest, chrome_version)

    # For the Commit Queue, apply the validation pool as part of checkout.
    if validation_pool:
      # If we have nothing that could apply from the validation pool and
      # we're not also a pfq type, we got nothing to do.
      assert self.cros_source.directory == validation_pool.build_root
      if (not validation_pool.ApplyPoolIntoRepo() and
          not config_lib.IsPFQType(self.build_type)):
        return None

      self._AddPatchesToManifest(new_manifest, validation_pool)

      # Add info about the last known good version to the manifest. This will
      # be used by slaves to calculate what artifacts from old builds are safe
      # to use.
      self._AddLKGMToManifest(new_manifest)

    last_error = None
    for attempt in range(0, retries + 1):
      try:
        # Refresh manifest logic from manifest_versions repository.
        # Note we don't need to do this on our first attempt as we needed to
        # have done it to get the LKGM.
        if attempt != 0:
          self.RefreshManifestCheckout()
          self.InitializeManifestVariables(version_info)

        # If we don't have any valid changes to test, make sure the checkout
        # is at least different.
        if ((not validation_pool or not validation_pool.applied) and
            not self.force and self.HasCheckoutBeenBuilt()):
          return None

        # Check whether the latest spec available in manifest-versions is
        # newer than our current version number. If so, use it as the base
        # version number. Otherwise, we default to 'rc1'.
        if self.latest:
          latest = max(self.latest, version_info.VersionString(),
                       key=self.compare_versions_fn)
          version_info = _LKGMCandidateInfo(
              latest, chrome_branch=version_info.chrome_branch,
              incr_type=self.incr_type)

        git.CreatePushBranch(manifest_version.PUSH_BRANCH, self.manifest_dir,
                             sync=False)
        version = self.GetNextVersion(version_info)
        self.PublishManifest(new_manifest, version, build_id=build_id)
        self.current_version = version
        return self.GetLocalManifest(version)
      except cros_build_lib.RunCommandError as e:
        err_msg = 'Failed to generate LKGM Candidate. error: %s' % e
        logging.error(err_msg)
        last_error = err_msg

    raise manifest_version.GenerateBuildSpecException(last_error)

  def CreateFromManifest(self, manifest, retries=manifest_version.NUM_RETRIES,
                         build_id=None):
    """Sets up an lkgm_manager from the given manifest.

    This method sets up an LKGM manager and publishes a new manifest to the
    manifest versions repo based on the passed in manifest but filtering
    internal repositories and changes out of it.

    Args:
      manifest: A manifest that possibly contains private changes/projects. It
        is named with the given version we want to create a new manifest from
        i.e R20-1920.0.1-rc7.xml where R20-1920.0.1-rc7 is the version.
      retries: Number of retries for updating the status.
      build_id: Optional integer cidb build id of the build publishing the
                manifest.

    Returns:
      Path to the manifest version file to use.

    Raises:
      GenerateBuildSpecException in case of failure to check-in the new
        manifest because of a git error or the manifest is already checked-in.
    """
    last_error = None
    new_manifest = manifest_version.FilterManifest(
        manifest, whitelisted_remotes=site_config.params.EXTERNAL_REMOTES)
    version_info = self.GetCurrentVersionInfo()
    for _attempt in range(0, retries + 1):
      try:
        self.RefreshManifestCheckout()
        self.InitializeManifestVariables(version_info)

        git.CreatePushBranch(manifest_version.PUSH_BRANCH, self.manifest_dir,
                             sync=False)
        version = os.path.splitext(os.path.basename(manifest))[0]
        logging.info('Publishing filtered build spec')
        self.PublishManifest(new_manifest, version, build_id=build_id)
        self.current_version = version
        return self.GetLocalManifest(version)
      except cros_build_lib.RunCommandError as e:
        err_msg = 'Failed to generate LKGM Candidate. error: %s' % e
        logging.error(err_msg)
        last_error = err_msg

    raise manifest_version.GenerateBuildSpecException(last_error)

  def PromoteCandidate(self, retries=manifest_version.NUM_RETRIES):
    """Promotes the current LKGM candidate to be a real versioned LKGM."""
    assert self.current_version, 'No current manifest exists.'

    last_error = None
    path_to_candidate = self.GetLocalManifest(self.current_version)
    assert os.path.exists(path_to_candidate), 'Candidate not found locally.'

    # This may potentially fail for not being at TOT while pushing.
    for attempt in range(0, retries + 1):
      try:
        if attempt > 0:
          self.RefreshManifestCheckout()
        git.CreatePushBranch(manifest_version.PUSH_BRANCH,
                             self.manifest_dir, sync=False)
        manifest_version.CreateSymlink(path_to_candidate, self.lkgm_path)
        git.RunGit(self.manifest_dir, ['add', self.lkgm_path])
        self.PushSpecChanges(
            'Automatic: %s promoting %s to LKGM' % (self.build_names[0],
                                                    self.current_version))
        return
      except cros_build_lib.RunCommandError as e:
        last_error = 'Failed to promote manifest. error: %s' % e
        logging.info(last_error)
        logging.info('Retrying to promote manifest:  Retry %d/%d', attempt + 1,
                     retries)

    raise PromoteCandidateException(last_error)

  def _ShouldGenerateBlameListSinceLKGM(self):
    """Returns True if we should generate the blamelist."""
    # We want to generate the blamelist only for valid pfq types and if we are
    # building on the master branch i.e. revving the build number.
    return (self.incr_type == 'build' and
            config_lib.IsPFQType(self.build_type) and
            self.build_type != constants.CHROME_PFQ_TYPE and
            self.build_type != constants.ANDROID_PFQ_TYPE)

  def GenerateBlameListSinceLKGM(self):
    """Prints out links to all CL's that have been committed since LKGM.

    Add buildbot trappings to print <a href='url'>text</a> in the waterfall for
    each CL committed since we last had a passing build.

    Returns:
      A boolean indicating whether the blamelist has chump CLs.
    """
    if not self._ShouldGenerateBlameListSinceLKGM():
      logging.info('Not generating blamelist for lkgm as it is not appropriate '
                   'for this build type.')
      return False
    # Suppress re-printing changes we tried ourselves on paladin
    # builders since they are redundant.
    only_print_chumps = self.build_type == constants.PALADIN_TYPE
    return GenerateBlameList(self.cros_source, self.lkgm_path,
                             only_print_chumps=only_print_chumps)

  def GetLatestPassingSpec(self):
    """Get the last spec file that passed in the current branch."""
    raise NotImplementedError()


def GenerateBlameList(source_repo, lkgm_path, only_print_chumps=False):
  """Generate the blamelist since the specified manifest.

  Args:
    source_repo: Repository object for the source code.
    lkgm_path: Path to LKGM manifest.
    only_print_chumps: If True, only print changes that were chumped.

  Returns:
    A boolean indicating whether the blamelist has chump CLs.
  """
  has_chump_cls = False
  handler = git.Manifest(lkgm_path)
  reviewed_on_re = re.compile(r'\s*Reviewed-on:\s*(\S+)')
  author_re = re.compile(r'\s*Author:.*<(\S+)@\S+>\s*')
  committer_re = re.compile(r'\s*Commit:.*<(\S+)@\S+>\s*')
  for rel_src_path, checkout in handler.checkouts_by_path.iteritems():
    project = checkout['name']

    # Additional case in case the repo has been removed from the manifest.
    src_path = source_repo.GetRelativePath(rel_src_path)
    if not os.path.exists(src_path):
      logging.info('Detected repo removed from manifest %s' % project)
      continue

    revision = checkout['revision']
    cmd = ['log', '--pretty=full', '%s..HEAD' % revision]
    try:
      result = git.RunGit(src_path, cmd)
    except cros_build_lib.RunCommandError as ex:
      # Git returns 128 when the revision does not exist.
      if ex.result.returncode != 128:
        raise
      logging.warning('Detected branch removed from local checkout.')
      logging.PrintBuildbotStepWarnings()
      return has_chump_cls
    current_author = None
    current_committer = None
    for line in unicode(result.output, 'ascii', 'ignore').splitlines():
      author_match = author_re.match(line)
      if author_match:
        current_author = author_match.group(1)

      committer_match = committer_re.match(line)
      if committer_match:
        current_committer = committer_match.group(1)

      review_match = reviewed_on_re.match(line)
      if review_match:
        review = review_match.group(1)
        _, _, change_number = review.rpartition('/')
        if not current_author:
          logging.notice('Failed to locate author before the line of review: '
                         '%s. Author name is set to <Unknown>', line)
          current_author = '<Unknown>'
        items = [
            os.path.basename(project),
            current_author,
            change_number,
        ]
        # TODO(phobbs) verify the domain of the email address as well.
        if current_committer not in ('chrome-bot', 'chrome-internal-fetch',
                                     'chromeos-commit-bot', '3su6n15k.default'):
          items.insert(0, 'CHUMP')
          has_chump_cls = True
        elif only_print_chumps:
          continue
        logging.PrintBuildbotLink(' | '.join(items), review)

  return has_chump_cls
