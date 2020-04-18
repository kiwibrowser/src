# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing stages that generate and/or archive artifacts."""

from __future__ import print_function

import datetime
import glob
import itertools
import json
import multiprocessing
import os
import shutil

from chromite.cbuildbot import commands
from chromite.lib import failures_lib
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.cbuildbot import prebuilts
from chromite.cbuildbot.stages import generic_stages
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import path_util
from chromite.lib import portage_util


_FULL_BINHOST = 'FULL_BINHOST'
_PORTAGE_BINHOST = 'PORTAGE_BINHOST'


class DebugSymbolsUploadException(Exception):
  """Thrown if DebugSymbols fails during upload."""


class NothingToArchiveException(Exception):
  """Thrown if ArchiveStage found nothing to archive."""
  def __init__(self, message='No images found to archive.'):
    super(NothingToArchiveException, self).__init__(message)


class ArchiveStage(generic_stages.BoardSpecificBuilderStage,
                   generic_stages.ArchivingStageMixin):
  """Archives build and test artifacts for developer consumption.

  Attributes:
    release_tag: The release tag. E.g. 2981.0.0
    version: The full version string, including the milestone.
        E.g. R26-2981.0.0-b123
  """

  option_name = 'archive'
  config_name = 'archive'

  # This stage is intended to run in the background, in parallel with tests.
  def __init__(self, builder_run, board, chrome_version=None, **kwargs):
    super(ArchiveStage, self).__init__(builder_run, board, **kwargs)
    self.chrome_version = chrome_version

    # TODO(mtennant): Places that use this release_tag attribute should
    # move to use self._run.attrs.release_tag directly.
    self.release_tag = getattr(self._run.attrs, 'release_tag', None)

    self._recovery_image_status_queue = multiprocessing.Queue()
    self._release_upload_queue = multiprocessing.Queue()
    self._upload_queue = multiprocessing.Queue()
    self.artifacts = []

  def WaitForRecoveryImage(self):
    """Wait until artifacts needed by SignerTest stage are created.

    Returns:
      True if artifacts created successfully.
      False otherwise.
    """
    logging.info('Waiting for recovery image...')
    status = self._recovery_image_status_queue.get()
    # Put the status back so other SignerTestStage instances don't starve.
    self._recovery_image_status_queue.put(status)
    return status

  def ArchiveStrippedPackages(self):
    """Generate and archive stripped versions of packages requested."""
    tarball = commands.BuildStrippedPackagesTarball(
        self._build_root,
        self._current_board,
        self._run.config.upload_stripped_packages,
        self.archive_path)
    if tarball is not None:
      self._upload_queue.put([tarball])

  def BuildAndArchiveDeltaSysroot(self):
    """Generate and upload delta sysroot for initial build_packages."""
    extra_env = {}
    if self._run.config.useflags:
      extra_env['USE'] = ' '.join(self._run.config.useflags)
    in_chroot_path = path_util.ToChrootPath(self.archive_path)
    cmd = ['generate_delta_sysroot', '--out-dir', in_chroot_path,
           '--board', self._current_board]
    # TODO(mtennant): Make this condition into one run param.
    if not self._run.options.tests:
      cmd.append('--skip-tests')
    cros_build_lib.RunCommand(cmd, cwd=self._build_root, enter_chroot=True,
                              extra_env=extra_env)
    self._upload_queue.put([constants.DELTA_SYSROOT_TAR])

  def LoadArtifactsList(self, board, image_dir):
    """Load the list of artifacts to upload for this board.

    It attempts to load a JSON file, scripts/artifacts.json, from the
    overlay directories for this board. This file specifies the artifacts
    to generate, if it can't be found, it will use a default set that
    uploads every .bin file as a .tar.xz file.

    See BuildStandaloneArchive in cbuildbot_commands.py for format docs.
    """
    custom_artifacts_file = portage_util.ReadOverlayFile(
        'scripts/artifacts.json', board=board)
    artifacts = None

    if custom_artifacts_file is not None:
      json_file = json.loads(custom_artifacts_file)
      artifacts = json_file.get('artifacts')

    if artifacts is None:
      artifacts = []
      for image_file in glob.glob(os.path.join(image_dir, '*.bin')):
        basename = os.path.basename(image_file)
        info = {'input': [basename], 'archive': 'tar', 'compress': 'xz'}
        artifacts.append(info)

    for artifact in artifacts:
      # Resolve the (possible) globs in the input list, and store
      # the actual set of files to use in 'paths'
      paths = []
      for s in artifact['input']:
        glob_paths = glob.glob(os.path.join(image_dir, s))
        if not glob_paths:
          logging.warning('No artifacts generated for input: %s', s)
        else:
          for path in glob_paths:
            paths.append(os.path.relpath(path, image_dir))
      artifact['paths'] = paths
    self.artifacts = artifacts

  def IsArchivedFile(self, filename):
    """Return True if filename is the name of a file being archived."""
    for artifact in self.artifacts:
      for path in itertools.chain(artifact['paths'], artifact['input']):
        if os.path.basename(path) == filename:
          return True
    return False

  def PerformStage(self):
    buildroot = self._build_root
    config = self._run.config
    board = self._current_board
    debug = self._run.debug
    upload_url = self.upload_url
    archive_path = self.archive_path
    image_dir = self.GetImageDirSymlink()

    extra_env = {}
    if config['useflags']:
      extra_env['USE'] = ' '.join(config['useflags'])

    if not archive_path:
      raise NothingToArchiveException()

    # The following functions are run in parallel (except where indicated
    # otherwise)
    # \- BuildAndArchiveArtifacts
    #    \- ArchiveReleaseArtifacts
    #       \- ArchiveFirmwareImages
    #       \- BuildAndArchiveAllImages
    #          (builds recovery image first, then launches functions below)
    #          \- BuildAndArchiveFactoryImages
    #          \- ArchiveStandaloneArtifacts
    #             \- ArchiveStandaloneArtifact
    #          \- ArchiveZipFiles
    #          \- ArchiveHWQual
    #          \- ArchiveLicenseFile
    #       \- PushImage (blocks on BuildAndArchiveAllImages)
    #    \- ArchiveManifest
    #    \- ArchiveStrippedPackages
    #    \- ArchiveImageScripts
    #    \- BuildAndArchiveDeltaSysroot
    #    \- ArchiveEbuildLogs

    def ArchiveManifest():
      """Create manifest.xml snapshot of the built code."""
      output_manifest = os.path.join(archive_path, 'manifest.xml')
      cmd = ['repo', 'manifest', '-r', '-o', output_manifest]
      cros_build_lib.RunCommand(cmd, cwd=buildroot, capture_output=True)
      self._upload_queue.put(['manifest.xml'])

    def BuildAndArchiveFactoryImages():
      """Build and archive the factory zip file.

      The factory zip file consists of the factory toolkit and the factory
      install image. Both are built here.
      """
      # Build factory install image and create a symlink to it.
      factory_install_symlink = None
      if 'factory_install' in config['images']:
        alias = commands.BuildFactoryInstallImage(buildroot, board, extra_env)
        factory_install_symlink = self.GetImageDirSymlink(alias)
        if config['factory_install_netboot']:
          commands.MakeNetboot(buildroot, board, factory_install_symlink)

      # Build and upload factory zip if needed.
      if factory_install_symlink or config['factory_toolkit']:
        filename = commands.BuildFactoryZip(
            buildroot, board, archive_path, factory_install_symlink,
            self._run.attrs.release_tag)
        self._release_upload_queue.put([filename])

    def ArchiveStandaloneArtifact(artifact_info):
      """Build and upload a single archive."""
      if artifact_info['paths']:
        for path in commands.BuildStandaloneArchive(archive_path, image_dir,
                                                    artifact_info):
          self._release_upload_queue.put([path])

    def ArchiveStandaloneArtifacts():
      """Build and upload standalone archives for each image."""
      if config['upload_standalone_images']:
        parallel.RunTasksInProcessPool(ArchiveStandaloneArtifact,
                                       [[x] for x in self.artifacts])

    def ArchiveEbuildLogs():
      """Tar and archive Ebuild logs.

      This includes all the files in /build/$BOARD/tmp/portage/logs.
      """
      tarpath = commands.BuildEbuildLogsTarball(self._build_root,
                                                self._current_board,
                                                self.archive_path)
      if tarpath is not None:
        self._upload_queue.put([tarpath])

    def ArchiveZipFiles():
      """Build and archive zip files.

      This includes:
        - image.zip (all images in one big zip file)
        - the au-generator.zip used for update payload generation.
      """
      # Zip up everything in the image directory.
      image_zip = commands.BuildImageZip(archive_path, image_dir)
      self._release_upload_queue.put([image_zip])

      # Archive au-generator.zip.
      filename = 'au-generator.zip'
      shutil.copy(os.path.join(image_dir, filename), archive_path)
      self._release_upload_queue.put([filename])

    def ArchiveHWQual():
      """Build and archive the HWQual images."""
      # TODO(petermayo): This logic needs to be exported from the BuildTargets
      # stage rather than copied/re-evaluated here.
      # TODO(mtennant): Make this autotest_built concept into a run param.
      autotest_built = (self._run.options.tests and
                        config['upload_hw_test_artifacts'])

      if config['hwqual'] and autotest_built:
        # Build the full autotest tarball for hwqual image. We don't upload it,
        # as it's fairly large and only needed by the hwqual tarball.
        logging.info('Archiving full autotest tarball locally ...')
        tarball = commands.BuildFullAutotestTarball(self._build_root,
                                                    self._current_board,
                                                    image_dir)
        commands.ArchiveFile(tarball, archive_path)

        # Build hwqual image and upload to Google Storage.
        hwqual_name = 'chromeos-hwqual-%s-%s' % (board, self.version)
        filename = commands.ArchiveHWQual(buildroot, hwqual_name, archive_path,
                                          image_dir)
        self._release_upload_queue.put([filename])

    def ArchiveLicenseFile():
      """Archive licensing file."""
      filename = 'license_credits.html'
      filepath = os.path.join(image_dir, filename)
      if os.path.isfile(filepath):
        shutil.copy(filepath, archive_path)
        self._release_upload_queue.put([filename])

    def ArchiveFirmwareImages():
      """Archive firmware images built from source if available."""
      archive = commands.BuildFirmwareArchive(buildroot, board, archive_path)
      if archive:
        self._release_upload_queue.put([archive])

    def BuildAndArchiveAllImages():
      # Generate the recovery image. To conserve loop devices, we try to only
      # run one instance of build_image at a time. TODO(davidjames): Move the
      # image generation out of the archive stage.
      self.LoadArtifactsList(self._current_board, image_dir)

      # For recovery image to be generated correctly, BuildRecoveryImage must
      # run before BuildAndArchiveFactoryImages.
      if 'recovery' in config.images:
        assert os.path.isfile(os.path.join(image_dir, constants.BASE_IMAGE_BIN))
        commands.BuildRecoveryImage(buildroot, board, image_dir, extra_env)
        self._recovery_image_status_queue.put(True)
        recovery_image = constants.RECOVERY_IMAGE_BIN
        if not self.IsArchivedFile(recovery_image):
          info = {'paths': [recovery_image],
                  'input': [recovery_image],
                  'archive': 'tar',
                  'compress': 'xz'}
          self.artifacts.append(info)
      else:
        self._recovery_image_status_queue.put(False)

      if config['images']:
        steps = [
            BuildAndArchiveFactoryImages,
            ArchiveLicenseFile,
            ArchiveHWQual,
            ArchiveStandaloneArtifacts,
            ArchiveZipFiles,
        ]
        parallel.RunParallelSteps(steps)

    def ArchiveImageScripts():
      """Archive tarball of generated image manipulation scripts."""
      target = os.path.join(archive_path, constants.IMAGE_SCRIPTS_TAR)
      files = glob.glob(os.path.join(image_dir, '*.sh'))
      files = [os.path.basename(f) for f in files]
      cros_build_lib.CreateTarball(target, image_dir, inputs=files)
      self._upload_queue.put([constants.IMAGE_SCRIPTS_TAR])

    def PushImage():
      # This helper script is only available on internal manifests currently.
      if not config['internal']:
        return

      self.GetParallel('debug_tarball_generated', pretty_name='debug tarball')

      # Needed for stateful.tgz
      self.GetParallel('test_artifacts_uploaded', pretty_name='test artifacts')

      # Now that all data has been generated, we can upload the final result to
      # the image server.
      # TODO: When we support branches fully, the friendly name of the branch
      # needs to be used with PushImages
      sign_types = []
      if config['sign_types']:
        sign_types = config['sign_types']
      urls = commands.PushImages(
          board=board,
          archive_url=upload_url,
          dryrun=debug or not config['push_image'],
          profile=self._run.options.profile or config['profile'],
          sign_types=sign_types)
      self.board_runattrs.SetParallel('instruction_urls_per_channel', urls)

    def ArchiveReleaseArtifacts():
      with self.ArtifactUploader(self._release_upload_queue, archive=False):
        steps = [BuildAndArchiveAllImages, ArchiveFirmwareImages]
        parallel.RunParallelSteps(steps)
      PushImage()

    def BuildAndArchiveArtifacts():
      # Run archiving steps in parallel.
      steps = [ArchiveReleaseArtifacts, ArchiveManifest,
               self.ArchiveStrippedPackages, ArchiveEbuildLogs]
      if config['images']:
        steps.append(ArchiveImageScripts)
      if config['create_delta_sysroot']:
        steps.append(self.BuildAndArchiveDeltaSysroot)

      with self.ArtifactUploader(self._upload_queue, archive=False):
        parallel.RunParallelSteps(steps)

      # Make sure no stage posted to the release queue when it should have used
      # the normal upload queue.  The release queue is processed in parallel and
      # then ignored, so there shouldn't be any items left in here.
      assert self._release_upload_queue.empty()

    if not self._run.config.afdo_generate_min:
      BuildAndArchiveArtifacts()

  def _HandleStageException(self, exc_info):
    # Tell the HWTestStage not to wait for artifacts to be uploaded
    # in case ArchiveStage throws an exception.
    self._recovery_image_status_queue.put(False)
    self.board_runattrs.SetParallel('instruction_urls_per_channel', None)
    return super(ArchiveStage, self)._HandleStageException(exc_info)


class CPEExportStage(generic_stages.BoardSpecificBuilderStage,
                     generic_stages.ArchivingStageMixin):
  """Handles generation & upload of package CPE information."""

  config_name = 'cpe_export'

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    """Generate and upload CPE files."""
    buildroot = self._build_root
    board = self._current_board
    useflags = self._run.config.useflags

    logging.info('Generating CPE export.')
    result = commands.GenerateCPEExport(buildroot, board, useflags)

    logging.info('Writing CPE export to files for archive.')
    warnings_filename = os.path.join(self.archive_path,
                                     'cpe-warnings-chromeos-%s.txt' % board)
    results_filename = os.path.join(self.archive_path,
                                    'cpe-chromeos-%s.json' % board)

    osutils.WriteFile(warnings_filename, result.error)
    osutils.WriteFile(results_filename, result.output)

    logging.info('Uploading CPE files.')
    self.UploadArtifact(os.path.basename(warnings_filename), archive=False)
    self.UploadArtifact(os.path.basename(results_filename), archive=False)


class DebugSymbolsStage(generic_stages.BoardSpecificBuilderStage,
                        generic_stages.ArchivingStageMixin):
  """Handles generation & upload of debug symbols."""

  config_name = 'debug_symbols'

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    """Generate debug symbols and upload debug.tgz."""
    buildroot = self._build_root
    board = self._current_board

    # Generate breakpad symbols of Chrome OS binaries.
    commands.GenerateBreakpadSymbols(buildroot, board, self._run.debug)

    # Generate breakpad symbols of Android binaries if we have a symbol archive.
    # This archive is created by AndroidDebugSymbolsStage in Android PFQ.
    # This must be done after GenerateBreakpadSymbols because it clobbers the
    # output directory.
    symbols_file = os.path.join(self.archive_path,
                                constants.ANDROID_SYMBOLS_FILE)
    if os.path.exists(symbols_file):
      commands.GenerateAndroidBreakpadSymbols(buildroot, board, symbols_file)

    self.board_runattrs.SetParallel('breakpad_symbols_generated', True)

    # Upload them.
    self.UploadDebugTarball()

    # Upload debug/breakpad tarball.
    self.UploadDebugBreakpadTarball()

    # Upload them to crash server.
    if self._run.config.upload_symbols:
      self.UploadSymbols(buildroot, board)

  def UploadDebugTarball(self):
    """Generate and upload the debug tarball."""
    filename = commands.GenerateDebugTarball(
        self._build_root, self._current_board, self.archive_path,
        self._run.config.archive_build_debug)
    self.UploadArtifact(filename, archive=False)
    logging.info('Announcing availability of debug tarball now.')
    self.board_runattrs.SetParallel('debug_tarball_generated', True)

  def UploadDebugBreakpadTarball(self):
    """Generate and upload the debug tarball with only breakpad files."""
    filename = commands.GenerateDebugTarball(
        self._build_root, self._current_board, self.archive_path, False,
        archive_name='debug_breakpad.tar.xz')
    self.UploadArtifact(filename, archive=False)

  def UploadSymbols(self, buildroot, board):
    """Upload generated debug symbols."""
    failed_list = os.path.join(self.archive_path, 'failed_upload_symbols.list')

    if self._run.options.remote_trybot or self._run.debug:
      # For debug builds, limit ourselves to just uploading 1 symbol.
      # This way trybots and such still exercise this code.
      cnt = 1
      official = False
    else:
      cnt = None
      official = self._run.config.chromeos_official

    try:
      commands.UploadSymbols(buildroot, board, official, cnt, failed_list)
    except failures_lib.BuildScriptFailure:
      raise DebugSymbolsUploadException('Failed to upload all symbols.')

    if os.path.exists(failed_list):
      self.UploadArtifact(os.path.basename(failed_list), archive=False)

  def _SymbolsNotGenerated(self):
    """Tell other stages that our symbols were not generated."""
    self.board_runattrs.SetParallelDefault('breakpad_symbols_generated', False)
    self.board_runattrs.SetParallelDefault('debug_tarball_generated', False)

  def HandleSkip(self):
    """Tell other stages to not wait on us if we are skipped."""
    self._SymbolsNotGenerated()
    return super(DebugSymbolsStage, self).HandleSkip()

  def _HandleStageException(self, exc_info):
    """Tell other stages to not wait on us if we die for some reason."""
    self._SymbolsNotGenerated()

    # TODO(dgarrett): Get failures tracked in metrics (crbug.com/652463).
    exc_type, e, _ = exc_info
    if (issubclass(exc_type, DebugSymbolsUploadException) or
        (isinstance(e, failures_lib.CompoundFailure) and
         e.MatchesFailureType(DebugSymbolsUploadException))):
      return self._HandleExceptionAsWarning(exc_info)

    return super(DebugSymbolsStage, self)._HandleStageException(exc_info)


class UploadPrebuiltsStage(generic_stages.BoardSpecificBuilderStage):
  """Uploads binaries generated by this build for developer use."""

  option_name = 'prebuilts'
  config_name = 'prebuilts'

  def __init__(self, builder_run, board, version=None, **kwargs):
    self.prebuilts_version = version
    super(UploadPrebuiltsStage, self).__init__(builder_run, board, **kwargs)

  def GenerateCommonArgs(self):
    """Generate common prebuilt arguments."""
    generated_args = []
    if self._run.options.debug:
      generated_args.extend(['--debug', '--dry-run'])

    profile = self._run.options.profile or self._run.config.profile
    if profile:
      generated_args.extend(['--profile', profile])

    # Generate the version if we are a manifest_version build.
    if self._run.config.manifest_version:
      version = self._run.GetVersion()
    else:
      version = self.prebuilts_version
    if version is not None:
      generated_args.extend(['--set-version', version])

    if self._run.config.git_sync:
      # Git sync should never be set for pfq type builds.
      assert not config_lib.IsPFQType(self._prebuilt_type)
      generated_args.extend(['--git-sync'])

    return generated_args

  @classmethod
  def _AddOptionsForSlave(cls, slave_config, board):
    """Private helper method to add upload_prebuilts args for a slave builder.

    Args:
      slave_config: The build config of a slave builder.
      board: The name of the "master" board on the master builder.

    Returns:
      An array of options to add to upload_prebuilts array that allow a master
      to submit prebuilt conf modifications on behalf of a slave.
    """
    args = []
    if slave_config['prebuilts']:
      for slave_board in slave_config['boards']:
        if slave_config['master'] and slave_board == board:
          # Ignore self.
          continue

        args.extend(['--slave-board', slave_board])
        slave_profile = slave_config['profile']
        if slave_profile:
          args.extend(['--slave-profile', slave_profile])

    return args

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    """Uploads prebuilts for master and slave builders."""
    prebuilt_type = self._prebuilt_type
    board = self._current_board
    binhosts = []

    # Whether we publish public or private prebuilts.
    public = self._run.config.prebuilts == constants.PUBLIC
    # Common args we generate for all types of builds.
    generated_args = self.GenerateCommonArgs()
    # Args we specifically add for public/private build types.
    public_args, private_args = [], []
    # Public / private builders.
    public_builders, private_builders = [], []

    # Distributed builders that use manifest-versions to sync with one another
    # share prebuilt logic by passing around versions.
    if config_lib.IsPFQType(prebuilt_type):
      # Public pfqs should upload host preflight prebuilts.
      if prebuilt_type != constants.CHROME_PFQ_TYPE:
        public_args.append('--sync-host')

      # Deduplicate against previous binhosts.
      binhosts.extend(self._GetPortageEnvVar(_PORTAGE_BINHOST, board).split())
      binhosts.extend(self._GetPortageEnvVar(_PORTAGE_BINHOST, None).split())
      for binhost in filter(None, binhosts):
        generated_args.extend(['--previous-binhost-url', binhost])

      if self._run.config.master and board == self._boards[-1]:
        # The master builder updates all the binhost conf files, and needs to do
        # so only once so as to ensure it doesn't try to update the same file
        # more than once. As multiple boards can be built on the same builder,
        # we arbitrarily decided to update the binhost conf files when we run
        # upload_prebuilts for the last board. The other boards are treated as
        # slave boards.
        generated_args.append('--sync-binhost-conf')
        for c in self._GetSlaveConfigs():
          if c['prebuilts'] == constants.PUBLIC:
            public_builders.append(c['name'])
            public_args.extend(self._AddOptionsForSlave(c, board))
          elif c['prebuilts'] == constants.PRIVATE:
            private_builders.append(c['name'])
            private_args.extend(self._AddOptionsForSlave(c, board))

    common_kwargs = {
        'buildroot': self._build_root,
        'category': prebuilt_type,
        'chrome_rev': self._chrome_rev,
        'version': self.prebuilts_version,
    }

    # Upload the public prebuilts, if any.
    if public_builders or public:
      public_board = board if public else None
      prebuilts.UploadPrebuilts(
          private_bucket=False, board=public_board,
          extra_args=generated_args + public_args,
          **common_kwargs)

    # Upload the private prebuilts, if any.
    if private_builders or not public:
      private_board = board if not public else None
      prebuilts.UploadPrebuilts(
          private_bucket=True, board=private_board,
          extra_args=generated_args + private_args,
          **common_kwargs)


class DevInstallerPrebuiltsStage(UploadPrebuiltsStage):
  """Stage that uploads DevInstaller prebuilts."""

  config_name = 'dev_installer_prebuilts'

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    generated_args = generated_args = self.GenerateCommonArgs()
    prebuilts.UploadDevInstallerPrebuilts(
        binhost_bucket=self._run.config.binhost_bucket,
        binhost_key=self._run.config.binhost_key,
        binhost_base_url=self._run.config.binhost_base_url,
        buildroot=self._build_root,
        board=self._current_board,
        extra_args=generated_args)


class UploadTestArtifactsStage(generic_stages.BoardSpecificBuilderStage,
                               generic_stages.ArchivingStageMixin):
  """Upload needed hardware test artifacts."""

  def BuildAutotestTarballs(self):
    """Build the autotest tarballs."""
    with osutils.TempDir(prefix='cbuildbot-autotest') as tempdir:
      with self.ArtifactUploader(strict=True) as queue:
        cwd = os.path.abspath(
            os.path.join(self._build_root, 'chroot', 'build',
                         self._current_board, constants.AUTOTEST_BUILD_PATH,
                         '..'))
        for tarball in commands.BuildAutotestTarballsForHWTest(
            self._build_root, cwd, tempdir):
          queue.put([tarball])

  def _GeneratePayloads(self, image_name, **kwargs):
    """Generate and upload payloads for |image_name|.

    Args:
      image_name: The image to use.
      **kwargs: Keyword arguments to pass to commands.GeneratePayloads.
    """
    with osutils.TempDir(prefix='cbuildbot-payloads') as tempdir:
      with self.ArtifactUploader() as queue:
        image_path = os.path.join(self.GetImageDirSymlink(), image_name)
        commands.GeneratePayloads(self._build_root, image_path, tempdir,
                                  **kwargs)
        for payload in os.listdir(tempdir):
          queue.put([os.path.join(tempdir, payload)])

  def BuildUpdatePayloads(self):
    """Archives update payloads when they are ready."""
    # If we are not configured to generate payloads, don't.
    if not (self._run.options.build and
            self._run.options.tests and
            self._run.config.upload_hw_test_artifacts and
            self._run.config.images):
      return

    # If there are no images to generate payloads from, don't.
    got_images = self.GetParallel('images_generated', pretty_name='images')
    if not got_images:
      return

    payload_type = self._run.config.payload_image
    if payload_type is None:
      payload_type = 'base'
      for t in ['test', 'dev']:
        if t in self._run.config.images:
          payload_type = t
          break
    image_name = constants.IMAGE_TYPE_TO_NAME[payload_type]
    logging.info('Generating payloads to upload for %s', image_name)
    self._GeneratePayloads(image_name, full=True, stateful=True, delta=True)

  @failures_lib.SetFailureType(failures_lib.InfrastructureFailure)
  def PerformStage(self):
    """Upload any needed HWTest artifacts."""
    # BuildUpdatePayloads also uploads the payloads to GS.
    steps = [self.BuildUpdatePayloads]

    if (self._run.ShouldBuildAutotest() and
        self._run.config.upload_hw_test_artifacts):
      steps.append(self.BuildAutotestTarballs)

    parallel.RunParallelSteps(steps)
    # If we encountered any exceptions with any of the steps, they should have
    # set the attribute to False.
    self.board_runattrs.SetParallelDefault('test_artifacts_uploaded', True)

  def _HandleStageException(self, exc_info):
    # Tell the test stages not to wait for artifacts to be uploaded in case
    # UploadTestArtifacts throws an exception.
    self.board_runattrs.SetParallel('test_artifacts_uploaded', False)

    return super(UploadTestArtifactsStage, self)._HandleStageException(exc_info)

# TODO(mtennant): This class continues to exist only for subclasses that still
# need self.archive_stage.  Hopefully, we can get rid of that need, eventually.
class ArchivingStage(generic_stages.BoardSpecificBuilderStage,
                     generic_stages.ArchivingStageMixin):
  """Helper for stages that archive files.

  See ArchivingStageMixin for functionality.

  Attributes:
    archive_stage: The ArchiveStage instance for this board.
  """

  def __init__(self, builder_run, board, archive_stage, **kwargs):
    super(ArchivingStage, self).__init__(builder_run, board, **kwargs)
    self.archive_stage = archive_stage

# This stage generates and uploads the sysroot for the build
# containing all the packages built previously in build packages stage.
class GenerateSysrootStage(generic_stages.BoardSpecificBuilderStage,
                           generic_stages.ArchivingStageMixin):
  """Generate and upload the sysroot for the board."""

  def __init__(self, *args, **kwargs):
    super(GenerateSysrootStage, self).__init__(*args, **kwargs)
    self._upload_queue = multiprocessing.Queue()

  def _GenerateSysroot(self):
    """Generate and upload a sysroot for the board."""
    assert self.archive_path.startswith(self._build_root)
    extra_env = {}
    pkgs = self.GetListOfPackagesToBuild()
    sysroot_tarball = constants.TARGET_SYSROOT_TAR
    if self._run.config.useflags:
      extra_env['USE'] = ' '.join(self._run.config.useflags)
    in_chroot_path = path_util.ToChrootPath(self.archive_path)
    cmd = ['cros_generate_sysroot', '--out-file', sysroot_tarball, '--out-dir',
           in_chroot_path, '--board', self._current_board, '--package',
           ' '.join(pkgs)]
    cros_build_lib.RunCommand(cmd, cwd=self._build_root, enter_chroot=True,
                              extra_env=extra_env)
    self._upload_queue.put([sysroot_tarball])

  def PerformStage(self):
    with self.ArtifactUploader(self._upload_queue, archive=False):
      self._GenerateSysroot()

# This stage generates and uploads the clang-tidy warnings files for the
# build, for all the packages built in build packages stage with
# WITH_TIDY=1.
class GenerateTidyWarningsStage(generic_stages.BoardSpecificBuilderStage,
                                generic_stages.ArchivingStageMixin):
  """Generate and upload the warnings files for the board."""

  CLANG_TIDY_TAR = 'clang_tidy_warnings.tar.xz'
  GS_URL = 'gs://chromeos-clang-tidy-artifacts/clang-tidy-1'

  def __init__(self, *args, **kwargs):
    super(GenerateTidyWarningsStage, self).__init__(*args, **kwargs)
    self._upload_queue = multiprocessing.Queue()

  def _UploadTidyWarnings(self, path, tar_file):
    """Upload the warnings tarball to the clang-tidy gs bucket."""
    gs_context = gs.GSContext()
    filename = os.path.join(path, tar_file)

    debug = self._run.debug
    if debug:
      logging.info('Debug run: not uploading tarball.')
      logging.info('If this were not a debug run, would upload %s to %s.',
                   filename, self.GS_URL)
      return

    try:
      logging.info('Uploading tarball %s to %s', filename, self.GS_URL)
      gs_context.CopyInto(filename, self.GS_URL)
    except:
      logging.info('Error: Unable to upload tarball %s to %s', filename,
                   self.GS_URL)
      raise

  def _GenerateTidyWarnings(self):
    """Generate and upload the tidy warnings files for the board."""
    assert self.archive_path.startswith(self._build_root)
    logs_dir = os.path.join('/tmp', 'clang-tidy-logs', self._current_board)
    timestamp = datetime.datetime.strftime(datetime.datetime.now(), '%Y%m%d')
    clang_tidy_tarball = "%s.%s.%s" % (self._current_board, timestamp,
                                       self.CLANG_TIDY_TAR)
    in_chroot_path = path_util.ToChrootPath(self.archive_path)
    out_chroot_path = os.path.abspath(
        os.path.join(self._build_root, 'chroot', self.archive_path))
    cmd = ['cros_generate_tidy_warnings', '--out-file', clang_tidy_tarball,
           '--out-dir', in_chroot_path, '--board', self._current_board,
           '--logs-dir', logs_dir]
    cros_build_lib.RunCommand(cmd, cwd=self._build_root, enter_chroot=True)
    self._UploadTidyWarnings(out_chroot_path, clang_tidy_tarball)
    self._upload_queue.put([clang_tidy_tarball])

  def PerformStage(self):
    with self.ArtifactUploader(self._upload_queue, archive=False):
      self._GenerateTidyWarnings()
