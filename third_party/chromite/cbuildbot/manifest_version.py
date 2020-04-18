# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A library to generate and store the manifests for cros builders to use."""

from __future__ import print_function

import datetime
import fnmatch
import glob
import os
import re
import shutil
import tempfile
from xml.dom import minidom

from chromite.cbuildbot import build_status
from chromite.cbuildbot import repository
from chromite.lib import buildbucket_lib
from chromite.lib import builder_status_lib
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import timeout_util


site_config = config_lib.GetConfig()

PUSH_BRANCH = 'temp_auto_checkin_branch'
NUM_RETRIES = 20

MANIFEST_ELEMENT = 'manifest'
DEFAULT_ELEMENT = 'default'
PROJECT_ELEMENT = 'project'
REMOTE_ELEMENT = 'remote'
PROJECT_NAME_ATTR = 'name'
PROJECT_REMOTE_ATTR = 'remote'
PROJECT_GROUP_ATTR = 'groups'
REMOTE_NAME_ATTR = 'name'

PALADIN_COMMIT_ELEMENT = 'pending_commit'
PALADIN_PROJECT_ATTR = 'project'


class FilterManifestException(Exception):
  """Exception thrown when failing to filter the internal manifest."""


class VersionUpdateException(Exception):
  """Exception gets thrown for failing to update the version file"""


class StatusUpdateException(Exception):
  """Exception gets thrown for failure to update the status"""


class GenerateBuildSpecException(Exception):
  """Exception gets thrown for failure to Generate a buildspec for the build"""


class BuildSpecsValueError(Exception):
  """Exception gets thrown when a encountering invalid values."""


def RefreshManifestCheckout(manifest_dir, manifest_repo):
  """Checks out manifest-versions into the manifest directory.

  If a repository is already present, it will be cleansed of any local
  changes and restored to its pristine state, checking out the origin.
  """
  reinitialize = True
  if os.path.exists(manifest_dir):
    result = git.RunGit(manifest_dir, ['config', 'remote.origin.url'],
                        error_code_ok=True)
    if (result.returncode == 0 and
        result.output.rstrip() == manifest_repo):
      logging.info('Updating manifest-versions checkout.')
      try:
        git.RunGit(manifest_dir, ['gc', '--auto'])
        git.CleanAndCheckoutUpstream(manifest_dir)
      except cros_build_lib.RunCommandError:
        logging.warning('Could not update manifest-versions checkout.')
      else:
        reinitialize = False
  else:
    logging.info('No manifest-versions checkout exists at %s', manifest_dir)

  if reinitialize:
    logging.info('Cloning fresh manifest-versions checkout.')
    osutils.RmDir(manifest_dir, ignore_missing=True)
    repository.CloneGitRepo(manifest_dir, manifest_repo)


def _PushGitChanges(git_repo, message, dry_run=False, push_to=None):
  """Push the final commit into the git repo.

  Args:
    git_repo: git repo to push
    message: Commit message
    dry_run: If true, don't actually push changes to the server
    push_to: A git.RemoteRef object specifying the remote branch to push to.
      Defaults to the tracking branch of the current branch.
  """
  if push_to is None:
    # TODO(akeshet): Clean up git.GetTrackingBranch to always or never return a
    # tuple.
    # pylint: disable=unpacking-non-sequence
    push_to = git.GetTrackingBranch(
        git_repo, for_checkout=False, for_push=True)

  git.RunGit(git_repo, ['add', '-A'])

  # It's possible that while we are running on dry_run, someone has already
  # committed our change.
  try:
    git.RunGit(git_repo, ['commit', '-m', message])
  except cros_build_lib.RunCommandError:
    if dry_run:
      return
    raise

  git.GitPush(git_repo, PUSH_BRANCH, push_to, skip=dry_run)


def CreateSymlink(src_file, dest_file):
  """Creates a relative symlink from src to dest with optional removal of file.

  More robust symlink creation that creates a relative symlink from src_file to
  dest_file.

  This is useful for multiple calls of CreateSymlink where you are using
  the dest_file location to store information about the status of the src_file.

  Args:
    src_file: source for the symlink
    dest_file: destination for the symlink
  """
  dest_dir = os.path.dirname(dest_file)
  osutils.SafeUnlink(dest_file)
  osutils.SafeMakedirs(dest_dir)

  rel_src_file = os.path.relpath(src_file, dest_dir)
  logging.debug('Linking %s to %s', rel_src_file, dest_file)
  os.symlink(rel_src_file, dest_file)


class VersionInfo(object):
  """Class to encapsulate the Chrome OS version info scheme.

  You can instantiate this class in three ways.
  1) using a version file, specifically chromeos_version.sh,
     which contains the version information.
  2) passing in a string with the 3 version components.
  3) using a source repo and calling from_repo().

  Args:
    version_string: Optional 3 component version string to parse.  Contains:
        build_number: release build number.
        branch_build_number: current build number on a branch.
        patch_number: patch number.
    chrome_branch: If version_string specified, specify chrome_branch i.e. 13.
    incr_type: How we should increment this version -
        chrome_branch|build|branch|patch
    version_file: version file location.
  """
  # Pattern for matching build name format.  Includes chrome branch hack.
  VER_PATTERN = r'(\d+).(\d+).(\d+)(?:-R(\d+))*'
  KEY_VALUE_PATTERN = r'%s=(\d+)\s*$'
  VALID_INCR_TYPES = ('chrome_branch', 'build', 'branch', 'patch')

  def __init__(self, version_string=None, chrome_branch=None,
               incr_type='build', version_file=None):
    if version_file:
      self.version_file = version_file
      logging.debug('Using VERSION _FILE = %s', version_file)
      self._LoadFromFile()
    else:
      match = re.search(self.VER_PATTERN, version_string)
      self.build_number = match.group(1)
      self.branch_build_number = match.group(2)
      self.patch_number = match.group(3)
      self.chrome_branch = chrome_branch
      self.version_file = None

    self.incr_type = incr_type

  @classmethod
  def from_repo(cls, source_repo, **kwargs):
    kwargs['version_file'] = os.path.join(source_repo, constants.VERSION_FILE)
    return cls(**kwargs)

  def _LoadFromFile(self):
    """Read the version file and set the version components"""
    with open(self.version_file, 'r') as version_fh:
      for line in version_fh:
        if not line.strip():
          continue

        match = self.FindValue('CHROME_BRANCH', line)
        if match:
          self.chrome_branch = match
          logging.debug('Set the Chrome branch number to:%s',
                        self.chrome_branch)
          continue

        match = self.FindValue('CHROMEOS_BUILD', line)
        if match:
          self.build_number = match
          logging.debug('Set the build version to:%s', self.build_number)
          continue

        match = self.FindValue('CHROMEOS_BRANCH', line)
        if match:
          self.branch_build_number = match
          logging.debug('Set the branch version to:%s',
                        self.branch_build_number)
          continue

        match = self.FindValue('CHROMEOS_PATCH', line)
        if match:
          self.patch_number = match
          logging.debug('Set the patch version to:%s', self.patch_number)
          continue

    logging.debug(self.VersionString())

  def FindValue(self, key, line):
    """Given the key find the value from the line, if it finds key = value

    Args:
      key: key to look for
      line: string to search

    Returns:
      None: on a non match
      value: for a matching key
    """
    match = re.search(self.KEY_VALUE_PATTERN % (key,), line)
    return match.group(1) if match else None

  def IncrementVersion(self):
    """Updates the version file by incrementing the patch component."""
    if not self.incr_type or self.incr_type not in self.VALID_INCR_TYPES:
      raise VersionUpdateException('Need to specify the part of the version to'
                                   ' increment')

    if self.incr_type == 'chrome_branch':
      self.chrome_branch = str(int(self.chrome_branch) + 1)

    # Increment build_number for 'chrome_branch' incr_type to avoid
    # crbug.com/213075.
    if self.incr_type in ('build', 'chrome_branch'):
      self.build_number = str(int(self.build_number) + 1)
      self.branch_build_number = '0'
      self.patch_number = '0'
    elif self.incr_type == 'branch' and self.patch_number == '0':
      self.branch_build_number = str(int(self.branch_build_number) + 1)
    else:
      self.patch_number = str(int(self.patch_number) + 1)

    return self.VersionString()

  def UpdateVersionFile(self, message, dry_run, push_to=None):
    """Update the version file with our current version.

    Args:
      message: Commit message.
      dry_run: Git dryrun.
      push_to: A git.RemoteRef object.
    """

    if not self.version_file:
      raise VersionUpdateException('Cannot call UpdateVersionFile without '
                                   'an associated version_file')

    components = (('CHROMEOS_BUILD', self.build_number),
                  ('CHROMEOS_BRANCH', self.branch_build_number),
                  ('CHROMEOS_PATCH', self.patch_number),
                  ('CHROME_BRANCH', self.chrome_branch))

    with tempfile.NamedTemporaryFile(prefix='mvp') as temp_fh:
      with open(self.version_file, 'r') as source_version_fh:
        for line in source_version_fh:
          for key, value in components:
            line = re.sub(self.KEY_VALUE_PATTERN % (key,),
                          '%s=%s\n' % (key, value), line)
          temp_fh.write(line)

      temp_fh.flush()

      repo_dir = os.path.dirname(self.version_file)

      try:
        git.CreateBranch(repo_dir, PUSH_BRANCH)
        shutil.copyfile(temp_fh.name, self.version_file)
        _PushGitChanges(repo_dir, message, dry_run=dry_run, push_to=push_to)
      finally:
        # Update to the remote version that contains our changes. This is needed
        # to ensure that we don't build a release using a local commit.
        git.CleanAndCheckoutUpstream(repo_dir)

  def VersionString(self):
    """returns the version string"""
    return '%s.%s.%s' % (self.build_number, self.branch_build_number,
                         self.patch_number)

  def VersionComponents(self):
    """Return an array of ints of the version fields for comparing."""
    return map(int, [self.build_number, self.branch_build_number,
                     self.patch_number])

  @classmethod
  def VersionCompare(cls, version_string):
    """Useful method to return a comparable version of a LKGM string."""
    return cls(version_string).VersionComponents()

  def __cmp__(self, other):
    sinfo = self.VersionComponents()
    oinfo = other.VersionComponents()

    for s, o in zip(sinfo, oinfo):
      if s != o:
        return -1 if s < o else 1
    return 0

  __hash__ = None

  def BuildPrefix(self):
    """Returns the build prefix to match the buildspecs in  manifest-versions"""
    if self.incr_type == 'branch':
      if self.patch_number == '0':
        return '%s.' % self.build_number
      else:
        return '%s.%s.' % (self.build_number, self.branch_build_number)
    # Default to build incr_type.
    return ''

  def __str__(self):
    return '%s(%s)' % (self.__class__, self.VersionString())


class BuildSpecsManager(object):
  """A Class to manage buildspecs and their states."""

  SLEEP_TIMEOUT = 1 * 60

  def __init__(self, source_repo, manifest_repo, build_names, incr_type, force,
               branch, manifest=constants.DEFAULT_MANIFEST, dry_run=True,
               config=None, metadata=None, db=None, buildbucket_client=None):
    """Initializes a build specs manager.

    Args:
      source_repo: Repository object for the source code.
      manifest_repo: Manifest repository for manifest versions / buildspecs.
      build_names: Identifiers for the build. Must match SiteConfig
          entries. If multiple identifiers are provided, the first item in the
          list must be an identifier for the group.
      incr_type: How we should increment this version - build|branch|patch
      force: Create a new manifest even if there are no changes.
      branch: Branch this builder is running on.
      manifest: Manifest to use for checkout. E.g. 'full' or 'buildtools'.
      dry_run: Whether we actually commit changes we make or not.
      config: Instance of config_lib.BuildConfig. Config dict of this builder.
      metadata: Instance of metadata_lib.CBuildbotMetadata. Metadata of this
                builder.
      db: Instance of cidb.CIDBConnection.
      buildbucket_client: Instance of buildbucket_lib.buildbucket_client.
    """
    self.cros_source = source_repo
    buildroot = source_repo.directory
    if manifest_repo.startswith(site_config.params.INTERNAL_GOB_URL):
      self.manifest_dir = os.path.join(buildroot, 'manifest-versions-internal')
    else:
      self.manifest_dir = os.path.join(buildroot, 'manifest-versions')

    self.manifest_repo = manifest_repo
    self.build_names = build_names
    self.incr_type = incr_type
    self.force = force
    self.branch = branch
    self.manifest = manifest
    self.dry_run = dry_run
    self.config = config
    self.master = False if config is None else config.master
    self.metadata = metadata
    self.db = db
    self.buildbucket_client = buildbucket_client

    # Directories and specifications are set once we load the specs.
    self.buildspecs_dir = None
    self.all_specs_dir = None
    self.pass_dirs = None
    self.fail_dirs = None

    # Specs.
    self.latest = None
    self._latest_build = None
    self.latest_unprocessed = None
    self.compare_versions_fn = VersionInfo.VersionCompare

    self.current_version = None
    self.rel_working_dir = ''

  def _LatestSpecFromList(self, specs):
    """Find the latest spec in a list of specs.

    Args:
      specs: List of specs.

    Returns:
      The latest spec if specs is non-empty.
      None otherwise.
    """
    if specs:
      return max(specs, key=self.compare_versions_fn)

  def _LatestSpecFromDir(self, version_info, directory):
    """Returns the latest buildspec that match '*.xml' in a directory.

    Args:
      version_info: A VersionInfo object which will provide a build prefix
                    to match for.
      directory: Directory of the buildspecs.
    """
    if os.path.exists(directory):
      match_string = version_info.BuildPrefix() + '*.xml'
      specs = fnmatch.filter(os.listdir(directory), match_string)
      return self._LatestSpecFromList([os.path.splitext(m)[0] for m in specs])

  def RefreshManifestCheckout(self):
    """Checks out manifest versions into the manifest directory."""
    RefreshManifestCheckout(self.manifest_dir, self.manifest_repo)

  def InitializeManifestVariables(self, version_info=None, version=None):
    """Initializes manifest-related instance variables.

    Args:
      version_info: Info class for version information of cros. If None,
                    version must be specified instead.
      version: Requested version. If None, build the latest version.

    Returns:
      Whether the requested version was found.
    """
    assert version_info or version, 'version or version_info must be specified'
    working_dir = os.path.join(self.manifest_dir, self.rel_working_dir)
    specs_for_builder = os.path.join(working_dir, 'build-name', '%(builder)s')
    self.buildspecs_dir = os.path.join(working_dir, 'buildspecs')

    # If version is specified, find out what Chrome branch it is on.
    if version is not None:
      dirs = glob.glob(os.path.join(self.buildspecs_dir, '*', version + '.xml'))
      if len(dirs) == 0:
        return False
      assert len(dirs) <= 1, 'More than one spec found for %s' % version
      dir_pfx = os.path.basename(os.path.dirname(dirs[0]))
      version_info = VersionInfo(chrome_branch=dir_pfx, version_string=version)
    else:
      dir_pfx = version_info.chrome_branch

    self.all_specs_dir = os.path.join(self.buildspecs_dir, dir_pfx)
    self.pass_dirs, self.fail_dirs = [], []
    for build_name in self.build_names:
      specs_for_build = specs_for_builder % {'builder': build_name}
      self.pass_dirs.append(
          os.path.join(specs_for_build, constants.BUILDER_STATUS_PASSED,
                       dir_pfx))
      self.fail_dirs.append(
          os.path.join(specs_for_build, constants.BUILDER_STATUS_FAILED,
                       dir_pfx))

    # Calculate the status of the latest build, and whether the build was
    # processed.
    if version is None:
      self.latest = self._LatestSpecFromDir(version_info, self.all_specs_dir)
      if self.latest is not None:
        latest_builds = None
        if self.db is not None:
          latest_builds = self.db.GetBuildHistory(
              self.build_names[0], 1, platform_version=self.latest)
        if not latest_builds:
          self.latest_unprocessed = self.latest
        else:
          self._latest_build = latest_builds[0]

    return True

  def GetBuildSpecFilePath(self, milestone, platform):
    """Get the file path given milestone and platform versions.

    Args:
      milestone: a string representing milestone, e.g. '44'
      platform: a string representing platform version, e.g. '7072.0.0-rc4'

    Returns:
      A string, representing the path to its spec file.
    """
    return os.path.join(self.buildspecs_dir, milestone, platform + '.xml')

  def GetCurrentVersionInfo(self):
    """Returns the current version info from the version file."""
    version_file_path = self.cros_source.GetRelativePath(constants.VERSION_FILE)
    return VersionInfo(version_file=version_file_path, incr_type=self.incr_type)

  def HasCheckoutBeenBuilt(self):
    """Checks to see if we've previously built this checkout."""
    if (self._latest_build and
        self._latest_build['status'] == constants.BUILDER_STATUS_PASSED):
      latest_spec_file = '%s.xml' % os.path.join(
          self.all_specs_dir, self.latest)
      # We've built this checkout before if the manifest isn't different than
      # the last one we've built.
      return not self.cros_source.IsManifestDifferent(latest_spec_file)
    else:
      # We've never built this manifest before so this checkout is always new.
      return False

  def CreateManifest(self):
    """Returns the path to a new manifest based on the current checkout."""
    new_manifest = tempfile.mkstemp('manifest_versions.manifest')[1]
    osutils.WriteFile(new_manifest,
                      self.cros_source.ExportManifest(mark_revision=True))
    return new_manifest

  def GetNextVersion(self, version_info):
    """Returns the next version string that should be built."""
    version = version_info.VersionString()
    if self.latest == version:
      message = ('Automatic: %s - Updating to a new version number from %s' %
                 (self.build_names[0], version))
      version = version_info.IncrementVersion()
      version_info.UpdateVersionFile(message, dry_run=self.dry_run)
      assert version != self.latest
      logging.info('Incremented version number to  %s', version)

    return version

  def PublishManifest(self, manifest, version, build_id=None):
    """Publishes the manifest as the manifest for the version to others.

    Args:
      manifest: Path to manifest file to publish.
      version: Manifest version string, e.g. 6102.0.0-rc4
      build_id: Optional integer giving build_id of the build that is
                publishing this manifest. If specified and non-negative,
                build_id will be included in the commit message.
    """
    # Note: This commit message is used by master.cfg for figuring out when to
    #       trigger slave builders.
    commit_message = 'Automatic: Start %s %s %s' % (self.build_names[0],
                                                    self.branch, version)
    if build_id is not None and build_id >= 0:
      commit_message += '\nCrOS-Build-Id: %s' % build_id

    logging.info('Publishing build spec for: %s', version)
    logging.info('Publishing with commit message: %s', commit_message)
    logging.debug('Manifest contents below.\n%s', osutils.ReadFile(manifest))

    # Copy the manifest into the manifest repository.
    spec_file = '%s.xml' % os.path.join(self.all_specs_dir, version)
    osutils.SafeMakedirs(os.path.dirname(spec_file))

    shutil.copyfile(manifest, spec_file)

    # Actually push the manifest.
    self.PushSpecChanges(commit_message)

  def DidLastBuildFail(self):
    """Returns True if the last build failed."""
    return (self._latest_build and
            self._latest_build['status'] == constants.BUILDER_STATUS_FAILED)

  def WaitForSlavesToComplete(self, master_build_id, db, builders_array,
                              pool=None, timeout=3 * 60,
                              ignore_timeout_exception=True):
    """Wait for all slaves to complete or timeout.

    This method checks the statuses of important builds in |builders_array|,
    waits for the builds to complete or timeout after given |timeout|. Builds
    marked as experimental through the tree status will not be considered
    in deciding whether to wait.

    Args:
      master_build_id: Master build id to check.
      db: An instance of cidb.CIDBConnection.
      builders_array: The name list of the build configs to check.
      pool: An instance of ValidationPool.validation_pool used by sync stage
            to apply changes.
      timeout: Number of seconds to wait for the results.
      ignore_timeout_exception: Whether to ignore when the timeout exception is
        raised in waiting. Default to True.
    """
    builders_array = buildbucket_lib.FetchCurrentSlaveBuilders(
        self.config, self.metadata, builders_array)
    logging.info('Waiting for the following builds to complete: %s',
                 builders_array)

    if not builders_array:
      return

    start_time = datetime.datetime.now()

    def _PrintRemainingTime(remaining):
      logging.info('%s until timeout...', remaining)

    slave_status = build_status.SlaveStatus(
        start_time, builders_array, master_build_id, db,
        config=self.config,
        metadata=self.metadata,
        buildbucket_client=self.buildbucket_client,
        version=self.current_version,
        pool=pool,
        dry_run=self.dry_run)

    try:
      timeout_util.WaitForSuccess(
          lambda x: slave_status.ShouldWait(),
          slave_status.UpdateSlaveStatus,
          timeout,
          period=self.SLEEP_TIMEOUT,
          side_effect_func=_PrintRemainingTime)
    except timeout_util.TimeoutError as e:
      logging.error('Not all builds finished before timeout (%d minutes)'
                    ' reached.', int((timeout / 60) + 0.5))

      if not ignore_timeout_exception:
        raise e

  def GetLatestPassingSpec(self):
    """Get the last spec file that passed in the current branch."""
    version_info = self.GetCurrentVersionInfo()
    return self._LatestSpecFromDir(version_info, self.pass_dirs[0])

  def GetLocalManifest(self, version=None):
    """Return path to local copy of manifest given by version.

    Returns:
      Path of |version|.  By default if version is not set, returns the path
      of the current version.
    """
    if not self.all_specs_dir:
      raise BuildSpecsValueError('GetLocalManifest failed, BuildSpecsManager '
                                 'instance not yet initialized by call to '
                                 'InitializeManifestVariables.')
    if version:
      return os.path.join(self.all_specs_dir, version + '.xml')
    elif self.current_version:
      return os.path.join(self.all_specs_dir, self.current_version + '.xml')

    return None

  def BootstrapFromVersion(self, version):
    """Initialize a manifest from a release version returning the path to it."""
    # Only refresh the manifest checkout if needed.
    if not self.InitializeManifestVariables(version=version):
      self.RefreshManifestCheckout()
      if not self.InitializeManifestVariables(version=version):
        raise BuildSpecsValueError('Failure in BootstrapFromVersion. '
                                   'InitializeManifestVariables failed after '
                                   'RefreshManifestCheckout for version '
                                   '%s.' % version)

    # Return the current manifest.
    self.current_version = version
    return self.GetLocalManifest(self.current_version)

  def CheckoutSourceCode(self):
    """Syncs the cros source to the latest git hashes for the branch."""
    self.cros_source.Sync(self.manifest)

  def GetNextBuildSpec(self, retries=NUM_RETRIES, build_id=None):
    """Returns a path to the next manifest to build.

    Args:
      retries: Number of retries for updating the status.
      build_id: Optional integer cidb id of this build, which will be used to
                annotate the manifest-version commit if one is created.

    Raises:
      GenerateBuildSpecException in case of failure to generate a buildspec
    """
    last_error = None
    for index in range(0, retries + 1):
      try:
        self.CheckoutSourceCode()

        version_info = self.GetCurrentVersionInfo()
        self.RefreshManifestCheckout()
        self.InitializeManifestVariables(version_info)

        if not self.force and self.HasCheckoutBeenBuilt():
          return None

        # If we're the master, always create a new build spec. Otherwise,
        # only create a new build spec if we've already built the existing
        # spec.
        if self.master or not self.latest_unprocessed:
          git.CreatePushBranch(PUSH_BRANCH, self.manifest_dir, sync=False)
          version = self.GetNextVersion(version_info)
          new_manifest = self.CreateManifest()
          self.PublishManifest(new_manifest, version, build_id=build_id)
        else:
          version = self.latest_unprocessed

        self.current_version = version
        return self.GetLocalManifest(version)
      except cros_build_lib.RunCommandError as e:
        last_error = 'Failed to generate buildspec. error: %s' % e
        logging.error(last_error)
        logging.error('Retrying to generate buildspec:  Retry %d/%d', index + 1,
                      retries)

    # Cleanse any failed local changes and throw an exception.
    self.RefreshManifestCheckout()
    raise GenerateBuildSpecException(last_error)

  def _SetPassSymlinks(self, success_map):
    """Marks the buildspec as passed by creating a symlink in passed dir.

    Args:
      success_map: Map of config names to whether they succeeded.
    """
    src_file = '%s.xml' % os.path.join(self.all_specs_dir, self.current_version)
    for i, build_name in enumerate(self.build_names):
      if success_map[build_name]:
        sym_dir = self.pass_dirs[i]
      else:
        sym_dir = self.fail_dirs[i]
      dest_file = '%s.xml' % os.path.join(sym_dir, self.current_version)
      status = builder_status_lib.BuilderStatus.GetCompletedStatus(
          success_map[build_name])
      logging.debug('Build %s: %s -> %s', status, src_file, dest_file)
      CreateSymlink(src_file, dest_file)

  def PushSpecChanges(self, commit_message):
    """Pushes any changes you have in the manifest directory."""
    _PushGitChanges(self.manifest_dir, commit_message, dry_run=self.dry_run)

  def UpdateStatus(self, success_map, message=None, retries=NUM_RETRIES):
    """Updates the status of the build for the current build spec.

    Args:
      success_map: Map of config names to whether they succeeded.
      message: Message accompanied with change in status.
      retries: Number of retries for updating the status
    """
    last_error = None
    if message:
      logging.info('Updating status with message %s', message)
    for index in range(0, retries + 1):
      try:
        self.RefreshManifestCheckout()
        git.CreatePushBranch(PUSH_BRANCH, self.manifest_dir, sync=False)
        success = all(success_map.values())
        commit_message = (
            'Automatic checkin: status=%s build_version %s for %s' %
            (builder_status_lib.BuilderStatus.GetCompletedStatus(success),
             self.current_version,
             self.build_names[0]))

        self._SetPassSymlinks(success_map)

        self.PushSpecChanges(commit_message)
        return
      except cros_build_lib.RunCommandError as e:
        last_error = ('Failed to update the status for %s during remote'
                      ' command: %s' % (self.build_names[0],
                                        e.message))
        logging.error(last_error)
        logging.error('Retrying to update the status:  Retry %d/%d', index + 1,
                      retries)

    # Cleanse any failed local changes and throw an exception.
    self.RefreshManifestCheckout()
    raise StatusUpdateException(last_error)


def _GetDefaultRemote(manifest_dom):
  """Returns the default remote in a manifest (if any).

  Args:
    manifest_dom: DOM Document object representing the manifest.

  Returns:
    Default remote if one exists, None otherwise.
  """
  default_nodes = manifest_dom.getElementsByTagName(DEFAULT_ELEMENT)
  if default_nodes:
    if len(default_nodes) > 1:
      raise FilterManifestException(
          'More than one <default> element found in manifest')
    return default_nodes[0].getAttribute(PROJECT_REMOTE_ATTR)
  return None


def _GetGroups(project_element):
  """Returns the default remote in a manifest (if any).

  Args:
    project_element: DOM Document object representing a project.

  Returns:
    List of names of the groups the project belongs too.
  """
  group = project_element.getAttribute(PROJECT_GROUP_ATTR)
  if not group:
    return []

  return [s.strip() for s in group.split(',')]


def FilterManifest(manifest, whitelisted_remotes=None, whitelisted_groups=None):
  """Returns a path to a new manifest with whitelists enforced.

  Args:
    manifest: Path to an existing manifest that should be filtered.
    whitelisted_remotes: Tuple of remotes to allow in the generated manifest.
      Only projects with those remotes will be included in the external
      manifest. (None means all remotes are acceptable)
    whitelisted_groups: Tuple of groups to allow in the generated manifest.
      (None means all groups are acceptable)

  Returns:
    Path to a new manifest that is a filtered copy of the original.
  """
  temp_fd, new_path = tempfile.mkstemp('external_manifest')
  manifest_dom = minidom.parse(manifest)
  manifest_node = manifest_dom.getElementsByTagName(MANIFEST_ELEMENT)[0]
  remotes = manifest_dom.getElementsByTagName(REMOTE_ELEMENT)
  projects = manifest_dom.getElementsByTagName(PROJECT_ELEMENT)
  pending_commits = manifest_dom.getElementsByTagName(PALADIN_COMMIT_ELEMENT)

  default_remote = _GetDefaultRemote(manifest_dom)

  # Remove remotes that don't match our whitelist.
  for remote_element in remotes:
    name = remote_element.getAttribute(REMOTE_NAME_ATTR)
    if (name is not None and
        whitelisted_remotes and
        name not in whitelisted_remotes):
      manifest_node.removeChild(remote_element)

  filtered_projects = set()
  for project_element in projects:
    project_remote = project_element.getAttribute(PROJECT_REMOTE_ATTR)
    project = project_element.getAttribute(PROJECT_NAME_ATTR)
    if not project_remote:
      if not default_remote:
        # This should not happen for a valid manifest. Either each
        # project must have a remote specified or there should
        # be manifest default we could use.
        raise FilterManifestException(
            'Project %s has unspecified remote with no default' % project)
      project_remote = default_remote

    groups = _GetGroups(project_element)

    filter_remote = (whitelisted_remotes and
                     project_remote not in whitelisted_remotes)

    filter_group = (whitelisted_groups and
                    not any([g in groups for g in whitelisted_groups]))

    if filter_remote or filter_group:
      filtered_projects.add(project)
      manifest_node.removeChild(project_element)

  for commit_element in pending_commits:
    if commit_element.getAttribute(
        PALADIN_PROJECT_ATTR) in filtered_projects:
      manifest_node.removeChild(commit_element)

  with os.fdopen(temp_fd, 'w') as manifest_file:
    # Filter out empty lines.
    filtered_manifest_noempty = filter(
        str.strip, manifest_dom.toxml('utf-8').splitlines())
    manifest_file.write(os.linesep.join(filtered_manifest_noempty))

  return new_path
