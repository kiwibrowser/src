# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the build stages."""

from __future__ import print_function

import base64
import glob
import os

from chromite.cbuildbot import cbuildbot_run
from chromite.cbuildbot import commands
from chromite.cbuildbot import goma_util
from chromite.cbuildbot import repository
from chromite.cbuildbot import topology
from chromite.cbuildbot.stages import generic_stages
from chromite.cbuildbot.stages import test_stages
from chromite.lib import buildbucket_lib
from chromite.lib import builder_status_lib
from chromite.lib import build_summary
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import cros_sdk_lib
from chromite.lib import failures_lib
from chromite.lib import git
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import portage_util
from chromite.lib import path_util
from chromite.lib import request_build


class CleanUpStage(generic_stages.BuilderStage):
  """Stages that cleans up build artifacts from previous runs.

  This stage cleans up previous KVM state, temporary git commits,
  clobbers, and wipes tmp inside the chroot.
  """

  option_name = 'clean'

  def _CleanChroot(self):
    logging.info('Cleaning chroot.')
    commands.CleanupChromeKeywordsFile(self._boards,
                                       self._build_root)
    chroot_dir = os.path.join(self._build_root, constants.DEFAULT_CHROOT_DIR)
    chroot_tmpdir = os.path.join(chroot_dir, 'tmp')
    if os.path.exists(chroot_tmpdir):
      osutils.RmDir(chroot_tmpdir, ignore_missing=True, sudo=True)
      cros_build_lib.SudoRunCommand(['mkdir', '--mode', '1777', chroot_tmpdir],
                                    print_cmd=False)

    # Clear out the incremental build cache between runs.
    cache_dir = 'var/cache/portage'
    d = os.path.join(chroot_dir, cache_dir)
    osutils.RmDir(d, ignore_missing=True, sudo=True)
    for board in self._boards:
      d = os.path.join(chroot_dir, 'build', board, cache_dir)
      osutils.RmDir(d, ignore_missing=True, sudo=True)

  def _RevertChrootToCleanSnapshot(self):
    try:
      logging.info('Attempting to revert chroot to %s snapshot',
                   constants.CHROOT_SNAPSHOT_CLEAN)
      snapshots = commands.ListChrootSnapshots(self._build_root)
      if constants.CHROOT_SNAPSHOT_CLEAN not in snapshots:
        logging.error(
            "Can't find %s snapshot.", constants.CHROOT_SNAPSHOT_CLEAN)
        return False

      return commands.RevertChrootToSnapshot(self._build_root,
                                             constants.CHROOT_SNAPSHOT_CLEAN)
    except failures_lib.BuildScriptFailure as e:
      logging.error('Failed to revert chroot to snapshot: %s', e)
      return False

  def _CreateCleanSnapshot(self):
    for snapshot in commands.ListChrootSnapshots(self._build_root):
      if not commands.DeleteChrootSnapshot(self._build_root, snapshot):
        logging.warning("Couldn't delete old snapshot %s", snapshot)
    commands.CreateChrootSnapshot(self._build_root,
                                  constants.CHROOT_SNAPSHOT_CLEAN)

  def _DeleteChroot(self):
    logging.info('Deleting chroot.')
    chroot = os.path.join(self._build_root, constants.DEFAULT_CHROOT_DIR)
    if os.path.exists(chroot) or os.path.exists(chroot + '.img'):
      # At this stage, it's not safe to run the cros_sdk inside the buildroot
      # itself because we haven't sync'd yet, and the version of the chromite
      # in there might be broken. Since we've already unmounted everything in
      # there, we can just remove it using rm -rf.
      cros_sdk_lib.CleanupChrootMount(chroot, delete_image=True)
      osutils.RmDir(chroot, ignore_missing=True, sudo=True)

  def _DeleteArchivedTrybotImages(self):
    """Clear all previous archive images to save space."""
    logging.info('Deleting archived trybot images.')
    for trybot in (False, True):
      archive_root = self._run.GetArchive().GetLocalArchiveRoot(trybot=trybot)
      osutils.RmDir(archive_root, ignore_missing=True)

  def _DeleteArchivedPerfResults(self):
    """Clear any previously stashed perf results from hw testing."""
    logging.info('Deleting archived perf results.')
    for result in glob.glob(os.path.join(
        self._run.options.log_dir,
        '*.%s' % test_stages.HWTestStage.PERF_RESULTS_EXTENSION)):
      os.remove(result)

  def _DeleteChromeBuildOutput(self):
    logging.info('Deleting Chrome build output.')
    chrome_src = os.path.join(self._run.options.chrome_root, 'src')
    for out_dir in glob.glob(os.path.join(chrome_src, 'out_*')):
      osutils.RmDir(out_dir)

  def _BuildRootGitCleanup(self):
    logging.info('Cleaning up buildroot git repositories.')
    # Run git gc --auto --prune=all on all repos in CleanUpStage
    repo = self.GetRepoRepository()
    repo.BuildRootGitCleanup(prune_all=True)

  def _DeleteAutotestSitePackages(self):
    """Clears any previously downloaded site-packages."""
    logging.info('Deleting autotest site packages.')
    site_packages_dir = os.path.join(self._build_root, 'src', 'third_party',
                                     'autotest', 'files', 'site-packages')
    # Note that these shouldn't be recreated but might be around from stale
    # builders.
    osutils.RmDir(site_packages_dir, ignore_missing=True)

  def _WipeOldOutput(self):
    logging.info('Wiping old output.')
    commands.WipeOldOutput(self._build_root)

  def _GetPreviousBuildStatus(self):
    """Extract the status of the previous build from command-line arguments.

    Returns:
      A BuildSummary object representing the previous build.
    """
    previous_state = build_summary.BuildSummary()
    if self._run.options.previous_build_state:
      try:
        state_json = base64.b64decode(
            self._run.options.previous_build_state)
        previous_state.from_json(state_json)
        logging.info('Previous local build %s finished in state %s.',
                     previous_state.build_description(), previous_state.status)
      except ValueError as e:
        logging.error('Failed to decode previous build state: %s', e)
    return previous_state

  def _GetPreviousMasterStatus(self, previous_state):
    """Get the state of the previous master build from CIDB.

    Args:
      previous_state: A BuildSummary object representing the previous build.

    Returns:
      A tuple containing the master build number and status, or None, None
      if there isn't one.
    """
    if not previous_state.master_build_id:
      return None, None

    _, db = self._run.GetCIDBHandle()
    if not db:
      return None, None

    master_status = db.GetBuildStatus(previous_state.master_build_id)
    if not master_status:
      logging.warning('Previous master build id %s not found.',
                      previous_state.master_build_id)
      return None, None
    logging.info('Previous master build %s finished in state %s',
                 master_status['build_number'],
                 master_status['status'])

    return master_status['build_number'], master_status['status']

  def CancelObsoleteSlaveBuilds(self):
    """Cancel the obsolete slave builds scheduled by the previous master."""
    logging.info('Cancelling obsolete slave builds.')

    buildbucket_client = self.GetBuildbucketClient()
    if not buildbucket_client:
      return

    # Find the 3 most recent master buildbucket ids.
    master_builds = buildbucket_client.SearchAllBuilds(
        self._run.options.debug,
        buckets=constants.ACTIVE_BUCKETS,
        limit=3,
        tags=['cbb_config:%s' % self._run.config.name,
              'cbb_branch:%s' % self._run.manifest_branch],
        status=constants.BUILDBUCKET_BUILDER_STATUS_COMPLETED)

    slave_ids = []

    # Find the scheduled or started slaves for those master builds.
    for master_id in buildbucket_lib.ExtractBuildIds(master_builds):
      for status in [constants.BUILDBUCKET_BUILDER_STATUS_SCHEDULED,
                     constants.BUILDBUCKET_BUILDER_STATUS_STARTED]:
        builds = buildbucket_client.SearchAllBuilds(
            self._run.options.debug,
            tags=['buildset:%s' % request_build.SlaveBuildSet(master_id)],
            status=status)

        ids = buildbucket_lib.ExtractBuildIds(builds)
        if ids:
          logging.info('Found builds %s in status %s from master %s.',
                       ids, status, master_id)
          slave_ids.extend(ids)

    if slave_ids:
      builder_status_lib.CancelBuilds(slave_ids,
                                      buildbucket_client,
                                      self._run.options.debug,
                                      self._run.config)

  def CanReuseChroot(self, chroot_path):
    """Determine if the chroot can be reused.

    A chroot can be reused if all of the following are true:
        1.  The existence of chroot.img matches what is requested in the config,
            i.e. exists when chroot_use_image is True or vice versa.
        2.  The build config doesn't request chroot_replace.
        3.  The previous local build succeeded.
        4.  If there was a previous master build, that build also succeeded.

    Args:
      chroot_path: Path to the chroot we want to reuse.

    Returns:
      True if the chroot at |chroot_path| can be reused, False if not.
    """

    chroot_img = chroot_path + '.img'
    chroot_img_exists = os.path.exists(chroot_img)
    if self._run.config.chroot_use_image != chroot_img_exists:
      logging.info('chroot image at %s %s but chroot_use_image=%s.  '
                   'Cannot reuse chroot.', chroot_img,
                   'exists' if chroot_img_exists else "doesn't exist",
                   self._run.config.chroot_use_image)
      return False

    if self._run.config.chroot_replace and self._run.options.build:
      logging.info('Build config has chroot_replace=True. Cannot reuse chroot.')
      return False

    previous_state = self._GetPreviousBuildStatus()
    if previous_state.status != constants.BUILDER_STATUS_PASSED:
      logging.info('Previous local build %s did not pass. Cannot reuse chroot.',
                   previous_state.build_number)
      return False

    if previous_state.master_build_id:
      build_number, status = self._GetPreviousMasterStatus(previous_state)
      if status != constants.BUILDER_STATUS_PASSED:
        logging.info('Previous master build %s did not pass (%s).  '
                     'Cannot reuse chroot.', build_number, status)
        return False

    return True

  def CanUseChrootSnapshotToDelete(self, chroot_path):
    """Determine if the chroot can be "deleted" by reverting to a snapshot.

    A chroot can be reverted instead of deleting if all of the following are
    true:
        1. The builder isn't being clobbered.
        2. The config allows chroot reuse, i.e. chroot_replace=False.
        3. The chroot actually supports snapshots, i.e. chroot_use_image is
           True and chroot.img exists.
    Note that the chroot might not contain any snapshots. This means that even
    if this function returns True, the chroot isn't guaranteed to be able to be
    reverted.

    Args:
      chroot_path: Path to the chroot we want to revert.

    Returns:
      True if it is safe to revert |chroot| to a snapshot instead of deleting.
    """
    if self._run.options.clobber:
      logging.info('Clobber is set.  Cannot revert to snapshot.')
      return False

    if self._run.config.chroot_replace:
      logging.info('Chroot will be replaced. Cannot revert to snapshot.')
      return False

    if not self._run.config.chroot_use_image:
      logging.info('chroot_use_image=false. Cannot revert to snapshot.')
      return False

    chroot_img = chroot_path + '.img'
    if not os.path.exists(chroot_img):
      logging.info('Chroot image %s does not exist. Cannot revert to snapshot.',
                   chroot_img)
      return False

    return True

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    if (not (self._run.options.buildbot or self._run.options.remote_trybot)
        and self._run.options.clobber):
      if not commands.ValidateClobber(self._build_root):
        cros_build_lib.Die("--clobber in local mode must be approved.")

    # If we can't get a manifest out of it, then it's not usable and must be
    # clobbered.
    manifest = None
    delete_chroot = False
    if not self._run.options.clobber:
      try:
        manifest = git.ManifestCheckout.Cached(self._build_root, search=False)
      except (KeyboardInterrupt, MemoryError, SystemExit):
        raise
      except Exception as e:
        # Either there is no repo there, or the manifest isn't usable.  If the
        # directory exists, log the exception for debugging reasons.  Either
        # way, the checkout needs to be wiped since it's in an unknown
        # state.
        if os.path.exists(self._build_root):
          logging.warning("ManifestCheckout at %s is unusable: %s",
                          self._build_root, e)
        delete_chroot = True

    # Clean mount points first to be safe about deleting.
    chroot_path = os.path.join(self._build_root, constants.DEFAULT_CHROOT_DIR)
    cros_sdk_lib.CleanupChrootMount(chroot=chroot_path)
    osutils.UmountTree(self._build_root)

    if not delete_chroot:
      delete_chroot = not self.CanReuseChroot(chroot_path)

    # If we're going to delete the chroot and we can use a snapshot instead,
    # try to revert.  If the revert succeeds, we don't need to delete after all.
    if delete_chroot and self.CanUseChrootSnapshotToDelete(chroot_path):
      delete_chroot = not self._RevertChrootToCleanSnapshot()

    # Re-mount chroot image if it exists so that subsequent steps can clean up
    # inside.
    if not delete_chroot and self._run.config.chroot_use_image:
      try:
        cros_sdk_lib.MountChroot(chroot=chroot_path, create=False)
      except cros_build_lib.RunCommandError as e:
        logging.error('Unable to mount chroot under %s.  Deleting chroot.  '
                      'Error: %s', self._build_root, e)
        delete_chroot = True

    if manifest is None:
      self._DeleteChroot()
      repository.ClearBuildRoot(self._build_root,
                                self._run.options.preserve_paths)
    else:
      tasks = [self._BuildRootGitCleanup,
               self._WipeOldOutput,
               self._DeleteArchivedTrybotImages,
               self._DeleteArchivedPerfResults,
               self._DeleteAutotestSitePackages]
      if self._run.options.chrome_root:
        tasks.append(self._DeleteChromeBuildOutput)
      if delete_chroot:
        tasks.append(self._DeleteChroot)
      else:
        tasks.append(self._CleanChroot)

      # Only enable CancelObsoleteSlaveBuilds on the master builds
      # which use the Buildbucket scheduler, it checks for builds in
      # ChromiumOs and ChromeOs waterfalls.
      if (config_lib.UseBuildbucketScheduler(self._run.config) and
          config_lib.IsMasterBuild(self._run.config)):
        tasks.append(self.CancelObsoleteSlaveBuilds)

      parallel.RunParallelSteps(tasks)

    # If chroot.img still exists after everything is cleaned up, it means we're
    # planning to reuse it. This chroot was created by the previous run, so its
    # creation isn't affected by any potential changes in the current run.
    # Therefore, if this run fails, having the subsequent run revert to this
    # snapshot will still produce a clean chroot.  If this run succeeds, the
    # next run will reuse the chroot without needing to revert it.  Thus, taking
    # a snapshot now should be correct regardless of whether this run will
    # ultimately succeed or not.
    if os.path.exists(chroot_path + '.img'):
      self._CreateCleanSnapshot()


class InitSDKStage(generic_stages.BuilderStage):
  """Stage that is responsible for initializing the SDK."""

  option_name = 'build'

  def __init__(self, builder_run, chroot_replace=False, **kwargs):
    """InitSDK constructor.

    Args:
      builder_run: Builder run instance for this run.
      chroot_replace: If True, force the chroot to be replaced.
    """
    super(InitSDKStage, self).__init__(builder_run, **kwargs)
    self.force_chroot_replace = chroot_replace

  def DepotToolsEnsureBootstrap(self):
    """Ensure that depot_tools binaries are populated."""
    depot_tools_path = constants.DEPOT_TOOLS_DIR
    ensure_bootstrap_script = os.path.join(depot_tools_path, 'ensure_bootstrap')
    cros_build_lib.RunCommand([ensure_bootstrap_script], cwd=depot_tools_path)

  def PerformStage(self):
    # This prepares depot_tools in the source tree, in advance.
    self.DepotToolsEnsureBootstrap()

    chroot_path = os.path.join(self._build_root, constants.DEFAULT_CHROOT_DIR)
    replace = self._run.config.chroot_replace or self.force_chroot_replace
    pre_ver = post_ver = None
    if os.path.isdir(self._build_root) and not replace:
      try:
        pre_ver = cros_sdk_lib.GetChrootVersion(chroot=chroot_path)
        if pre_ver is not None:
          commands.RunChrootUpgradeHooks(
              self._build_root, chrome_root=self._run.options.chrome_root,
              extra_env=self._portage_extra_env)
      except failures_lib.BuildScriptFailure:
        logging.PrintBuildbotStepText('Replacing broken chroot')
        logging.PrintBuildbotStepWarnings()

    if not os.path.isdir(chroot_path) or replace:
      use_sdk = (self._run.config.use_sdk and not self._run.options.nosdk)
      pre_ver = None
      commands.MakeChroot(
          buildroot=self._build_root,
          replace=replace,
          use_sdk=use_sdk,
          chrome_root=self._run.options.chrome_root,
          extra_env=self._portage_extra_env,
          use_image=self._run.config.chroot_use_image)

    post_ver = cros_sdk_lib.GetChrootVersion(chroot=chroot_path)
    if pre_ver is not None and pre_ver != post_ver:
      logging.PrintBuildbotStepText('%s->%s' % (pre_ver, post_ver))
    else:
      logging.PrintBuildbotStepText(post_ver)

    commands.SetSharedUserPassword(
        self._build_root,
        password=self._run.config.shared_user_password)


class SetupBoardStage(generic_stages.BoardSpecificBuilderStage, InitSDKStage):
  """Stage that is responsible for building host pkgs and setting up a board."""

  option_name = 'build'

  def PerformStage(self):
    build_id, _ = self._run.GetCIDBHandle()
    install_plan_fn = ('/tmp/%s_install_plan.%s' %
                       (self._current_board, build_id))

    # We need to run chroot updates on most builders because they uprev after
    # the InitSDK stage. For the SDK builder, we can skip updates because uprev
    # is run prior to InitSDK. This is not just an optimization: It helps
    # workaround http://crbug.com/225509
    if self._run.config.build_type != constants.CHROOT_BUILDER_TYPE:
      usepkg_toolchain = (self._run.config.usepkg_toolchain and
                          not self._latest_toolchain)
      commands.UpdateChroot(
          self._build_root, toolchain_boards=[self._current_board],
          usepkg=usepkg_toolchain, extra_env=self._portage_extra_env,
          save_install_plan=install_plan_fn)

    # Always update the board.
    usepkg = self._run.config.usepkg_build_packages
    commands.SetupBoard(
        self._build_root, board=self._current_board, usepkg=usepkg,
        chrome_binhost_only=self._run.config.chrome_binhost_only,
        force=self._run.config.board_replace,
        extra_env=self._portage_extra_env, chroot_upgrade=False,
        profile=self._run.options.profile or self._run.config.profile,
        save_install_plan=install_plan_fn)


class BuildPackagesStage(generic_stages.BoardSpecificBuilderStage,
                         generic_stages.ArchivingStageMixin):
  """Build Chromium OS packages."""

  option_name = 'build'
  def __init__(self, builder_run, board, suffix=None, afdo_generate_min=False,
               afdo_use=False, update_metadata=False, **kwargs):
    if afdo_use:
      suffix = self.UpdateSuffix(constants.USE_AFDO_USE, suffix)
    super(BuildPackagesStage, self).__init__(builder_run, board, suffix=suffix,
                                             **kwargs)
    self._afdo_generate_min = afdo_generate_min
    self._update_metadata = update_metadata
    assert not afdo_generate_min or not afdo_use

    useflags = self._portage_extra_env.get('USE', '').split()
    if afdo_use:
      useflags.append(constants.USE_AFDO_USE)

    if useflags:
      self._portage_extra_env['USE'] = ' '.join(useflags)

  def VerifyChromeBinpkg(self, packages):
    # Sanity check: If we didn't check out Chrome (and we're running on ToT),
    # we should be building Chrome from a binary package.
    if (not self._run.options.managed_chrome and
        self._run.manifest_branch == 'master'):
      commands.VerifyBinpkg(self._build_root,
                            self._current_board,
                            constants.CHROME_CP,
                            packages,
                            extra_env=self._portage_extra_env)

  def RecordPackagesUnderTest(self, packages_to_build):
    """Records all packages that may affect the board to BuilderRun."""
    deps = dict()
    # Include packages that are built in chroot because they can
    # affect any board.
    packages = ['virtual/target-sdk']
    # Include chromite because we are running cbuildbot.
    packages += ['chromeos-base/chromite']
    try:
      deps.update(commands.ExtractDependencies(self._build_root, packages))

      # Include packages that will be built as part of the board.
      deps.update(commands.ExtractDependencies(self._build_root,
                                               packages_to_build,
                                               board=self._current_board))
    except Exception as e:
      # Dependency extraction may fail due to bad ebuild changes. Let
      # the build continues because we have logic to triage build
      # packages failures separately. Note that we only categorize CLs
      # on the package-level if dependencies are extracted
      # successfully, so it is safe to ignore the exception.
      logging.warning('Unable to gather packages under test: %s', e)
    else:
      logging.info('Recording packages under test')
      self.board_runattrs.SetParallel('packages_under_test', set(deps.keys()))

  def _ShouldEnableGoma(self):
    # Enable goma if 1) chrome actually needs to be built, 2) not
    # latest_toolchain (because toolchain prebuilt package may not be available
    # for goma, crbug.com/728971) and 3) goma is available.
    return (self._run.options.managed_chrome and
            not self._latest_toolchain and
            self._run.options.goma_dir)

  def _SetupGomaIfNecessary(self):
    """Sets up goma envs if necessary.

    Updates related env vars, and returns args to chroot.

    Returns:
      args which should be provided to chroot in order to enable goma.
      If goma is unusable or disabled, None is returned.
    """
    if not self._ShouldEnableGoma():
      return None

    # TODO(crbug.com/751010): Revisit to enable DepsCache for non-chrome-pfq
    # bots, too.
    use_goma_deps_cache = self._run.config.name.endswith('chrome-pfq')
    goma = goma_util.Goma(
        self._run.options.goma_dir, self._run.options.goma_client_json,
        stage_name=self.StageNamePrefix() if use_goma_deps_cache else None)

    # Set USE_GOMA env var so that chrome is built with goma.
    self._portage_extra_env['USE_GOMA'] = 'true'
    self._portage_extra_env.update(goma.GetChrootExtraEnv())

    # Keep GOMA_TMP_DIR for Report stage to upload logs.
    self._run.attrs.metadata.UpdateWithDict(
        {'goma_tmp_dir': goma.goma_tmp_dir})

    # Mount goma directory and service account json file (if necessary)
    # into chroot.
    chroot_args = ['--goma_dir', goma.goma_dir]
    if goma.goma_client_json:
      chroot_args.extend(['--goma_client_json', goma.goma_client_json])
    return chroot_args

  def PerformStage(self):
    packages = self.GetListOfPackagesToBuild()
    self.VerifyChromeBinpkg(packages)
    self.RecordPackagesUnderTest(packages)

    try:
      event_filename = 'build-events.json'
      event_file = os.path.join(self.archive_path, event_filename)
      logging.info('Logging events to %s', event_file)
      event_file_in_chroot = path_util.ToChrootPath(event_file)
    except cbuildbot_run.VersionNotSetError:
      #TODO(chingcodes): Add better detection of archive options
      logging.info('Unable to archive, disabling build events file')
      event_filename = None
      event_file = None
      event_file_in_chroot = None

    # Set up goma. Use goma iff chrome needs to be built.
    chroot_args = self._SetupGomaIfNecessary()

    build_id, _ = self._run.GetCIDBHandle()
    install_plan_fn = ('/tmp/%s_install_plan.%s' %
                       (self._current_board, build_id))

    commands.Build(self._build_root,
                   self._current_board,
                   build_autotest=self._run.ShouldBuildAutotest(),
                   usepkg=self._run.config.usepkg_build_packages,
                   chrome_binhost_only=self._run.config.chrome_binhost_only,
                   packages=packages,
                   skip_chroot_upgrade=True,
                   chrome_root=self._run.options.chrome_root,
                   noretry=self._run.config.nobuildretry,
                   chroot_args=chroot_args,
                   extra_env=self._portage_extra_env,
                   event_file=event_file_in_chroot,
                   run_goma=bool(chroot_args),
                   save_install_plan=install_plan_fn)

    if event_file and os.path.isfile(event_file):
      logging.info('Archive build-events.json file')
      #TODO: @chingcodes Remove upload after events DB is final
      self.UploadArtifact(event_filename, archive=False, strict=True)

      creds_file = topology.topology.get(topology.DATASTORE_WRITER_CREDS_KEY)

      build_id, db = self._run.GetCIDBHandle()
      if db and creds_file:
        parent_key = ('Build',
                      build_id,
                      'BuildStage',
                      self._build_stage_id)

        commands.ExportToGCloud(self._build_root,
                                creds_file,
                                event_file,
                                parent_key=parent_key,
                                caller=type(self).__name__)
    else:
      logging.info('No build-events.json file to archive')

    if self._update_metadata:
      # Extract firmware version information from the newly created updater.
      fw_versions = commands.GetFirmwareVersions(self._build_root,
                                                 self._current_board)
      main = fw_versions.main_rw or fw_versions.main
      ec = fw_versions.ec_rw or fw_versions.ec
      update_dict = {'main-firmware-version': main, 'ec-firmware-version': ec}
      self._run.attrs.metadata.UpdateBoardDictWithDict(
          self._current_board, update_dict)

      # Write board metadata update to cidb
      build_id, db = self._run.GetCIDBHandle()
      if db:
        db.UpdateBoardPerBuildMetadata(build_id, self._current_board,
                                       update_dict)

      # Get a list of models supported by this board.
      models = commands.GetModels(
          self._build_root, self._current_board, log_output=False)
      self._run.attrs.metadata.UpdateWithDict({'unibuild': bool(models)})
      if models:
        all_fw_versions = commands.GetAllFirmwareVersions(self._build_root,
                                                          self._current_board)
        models_data = {}
        for model in models:
          if model in all_fw_versions:
            fw_versions = all_fw_versions[model]

            ec = fw_versions.ec_rw or fw_versions.ec
            main_ro = fw_versions.main
            main_rw = fw_versions.main_rw or main_ro

            # Get the firmware key-id for the current board and model.
            model_arg = '--model=' + model
            key_id_list = commands.RunCrosConfigHost(
                self._build_root,
                self._current_board,
                [model_arg, 'get', '/firmware', 'key-id'])
            key_id = None
            if len(key_id_list) == 1:
              key_id = key_id_list[0]

            models_data[model] = {'main-readonly-firmware-version': main_ro,
                                  'main-readwrite-firmware-version': main_rw,
                                  'ec-firmware-version': ec,
                                  'firmware-key-id': key_id}
        if models_data:
          self._run.attrs.metadata.UpdateBoardDictWithDict(
              self._current_board, {'models': models_data})


class BuildImageStage(BuildPackagesStage):
  """Build standard Chromium OS images."""

  option_name = 'build'
  config_name = 'images'

  def _BuildImages(self):
    # We only build base, dev, and test images from this stage.
    if self._afdo_generate_min:
      images_can_build = set(['test'])
    else:
      images_can_build = set(['base', 'dev', 'test'])
    images_to_build = set(self._run.config.images).intersection(
        images_can_build)

    version = self._run.attrs.release_tag
    disk_layout = self._run.config.disk_layout
    if self._afdo_generate_min and version:
      version = '%s-afdo-generate' % version

    rootfs_verification = self._run.config.rootfs_verification
    builder_path = '/'.join([self._bot_id, self.version])
    commands.BuildImage(self._build_root,
                        self._current_board,
                        sorted(images_to_build),
                        rootfs_verification=rootfs_verification,
                        version=version,
                        builder_path=builder_path,
                        disk_layout=disk_layout,
                        extra_env=self._portage_extra_env)

    # Update link to latest image.
    latest_image = os.readlink(self.GetImageDirSymlink('latest'))
    cbuildbot_image_link = self.GetImageDirSymlink()
    if os.path.lexists(cbuildbot_image_link):
      os.remove(cbuildbot_image_link)

    os.symlink(latest_image, cbuildbot_image_link)

    self.board_runattrs.SetParallel('images_generated', True)

    parallel.RunParallelSteps(
        [self._BuildVMImage, lambda: self._GenerateAuZip(cbuildbot_image_link),
         self._BuildGceTarballs])

  def _BuildVMImage(self):
    if self._run.config.vm_tests and not self._afdo_generate_min:
      commands.BuildVMImageForTesting(
          self._build_root,
          self._current_board,
          extra_env=self._portage_extra_env,
          disk_layout=self._run.config.disk_layout)

  def _GenerateAuZip(self, image_dir):
    """Create au-generator.zip."""
    if not self._afdo_generate_min:
      commands.GenerateAuZip(self._build_root,
                             image_dir,
                             extra_env=self._portage_extra_env)

  def _BuildGceTarballs(self):
    """Creates .tar.gz files that can be converted to GCE images.

    These files will be used by VMTestStage for tests on GCE. They will also be
    be uploaded to GCS buckets, where they can be used as input to the "gcloud
    compute images create" command. This will convert them into images that can
    be used to create GCE VM instances.
    """
    if self._run.config.gce_tests:
      image_bins = []
      if 'base' in self._run.config['images']:
        image_bins.append(constants.IMAGE_TYPE_TO_NAME['base'])
      if 'test' in self._run.config['images']:
        image_bins.append(constants.IMAGE_TYPE_TO_NAME['test'])

      image_dir = self.GetImageDirSymlink('latest')
      for image_bin in image_bins:
        if os.path.exists(os.path.join(image_dir, image_bin)):
          commands.BuildGceTarball(image_dir, image_dir, image_bin)
        else:
          logging.warning('Missing image file skipped: %s', image_bin)

  def _UpdateBuildImageMetadata(self):
    """Update the new metadata available to the build image stage."""
    update = {}
    fingerprints = self._FindFingerprints()
    if fingerprints:
      update['fingerprints'] = fingerprints
    kernel_version = self._FindKernelVersion()
    if kernel_version:
      update['kernel-version'] = kernel_version
    self._run.attrs.metadata.UpdateBoardDictWithDict(self._current_board,
                                                     update)

  def _FindFingerprints(self):
    """Returns a list of build fingerprints for this build."""
    fp_file = 'cheets-fingerprint.txt'
    fp_path = os.path.join(self.GetImageDirSymlink('latest'), fp_file)
    if not os.path.isfile(fp_path):
      return None
    fingerprints = osutils.ReadFile(fp_path).splitlines()
    logging.info('Found build fingerprint(s): %s', fingerprints)
    return fingerprints

  def _FindKernelVersion(self):
    """Returns a string containing the kernel version for this build."""
    try:
      packages = portage_util.GetPackageDependencies(self._current_board,
                                                     'virtual/linux-sources')
    except cros_build_lib.RunCommandError:
      logging.warning('Unable to get package list for metadata.')
      return None
    for package in packages:
      if package.startswith('sys-kernel/chromeos-kernel-'):
        kernel_version = portage_util.SplitCPV(package).version
        logging.info('Found active kernel version: %s', kernel_version)
        return kernel_version
    return None

  def _HandleStageException(self, exc_info):
    """Tell other stages to not wait on us if we die for some reason."""
    self.board_runattrs.SetParallelDefault('images_generated', False)
    return super(BuildImageStage, self)._HandleStageException(exc_info)

  def PerformStage(self):
    self._BuildImages()
    self._UpdateBuildImageMetadata()


class UprevStage(generic_stages.BuilderStage):
  """Uprevs Chromium OS packages that the builder intends to validate."""

  config_name = 'uprev'
  option_name = 'uprev'

  def __init__(self, builder_run, boards=None, **kwargs):
    super(UprevStage, self).__init__(builder_run, **kwargs)
    if boards is not None:
      self._boards = boards

  def PerformStage(self):
    # Perform other uprevs.
    overlays, _ = self._ExtractOverlays()
    commands.UprevPackages(self._build_root,
                           self._boards,
                           overlays)


class RegenPortageCacheStage(generic_stages.BuilderStage):
  """Regenerates the Portage ebuild cache."""

  # We only need to run this if we're pushing at least one overlay.
  config_name = 'push_overlays'

  def PerformStage(self):
    _, push_overlays = self._ExtractOverlays()
    commands.RegenPortageCache(push_overlays)
