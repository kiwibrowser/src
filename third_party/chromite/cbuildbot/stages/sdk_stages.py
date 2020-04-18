# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing SDK stages."""

from __future__ import print_function

import glob
import json
import os

from chromite.cbuildbot import commands
from chromite.cbuildbot import prebuilts
from chromite.cbuildbot.stages import generic_stages
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.lib import perf_uploader
from chromite.lib import portage_util
from chromite.lib import toolchain
from chromite.scripts import upload_prebuilts


# Version of the Manifest file being generated for SDK artifacts. Should be
# incremented for major format changes.
PACKAGE_MANIFEST_VERSION = '1'

# Paths excluded when packaging SDK artifacts. These are relative to the target
# build root where SDK packages are being installed (e.g. /build/amd64-host).
PACKAGE_EXCLUDED_PATHS = (
    'usr/lib/debug',
    'usr/lib64/debug',
    constants.AUTOTEST_BUILD_PATH,
    'packages',
    'tmp'
)

# Names of various packaged artifacts.
SDK_TARBALL_NAME = 'built-sdk.tar.xz'
TOOLCHAINS_OVERLAY_TARBALL_TEMPLATE = \
    'built-sdk-overlay-toolchains-%(toolchains)s.tar.xz'


def SdkPerfPath(buildroot):
  """Return the path to the perf file for sdk stages."""
  return os.path.join(buildroot, constants.DEFAULT_CHROOT_DIR, 'tmp',
                      'cros-sdk.perf')


def CreateTarball(source_root, tarball_path, exclude_paths=None):
  """Packs |source_root| into |tarball_path|.

  Args:
    source_root: Path to the directory we want to package.
    tarball_path: Path of the tarball that should be created.
    exclude_paths: Subdirectories to exclude.
  """
  # TODO(zbehan): We cannot use xz from the chroot unless it's
  # statically linked.
  extra_args = None
  if exclude_paths is not None:
    extra_args = ['--exclude=%s/*' % path for path in exclude_paths]
  # Options for maximum compression.
  extra_env = {'XZ_OPT': '-e9'}
  cros_build_lib.CreateTarball(
      tarball_path, source_root, sudo=True, extra_args=extra_args,
      debug_level=logging.INFO, extra_env=extra_env)
  # Make sure the regular user has the permission to read.
  cmd = ['chmod', 'a+r', tarball_path]
  cros_build_lib.SudoRunCommand(cmd)


class SDKBuildToolchainsStage(generic_stages.BuilderStage,
                              generic_stages.ArchivingStageMixin):
  """Stage that builds all the cross-compilers we care about"""

  def PerformStage(self):
    chroot_location = os.path.join(self._build_root,
                                   constants.DEFAULT_CHROOT_DIR)

    # Build the toolchains first.  Since we're building & installing the
    # compilers, need to run as root.
    self.CrosSetupToolchains(['--nousepkg'], sudo=True)

    # Create toolchain packages.
    self.CreateRedistributableToolchains(chroot_location)
    toolchain_path = os.path.join(chroot_location,
                                  constants.SDK_TOOLCHAINS_OUTPUT)
    for files in os.listdir(toolchain_path):
      self.UploadArtifact(
          os.path.join(toolchain_path, files), strict=True, archive=True)

  def CrosSetupToolchains(self, cmd_args, **kwargs):
    """Wrapper around cros_setup_toolchains to simplify things."""
    commands.RunBuildScript(self._build_root,
                            ['cros_setup_toolchains'] + list(cmd_args),
                            chromite_cmd=True, enter_chroot=True, **kwargs)

  def CreateRedistributableToolchains(self, chroot_location):
    """Create the toolchain packages"""
    osutils.RmDir(os.path.join(chroot_location,
                               constants.SDK_TOOLCHAINS_OUTPUT),
                  ignore_missing=True)

    # We need to run this as root because the tool creates hard links to root
    # owned files and our bots enable security features which disallow that.
    # Specifically, these features cause problems:
    #  /proc/sys/kernel/yama/protected_nonaccess_hardlinks
    #  /proc/sys/fs/protected_hardlinks
    self.CrosSetupToolchains([
        '--create-packages',
        '--output-dir', os.path.join('/', constants.SDK_TOOLCHAINS_OUTPUT),
    ], sudo=True)


class SDKPackageStage(generic_stages.BuilderStage,
                      generic_stages.ArchivingStageMixin):
  """Stage that performs preparing and packaging SDK files"""

  def __init__(self, builder_run, version=None, **kwargs):
    self.sdk_version = version
    super(SDKPackageStage, self).__init__(builder_run, **kwargs)

  def PerformStage(self):
    tarball_location = os.path.join(self._build_root, SDK_TARBALL_NAME)
    chroot_location = os.path.join(self._build_root,
                                   constants.DEFAULT_CHROOT_DIR)
    board_location = os.path.join(chroot_location, 'build/amd64-host')
    manifest_location = tarball_location + '.Manifest'

    # Create a tarball of the latest SDK.
    CreateTarball(board_location, tarball_location)
    self.UploadArtifact(tarball_location, strict=True, archive=True)

    # Create a package manifest for the tarball.
    self.CreateManifestFromSDK(board_location, manifest_location)

    self.SendPerfValues(tarball_location)

  def CreateManifestFromSDK(self, sdk_path, dest_manifest):
    """Creates a manifest from a given source chroot.

    Args:
      sdk_path: Path to the root of the SDK to describe.
      dest_manifest: Path to the manifest that should be generated.
    """
    logging.info('Generating manifest for new sdk')
    package_data = {}
    for key, version in portage_util.ListInstalledPackages(sdk_path):
      package_data.setdefault(key, []).append((version, {}))
    self._WriteManifest(package_data, dest_manifest)

  def _WriteManifest(self, data, manifest):
    """Encode manifest into a json file."""
    json_input = dict(version=PACKAGE_MANIFEST_VERSION, packages=data)
    osutils.WriteFile(manifest, json.dumps(json_input))

  def _SendPerfValues(self, buildroot, sdk_tarball, buildbot_uri_log, version,
                      platform_name):
    """Generate & upload perf data for the build"""
    perf_path = SdkPerfPath(buildroot)
    test_name = 'sdk'
    units = 'bytes'

    # Make sure the file doesn't contain previous data.
    osutils.SafeUnlink(perf_path)

    common_kwargs = {
        'higher_is_better': False,
        'graph': 'cros-sdk-size',
        'stdio_uri': buildbot_uri_log,
    }

    sdk_size = os.path.getsize(sdk_tarball)
    perf_uploader.OutputPerfValue(perf_path, 'base', sdk_size, units,
                                  **common_kwargs)

    for tarball in glob.glob(os.path.join(
        buildroot, constants.DEFAULT_CHROOT_DIR,
        constants.SDK_TOOLCHAINS_OUTPUT, '*.tar.*')):
      name = os.path.basename(tarball).rsplit('.', 2)[0]
      size = os.path.getsize(tarball)
      perf_uploader.OutputPerfValue(perf_path, name, size, units,
                                    **common_kwargs)
      perf_uploader.OutputPerfValue(perf_path, 'base_plus_%s' % name,
                                    sdk_size + size, units, **common_kwargs)

    # Due to limitations in the perf dashboard, we have to create an integer
    # based on the current timestamp.  This field only accepts integers, and
    # the perf dashboard accepts this or CrOS+Chrome official versions.
    revision = int(version.replace('.', ''))
    perf_values = perf_uploader.LoadPerfValues(perf_path)
    self._UploadPerfValues(perf_values, platform_name, test_name,
                           revision=revision)

  def SendPerfValues(self, sdk_tarball):
    """Generate & upload perf data for the build"""
    self._SendPerfValues(self._build_root, sdk_tarball,
                         self.ConstructDashboardURL(), self.sdk_version,
                         self._run.bot_id)


class SDKPackageToolchainOverlaysStage(generic_stages.BuilderStage):
  """Stage that creates and packages per-board toolchain overlays."""

  def __init__(self, builder_run, version=None, **kwargs):
    self.sdk_version = version
    super(SDKPackageToolchainOverlaysStage, self).__init__(builder_run,
                                                           **kwargs)

  def PerformStage(self):
    chroot_dir = os.path.join(self._build_root, constants.DEFAULT_CHROOT_DIR)
    sdk_dir = os.path.join(chroot_dir, 'build/amd64-host')
    tmp_dir = os.path.join(chroot_dir, 'tmp')
    osutils.SafeMakedirs(tmp_dir, mode=0o777, sudo=True)
    overlay_output_dir = os.path.join(chroot_dir,
                                      constants.SDK_OVERLAYS_OUTPUT)
    osutils.RmDir(overlay_output_dir, ignore_missing=True, sudo=True)
    osutils.SafeMakedirs(overlay_output_dir, mode=0o777, sudo=True)
    overlay_tarball_template = os.path.join(
        overlay_output_dir, TOOLCHAINS_OVERLAY_TARBALL_TEMPLATE)

    # Generate an overlay tarball for each unique toolchain combination. We
    # restrict ourselves to (a) board configs that are available to the builder
    # (naturally), and (b) toolchains that are part of the 'sdk' set.
    sdk_toolchains = set(toolchain.GetToolchainsForBoard('sdk'))
    generated = set()
    for board in self._run.site_config.GetBoards():
      try:
        toolchains = set(toolchain.GetToolchainsForBoard(board).iterkeys())
      except portage_util.MissingOverlayException:
        # The board overlay may not exist, e.g. on external builders.
        continue

      toolchains_str = '-'.join(sorted(toolchains))
      if not toolchains.issubset(sdk_toolchains) or toolchains_str in generated:
        continue

      with osutils.TempDir(prefix='toolchains-overlay-%s.' % toolchains_str,
                           base_dir=tmp_dir, sudo_rm=True) as overlay_dir:
        # NOTE: We let MountOverlayContext remove the mount point created by
        # the TempDir context below, because it has built-in retries for rmdir
        # EBUSY errors that are due to unmount lag.
        with osutils.TempDir(prefix='amd64-host-%s.' % toolchains_str,
                             base_dir=tmp_dir, delete=False) as merged_dir:
          with osutils.MountOverlayContext(sdk_dir, overlay_dir, merged_dir,
                                           cleanup=True):
            sysroot = merged_dir[len(chroot_dir):]
            cmd = ['cros_setup_toolchains', '--targets=boards',
                   '--include-boards=%s' % board,
                   '--sysroot=%s' % sysroot]
            commands.RunBuildScript(self._build_root, cmd, chromite_cmd=True,
                                    enter_chroot=True, sudo=True,
                                    extra_env=self._portage_extra_env)

        # NOTE: Make sure that the overlay directory is owned root:root and has
        # 0o755 perms; apparently, these things are preserved through
        # tarring/untarring and might cause havoc if overlooked.
        os.chmod(overlay_dir, 0o755)
        cros_build_lib.SudoRunCommand(['chown', 'root:root', overlay_dir])
        CreateTarball(overlay_dir,
                      overlay_tarball_template % {'toolchains': toolchains_str})

      generated.add(toolchains_str)


class SDKTestStage(generic_stages.BuilderStage):
  """Stage that performs testing an SDK created in a previous stage"""

  option_name = 'tests'

  def PerformStage(self):
    new_chroot_dir = 'new-sdk-chroot'
    tarball_location = os.path.join(self._build_root, SDK_TARBALL_NAME)
    new_chroot_args = ['--chroot', new_chroot_dir]
    if self._run.options.chrome_root:
      new_chroot_args += ['--chrome_root', self._run.options.chrome_root]

    # Build a new SDK using the provided tarball.
    chroot_args = new_chroot_args + ['--download', '--replace', '--nousepkg',
                                     '--url', 'file://' + tarball_location]
    cros_build_lib.RunCommand(
        [], cwd=self._build_root, enter_chroot=True, chroot_args=chroot_args,
        extra_env=self._portage_extra_env)

    # Inject the toolchain binpkgs from the previous sdk build.  On end user
    # systems, they'd be fetched from the binpkg mirror, but we don't have one
    # set up for this local build.
    pkgdir = os.path.join('var', 'lib', 'portage', 'pkgs')
    old_pkgdir = os.path.join(self._build_root, constants.DEFAULT_CHROOT_DIR,
                              pkgdir)
    new_pkgdir = os.path.join(self._build_root, new_chroot_dir, pkgdir)
    osutils.SafeMakedirs(new_pkgdir, sudo=True)
    cros_build_lib.SudoRunCommand(
        ['cp', '-r'] + glob.glob(os.path.join(old_pkgdir, '*')) +
        [new_pkgdir])

    # Now install those toolchains in the new chroot.  We skip the chroot
    # upgrade below which means we need to install the toolchain manually.
    cmd = ['cros_setup_toolchains', '--targets=boards',
           '--include-boards=%s' % ','.join(self._boards)]
    commands.RunBuildScript(self._build_root, cmd, chromite_cmd=True,
                            enter_chroot=True, sudo=True,
                            chroot_args=new_chroot_args,
                            extra_env=self._portage_extra_env)

    # Build all the boards with the new sdk.
    for board in self._boards:
      logging.PrintBuildbotStepText(board)
      commands.SetupBoard(self._build_root, board, usepkg=True,
                          chroot_upgrade=False,
                          extra_env=self._portage_extra_env,
                          chroot_args=new_chroot_args)
      commands.Build(self._build_root, board, build_autotest=True,
                     usepkg=False, chrome_binhost_only=False,
                     extra_env=self._portage_extra_env,
                     chroot_args=new_chroot_args)


class SDKUprevStage(generic_stages.BuilderStage):
  """Stage that uprevs SDK version."""

  def __init__(self, builder_run, version=None, **kwargs):
    super(SDKUprevStage, self).__init__(builder_run, **kwargs)
    self._version = version

  def PerformStage(self):
    if self._run.config.prebuilts == constants.PUBLIC:
      binhost_conf_dir = prebuilts.PUBLIC_BINHOST_CONF_DIR
    else:
      binhost_conf_dir = prebuilts.PRIVATE_BINHOST_CONF_DIR
    sdk_conf = os.path.join(
        self._build_root, binhost_conf_dir, 'host', 'sdk_version.conf')

    tc_path_format = prebuilts.GetToolchainSdkUploadFormat(
        self._version, prebuilts.GetToolchainSdkPaths(self._build_root)[0][1])
    sdk_settings = {
        'SDK_LATEST_VERSION': self._version,
        'TC_PATH': tc_path_format % {'version': self._version},
    }
    upload_prebuilts.RevGitFile(
        sdk_conf, sdk_settings, dryrun=self._run.options.debug)
