# -*- coding: utf-8 -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""cbuildbot logic for uploading prebuilts and managing binhosts."""

from __future__ import print_function

import glob
import os

from chromite.cbuildbot import commands
from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import portage_util

_PREFLIGHT_BINHOST = 'PREFLIGHT_BINHOST'
_CHROME_BINHOST = 'CHROME_BINHOST'
_FULL_BINHOST = 'FULL_BINHOST'
_BINHOST_PACKAGE_FILE = ('/usr/share/dev-install/portage/make.profile/'
                         'package.installable')
PRIVATE_BINHOST_CONF_DIR = ('src/private-overlays/chromeos-partner-overlay/'
                            'chromeos/binhost')
PUBLIC_BINHOST_CONF_DIR = 'src/third_party/chromiumos-overlay/chromeos/binhost'


def _AddPackagesForPrebuilt(filename):
  """Add list of packages for upload.

  Process a file that lists all the packages that can be uploaded to the
  package prebuilt bucket and generates the command line args for
  upload_prebuilts.

  Args:
    filename: file with the package full name (category/name-version), one
              package per line.

  Returns:
    A list of parameters for upload_prebuilts. For example:
    ['--packages=net-misc/dhcp', '--packages=app-admin/eselect-python']
  """
  try:
    cmd = []
    with open(filename) as f:
      # Get only the package name and category as that is what upload_prebuilts
      # matches on.
      for line in f:
        atom = line.split('#', 1)[0].strip()
        try:
          cpv = portage_util.SplitCPV(atom)
        except ValueError:
          logging.warning('Could not split atom %r (line: %r)', atom, line)
          continue
        if cpv:
          cmd.extend(['--packages=%s/%s' % (cpv.category, cpv.package)])
    return cmd
  except IOError as e:
    logging.warning('Problem with package file %s' % filename)
    logging.warning('Skipping uploading of prebuilts.')
    logging.warning('ERROR(%d): %s' % (e.errno, e.strerror))
    return None


def GetToolchainSdkPaths(build_root, is_overlay=False):
  """Returns toolchain-sdk's built tar paths, and their target names.

  Args:
    build_root: Path to the build root directory.
    is_overlay: True if finding toolchain-sdk-overlay tars.

  Returns:
    A list of pairs of (upload_sdk_target_name, toolchain_sdk_tarball_path).
  """
  if is_overlay:
    prefix = 'built-sdk-overlay-toolchains-'
    out_dir = constants.SDK_OVERLAYS_OUTPUT
  else:
    prefix = ''
    out_dir = constants.SDK_TOOLCHAINS_OUTPUT

  glob_pattern = os.path.join(
      build_root, constants.DEFAULT_CHROOT_DIR, out_dir, prefix + '*.tar.*')
  result = []
  for tarball in sorted(glob.glob(glob_pattern)):
    name = os.path.basename(tarball).split('.', 1)[0]
    target = name[len(prefix):]
    result.append((target, tarball))
  return result


def GetToolchainSdkUploadFormat(version, tarball, is_overlay=False):
  """Returns format string of the upload toolchain path.

  Args:
    version: Dot-delimited version number string of the toolchain sdk.
    tarball: Path to the tarball to be uploaded.
    is_overlay: True if the format is for toolchain-sdk-overlay.

  Returns:
    Upload format string for the given toolchain tarball.
  """
  # Remaining artifacts get uploaded into <year>/<month>/ subdirs so we don't
  # start dumping even more stuff into the top level. Also, the following
  # code handles any tarball suffix (.tar.*). For each of the artifact types
  # below, we also generate a single upload path template to be filled by the
  # uploading script. This has placeholders for the version (substituted
  # first) and another qualifier (either board or target, substituted second
  # and therefore uses a quoted %% modifier).
  # TODO(garnold) Using a mix of quoted/unquoted template variables is
  # confusing and error-prone, we should get rid of it.
  # TODO(garnold) Be specific about matching file suffixes, like making sure
  # there's nothing past the compression suffix (for example, .tar.xz.log).
  subdir_prefix = os.path.join(*version.split('.')[0:2])
  suffix = os.path.basename(tarball).split('.', 1)[1]
  if is_overlay:
    template = 'cros-sdk-overlay-toolchains-%%(toolchains)s-%(version)s.'
  else:
    template = '%%(target)s-%(version)s.'

  return os.path.join(subdir_prefix, template + suffix)


def UploadPrebuilts(category, chrome_rev, private_bucket, buildroot,
                    version=None, **kwargs):
  """Upload Prebuilts for non-dev-installer use cases.

  Args:
    category: Build type. Can be [binary|full|chrome|chroot|paladin].
    chrome_rev: Chrome_rev of type constants.VALID_CHROME_REVISIONS.
    private_bucket: True if we are uploading to a private bucket.
    buildroot: The root directory where the build occurs.
    version: Specific version to set.
    board: Board type that was built on this machine.
    extra_args: Extra args to pass to prebuilts script.
  """
  extra_args = ['--prepend-version', category]
  extra_args.extend(['--upload', 'gs://chromeos-prebuilt'])
  if private_bucket:
    extra_args.extend(['--private', '--binhost-conf-dir',
                       PRIVATE_BINHOST_CONF_DIR])
  else:
    extra_args.extend(['--binhost-conf-dir', PUBLIC_BINHOST_CONF_DIR])

  if version is not None:
    extra_args.extend(['--set-version', version])

  if category == constants.CHROOT_BUILDER_TYPE:
    extra_args.extend(['--sync-host',
                       '--upload-board-tarball'])
    tarball_location = os.path.join(buildroot, 'built-sdk.tar.xz')
    extra_args.extend(['--prepackaged-tarball', tarball_location])

    # Find toolchain overlay tarballs of the form
    # built-sdk-overlay-toolchains-<toolchains_spec>.tar.* and create an upload
    # specification for each of them. The upload path template has the form
    # cros-sdk-overlay-toolchains-<toolchain_spec>-<version>.tar.*.
    toolchain_overlay_paths = GetToolchainSdkPaths(buildroot, is_overlay=True)
    if toolchain_overlay_paths:
      # Only add the upload path arg when processing the first tarball.
      extra_args.extend([
          '--toolchains-overlay-upload-path',
          GetToolchainSdkUploadFormat(
              version, toolchain_overlay_paths[0][1], is_overlay=True)])
      for entry in toolchain_overlay_paths:
        extra_args.extend(['--toolchains-overlay-tarball', '%s:%s' % entry])

    # Find toolchain package tarballs of the form <target>.tar.* and create an
    # upload specificion for each fo them. The upload path template has the
    # form <target>-<version>.tar.*.
    toolchain_paths = GetToolchainSdkPaths(buildroot)
    if toolchain_paths:
      # Only add the path arg when processing the first tarball.  We do
      # this to get access to the tarball suffix dynamically (so it can
      # change and this code will still work).
      extra_args.extend([
          '--toolchain-upload-path',
          GetToolchainSdkUploadFormat(version, toolchain_paths[0][1])])
      for entry in toolchain_paths:
        extra_args.extend(['--toolchain-tarball', '%s:%s' % entry])

  if category == constants.CHROME_PFQ_TYPE:
    assert chrome_rev
    key = '%s_%s' % (chrome_rev, _CHROME_BINHOST)
    extra_args.extend(['--key', key.upper()])
  elif config_lib.IsPFQType(category):
    extra_args.extend(['--key', _PREFLIGHT_BINHOST])
  else:
    assert category in (constants.BUILD_FROM_SOURCE_TYPE,
                        constants.CHROOT_BUILDER_TYPE)
    extra_args.extend(['--key', _FULL_BINHOST])

  if category == constants.CHROME_PFQ_TYPE:
    extra_args += ['--packages=%s' % x
                   for x in ([constants.CHROME_PN] +
                             constants.OTHER_CHROME_PACKAGES)]

  kwargs.setdefault('extra_args', []).extend(extra_args)
  return _UploadPrebuilts(buildroot=buildroot, **kwargs)


class PackageFileMissing(Exception):
  """Raised when the dev installer package file is missing."""


def UploadDevInstallerPrebuilts(binhost_bucket, binhost_key, binhost_base_url,
                                buildroot, board, **kwargs):
  """Upload Prebuilts for dev-installer use case.

  Args:
    binhost_bucket: bucket for uploading prebuilt packages. If it equals None
                    then the default bucket is used.
    binhost_key: key parameter to pass onto upload_prebuilts. If it equals
                 None, then chrome_rev is used to select a default key.
    binhost_base_url: base url for upload_prebuilts. If None the parameter
                      --binhost-base-url is absent.
    buildroot: The root directory where the build occurs.
    board: Board type that was built on this machine.
    extra_args: Extra args to pass to prebuilts script.
  """
  extra_args = ['--prepend-version', constants.CANARY_TYPE]
  extra_args.extend(['--binhost-base-url', binhost_base_url])
  extra_args.extend(['--upload', binhost_bucket])
  extra_args.extend(['--key', binhost_key])

  filename = os.path.join(buildroot, 'chroot', 'build', board,
                          _BINHOST_PACKAGE_FILE.lstrip('/'))
  cmd_packages = _AddPackagesForPrebuilt(filename)
  if cmd_packages:
    extra_args.extend(cmd_packages)
  else:
    raise PackageFileMissing()

  kwargs.setdefault('extra_args', []).extend(extra_args)
  return _UploadPrebuilts(buildroot=buildroot, board=board, **kwargs)


def _UploadPrebuilts(buildroot, board, extra_args):
  """Upload prebuilts.

  Args:
    buildroot: The root directory where the build occurs.
    board: Board type that was built on this machine.
    extra_args: Extra args to pass to prebuilts script.
  """
  cmd = ['upload_prebuilts', '--build-path', buildroot]
  if board:
    cmd.extend(['--board', board])
  cmd.extend(extra_args)
  commands.RunBuildScript(buildroot, cmd, chromite_cmd=True)


class BinhostConfWriter(object):
  """Writes *BINHOST.conf commits on master, on behalf of slaves."""
  # TODO(mtennant): This class represents logic spun out from
  # UploadPrebuiltsStage that is specific to a master builder. This is
  # currently used by the Commit Queue and the Master PFQ builder, but
  # could be used by other master builders that upload prebuilts,
  # e.g., x86-alex-pre-flight-branch. When completed the
  # UploadPrebuiltsStage code can be thinned significantly.

  def __init__(self, builder_run):
    """BinhostConfWriter constructor.

    Args:
      builder_run: BuilderRun instance of the currently running build.
    """
    self._run = builder_run
    self._prebuilt_type = self._run.config.build_type
    self._chrome_rev = (self._run.options.chrome_rev or
                        self._run.config.chrome_rev)
    self._build_root = os.path.abspath(self._run.buildroot)

  def _GenerateCommonArgs(self):
    """Generate common prebuilt arguments."""
    generated_args = []
    if self._run.options.debug:
      generated_args.extend(['--debug', '--dry-run'])

    profile = self._run.options.profile or self._run.config['profile']
    if profile:
      generated_args.extend(['--profile', profile])

    # Generate the version if we are a manifest_version build.
    if self._run.config.manifest_version:
      version = self._run.GetVersion()
      generated_args.extend(['--set-version', version])

    return generated_args

  @staticmethod
  def _AddOptionsForSlave(slave_config):
    """Private helper method to add upload_prebuilts args for a slave builder.

    Args:
      slave_config: The build config of a slave builder.

    Returns:
      An array of options to add to upload_prebuilts array that allow a master
      to submit prebuilt conf modifications on behalf of a slave.
    """
    args = []
    if slave_config['prebuilts']:
      for slave_board in slave_config['boards']:
        args.extend(['--slave-board', slave_board])
        slave_profile = slave_config['profile']
        if slave_profile:
          args.extend(['--slave-profile', slave_profile])

    return args

  def Perform(self):
    """Write and commit *BINHOST.conf files."""
    # Common args we generate for all types of builds.
    generated_args = self._GenerateCommonArgs()
    # Args we specifically add for public/private build types.
    public_args, private_args = [], []
    # Gather public/private (slave) builders.
    public_builders, private_builders = [], []

    # Distributed builders that use manifest-versions to sync with one another
    # share prebuilt logic by passing around versions.
    assert config_lib.IsPFQType(self._prebuilt_type)

    # Public pfqs should upload host preflight prebuilts.
    public_args.append('--sync-host')

    # Update all the binhost conf files.
    generated_args.append('--sync-binhost-conf')

    slave_configs = self._run.site_config.GetSlavesForMaster(
        self._run.config, self._run.options)
    experimental_builders = self._run.attrs.metadata.GetValueWithDefault(
        constants.METADATA_EXPERIMENTAL_BUILDERS, [])
    for slave_config in slave_configs:
      if slave_config in experimental_builders:
        continue
      if slave_config['prebuilts'] == constants.PUBLIC:
        public_builders.append(slave_config['name'])
        public_args.extend(self._AddOptionsForSlave(slave_config))
      elif slave_config['prebuilts'] == constants.PRIVATE:
        private_builders.append(slave_config['name'])
        private_args.extend(self._AddOptionsForSlave(slave_config))

    # Upload the public prebuilts, if any.
    if public_builders:
      UploadPrebuilts(
          category=self._prebuilt_type, chrome_rev=self._chrome_rev,
          private_bucket=False, buildroot=self._build_root, board=None,
          extra_args=generated_args + public_args)

    # Upload the private prebuilts, if any.
    if private_builders:
      UploadPrebuilts(
          category=self._prebuilt_type, chrome_rev=self._chrome_rev,
          private_bucket=True, buildroot=self._build_root, board=None,
          extra_args=generated_args + private_args)

    # If we're the Chrome PFQ master, update our binhost JSON file.
    if self._run.config.build_type == constants.CHROME_PFQ_TYPE:
      commands.UpdateBinhostJson(self._build_root)
