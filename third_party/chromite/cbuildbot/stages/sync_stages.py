# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the sync stages."""

from __future__ import print_function

import collections
import contextlib
import datetime
import itertools
import json
import os
import pprint
import re
import sys
import time
from xml.etree import ElementTree
from xml.dom import minidom

from chromite.cbuildbot import lkgm_manager
from chromite.cbuildbot import manifest_version
from chromite.cbuildbot import patch_series
from chromite.cbuildbot import repository
from chromite.cbuildbot import trybot_patch_pool
from chromite.cbuildbot import validation_pool
from chromite.cbuildbot.stages import generic_stages
from chromite.cbuildbot.stages import build_stages
from chromite.lib import buildbucket_lib
from chromite.lib import build_requests
from chromite.lib import clactions
from chromite.lib import clactions_metrics
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import cq_config
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import failures_lib
from chromite.lib import git
from chromite.lib import metrics
from chromite.lib import osutils
from chromite.lib import patch as cros_patch
from chromite.lib import timeout_util
from chromite.lib import tree_status
from chromite.scripts import cros_mark_android_as_stable
from chromite.scripts import cros_mark_chrome_as_stable


site_config = config_lib.GetConfig()


PRE_CQ = validation_pool.PRE_CQ

PRECQ_LAUNCH_TIMEOUT_MSG = (
    'We were not able to launch a %s trybot for your change within '
    '%s minutes.\n\n'
    'This problem can happen if the trybot waterfall is very '
    'busy, or if there is an infrastructure issue. Please '
    'notify the sheriff and mark your change as ready again. If '
    'this problem occurs multiple times in a row, please file a '
    'bug.')
PRECQ_INFLIGHT_TIMEOUT_MSG = (
    'The %s trybot for your change timed out after %s minutes.'
    '\n\n'
    'This problem can happen if your change causes the builder '
    'to hang, or if there is some infrastructure issue. If your '
    'change is not at fault you may mark your change as ready '
    'again. If this problem occurs multiple times please notify '
    'the sheriff and file a bug.')
PRECQ_EXPIRY_MSG = (
    'The pre-cq verification for this change expired after %s minutes. No '
    'action is required on your part.'
    '\n\n'
    'In order to protect the CQ from picking up stale changes, the pre-cq '
    'status for changes are cleared after a generous timeout. This change '
    'will be re-tested by the pre-cq before the CQ picks it up.')


# Default limit for the size of Pre-CQ configs to test for unioned options
# TODO(nxia): make this configurable in the COMMIT-QUEUE.ini
DEFAULT_UNION_PRE_CQ_LIMIT = 15


class ExceedUnionPreCQLimitException(Exception):
  """Exception raised when unioned Pre-CQ config size exceeds the limit."""

  def __init__(self, pre_cq_configs, limit, message=''):
    """Initialize a ExceedUnionPreCQLimitException.

    Args:
      pre_cq_configs: A set of Pre-CQ configs (strings) which exceeds the limit.
      limit: The limit for the size of the Pre-CQ configs.
      message: An error message (optional).
    """
    Exception.__init__(self, message)
    self.pre_cq_configs = pre_cq_configs
    self.limit = limit


class UnknownPreCQConfigRequestedError(Exception):
  """Raised when a config file asked for a config that doesn't exist."""

  def __init__(self, pre_cq_configs):
    super(UnknownPreCQConfigRequestedError, self).__init__(
        'One of the requested pre-cq configs is invalid or nonexistant: %s'
        % pre_cq_configs)
    self.pre_cq_configs = pre_cq_configs


class PatchChangesStage(generic_stages.BuilderStage):
  """Stage that patches a set of Gerrit changes to the buildroot source tree."""

  def __init__(self, builder_run, patch_pool, **kwargs):
    """Construct a PatchChangesStage.

    Args:
      builder_run: BuilderRun object.
      patch_pool: A TrybotPatchPool object containing the different types of
                  patches to apply.
    """
    super(PatchChangesStage, self).__init__(builder_run, **kwargs)
    self.patch_pool = patch_pool

  @staticmethod
  def _CheckForDuplicatePatches(_series, changes):
    conflicts = {}
    duplicates = []
    for change in changes:
      if change.id is None:
        logging.warning(
            "Change %s lacks a usable ChangeId; duplicate checking cannot "
            "be done for this change.  If cherry-picking fails, this is a "
            "potential cause.", change)
        continue
      conflicts.setdefault(change.id, []).append(change)

    duplicates = [x for x in conflicts.itervalues() if len(x) > 1]
    if not duplicates:
      return changes

    for conflict in duplicates:
      logging.error(
          "Changes %s conflict with each other- they have same id %s., "
          .join(map(str, conflict)), conflict[0].id)

    cros_build_lib.Die("Duplicate patches were encountered: %s", duplicates)

  def _PatchSeriesFilter(self, series, changes):
    return self._CheckForDuplicatePatches(series, changes)

  def _ApplyPatchSeries(self, series, patch_pool, **kwargs):
    """Applies a patch pool using a patch series."""
    kwargs.setdefault('frozen', False)
    # Honor the given ordering, so that if a gerrit/remote patch
    # conflicts w/ a local patch, the gerrit/remote patch are
    # blamed rather than local (patch ordering is typically
    # local, gerrit, then remote).
    kwargs.setdefault('honor_ordering', True)
    kwargs['changes_filter'] = self._PatchSeriesFilter

    _applied, failed_tot, failed_inflight = series.Apply(
        list(patch_pool), **kwargs)

    failures = failed_tot + failed_inflight
    if failures:
      self.HandleApplyFailures(failures)

  def HandleApplyFailures(self, failures):
    cros_build_lib.Die("Failed applying patches: %s",
                       "\n".join(map(str, failures)))

  def PerformStage(self):
    class NoisyPatchSeries(patch_series.PatchSeries):
      """Custom PatchSeries that adds links to buildbot logs for remote trys."""

      def ApplyChange(self, change):
        if isinstance(change, cros_patch.GerritPatch):
          logging.PrintBuildbotLink(str(change), change.url)
        elif isinstance(change, cros_patch.UploadedLocalPatch):
          logging.PrintBuildbotStepText(str(change))

        return patch_series.PatchSeries.ApplyChange(self, change)

    # If we're an external builder, ignore internal patches.
    helper_pool = patch_series.HelperPool.SimpleCreate(
        cros_internal=self._run.config.internal, cros=True)

    # Limit our resolution to non-manifest patches.
    patches = NoisyPatchSeries(
        self._build_root,
        helper_pool=helper_pool,
        deps_filter_fn=lambda p: not trybot_patch_pool.ManifestFilter(p))

    self._ApplyPatchSeries(patches, self.patch_pool)


class BootstrapStage(PatchChangesStage):
  """Stage that patches a chromite repo and re-executes inside it.

  Attributes:
    returncode - the returncode of the cbuildbot re-execution.  Valid after
                 calling stage.Run().
  """
  option_name = 'bootstrap'

  def __init__(self, builder_run, patch_pool, **kwargs):
    super(BootstrapStage, self).__init__(
        builder_run, trybot_patch_pool.TrybotPatchPool(), **kwargs)

    self.patch_pool = patch_pool
    self.returncode = None
    self.tempdir = None

  def _ApplyManifestPatches(self, patch_pool):
    """Apply a pool of manifest patches to a temp manifest checkout.

    Args:
      patch_pool: The pool to apply.

    Returns:
      The path to the patched manifest checkout.

    Raises:
      Exception, if the new patched manifest cannot be parsed.
    """
    checkout_dir = os.path.join(self.tempdir, 'manfest-checkout')
    repository.CloneGitRepo(checkout_dir,
                            self._run.config.manifest_repo_url)

    patches = patch_series.PatchSeries.WorkOnSingleRepo(
        checkout_dir, tracking_branch=self._run.manifest_branch)
    self._ApplyPatchSeries(patches, patch_pool)

    # Verify that the patched manifest loads properly. Propagate any errors as
    # exceptions.
    manifest = os.path.join(checkout_dir, self._run.config.manifest)
    git.Manifest.Cached(manifest, manifest_include_dir=checkout_dir)
    return checkout_dir

  @staticmethod
  def _FilterArgsForApi(parsed_args, api_minor):
    """Remove arguments that are introduced after an api version."""
    def filter_fn(passed_arg):
      return passed_arg.opt_inst.api_version <= api_minor

    accepted, removed = commandline.FilteringParser.FilterArgs(
        parsed_args, filter_fn)

    if removed:
      logging.warning("The following arguments were removed due to api: '%s'"
                      % ' '.join(removed))
    return accepted

  @classmethod
  def FilterArgsForTargetCbuildbot(cls, buildroot, cbuildbot_path, options):
    _, minor = cros_build_lib.GetTargetChromiteApiVersion(buildroot)
    args = [cbuildbot_path]
    args.append(options.build_config_name)
    args.extend(cls._FilterArgsForApi(options.parsed_args, minor))

    # Only pass down --cache-dir if it was specified. By default, we want
    # the cache dir to live in the root of each checkout, so this means that
    # each instance of cbuildbot needs to calculate the default separately.
    if minor >= 2 and options.cache_dir_specified:
      args += ['--cache-dir', options.cache_dir]

    if minor >= constants.REEXEC_API_TSMON_TASK_NUM:
      # Increment the ts-mon task_num so the metrics don't collide.
      args.extend(['--ts-mon-task-num', str(options.ts_mon_task_num + 1)])

    return args

  @classmethod
  def BootstrapPatchesNeeded(cls, builder_run, patch_pool):
    """See if bootstrapping is needed for any of the given patches.

    Does NOT determine if they have already been applied.

    Args:
      builder_run: BuilderRun object for this build.
      patch_pool: All patches to be applied this run.

    Returns:
      boolean True if bootstrapping is needed.
    """
    chromite_pool = patch_pool.Filter(project=constants.CHROMITE_PROJECT)
    if builder_run.config.internal:
      manifest_pool = patch_pool.FilterIntManifest()
    else:
      manifest_pool = patch_pool.FilterExtManifest()

    return bool(chromite_pool or manifest_pool)

  def HandleApplyFailures(self, failures):
    """Handle the case where patches fail to apply."""
    if self._run.config.pre_cq:
      # Let the PreCQSync stage handle this failure. The PreCQSync stage will
      # comment on CLs with the appropriate message when they fail to apply.
      #
      # WARNING: For manifest patches, the Pre-CQ attempts to apply external
      # patches to the internal manifest, and this means we may flag a conflict
      # here even if the patch applies cleanly. TODO(davidjames): Fix this.
      logging.PrintBuildbotStepWarnings()
      logging.error('Failed applying patches: %s\n'.join(map(str, failures)))
    else:
      PatchChangesStage.HandleApplyFailures(self, failures)

  def _PerformStageInTempDir(self):
    # The plan for the builders is to use master branch to bootstrap other
    # branches. Now, if we wanted to test patches for both the bootstrap code
    # (on master) and the branched chromite (say, R20), we need to filter the
    # patches by branch.
    filter_branch = self._run.manifest_branch
    if self._run.options.test_bootstrap:
      filter_branch = 'master'

    # Filter all requested patches for the branch.
    branch_pool = self.patch_pool.FilterBranch(filter_branch)

    # Checkout the new version of chromite, and patch it.
    chromite_dir = os.path.join(self.tempdir, 'chromite')
    reference_repo = os.path.join(constants.CHROMITE_DIR, '.git')
    repository.CloneGitRepo(chromite_dir, constants.CHROMITE_URL,
                            reference=reference_repo)
    git.RunGit(chromite_dir, ['checkout', filter_branch])

    chromite_pool = branch_pool.Filter(project=constants.CHROMITE_PROJECT)
    if chromite_pool:
      patches = patch_series.PatchSeries.WorkOnSingleRepo(
          chromite_dir, filter_branch)
      self._ApplyPatchSeries(patches, chromite_pool)

    # Re-exec into new instance of cbuildbot, with proper command line args.
    cbuildbot_path = constants.PATH_TO_CBUILDBOT
    if not os.path.exists(os.path.join(self.tempdir, cbuildbot_path)):
      cbuildbot_path = 'chromite/cbuildbot/cbuildbot'
    cmd = self.FilterArgsForTargetCbuildbot(self.tempdir, cbuildbot_path,
                                            self._run.options)

    extra_params = ['--sourceroot', self._run.options.sourceroot]
    extra_params.extend(self._run.options.bootstrap_args)
    if self._run.options.test_bootstrap:
      # We don't want re-executed instance to see this.
      cmd = [a for a in cmd if a != '--test-bootstrap']
    else:
      # If we've already done the desired number of bootstraps, disable
      # bootstrapping for the next execution.  Also pass in the patched manifest
      # repository.
      extra_params.append('--nobootstrap')
      if self._run.config.internal:
        manifest_pool = branch_pool.FilterIntManifest()
      else:
        manifest_pool = branch_pool.FilterExtManifest()

      if manifest_pool:
        manifest_dir = self._ApplyManifestPatches(manifest_pool)
        extra_params.extend(['--manifest-repo-url', manifest_dir])

    cmd += extra_params
    result_obj = cros_build_lib.RunCommand(
        cmd, cwd=self.tempdir, kill_timeout=30, error_code_ok=True)
    self.returncode = result_obj.returncode

  def PerformStage(self):
    with osutils.TempDir(base_dir=self._run.options.bootstrap_dir) as tempdir:
      self.tempdir = tempdir
      self._PerformStageInTempDir()
    self.tempdir = None


class SyncStage(generic_stages.BuilderStage):
  """Stage that performs syncing for the builder."""

  option_name = 'sync'
  output_manifest_sha1 = True

  def __init__(self, builder_run, **kwargs):
    super(SyncStage, self).__init__(builder_run, **kwargs)
    self.repo = None
    self.skip_sync = False

    # TODO(mtennant): Why keep a duplicate copy of this config value
    # at self.internal when it can always be retrieved from config?
    self.internal = self._run.config.internal
    self.buildbucket_client = self.GetBuildbucketClient()

  def _GetManifestVersionsRepoUrl(self, internal=None, test=False):
    if internal is None:
      internal = self._run.config.internal

    if internal:
      if test:
        return site_config.params.MANIFEST_VERSIONS_INT_GOB_URL_TEST
      else:
        return site_config.params.MANIFEST_VERSIONS_INT_GOB_URL
    else:
      if test:
        return site_config.params.MANIFEST_VERSIONS_GOB_URL_TEST
      else:
        return site_config.params.MANIFEST_VERSIONS_GOB_URL

  def Initialize(self):
    self._InitializeRepo()

  def _InitializeRepo(self):
    """Set up the RepoRepository object."""
    self.repo = self.GetRepoRepository()

  def GetNextManifest(self):
    """Returns the manifest to use."""
    return self._run.config.manifest

  def ManifestCheckout(self, next_manifest):
    """Checks out the repository to the given manifest."""
    self._Print('\n'.join(['BUILDROOT: %s' % self.repo.directory,
                           'TRACKING BRANCH: %s' % self.repo.branch,
                           'NEXT MANIFEST: %s' % next_manifest]))

    if not self.skip_sync:
      self.repo.Sync(next_manifest)

    print(self.repo.ExportManifest(mark_revision=self.output_manifest_sha1),
          file=sys.stderr)

  def RunPrePatchBuild(self):
    """Run through a pre-patch build to prepare for incremental build.

    This function runs though the InitSDKStage, SetupBoardStage, and
    BuildPackagesStage. It is intended to be called before applying
    any patches under test, to prepare the chroot and sysroot in a state
    corresponding to ToT prior to an incremental build.

    Returns:
      True if all stages were successful, False if any of them failed.
    """
    suffix = ' (pre-Patch)'
    try:
      build_stages.InitSDKStage(
          self._run, chroot_replace=True, suffix=suffix).Run()
      for builder_run in self._run.GetUngroupedBuilderRuns():
        for board in builder_run.config.boards:
          build_stages.SetupBoardStage(
              builder_run, board=board, suffix=suffix).Run()
          build_stages.BuildPackagesStage(
              builder_run, board=board, suffix=suffix).Run()
    except failures_lib.StepFailure:
      return False

    return True

  def WriteChangesToMetadata(self, changes):
    """Write the changes under test into the metadata.

    Args:
      changes: A list of GerritPatch instances.
    """
    changes_list = self._run.attrs.metadata.GetDict().get('changes', [])
    changes_list = changes_list + [c.GetAttributeDict() for c in set(changes)]
    changes_list = sorted(changes_list,
                          key=lambda x: (x[cros_patch.ATTR_GERRIT_NUMBER],
                                         x[cros_patch.ATTR_PATCH_NUMBER],
                                         x[cros_patch.ATTR_REMOTE]))
    self._run.attrs.metadata.UpdateWithDict({'changes': changes_list})
    change_ids = []
    change_gerrit_ids = []
    change_gerrit_numbers = []
    for c in changes_list:
      change_ids.append(c[cros_patch.ATTR_CHANGE_ID])
      gerrit_number = c[cros_patch.ATTR_GERRIT_NUMBER]
      gerrit_id = '/'.join([c[cros_patch.ATTR_REMOTE], gerrit_number,
                            c[cros_patch.ATTR_PATCH_NUMBER]])
      change_gerrit_ids.append(gerrit_id)
      change_gerrit_numbers.append(gerrit_number)
    tags = {
        'change_ids': change_ids,
        'change_gerrit_ids': change_gerrit_ids,
        'change_gerrit_numbers': change_gerrit_numbers,
    }
    self._run.attrs.metadata.UpdateKeyDictWithDict(constants.METADATA_TAGS,
                                                   tags)

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    self.Initialize()
    with osutils.TempDir() as tempdir:
      # Save off the last manifest.
      fresh_sync = True
      if os.path.exists(self.repo.directory) and not self._run.options.clobber:
        old_filename = os.path.join(tempdir, 'old.xml')
        try:
          old_contents = self.repo.ExportManifest()
        except cros_build_lib.RunCommandError as e:
          logging.warning(str(e))
        else:
          osutils.WriteFile(old_filename, old_contents)
          fresh_sync = False

      # Sync.
      self.ManifestCheckout(self.GetNextManifest())

      # Print the blamelist.
      if fresh_sync:
        logging.PrintBuildbotStepText('(From scratch)')
      elif self._run.options.buildbot:
        lkgm_manager.GenerateBlameList(self.repo, old_filename)

      # Incremental builds request an additional build before patching changes.
      if self._run.config.build_before_patching:
        pre_build_passed = self.RunPrePatchBuild()
        if not pre_build_passed:
          logging.PrintBuildbotStepText('Pre-patch build failed.')


class LKGMSyncStage(SyncStage):
  """Stage that syncs to the last known good manifest blessed by builders."""

  output_manifest_sha1 = False

  def GetNextManifest(self):
    """Override: Gets the LKGM."""
    # TODO(sosa):  Should really use an initialized manager here.
    if self.internal:
      mv_dir = site_config.params.INTERNAL_MANIFEST_VERSIONS_PATH
    else:
      mv_dir = site_config.params.EXTERNAL_MANIFEST_VERSIONS_PATH

    manifest_path = os.path.join(self._build_root, mv_dir)
    manifest_repo = self._GetManifestVersionsRepoUrl()
    manifest_version.RefreshManifestCheckout(manifest_path, manifest_repo)
    return os.path.join(manifest_path, self._run.config.lkgm_manifest)


class ManifestVersionedSyncStage(SyncStage):
  """Stage that generates a unique manifest file, and sync's to it."""

  # TODO(mtennant): Make this into a builder run value.
  output_manifest_sha1 = False

  def __init__(self, builder_run, **kwargs):
    # Perform the sync at the end of the stage to the given manifest.
    super(ManifestVersionedSyncStage, self).__init__(builder_run, **kwargs)
    self.repo = None
    self.manifest_manager = None

    # If a builder pushes changes (even with dryrun mode), we need a writable
    # repository. Otherwise, the push will be rejected by the server.
    self.manifest_repo = self._GetManifestVersionsRepoUrl()

    # 1. Our current logic for calculating whether to re-run a build assumes
    #    that if the build is green, then it doesn't need to be re-run. This
    #    isn't true for canary masters, because the canary master ignores the
    #    status of its slaves and is green even if they fail. So set
    #    force=True in this case.
    # 2. If we're running with --debug, we should always run through to
    #    completion, so as to ensure a complete test.
    self._force = self._run.config.master or self._run.options.debug

  def HandleSkip(self):
    """Initializes a manifest manager to the specified version if skipped."""
    super(ManifestVersionedSyncStage, self).HandleSkip()
    if self._run.options.force_version:
      self.Initialize()
      self.ForceVersion(self._run.options.force_version)

  def ForceVersion(self, version):
    """Creates a manifest manager from given version and returns manifest."""
    logging.PrintBuildbotStepText(version)
    return self.manifest_manager.BootstrapFromVersion(version)

  def VersionIncrementType(self):
    """Return which part of the version number should be incremented."""
    if self._run.manifest_branch == 'master':
      return 'build'

    return 'branch'

  def RegisterManifestManager(self, manifest_manager):
    """Save the given manifest manager for later use in this run.

    Args:
      manifest_manager: Expected to be a BuildSpecsManager.
    """
    self._run.attrs.manifest_manager = self.manifest_manager = manifest_manager

  def Initialize(self):
    """Initializes a manager that manages manifests for associated stages."""

    dry_run = self._run.options.debug

    self._InitializeRepo()

    # If chrome_rev is somehow set, fail.
    assert not self._chrome_rev, \
        'chrome_rev is unsupported on release builders.'

    _, db = self._run.GetCIDBHandle()
    self.RegisterManifestManager(manifest_version.BuildSpecsManager(
        source_repo=self.repo,
        manifest_repo=self.manifest_repo,
        manifest=self._run.config.manifest,
        build_names=self._run.GetBuilderIds(),
        incr_type=self.VersionIncrementType(),
        force=self._force,
        branch=self._run.manifest_branch,
        dry_run=dry_run,
        config=self._run.config,
        metadata=self._run.attrs.metadata,
        db=db,
        buildbucket_client=self.buildbucket_client))

  def _SetAndroidVersionIfApplicable(self, manifest):
    """If 'android' is in |manifest|, write version to the BuilderRun object.

    Args:
      manifest: Path to the manifest.
    """
    manifest_dom = minidom.parse(manifest)
    elements = manifest_dom.getElementsByTagName(lkgm_manager.ANDROID_ELEMENT)

    if elements:
      android_version = elements[0].getAttribute(
          lkgm_manager.ANDROID_VERSION_ATTR)
      logging.info(
          'Android version was found in the manifest: %s', android_version)
      # Update the metadata dictionary. This is necessary because the
      # metadata dictionary is preserved through re-executions, so
      # UprevAndroidStage can read the version from the dictionary
      # later. This is easier than parsing the manifest again after
      # the re-execution.
      self._run.attrs.metadata.UpdateKeyDictWithDict(
          'version', {'android': android_version})

  def _SetChromeVersionIfApplicable(self, manifest):
    """If 'chrome' is in |manifest|, write the version to the BuilderRun object.

    Args:
      manifest: Path to the manifest.
    """
    manifest_dom = minidom.parse(manifest)
    elements = manifest_dom.getElementsByTagName(lkgm_manager.CHROME_ELEMENT)

    if elements:
      chrome_version = elements[0].getAttribute(
          lkgm_manager.CHROME_VERSION_ATTR)
      logging.info(
          'Chrome version was found in the manifest: %s', chrome_version)
      # Update the metadata dictionary. This is necessary because the
      # metadata dictionary is preserved through re-executions, so
      # SyncChromeStage can read the version from the dictionary
      # later. This is easier than parsing the manifest again after
      # the re-execution.
      self._run.attrs.metadata.UpdateKeyDictWithDict(
          'version', {'chrome': chrome_version})

  def GetNextManifest(self):
    """Uses the initialized manifest manager to get the next manifest."""
    assert self.manifest_manager, \
        'Must run GetStageManager before checkout out build.'

    build_id = self._run.attrs.metadata.GetDict().get('build_id')

    to_return = self.manifest_manager.GetNextBuildSpec(build_id=build_id)
    previous_version = self.manifest_manager.GetLatestPassingSpec()
    target_version = self.manifest_manager.current_version

    # Print the Blamelist here.
    url_prefix = 'https://crosland.corp.google.com/log/'
    url = url_prefix + '%s..%s' % (previous_version, target_version)
    logging.PrintBuildbotLink('Blamelist', url)
    # The testManifestVersionedSyncOnePartBranch interacts badly with this
    # function.  It doesn't fully initialize self.manifest_manager which
    # causes target_version to be None.  Since there isn't a clean fix in
    # either direction, just throw this through str().  In the normal case,
    # it's already a string anyways.
    logging.PrintBuildbotStepText(str(target_version))

    return to_return

  @contextlib.contextmanager
  def LocalizeManifest(self, manifest, filter_cros=False):
    """Remove restricted checkouts from the manifest if needed.

    Args:
      manifest: The manifest to localize.
      filter_cros: If set, then only checkouts with a remote of 'cros' or
        'cros-internal' are kept, and the rest are filtered out.
    """
    if filter_cros:
      with osutils.TempDir() as tempdir:
        filtered_manifest = os.path.join(tempdir, 'filtered.xml')
        doc = ElementTree.parse(manifest)
        root = doc.getroot()
        for node in root.findall('project'):
          remote = node.attrib.get('remote')
          if remote and remote not in site_config.params.GIT_REMOTES:
            root.remove(node)
        doc.write(filtered_manifest)
        yield filtered_manifest
    else:
      yield manifest

  def _GetMasterVersion(self, master_id, timeout=5 * 60):
    """Get the platform version associated with the master_build_id.

    Args:
      master_id: Our master build id.
      timeout: How long to wait for the platform version to show up
        in the database. This is needed because the slave builders are
        triggered slightly before the platform version is written. Default
        is 5 minutes.
    """
    # TODO(davidjames): Remove the wait loop here once we've updated slave
    # builders to only get triggered after the platform version is written.
    def _PrintRemainingTime(remaining):
      logging.info('%s until timeout...', remaining)

    def _GetPlatformVersion():
      return db.GetBuildStatus(master_id)['platform_version']

    # Retry until non-None version is returned.
    def _ShouldRetry(x):
      return not x

    _, db = self._run.GetCIDBHandle()
    return timeout_util.WaitForSuccess(_ShouldRetry,
                                       _GetPlatformVersion,
                                       timeout,
                                       period=constants.SLEEP_TIMEOUT,
                                       side_effect_func=_PrintRemainingTime)

  def _VerifyMasterId(self, master_id):
    """Verify that our master id is current and valid.

    Args:
      master_id: Our master build id.
    """
    _, db = self._run.GetCIDBHandle()
    if db and master_id:
      assert not self._run.options.force_version
      master_build_status = db.GetBuildStatus(master_id)
      latest = db.GetBuildHistory(
          master_build_status['build_config'], 1,
          milestone_version=master_build_status['milestone_version'])
      if latest and latest[0]['id'] != master_id:
        raise failures_lib.MasterSlaveVersionMismatchFailure(
            'This slave\'s master (id=%s) has been supplanted by a newer '
            'master (id=%s). Aborting.' % (master_id, latest[0]['id']))

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    self.Initialize()

    self._VerifyMasterId(self._run.options.master_build_id)
    version = self._run.options.force_version
    if self._run.options.master_build_id:
      version = self._GetMasterVersion(self._run.options.master_build_id)

    next_manifest = None
    if version:
      next_manifest = self.ForceVersion(version)
    else:
      self.skip_sync = True
      try:
        next_manifest = self.GetNextManifest()
      except validation_pool.TreeIsClosedException as e:
        logging.warning(str(e))

    if not next_manifest:
      logging.info('Found no work to do.')
      if self._run.attrs.manifest_manager.DidLastBuildFail():
        raise failures_lib.StepFailure('The previous build failed.')
      else:
        raise failures_lib.ExitEarlyException(
            'ManifestVersionedSyncStage finished and exited early.')

    # Log this early on for the release team to grep out before we finish.
    if self.manifest_manager:
      self._Print('\nRELEASETAG: %s\n' % (
          self.manifest_manager.current_version))

    self._SetAndroidVersionIfApplicable(next_manifest)
    self._SetChromeVersionIfApplicable(next_manifest)
    # To keep local trybots working, remove restricted checkouts from the
    # official manifest we get from manifest-versions.
    with self.LocalizeManifest(
        next_manifest, filter_cros=self._run.options.local) as new_manifest:
      self.ManifestCheckout(new_manifest)


class MasterSlaveLKGMSyncStage(ManifestVersionedSyncStage):
  """Stage that generates a unique manifest file candidate, and sync's to it.

  This stage uses an LKGM manifest manager that handles LKGM
  candidates and their states.
  """
  # If we are using an internal manifest, but need to be able to create an
  # external manifest, we create a second manager for that manifest.
  external_manager = None
  MAX_BUILD_HISTORY_LENGTH = 10
  MilestoneVersion = collections.namedtuple(
      'MilestoneVersion', ['milestone', 'platform'])

  def __init__(self, builder_run, **kwargs):
    super(MasterSlaveLKGMSyncStage, self).__init__(builder_run, **kwargs)
    # lkgm_manager deals with making sure we're synced to whatever manifest
    # we get back in GetNextManifest so syncing again is redundant.
    self._android_version = None
    self._chrome_version = None

  def _GetInitializedManager(self, internal):
    """Returns an initialized lkgm manager.

    Args:
      internal: Boolean.  True if this is using an internal manifest.

    Returns:
      lkgm_manager.LKGMManager.
    """
    increment = self.VersionIncrementType()
    return lkgm_manager.LKGMManager(
        source_repo=self.repo,
        manifest_repo=self._GetManifestVersionsRepoUrl(internal=internal),
        manifest=self._run.config.manifest,
        build_names=self._run.GetBuilderIds(),
        build_type=self._run.config.build_type,
        incr_type=increment,
        force=self._force,
        branch=self._run.manifest_branch,
        dry_run=self._run.options.debug,
        config=self._run.config,
        metadata=self._run.attrs.metadata,
        buildbucket_client=self.buildbucket_client)

  def Initialize(self):
    """Override: Creates an LKGMManager rather than a ManifestManager."""
    self._InitializeRepo()
    self.RegisterManifestManager(self._GetInitializedManager(self.internal))
    if self._run.config.master and self._GetSlaveConfigs():
      assert self.internal, 'Unified masters must use an internal checkout.'
      MasterSlaveLKGMSyncStage.external_manager = \
          self._GetInitializedManager(False)

  def ForceVersion(self, version):
    manifest = super(MasterSlaveLKGMSyncStage, self).ForceVersion(version)
    if MasterSlaveLKGMSyncStage.external_manager:
      MasterSlaveLKGMSyncStage.external_manager.BootstrapFromVersion(version)

    return manifest

  def _VerifyMasterId(self, master_id):
    """Verify that our master id is current and valid."""
    super(MasterSlaveLKGMSyncStage, self)._VerifyMasterId(master_id)
    if not self._run.config.master and not master_id:
      raise failures_lib.StepFailure(
          'Cannot start build without a master_build_id. Did you hit force '
          'build on a slave? Please hit force build on the master instead.')

  def GetNextManifest(self):
    """Gets the next manifest using LKGM logic."""
    assert self.manifest_manager, \
        'Must run Initialize before we can get a manifest.'
    assert isinstance(self.manifest_manager, lkgm_manager.LKGMManager), \
        'Manifest manager instantiated with wrong class.'
    assert self._run.config.master

    build_id = self._run.attrs.metadata.GetDict().get('build_id')
    logging.info('Creating new candidate manifest, including chrome version '
                 '%s.', self._chrome_version)
    if self._android_version:
      logging.info('Adding Android version to new candidate manifest %s.',
                   self._android_version)
    manifest = self.manifest_manager.CreateNewCandidate(
        android_version=self._android_version,
        chrome_version=self._chrome_version,
        build_id=build_id)
    if MasterSlaveLKGMSyncStage.external_manager:
      MasterSlaveLKGMSyncStage.external_manager.CreateFromManifest(
          manifest, build_id=build_id)

    return manifest

  def GetLatestAndroidVersion(self):
    """Returns the version of Android to uprev."""
    return cros_mark_android_as_stable.GetLatestBuild(
        constants.ANDROID_BUCKET_URL, self._run.config.android_import_branch,
        cros_mark_android_as_stable.MakeBuildTargetDict(
            self._run.config.android_import_branch))[0]

  def GetLatestChromeVersion(self):
    """Returns the version of Chrome to uprev."""
    return cros_mark_chrome_as_stable.GetLatestRelease(
        constants.CHROMIUM_GOB_URL)

  def GetLastChromeOSVersion(self):
    """Fetching ChromeOS version from the last run.

    Fetching the chromeos version from the last run that published a manifest
    by querying CIDB. Master builds that failed before publishing a manifest
    will be ignored.

    Returns:
      A namedtuple MilestoneVersion,
      e.g. MilestoneVersion(milestone='44', platform='7072.0.0-rc4')
      or None if failed to retrieve milestone and platform versions.
    """
    build_id, db = self._run.GetCIDBHandle()

    if db is None:
      return None

    builds = db.GetBuildHistory(
        build_config=self._run.config.name,
        num_results=self.MAX_BUILD_HISTORY_LENGTH,
        ignore_build_id=build_id)
    full_versions = [b.get('full_version') for b in builds]
    old_version = next(itertools.ifilter(bool, full_versions), None)
    if old_version:
      pattern = r'^R(\d+)-(\d+.\d+.\d+(-rc\d+)*)'
      m = re.match(pattern, old_version)
      if m:
        milestone = m.group(1)
        platform = m.group(2)
      return self.MilestoneVersion(
          milestone=milestone, platform=platform)
    return None

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    """Performs the stage."""
    if self._android_rev and self._run.config.master:
      self._android_version = self.GetLatestAndroidVersion()
      logging.info('Latest Android version is: %s', self._android_version)

    if (self._chrome_rev == constants.CHROME_REV_LATEST and
        self._run.config.master):
      # PFQ master needs to determine what version of Chrome to build
      # for all slaves.
      logging.info('I am a master running with CHROME_REV_LATEST, '
                   'therefore getting latest chrome version.')
      self._chrome_version = self.GetLatestChromeVersion()
      logging.info('Latest chrome version is: %s', self._chrome_version)

    ManifestVersionedSyncStage.PerformStage(self)

    # Generate blamelist
    cros_version = self.GetLastChromeOSVersion()
    if cros_version:
      old_filename = self.manifest_manager.GetBuildSpecFilePath(
          cros_version.milestone, cros_version.platform)
      if not os.path.exists(old_filename):
        logging.error('Could not generate blamelist, '
                      'manifest file does not exist: %s', old_filename)
      else:
        logging.debug('Generate blamelist against: %s', old_filename)
        lkgm_manager.GenerateBlameList(self.repo, old_filename)

class CommitQueueSyncStage(MasterSlaveLKGMSyncStage):
  """Commit Queue Sync stage that handles syncing and applying patches.

  Similar to the MasterSlaveLKGMsync Stage, this stage handles syncing
  to a manifest, passing around that manifest to other builders.

  What makes this stage different is that the CQ master finds the
  patches on Gerrit which are ready to be committed, apply them, and
  includes the patches in the new manifest. The slaves sync to the
  manifest, and apply the patches written in the manifest.
  """

  # The amount of time we wait before assuming that the Pre-CQ is down and
  # that we should start testing changes that haven't been tested by the Pre-CQ.
  PRE_CQ_TIMEOUT = 2 * 60 * 60

  def __init__(self, builder_run, **kwargs):
    super(CommitQueueSyncStage, self).__init__(builder_run, **kwargs)

    # The pool of patches to be picked up by the commit queue.
    # - For the master commit queue, it's initialized in GetNextManifest.
    # - For slave commit queues, it's initialized in _SetPoolFromManifest.
    #
    # In all cases, the pool is saved to disk.
    self.pool = None

  def HandleSkip(self):
    """Handles skip and initializes validation pool from manifest."""
    super(CommitQueueSyncStage, self).HandleSkip()
    filename = self._run.options.validation_pool
    if filename:
      self.pool = validation_pool.ValidationPool.Load(
          filename, builder_run=self._run)
    else:
      self._SetPoolFromManifest(self.manifest_manager.GetLocalManifest())

  def _ChangeFilter(self, _pool, changes, non_manifest_changes):
    # First, look for changes that were tested by the Pre-CQ.
    changes_to_test = []

    _, db = self._run.GetCIDBHandle()
    if db:
      actions_for_changes = db.GetActionsForChanges(changes)
      for change in changes:
        status = clactions.GetCLPreCQStatus(change, actions_for_changes)
        if status == constants.CL_STATUS_PASSED:
          changes_to_test.append(change)
    else:
      logging.warning("DB not available, unable to filter for PreCQ passed.")

    # Allow Commit-Ready=+2 changes to bypass the Pre-CQ, if there are no other
    # changes.
    if not changes_to_test:
      changes_to_test = [x for x in changes if x.HasApproval('COMR', '2')]

    # If we only see changes that weren't verified by Pre-CQ, and some of them
    # are really old changes, try all of the changes. This ensures that the CQ
    # continues to work (albeit slowly) even if the Pre-CQ is down.
    if changes and not changes_to_test:
      oldest = min(x.approval_timestamp for x in changes)
      if time.time() > oldest + self.PRE_CQ_TIMEOUT:
        # It's safest to try all changes here because some of the old changes
        # might depend on newer changes (e.g. via CQ-DEPEND).
        changes_to_test = changes

    return changes_to_test, non_manifest_changes

  def _SetPoolFromManifest(self, manifest):
    """Sets validation pool based on manifest path passed in."""
    # Note that this function is only called after the repo is already
    # sync'd, so AcquirePoolFromManifest does not need to sync.
    self.pool = validation_pool.ValidationPool.AcquirePoolFromManifest(
        manifest=manifest,
        overlays=self._run.config.overlays,
        repo=self.repo,
        build_number=self._run.buildnumber,
        builder_name=self._run.GetBuilderName(),
        buildbucket_id=self._run.options.buildbucket_id,
        is_master=self._run.config.master,
        dryrun=self._run.options.debug,
        builder_run=self._run)

  def _GetLKGMVersionFromManifest(self, manifest):
    manifest_dom = minidom.parse(manifest)
    elements = manifest_dom.getElementsByTagName(lkgm_manager.LKGM_ELEMENT)
    if elements:
      lkgm_version = elements[0].getAttribute(lkgm_manager.LKGM_VERSION_ATTR)
      logging.info(
          'LKGM version was found in the manifest: %s', lkgm_version)
      return lkgm_version

  def GetNextManifest(self):
    """Gets the next manifest using LKGM logic."""
    assert self.manifest_manager, \
        'Must run Initialize before we can get a manifest.'
    assert isinstance(self.manifest_manager, lkgm_manager.LKGMManager), \
        'Manifest manager instantiated with wrong class.'
    assert self._run.config.master

    build_id = self._run.attrs.metadata.GetDict().get('build_id')

    try:
      # In order to acquire a pool, we need an initialized buildroot.
      if not git.FindRepoDir(self.repo.directory):
        self.repo.Initialize()

      query = constants.CQ_READY_QUERY
      if self._run.options.cq_gerrit_override:
        query = (self._run.options.cq_gerrit_override, None)

      self.pool = validation_pool.ValidationPool.AcquirePool(
          overlays=self._run.config.overlays,
          repo=self.repo,
          build_number=self._run.buildnumber,
          builder_name=self._run.GetBuilderName(),
          buildbucket_id=self._run.options.buildbucket_id,
          query=query,
          dryrun=self._run.options.debug,
          check_tree_open=(not self._run.options.debug or
                           self._run.options.mock_tree_status),
          change_filter=self._ChangeFilter, builder_run=self._run)
    except validation_pool.TreeIsClosedException as e:
      logging.warning(str(e))
      return None

    # We must extend the builder deadline before publishing a new manifest to
    # ensure that slaves have enough time to complete the builds about to
    # start.
    build_id, db = self._run.GetCIDBHandle()
    if db:
      db.ExtendDeadline(build_id, self._run.config.build_timeout)

    logging.info('Creating new candidate manifest.')
    manifest = self.manifest_manager.CreateNewCandidate(
        validation_pool=self.pool, build_id=build_id)
    if MasterSlaveLKGMSyncStage.external_manager:
      MasterSlaveLKGMSyncStage.external_manager.CreateFromManifest(
          manifest, build_id=build_id)

    return manifest

  def ManifestCheckout(self, next_manifest):
    """Checks out the repository to the given manifest."""
    # Sync to the provided manifest on slaves. On the master, we're
    # already synced to this manifest, so self.skip_sync is set and
    # this is a no-op.
    super(CommitQueueSyncStage, self).ManifestCheckout(next_manifest)

    if self._run.config.build_before_patching:
      assert not self._run.config.master
      pre_build_passed = self.RunPrePatchBuild()
      logging.PrintBuildbotStepName('CommitQueueSync : Apply Patches')
      if not pre_build_passed:
        logging.PrintBuildbotStepText('Pre-patch build failed.')

    # On slaves, initialize our pool and apply patches. On the master,
    # we've already done that in GetNextManifest, so this is a no-op.
    if not self._run.config.master:
      # Print the list of CHUMP changes since the LKGM, then apply changes and
      # print the list of applied changes.
      self.manifest_manager.GenerateBlameListSinceLKGM()
      self._SetPoolFromManifest(next_manifest)
      self.pool.ApplyPoolIntoRepo()

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    """Performs normal stage and prints blamelist at end."""
    if self._run.options.force_version:
      self.HandleSkip()
    else:
      ManifestVersionedSyncStage.PerformStage(self)

    self.WriteChangesToMetadata(self.pool.applied)


class PreCQSyncStage(SyncStage):
  """Sync and apply patches to test if they compile."""

  def __init__(self, builder_run, patches, **kwargs):
    super(PreCQSyncStage, self).__init__(builder_run, **kwargs)

    # As a workaround for crbug.com/432706, we scan patches to see if they
    # are already being merged. If they are, we don't test them in the PreCQ.
    self.patches = [p for p in patches if not p.IsBeingMerged()]

    if patches and not self.patches:
      cros_build_lib.Die('No patches that still need testing.')

    # The ValidationPool of patches to test. Initialized in PerformStage, and
    # refreshed after bootstrapping by HandleSkip.
    self.pool = None

  def HandleSkip(self):
    """Handles skip and loads validation pool from disk."""
    super(PreCQSyncStage, self).HandleSkip()
    filename = self._run.options.validation_pool
    if filename:
      self.pool = validation_pool.ValidationPool.Load(
          filename, builder_run=self._run)

  def PerformStage(self):
    super(PreCQSyncStage, self).PerformStage()
    self.pool = validation_pool.ValidationPool.AcquirePreCQPool(
        overlays=self._run.config.overlays,
        build_root=self._build_root,
        build_number=self._run.buildnumber,
        builder_name=self._run.config.name,
        buildbucket_id=self._run.options.buildbucket_id,
        dryrun=self._run.options.debug_forced,
        candidates=self.patches,
        builder_run=self._run)
    self.pool.ApplyPoolIntoRepo()

    if len(self.pool.applied) == 0 and self.patches:
      cros_build_lib.Die('No changes have been applied.')

    changes = self.pool.applied or self.patches
    self.WriteChangesToMetadata(changes)


class PreCQLauncherStage(SyncStage):
  """Scans for CLs and automatically launches Pre-CQ jobs to test them."""

  # The number of minutes we wait before launching Pre-CQ jobs. This measures
  # the idle time of a given patch series, so, for example, if a user takes
  # 20 minutes to mark a series of 20 patches as ready, we won't launch a
  # tryjob on any of the patches until the user has been idle for 2 minutes.
  LAUNCH_DELAY = 2

  # The number of minutes we allow before considering a launch attempt failed.
  LAUNCH_TIMEOUT = 90

  # The number of minutes we allow before considering an in-flight job failed.
  INFLIGHT_TIMEOUT = 240

  # The number of minutes we allow before expiring a pre-cq PASSED or
  # FULLY_VERIFIED status. After this timeout is hit, a CL's status will be
  # reset to None. This prevents very stale CLs from entering the CQ.
  STATUS_EXPIRY_TIMEOUT = 60 * 24 * 7

  # The maximum number of patches we will allow in a given trybot run. This is
  # needed because our trybot infrastructure can only handle so many patches at
  # once.
  MAX_PATCHES_PER_TRYBOT_RUN = 50

  # The maximum derivative of the number of tryjobs we will launch in a given
  # cycle of ProcessChanges. Used to rate-limit the launcher when reopening the
  # tree after building up a large backlog.
  MAX_LAUNCHES_PER_CYCLE_DERIVATIVE = 20

  # Delta time constant for checking buildbucket. Do not check status or
  # cancel builds which were launched >= BUILDBUCKET_DELTA_TIME_HOUR ago.
  BUILDBUCKET_DELTA_TIME_HOUR = 4

  # Delay between launches of sanity-pre-cq builds for the same build config
  PRE_CQ_SANITY_CHECK_PERIOD_HOURS = 5

  # How many days to look back in build history to check for sanity-pre-cq
  PRE_CQ_SANITY_CHECK_LOOK_BACK_HISTORY_DAYS = 1

  def __init__(self, builder_run, **kwargs):
    super(PreCQLauncherStage, self).__init__(builder_run, **kwargs)
    self.skip_sync = True
    self.last_cycle_launch_count = 0


  def _HasTimedOut(self, start, now, timeout_minutes):
    """Check whether |timeout_minutes| has elapsed between |start| and |now|.

    Args:
      start: datetime.datetime start time.
      now: datetime.datetime current time.
      timeout_minutes: integer number of minutes for timeout.

    Returns:
      True if (now-start) > timeout_minutes.
    """
    diff = datetime.timedelta(minutes=timeout_minutes)
    return (now - start) > diff

  @staticmethod
  def _PrintPatchStatus(patch, status):
    """Print a link to |patch| with |status| info."""
    items = (
        status,
        os.path.basename(patch.project),
        str(patch),
    )
    logging.PrintBuildbotLink(' | '.join(items), patch.url)

  def _GetPreCQConfigsFromOptions(self, change, union_pre_cq_limit=None):
    """Get Pre-CQ configs from CQ config options.

    If union-pre-cq-sub-configs flag is True in the default config file, get
    unioned Pre-CQ configs from the sub configs; else, get Pre-CQ configs from
    the default config file.

    Args:
      change: The instance of cros_patch.GerritPatch to get Pre-CQ configs.
      union_pre_cq_limit: The limit size for unioned Pre-CQ configs if provided.
        Default to None.

    Returns:
      A set of valid Pre-CQ configs (strings) or None.
    """
    try:
      cq_config_parser = cq_config.CQConfigParser(self._build_root, change,
                                                  forgiving=False)
      pre_cq_configs = None
      if cq_config_parser.GetUnionPreCQSubConfigsFlag():
        pre_cq_configs = self._ParsePreCQsFromOption(
            cq_config_parser.GetUnionedPreCQConfigs())
        if (union_pre_cq_limit is not None and pre_cq_configs and
            len(pre_cq_configs) > union_pre_cq_limit):
          raise ExceedUnionPreCQLimitException(pre_cq_configs,
                                               union_pre_cq_limit)

        return pre_cq_configs
      else:
        return self._ParsePreCQsFromOption(
            cq_config_parser.GetPreCQConfigs())
    except (UnknownPreCQConfigRequestedError,
            cq_config.MalformedCQConfigException):
      logging.exception('Exception encountered when parsing pre-cq options '
                        'for change %s. Falling back to default set.', change)
      m = 'chromeos/cbuildbot/pre-cq/bad_pre_cq_options_count'
      metrics.Counter(m).increment()
      return None

  def _ConfiguredVerificationsForChange(self, change):
    """Determine which configs to test |change| with.

    This method returns only the configs that are asked for by the config
    file. It does not include special-case logic for adding additional bots
    based on the type of the repository (see VerificationsForChange for that).

    Args:
      change: GerritPatch instance to get configs-to-test for.

    Returns:
      A set of configs to test.
    """
    configs_to_test = None
    # If a pre-cq config is specified in the commit message, use that.
    # Otherwise, look in appropriate COMMIT-QUEUE.ini. Otherwise, default to
    # constants.PRE_CQ_DEFAULT_CONFIGS
    lines = cros_patch.GetOptionLinesFromCommitMessage(
        change.commit_message, constants.CQ_CONFIG_PRE_CQ_CONFIGS_REGEX)
    if lines is not None:
      try:
        configs_to_test = self._ParsePreCQsFromOption(lines)
      except UnknownPreCQConfigRequestedError:
        logging.exception('Unknown config requested in commit message '
                          'for change %s. Falling back to default set.',
                          change)

    configs_from_options = None
    try:
      configs_from_options = self._GetPreCQConfigsFromOptions(
          change, union_pre_cq_limit=DEFAULT_UNION_PRE_CQ_LIMIT)
    except ExceedUnionPreCQLimitException as e:
      pre_cq_configs = list(e.pre_cq_configs)
      pre_cq_configs.sort()
      configs_from_options = pre_cq_configs[:DEFAULT_UNION_PRE_CQ_LIMIT]
      logging.info('Unioned Pre-CQs %s for change %s exceed the limit %d. '
                   'Will launch the following Pre-CQ configs: %s',
                   e.pre_cq_configs, change.PatchLink(),
                   DEFAULT_UNION_PRE_CQ_LIMIT, configs_from_options)

    configs_to_test = configs_to_test or configs_from_options
    return set(configs_to_test or constants.PRE_CQ_DEFAULT_CONFIGS)

  def VerificationsForChange(self, change):
    """Determine which configs to test |change| with.

    Args:
      change: GerritPatch instance to get configs-to-test for.

    Returns:
      A set of configs to test.
    """
    configs_to_test = self._ConfiguredVerificationsForChange(change)

    # Add the BINHOST_PRE_CQ to any changes that affect an overlay.
    if '/overlays/' in change.project:
      configs_to_test.add(constants.BINHOST_PRE_CQ)

    return configs_to_test

  def _ParsePreCQsFromOption(self, pre_cq_configs):
    """Parse Pre-CQ configs got from option.

    Args:
      pre_cq_configs: A list of Pre-CQ configs got from option, or None.

    Returns:
      A valid Pre-CQ config list, or None.
    """
    if pre_cq_configs:
      configs_to_test = set(pre_cq_configs)

      # Replace 'default' with the default configs.
      if 'default' in configs_to_test:
        configs_to_test.discard('default')
        configs_to_test.update(constants.PRE_CQ_DEFAULT_CONFIGS)

      # Verify that all of the configs are valid.
      if all(c in self._run.site_config for c in configs_to_test):
        return configs_to_test
      else:
        raise UnknownPreCQConfigRequestedError(configs_to_test)

    return None

  def ScreenChangeForPreCQ(self, change):
    """Record which pre-cq tryjobs to test |change| with.

    This method determines which configs to test a given |change| with, and
    writes those as pending tryjobs to the cidb.

    Args:
      change: GerritPatch instance to screen. This change should not yet have
              been screened.
    """
    actions = []
    configs_to_test = self.VerificationsForChange(change)
    for c in configs_to_test:
      actions.append(clactions.CLAction.FromGerritPatchAndAction(
          change, constants.CL_ACTION_VALIDATION_PENDING_PRE_CQ,
          reason=c))
    actions.append(clactions.CLAction.FromGerritPatchAndAction(
        change, constants.CL_ACTION_SCREENED_FOR_PRE_CQ))

    build_id, db = self._run.GetCIDBHandle()
    db.InsertCLActions(build_id, actions)

  def CanSubmitChangeInPreCQ(self, change):
    """Look up whether |change| is configured to be submitted in the pre-CQ.

    Args:
      change: Change to examine.

    Returns:
      Boolean indicating if this change is configured to be submitted in the
      pre-CQ.
    """
    cq_config_parser = cq_config.CQConfigParser(self._build_root, change)
    return cq_config_parser.CanSubmitChangeInPreCQ()

  def GetConfigBuildbucketIdMap(self, output):
    """Convert tryjob json output into a config:buildbucket_id map.

    Config is the config-name of a pre-cq triggered by the pre-cq-launcher.
    buildbucket_id is the request id of the pre-cq build.
    """
    # List of dicts containing 'build_config', 'buildbucket_id', 'url'
    tryjob_output = json.loads(output)
    return {t['build_config']: t['buildbucket_id'] for t in tryjob_output}

  def _LaunchTrybots(self, pool, configs, plan=None,
                     sanity_check_build=False, swarming=True):
    """Launch tryjobs on the configs with patches if provided.

    Args:
      pool: An instance of ValidationPool.validation_pool.
      configs: A list of pre-cq config names to launch.
      plan: A list of patches to test in the pre-cq tryjob, default to None.
      sanity_check_build: Boolean indicating whether to run the tryjobs as
        sanity-check-build.
      swarming: Boolean indicating jobs should running as swarming builds.

    Returns:
      A dict mapping from build_config (string) to the buildbucket_id (string)
      of the launched Pre-CQs. An empty dict if any configuration target doesn't
      exist.
    """
    # Verify the configs to test are in the cbuildbot config list.
    for config in configs:
      if config not in self._run.site_config:
        logging.error('No such configuraton target: %s.', config)

        if plan is not None:
          for change in plan:
            logging.error('Skipping trybots on nonexistent config %s for '
                          '%s %s', config, str(change), change.url)
            pool.HandleNoConfigTargetFailure(change, config)

        return {}

    cmd = ['cros', 'tryjob', '--yes', '--json',
           '--timeout', str(self.INFLIGHT_TIMEOUT * 60)] + configs

    if sanity_check_build:
      cmd += ['--sanity-check-build']

    if swarming:
      cmd += ['--swarming']

    if plan is not None:
      for patch in plan:
        cmd += ['-g', cros_patch.AddPrefix(patch, patch.gerrit_number)]
        self._PrintPatchStatus(patch, 'testing')

    config_buildbucket_id_map = {}
    if self._run.options.debug:
      logging.debug('Would have launched tryjob with %s', cmd)
    else:
      result = cros_build_lib.RunCommand(
          cmd, cwd=self._build_root, capture_output=True)
      if result and result.output:
        logging.info('output: %s' % result.output)
        config_buildbucket_id_map = self.GetConfigBuildbucketIdMap(
            result.output)

    return config_buildbucket_id_map

  def LaunchPreCQs(self, build_id, db, pool, configs, plan):
    """Launch Pre-CQ tryjobs on the configs with patches.

    Args:
      build_id: build_id (string) of the pre-cq-launcher build.
      db: An instance of cidb.CIDBConnection.
      pool: An instance of ValidationPool.validation_pool.
      configs: A list of pre-cq config names to launch.
      plan: The list of patches to test in the pre-cq tryjob.
    """
    logging.info('Launching Pre-CQs for configs %s with changes %s',
                 configs, cros_patch.GetChangesAsString(plan))
    config_buildbucket_id_map = self._LaunchTrybots(pool, configs, plan=plan)

    if not config_buildbucket_id_map:
      return

    # Update cidb clActionTable.
    actions = []
    for config in configs:
      if config in config_buildbucket_id_map:
        for patch in plan:
          actions.append(clactions.CLAction.FromGerritPatchAndAction(
              patch, constants.CL_ACTION_TRYBOT_LAUNCHING, config,
              buildbucket_id=config_buildbucket_id_map[config]))

    db.InsertCLActions(build_id, actions)

  def LaunchSanityPreCQs(self, build_id, db, pool, configs):
    """Launch Sanity-Pre-CQ tryjobs on the configs.

    Args:
      build_id: build_id (string) of the pre-cq-launcher build.
      db: An instance of cidb.CIDBConnection or None.
      pool: An instance of ValidationPool.validation_pool.
      configs: A set of pre-cq config names to launch.
    """
    logging.info('Launching Sanity-Pre-CQs for configs %s.', configs)
    config_buildbucket_id_map = self._LaunchTrybots(
        pool, configs, sanity_check_build=True)

    if not config_buildbucket_id_map:
      return

    if db:
      launched_build_reqs = []
      for config in configs:
        launched_build_reqs.append(build_requests.BuildRequest(
            None, build_id, config, None, config_buildbucket_id_map[config],
            build_requests.REASON_SANITY_PRE_CQ, None))

      if launched_build_reqs:
        db.InsertBuildRequests(launched_build_reqs)

  def GetDisjointTransactionsToTest(self, pool, progress_map):
    """Get the list of disjoint transactions to test.

    Side effect: reject or retry changes that have timed out.

    Args:
      pool: The validation pool.
      progress_map: See return type of clactions.GetPreCQProgressMap.

    Returns:
      A list of (transaction, config) tuples corresponding to different trybots
      that should be launched.
    """
    # Get the set of busy and passed CLs.
    busy, _, verified = clactions.GetPreCQCategories(progress_map)

    screened_changes = set(progress_map)

    # Create a list of disjoint transactions to test.
    manifest = git.ManifestCheckout.Cached(self._build_root)
    logging.info('Creating disjoint transactions.')
    plans, failed = pool.CreateDisjointTransactions(
        manifest, screened_changes,
        max_txn_length=self.MAX_PATCHES_PER_TRYBOT_RUN)
    logging.info('Created %s disjoint transactions.', len(plans))

    # Note: |failed| is a list of cros_patch.PatchException instances.
    logging.info('Failed to apply %s CLs. Marked them as failed.', len(failed))
    for f in failed:
      pool.UpdateCLPreCQStatus(f.patch, constants.CL_STATUS_FAILED)

    for plan in plans:
      # If any of the CLs in the plan is not yet screened, wait for them to
      # be screened.
      #
      # If any of the CLs in the plan are currently "busy" being tested,
      # wait until they're done before starting to test this plan.
      #
      # Similarly, if all of the CLs in the plan have already been validated,
      # there's no need to launch a trybot run.
      plan = set(plan)
      if not plan.issubset(screened_changes):
        logging.info('CLs waiting to be screened: %s',
                     cros_patch.GetChangesAsString(
                         plan.difference(screened_changes)))
      elif plan.issubset(verified):
        logging.info('CLs already verified: %s',
                     cros_patch.GetChangesAsString(plan))
      elif plan.intersection(busy):
        logging.info('CLs currently being verified: %s',
                     cros_patch.GetChangesAsString(plan.intersection(busy)))
        if plan.difference(busy):
          logging.info('CLs waiting on verification of dependencies: %r',
                       cros_patch.GetChangesAsString(plan.difference(busy)))
      # TODO(akeshet): Consider using a database time rather than gerrit
      # approval time and local clock for launch delay.
      elif any(x.approval_timestamp + self.LAUNCH_DELAY * 60 > time.time()
               for x in plan):
        logging.info('CLs waiting on launch delay: %s',
                     cros_patch.GetChangesAsString(plan))
      else:
        pending_configs = clactions.GetPreCQConfigsToTest(plan, progress_map)
        for config in pending_configs:
          yield (plan, config)

  def _ProcessRequeuedAndSpeculative(self, change, action_history):
    """Detect if |change| was requeued by developer, and mark in cidb.

    Args:
      change: GerritPatch instance to check.
      action_history: List of CLActions.
    """
    action_string = clactions.GetRequeuedOrSpeculative(
        change, action_history, not change.IsMergeable())
    if action_string:
      build_id, db = self._run.GetCIDBHandle()
      action = clactions.CLAction.FromGerritPatchAndAction(
          change, action_string)
      db.InsertCLActions(build_id, [action])
      logging.info('Record change %s with action %s build_id %s.',
                   change, action_string, build_id)

  def _ProcessExpiry(self, change, status, timestamp, pool, current_time):
    """Enforce expiry of a PASSED or FULLY_VERIFIED status.

    Args:
      change: GerritPatch instance to process.
      status: |change|'s pre-cq status.
      timestamp: datetime.datetime for when |status| was achieved.
      pool: The current validation pool.
      current_time: datetime.datetime for current database time.
    """
    if not timestamp:
      return
    timed_out = self._HasTimedOut(timestamp, current_time,
                                  self.STATUS_EXPIRY_TIMEOUT)
    verified = status in (constants.CL_STATUS_PASSED,
                          constants.CL_STATUS_FULLY_VERIFIED)
    if timed_out and verified:
      msg = PRECQ_EXPIRY_MSG % self.STATUS_EXPIRY_TIMEOUT
      build_id, db = self._run.GetCIDBHandle()
      if db:
        pool.SendNotification(change, '%(details)s', details=msg)
        action = clactions.CLAction.FromGerritPatchAndAction(
            change, constants.CL_ACTION_PRE_CQ_RESET)
        db.InsertCLActions(build_id, [action])

  def _ProcessTimeouts(self, change, progress_map, pool, current_time):
    """Enforce per-config launch and inflight timeouts.

    Args:
      change: GerritPatch instance to process.
      progress_map: See return type of clactions.GetPreCQProgressMap.
      pool: The current validation pool.
      current_time: datetime.datetime timestamp giving current database time.
    """
    timeout_statuses = (constants.CL_PRECQ_CONFIG_STATUS_LAUNCHED,
                        constants.CL_PRECQ_CONFIG_STATUS_INFLIGHT)
    config_progress = progress_map[change]
    for config, pre_cq_progress_tuple in config_progress.iteritems():
      if not pre_cq_progress_tuple.status in timeout_statuses:
        continue
      launched = (pre_cq_progress_tuple.status ==
                  constants.CL_PRECQ_CONFIG_STATUS_LAUNCHED)
      timeout = self.LAUNCH_TIMEOUT if launched else self.INFLIGHT_TIMEOUT
      msg = (PRECQ_LAUNCH_TIMEOUT_MSG if launched
             else PRECQ_INFLIGHT_TIMEOUT_MSG) % (config, timeout)

      if self._HasTimedOut(pre_cq_progress_tuple.timestamp, current_time,
                           timeout):
        pool.SendNotification(change, '%(details)s', details=msg)
        pool.RemoveReady(change, reason=config)
        pool.UpdateCLPreCQStatus(change, constants.CL_STATUS_FAILED)

  def _CancelPreCQIfNeeded(self, db, old_build_action):
    """Cancel the pre-cq if it's still running.

    Args:
      db: CIDB connection instance.
      old_build_action: Old patch build action.
    """
    buildbucket_id = old_build_action.buildbucket_id
    get_content = self.buildbucket_client.GetBuildRequest(
        buildbucket_id, dryrun=self._run.options.debug)

    status = buildbucket_lib.GetBuildStatus(get_content)
    if status in [constants.BUILDBUCKET_BUILDER_STATUS_SCHEDULED,
                  constants.BUILDBUCKET_BUILDER_STATUS_STARTED]:
      logging.info('Cancelling old build buildbucket_id: %s, '
                   'current status: %s.', buildbucket_id, status)

      cancel_content = self.buildbucket_client.CancelBuildRequest(
          buildbucket_id, dryrun=self._run.options.debug)
      cancel_status = buildbucket_lib.GetBuildStatus(cancel_content)
      if cancel_status:
        logging.info('Cancelled old build buildbucket_id: %s, '
                     'current status: %s', buildbucket_id, cancel_status)
        metrics.Counter(constants.MON_BB_CANCEL_PRE_CQ_BUILD_COUNT).increment()

        if db:
          old_build = db.GetBuildStatusWithBuildbucketId(buildbucket_id)
          if old_build is not None:
            cancel_action = old_build_action._replace(
                action=constants.CL_ACTION_TRYBOT_CANCELLED)
            db.InsertCLActions(old_build['id'], [cancel_action])
      else:
        # If the old pre-cq build already completed, CANCEL response will
        # give 200 returncode with error reasons.
        logging.info('Failed to cancel build buildbucket_id: %s, reason: %s',
                     buildbucket_id,
                     buildbucket_lib.GetErrorReason(cancel_content))

  def _ProcessOldPatchPreCQRuns(self, db, change, action_history):
    """Process Pre-cq runs for change with old patch numbers.

    Args:
      db: CIDB connection instance.
      change: GerritPatch instance to process.
      action_history: List of CLActions.
    """
    min_timestamp = datetime.datetime.now() - datetime.timedelta(
        hours=self.BUILDBUCKET_DELTA_TIME_HOUR)
    old_pre_cq_build_actions = clactions.GetOldPreCQBuildActions(
        change, action_history, min_timestamp)
    for old_build_action in old_pre_cq_build_actions:
      try:
        self._CancelPreCQIfNeeded(db, old_build_action)
      except buildbucket_lib.BuildbucketResponseException as e:
        # Do not raise if it's buildbucket_lib.BuildbucketResponseException.
        logging.error('Failed to cancel the old pre cq run through Buildbucket.'
                      ' change: %s buildbucket_id: %s error: %r',
                      change, old_build_action.buildbucket_id, e)

  def _GetFailedPreCQConfigs(self, action_history):
    """Get failed Pre-CQ build configs from action history.

    Args:
      action_history: A list of clactions.CLAction.

    Returns:
      A set of failed Pre-CQ build configs.
    """
    failed_build_configs = set()
    for action in action_history:
      build_config = action.build_config
      if (build_config not in failed_build_configs and
          site_config.get(build_config) is not None and
          site_config[build_config].build_type == constants.PRE_CQ_TYPE and
          action.action == constants.CL_ACTION_PICKED_UP and
          action.status == constants.BUILDER_STATUS_FAILED):
        failed_build_configs.add(build_config)

    return failed_build_configs

  def _FailureStreakCounterExceedsThreshold(self, build_config, build_history):
    """Check whether the consecutive failure counter exceeds the threshold.

    Args:
      db: CIDBConnection instance.
      build_config: The build config to check.
      build_history: A sorted list of dict containing build information. See
        Return types of cidb.CIDBConnection.GetBuildsHistory.

    Returns:
      A boolean indicating whether the consecutive failure counter of
        build_config exceeds its sanity_check_threshold.
    """
    sanity_check_threshold = site_config[build_config].sanity_check_threshold

    if sanity_check_threshold <= 0:
      return False

    streak_counter = 0
    for build in build_history:
      if build['status'] == constants.BUILDER_STATUS_PASSED:
        return False
      elif build['status'] == constants.BUILDER_STATUS_FAILED:
        streak_counter += 1

      if streak_counter >= sanity_check_threshold:
        return True

    return False

  def _GetBuildConfigsToSanityCheck(self, db, build_configs):
    """Get build configs to sanity check.

    Args:
      db: An instance of cidb.CIDBConnection.
      build_configs: A list of build configs (strings) to check whether to
        sanity check.

    Returns:
      A list of build_configs (strings) to sanity check.
    """
    start_date = datetime.datetime.now().date() - datetime.timedelta(
        days=self.PRE_CQ_SANITY_CHECK_LOOK_BACK_HISTORY_DAYS)
    builds_history = db.GetBuildsHistory(
        build_configs, db.NUM_RESULTS_NO_LIMIT, start_date=start_date,
        final=True)
    build_history_by_build_config = cros_build_lib.GroupByKey(
        builds_history, 'build_config')

    start_time = datetime.datetime.now() - datetime.timedelta(
        hours=self.PRE_CQ_SANITY_CHECK_PERIOD_HOURS)
    builds_requests = db.GetBuildRequestsForBuildConfigs(
        build_configs, start_time=start_time)
    build_requests_by_build_config = cros_build_lib.GroupNamedtuplesByKey(
        builds_requests, 'request_build_config')

    sanity_check_build_configs = set()
    for build_config in build_configs:
      build_history = build_history_by_build_config.get(build_config, [])
      build_reqs = build_requests_by_build_config.get(build_config, [])
      if (build_history and
          not build_reqs and
          self._FailureStreakCounterExceedsThreshold(
              build_config, build_history)):
        sanity_check_build_configs.add(build_config)

    return list(sanity_check_build_configs)

  def _LaunchSanityCheckPreCQsIfNeeded(self, build_id, db, pool,
                                       action_history):
    """Check the Pre-CQs of changes and launch Sanity-Pre-CQs if needed.

    Args:
      build_id: build_id (string) of the pre-cq-launcher build.
      db: An instance of cidb.CIDBConnection.
      pool: An instance of ValidationPool.validation_pool.
      action_history: A list of clactions.CLActions.
    """
    failed_build_configs = self._GetFailedPreCQConfigs(action_history)

    if not failed_build_configs:
      return

    sanity_check_build_configs = self._GetBuildConfigsToSanityCheck(
        db, failed_build_configs)

    if sanity_check_build_configs:
      self.LaunchSanityPreCQs(build_id, db, pool, sanity_check_build_configs)

  def _ProcessVerified(self, change, can_submit, will_submit):
    """Process a change that is fully pre-cq verified.

    Args:
      change: GerritPatch instance to process.
      can_submit: set of changes that can be submitted by the pre-cq.
      will_submit: set of changes that will be submitted by the pre-cq.

    Returns:
      A tuple of (set of changes that should be submitted by pre-cq,
                  set of changes that should be passed by pre-cq)
    """
    # If this change and all its dependencies are pre-cq submittable,
    # and none of them have yet been marked as pre-cq passed, then
    # mark them for submission. Otherwise, mark this change as passed.
    if change in will_submit:
      return set(), set()

    if change in can_submit:
      logging.info('Attempting to determine if %s can be submitted.', change)
      patches = patch_series.PatchSeries(self._build_root)
      try:
        plan = patches.CreateTransaction(change, limit_to=can_submit)
        return plan, set()
      except cros_patch.DependencyError:
        pass

    # Changes that cannot be submitted are marked as passed.
    return set(), set([change])

  def UpdateChangeStatuses(self, changes, status):
    """Update |changes| to |status|.

    Args:
      changes: A set of GerritPatch instances.
      status: One of constants.CL_STATUS_* statuses.
    """
    if changes:
      build_id, db = self._run.GetCIDBHandle()
      a = clactions.TranslatePreCQStatusToAction(status)
      actions = [clactions.CLAction.FromGerritPatchAndAction(c, a)
                 for c in changes]
      db.InsertCLActions(build_id, actions)

  def _LaunchPreCQsIfNeeded(self, pool, changes):
    """Find ready changes and launch Pre-CQs.

    Args:
      pool: An instance of ValidationPool.validation_pool.
      changes: GerritPatch instances.
    """
    build_id, db = self._run.GetCIDBHandle()

    action_history, _, status_map = (
        self._GetUpdatedActionHistoryAndStatusMaps(db, changes))

    # Filter out failed speculative changes.
    changes = [c for c in changes if status_map[c] != constants.CL_STATUS_FAILED
               or c.HasReadyFlag()]

    progress_map = clactions.GetPreCQProgressMap(changes, action_history)

    # Filter out changes that have already failed, and aren't marked trybot
    # ready or commit ready, before launching.
    launchable_progress_map = {
        k: v for k, v in progress_map.iteritems()
        if k.HasReadyFlag() or status_map[k] != constants.CL_STATUS_FAILED}

    is_tree_open = tree_status.IsTreeOpen(throttled_ok=True)
    launch_count = 0
    cl_launch_count = 0
    launch_count_limit = (self.last_cycle_launch_count +
                          self.MAX_LAUNCHES_PER_CYCLE_DERIVATIVE)

    launches = {}
    for plan, config in self.GetDisjointTransactionsToTest(
        pool, launchable_progress_map):
      launches.setdefault(frozenset(plan), []).append(config)

    for plan, configs in launches.iteritems():
      if not is_tree_open:
        logging.info('Tree is closed, not launching configs %r for plan %s.',
                     configs, cros_patch.GetChangesAsString(plan))
      elif launch_count >= launch_count_limit:
        logging.info('Hit or exceeded maximum launch count of %s this cycle, '
                     'not launching configs %r for plan %s.',
                     launch_count_limit, configs,
                     cros_patch.GetChangesAsString(plan))
      else:
        self.LaunchPreCQs(build_id, db, pool, configs, plan)
        launch_count += len(configs)
        cl_launch_count += len(configs) * len(plan)

    metrics.Counter(constants.MON_PRECQ_LAUNCH_COUNT).increment_by(
        launch_count)
    metrics.Counter(constants.MON_PRECQ_CL_LAUNCH_COUNT).increment_by(
        cl_launch_count)
    metrics.Counter(constants.MON_PRECQ_TICK_COUNT).increment()

    self.last_cycle_launch_count = launch_count

  def _GetUpdatedActionHistoryAndStatusMaps(self, db, changes):
    """Get updated action_history and Pre-CQ status for changes.

    Args:
      db: cidb.CIDBConnection instance.
      changes: GerritPatch instances to process.

    Returns:
      (The current CLAction list of the changes, the current map from changes to
       (status, timestamp) tuple, the current map from changes to status).
    """
    action_history = db.GetActionsForChanges(changes)

    status_and_timestamp_map = {
        c: clactions.GetCLPreCQStatusAndTime(c, action_history)
        for c in changes}
    status_map = {c: v[0] for c, v in status_and_timestamp_map.items()}

    status_str_map = {c.PatchLink(): s for c, s in status_map.iteritems()}
    logging.info('Processing status_map: %s', pprint.pformat(status_str_map))

    return action_history, status_and_timestamp_map, status_map

  def ProcessChanges(self, pool, changes, _non_manifest_changes):
    """Process a list of changes that were marked as Ready.

    From our list of changes that were marked as Ready, we create a
    list of disjoint transactions and send each one to a separate Pre-CQ
    trybot.

    Non-manifest changes are just submitted here because they don't need to be
    verified by either the Pre-CQ or CQ.
    """
    build_id, db = self._run.GetCIDBHandle()

    action_history = db.GetActionsForChanges(changes)

    self._LaunchSanityCheckPreCQsIfNeeded(build_id, db, pool, action_history)

    if self.buildbucket_client is not None:
      for change in changes:
        self._ProcessOldPatchPreCQRuns(db, change, action_history)

    for change in changes:
      self._ProcessRequeuedAndSpeculative(change, action_history)

    action_history, status_and_timestamp_map, status_map = (
        self._GetUpdatedActionHistoryAndStatusMaps(db, changes))

    # Filter out failed speculative changes.
    changes = [c for c in changes if status_map[c] != constants.CL_STATUS_FAILED
               or c.HasReadyFlag()]

    progress_map = clactions.GetPreCQProgressMap(changes, action_history)
    busy, inflight, verified = clactions.GetPreCQCategories(progress_map)
    logging.info('Changes in busy: %s.\nChanges in inflight: %s.\nChanges in '
                 'verified: %s.',
                 cros_patch.GetChangesAsString(busy),
                 cros_patch.GetChangesAsString(inflight),
                 cros_patch.GetChangesAsString(verified))

    current_db_time = db.GetTime()

    to_process = set(c for c in changes
                     if status_map[c] != constants.CL_STATUS_PASSED)

    # Mark verified changes verified.
    to_mark_verified = [c for c in verified.intersection(to_process) if
                        status_map[c] != constants.CL_STATUS_FULLY_VERIFIED]
    self.UpdateChangeStatuses(to_mark_verified,
                              constants.CL_STATUS_FULLY_VERIFIED)
    # Send notifications to the fully verified changes.
    if to_mark_verified:
      pool.HandlePreCQSuccess(to_mark_verified)

    # Changes that can be submitted, if their dependencies can be too. Only
    # include changes that have not already been marked as passed.
    can_submit = set(c for c in (verified.intersection(to_process)) if
                     c.IsMergeable() and self.CanSubmitChangeInPreCQ(c))

    self.SendChangeCountStats(status_map)

    # Changes that will be submitted.
    will_submit = set()
    # Changes that will be passed.
    will_pass = set()

    for change in inflight:
      if status_map[change] != constants.CL_STATUS_INFLIGHT:
        build_ids = [x.build_id for x in progress_map[change].values()]
        # Change the status to inflight.
        self.UpdateChangeStatuses([change], constants.CL_STATUS_INFLIGHT)
        build_dicts = db.GetBuildStatuses(build_ids)
        lines = []
        for b in build_dicts:
          url = tree_status.ConstructLegolandBuildURL(b['buildbucket_id'])
          lines.append('(%s) : %s ' % (b['build_config'], url))

        # Send notifications.
        pool.HandleApplySuccess(change, build_log=('\n' + '\n'.join(lines)))

    for change in to_process:
      # Detect if change is ready to be marked as passed, or ready to submit.
      if change in verified and change.IsMergeable():
        to_submit, to_pass = self._ProcessVerified(change, can_submit,
                                                   will_submit)
        will_submit.update(to_submit)
        will_pass.update(to_pass)
        continue

      # Screen unscreened changes to determine which trybots to test them with.
      if not clactions.IsChangeScreened(change, action_history):
        self.ScreenChangeForPreCQ(change)
        continue

      self._ProcessTimeouts(change, progress_map, pool, current_db_time)

    # Mark passed changes as passed
    self.UpdateChangeStatuses(will_pass, constants.CL_STATUS_PASSED)

    # Expire any very stale passed or fully verified changes.
    for c, v in status_and_timestamp_map.items():
      self._ProcessExpiry(c, v[0], v[1], pool, current_db_time)

    # Submit changes that are ready to submit, if we can.
    if tree_status.IsTreeOpen(throttled_ok=True):
      pool.SubmitNonManifestChanges(check_tree_open=False,
                                    reason=constants.STRATEGY_NONMANIFEST)
      submit_reason = constants.STRATEGY_PRECQ_SUBMIT
      will_submit = {c:submit_reason for c in will_submit}
      submitted, _ = pool.SubmitChanges(will_submit, check_tree_open=False)

      # Record stats about submissions in monarch.
      if db:
        submitted_change_actions = db.GetActionsForChanges(submitted)
        strategies = {m: constants.STRATEGY_PRECQ_SUBMIT for m in submitted}
        clactions_metrics.RecordSubmissionMetrics(
            clactions.CLActionHistory(submitted_change_actions), strategies)

    self._LaunchPreCQsIfNeeded(pool, changes)

    # Tell ValidationPool to keep waiting for more changes until we hit
    # its internal timeout.
    return [], []

  def SendChangeCountStats(self, status_map):
    """Sends metrics of the CL counts to Monarch.

    Args:
      status_map: A map from CLs to statuses.
    """
    # Separately count and log the number of mergable and speculative changes in
    # each of the possible pre-cq statuses (or in status None).
    POSSIBLE_STATUSES = clactions.PRE_CQ_CL_STATUSES | {None}
    status_counts = {}
    for count_bin in itertools.product((True, False), POSSIBLE_STATUSES):
      status_counts[count_bin] = 0
    for c, status in status_map.iteritems():
      count_bin = (c.IsMergeable(), status)
      status_counts[count_bin] += 1
    for (is_mergable, status), count in sorted(status_counts.items()):
      subtype = 'mergeable' if is_mergable else 'speculative'
      metrics.Gauge('chromeos/cbuildbot/pre-cq/cl-count').set(
          count, {'status': str(status), 'subtype': subtype})

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    # Setup and initialize the repo.
    super(PreCQLauncherStage, self).PerformStage()

    query = constants.PRECQ_READY_QUERY
    if self._run.options.cq_gerrit_override:
      query = (self._run.options.cq_gerrit_override, None)

    # Loop through all of the changes until we hit a timeout.
    validation_pool.ValidationPool.AcquirePool(
        overlays=self._run.config.overlays,
        repo=self.repo,
        build_number=self._run.buildnumber,
        builder_name=self._run.GetBuilderName(),
        buildbucket_id=self._run.options.buildbucket_id,
        query=query,
        dryrun=self._run.options.debug,
        check_tree_open=False, change_filter=self.ProcessChanges,
        builder_run=self._run)
