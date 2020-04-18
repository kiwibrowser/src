# -*- coding: utf-8 -*-
 # Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script is used to upload host prebuilts as well as board BINHOSTS.

Prebuilts are uploaded using gsutil to Google Storage. After these prebuilts
are successfully uploaded, a file is updated with the proper BINHOST version.

To read more about prebuilts/binhost binary packages please refer to:
http://goto/chromeos-prebuilts

Example of uploading prebuilt amd64 host files to Google Storage:
upload_prebuilts -p /b/cbuild/build -s -u gs://chromeos-prebuilt

Example of uploading x86-dogfood binhosts to Google Storage:
upload_prebuilts -b x86-dogfood -p /b/cbuild/build/ -u gs://chromeos-prebuilt -g
"""

from __future__ import print_function

import argparse
import datetime
import functools
import glob
import multiprocessing
import os
import tempfile

from chromite.lib import constants
from chromite.cbuildbot import commands
from chromite.lib import binpkg
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import portage_util
from chromite.lib import toolchain

# How many times to retry uploads.
_RETRIES = 10

# Multiplier for how long to sleep (in seconds) between retries; will delay
# (1*sleep) the first time, then (2*sleep), continuing via attempt * sleep.
_SLEEP_TIME = 60

# The length of time (in seconds) that Portage should wait before refetching
# binpkgs from the same binhost. We don't ever modify binhosts, so this should
# be something big.
_BINPKG_TTL = 60 * 60 * 24 * 365

_HOST_PACKAGES_PATH = 'chroot/var/lib/portage/pkgs'
_CATEGORIES_PATH = 'chroot/etc/portage/categories'
_PYM_PATH = 'chroot/usr/lib/portage/pym'
_HOST_ARCH = 'amd64'
_BOARD_PATH = 'chroot/build/%(board)s'
_REL_BOARD_PATH = 'board/%(target)s/%(version)s'
_REL_HOST_PATH = 'host/%(host_arch)s/%(target)s/%(version)s'
# Private overlays to look at for builds to filter
# relative to build path
_PRIVATE_OVERLAY_DIR = 'src/private-overlays'
_GOOGLESTORAGE_GSUTIL_FILE = 'googlestorage_acl.txt'
_BINHOST_BASE_URL = 'gs://chromeos-prebuilt'
_PREBUILT_BASE_DIR = 'src/third_party/chromiumos-overlay/chromeos/config/'
# Created in the event of new host targets becoming available
_PREBUILT_MAKE_CONF = {'amd64': os.path.join(_PREBUILT_BASE_DIR,
                                             'make.conf.amd64-host')}


class BuildTarget(object):
  """A board/variant/profile tuple."""

  def __init__(self, board_variant, profile=None):
    self.board_variant = board_variant
    self.board, _, self.variant = board_variant.partition('_')
    self.profile = profile

  def __str__(self):
    if self.profile:
      return '%s_%s' % (self.board_variant, self.profile)
    else:
      return self.board_variant

  def __eq__(self, other):
    return str(other) == str(self)

  def __hash__(self):
    return hash(str(self))


def UpdateLocalFile(filename, value, key='PORTAGE_BINHOST'):
  """Update the key in file with the value passed.

  File format:
    key="value"
  Note quotes are added automatically

  Args:
    filename: Name of file to modify.
    value: Value to write with the key.
    key: The variable key to update. (Default: PORTAGE_BINHOST)

  Returns:
    True if changes were made to the file.
  """
  if os.path.exists(filename):
    file_fh = open(filename)
  else:
    file_fh = open(filename, 'w+')
  file_lines = []
  found = False
  made_changes = False
  keyval_str = '%(key)s=%(value)s'
  for line in file_fh:
    # Strip newlines from end of line. We already add newlines below.
    line = line.rstrip("\n")

    if len(line.split('=')) != 2:
      # Skip any line that doesn't fit key=val.
      file_lines.append(line)
      continue

    file_var, file_val = line.split('=')
    if file_var == key:
      found = True
      print('Updating %s=%s to %s="%s"' % (file_var, file_val, key, value))
      value = '"%s"' % value
      made_changes |= (file_val != value)
      file_lines.append(keyval_str % {'key': key, 'value': value})
    else:
      file_lines.append(keyval_str % {'key': file_var, 'value': file_val})

  if not found:
    value = '"%s"' % value
    made_changes = True
    file_lines.append(keyval_str % {'key': key, 'value': value})

  file_fh.close()
  # write out new file
  osutils.WriteFile(filename, '\n'.join(file_lines) + '\n')
  return made_changes


def RevGitFile(filename, data, dryrun=False):
  """Update and push the git file.

  Args:
    filename: file to modify that is in a git repo already
    data: A dict of key/values to update in |filename|
    dryrun: If True, do not actually commit the change.
  """
  prebuilt_branch = 'prebuilt_branch'
  cwd = os.path.abspath(os.path.dirname(filename))
  commit = git.RunGit(cwd, ['rev-parse', 'HEAD']).output.rstrip()
  description = '%s: updating %s' % (os.path.basename(filename),
                                     ', '.join(data.keys()))
  # UpdateLocalFile will print out the keys/values for us.
  print('Revving git file %s' % filename)

  try:
    git.CreatePushBranch(prebuilt_branch, cwd)
    for key, value in data.iteritems():
      UpdateLocalFile(filename, value, key)
    git.RunGit(cwd, ['add', filename])
    git.RunGit(cwd, ['commit', '-m', description])
    git.PushBranch(prebuilt_branch, cwd, dryrun=dryrun)
  finally:
    # We reset the index and the working tree state in case there are any
    # uncommitted or pending changes, but we don't change any existing commits.
    git.RunGit(cwd, ['reset', '--hard'])

    # Check out the last good commit as a sanity fallback.
    git.RunGit(cwd, ['checkout', commit])


def GetVersion():
  """Get the version to put in LATEST and update the git version with."""
  return datetime.datetime.now().strftime('%Y.%m.%d.%H%M%S')


def _GsUpload(gs_context, acl, local_file, remote_file):
  """Upload to GS bucket.

  Args:
    gs_context: A lib.gs.GSContext instance.
    acl: The ACL to use for uploading the file.
    local_file: The local file to be uploaded.
    remote_file: The remote location to upload to.
  """
  CANNED_ACLS = ['public-read', 'private', 'bucket-owner-read',
                 'authenticated-read', 'bucket-owner-full-control',
                 'public-read-write']
  if acl in CANNED_ACLS:
    gs_context.Copy(local_file, remote_file, acl=acl)
  else:
    # For private uploads we assume that the overlay board is set up properly
    # and a googlestore_acl.xml is present. Otherwise, this script errors.
    # We set version=0 here to ensure that the ACL is set only once (see
    # http://b/15883752#comment54).
    try:
      gs_context.Copy(local_file, remote_file, version=0)
    except gs.GSContextPreconditionFailed as ex:
      # If we received a GSContextPreconditionFailed error, we know that the
      # file exists now, but we don't know whether our specific update
      # succeeded. See http://b/15883752#comment62
      logging.warning(
          'Assuming upload succeeded despite PreconditionFailed errors: %s', ex)

    if acl.endswith('.xml'):
      # Apply the passed in ACL xml file to the uploaded object.
      gs_context.SetACL(remote_file, acl=acl)
    else:
      gs_context.ChangeACL(remote_file, acl_args_file=acl)


def RemoteUpload(gs_context, acl, files, pool=10):
  """Upload to google storage.

  Create a pool of process and call _GsUpload with the proper arguments.

  Args:
    gs_context: A lib.gs.GSContext instance.
    acl: The canned acl used for uploading. acl can be one of: "public-read",
         "public-read-write", "authenticated-read", "bucket-owner-read",
         "bucket-owner-full-control", or "private".
    files: dictionary with keys to local files and values to remote path.
    pool: integer of maximum proesses to have at the same time.

  Returns:
    Return a set of tuple arguments of the failed uploads
  """
  upload = functools.partial(_GsUpload, gs_context, acl)
  tasks = [[key, value] for key, value in files.iteritems()]
  parallel.RunTasksInProcessPool(upload, tasks, pool)


def GenerateUploadDict(base_local_path, base_remote_path, pkgs):
  """Build a dictionary of local remote file key pairs to upload.

  Args:
    base_local_path: The base path to the files on the local hard drive.
    base_remote_path: The base path to the remote paths.
    pkgs: The packages to upload.

  Returns:
    Returns a dictionary of local_path/remote_path pairs
  """
  upload_files = {}
  for pkg in pkgs:
    suffix = pkg['CPV'] + '.tbz2'
    local_path = os.path.join(base_local_path, suffix)
    assert os.path.exists(local_path), '%s does not exist' % local_path
    upload_files[local_path] = os.path.join(base_remote_path, suffix)

    if pkg.get('DEBUG_SYMBOLS') == 'yes':
      debugsuffix = pkg['CPV'] + '.debug.tbz2'
      local_path = os.path.join(base_local_path, debugsuffix)
      assert os.path.exists(local_path)
      upload_files[local_path] = os.path.join(base_remote_path, debugsuffix)

  return upload_files


def GetBoardOverlay(build_path, target):
  """Get the path to the board variant.

  Args:
    build_path: The path to the root of the build directory
    target: The target board as a BuildTarget object.

  Returns:
    The last overlay configured for the given board as a string.
  """
  board = target.board_variant
  overlays = portage_util.FindOverlays(constants.BOTH_OVERLAYS, board,
                                       buildroot=build_path)
  # We only care about the last entry.
  return overlays[-1]


def DeterminePrebuiltConfFile(build_path, target):
  """Determine the prebuilt.conf file that needs to be updated for prebuilts.

  Args:
    build_path: The path to the root of the build directory
    target: String representation of the board. This includes host and board
      targets

  Returns:
    A string path to a prebuilt.conf file to be updated.
  """
  if _HOST_ARCH == target:
    # We are host.
    # Without more examples of hosts this is a kludge for now.
    # TODO(Scottz): as new host targets come online expand this to
    # work more like boards.
    make_path = _PREBUILT_MAKE_CONF[target]
  else:
    # We are a board
    board = GetBoardOverlay(build_path, target)
    make_path = os.path.join(board, 'prebuilt.conf')

  return make_path


def UpdateBinhostConfFile(path, key, value):
  """Update binhost config file file with key=value.

  Args:
    path: Filename to update.
    key: Key to update.
    value: New value for key.
  """
  cwd, filename = os.path.split(os.path.abspath(path))
  osutils.SafeMakedirs(cwd)
  if not git.GetCurrentBranch(cwd):
    git.CreatePushBranch(constants.STABLE_EBUILD_BRANCH, cwd, sync=False)
  osutils.WriteFile(path, '', mode='a')
  if UpdateLocalFile(path, value, key):
    desc = '%s: %s %s' % (filename, 'updating' if value else 'clearing', key)
    git.AddPath(path)
    git.Commit(cwd, desc)

def GenerateHtmlIndex(files, index, board, version, remote_location):
  """Given the list of |files|, generate an index.html at |index|.

  Args:
    files: The list of files to link to.
    index: The path to the html index.
    board: Name of the board this index is for.
    version: Build version this index is for.
    remote_location: Remote gs location prebuilts are uploaded to.
  """
  title = 'Package Prebuilt Index: %s / %s' % (board, version)

  files = files + [
      '.|Google Storage Index',
      '..|',
  ]
  commands.GenerateHtmlIndex(index, files, title=title,
                             url_base=gs.GsUrlToHttp(remote_location))


def _GrabAllRemotePackageIndexes(binhost_urls):
  """Grab all of the packages files associated with a list of binhost_urls.

  Args:
    binhost_urls: The URLs for the directories containing the Packages files we
                  want to grab.

  Returns:
    A list of PackageIndex objects.
  """
  pkg_indexes = []
  for url in binhost_urls:
    pkg_index = binpkg.GrabRemotePackageIndex(url)
    if pkg_index:
      pkg_indexes.append(pkg_index)
  return pkg_indexes


class PrebuiltUploader(object):
  """Synchronize host and board prebuilts."""

  def __init__(self, upload_location, acl, binhost_base_url, pkg_indexes,
               build_path, packages, skip_upload, binhost_conf_dir, dryrun,
               target, slave_targets, version):
    """Constructor for prebuilt uploader object.

    This object can upload host or prebuilt files to Google Storage.

    Args:
      upload_location: The upload location.
      acl: The canned acl used for uploading to Google Storage. acl can be one
           of: "public-read", "public-read-write", "authenticated-read",
           "bucket-owner-read", "bucket-owner-full-control", "project-private",
           or "private" (see "gsutil help acls"). If we are not uploading to
           Google Storage, this parameter is unused.
      binhost_base_url: The URL used for downloading the prebuilts.
      pkg_indexes: Old uploaded prebuilts to compare against. Instead of
          uploading duplicate files, we just link to the old files.
      build_path: The path to the directory containing the chroot.
      packages: Packages to upload.
      skip_upload: Don't actually upload the tarballs.
      binhost_conf_dir: Directory where to store binhost.conf files.
      dryrun: Don't push or upload prebuilts.
      target: BuildTarget managed by this builder.
      slave_targets: List of BuildTargets managed by slave builders.
      version: A unique string, intended to be included in the upload path,
          which identifies the version number of the uploaded prebuilts.
    """
    self._upload_location = upload_location
    self._acl = acl
    self._binhost_base_url = binhost_base_url
    self._pkg_indexes = pkg_indexes
    self._build_path = build_path
    self._packages = set(packages)
    self._found_packages = set()
    self._skip_upload = skip_upload
    self._binhost_conf_dir = binhost_conf_dir
    self._dryrun = dryrun
    self._target = target
    self._slave_targets = slave_targets
    self._version = version
    self._gs_context = gs.GSContext(retries=_RETRIES, sleep=_SLEEP_TIME,
                                    dry_run=self._dryrun)

  def _Upload(self, local_file, remote_file):
    """Wrapper around _GsUpload"""
    _GsUpload(self._gs_context, self._acl, local_file, remote_file)

  def _ShouldFilterPackage(self, pkg):
    if not self._packages:
      return False
    cpv = portage_util.SplitCPV(pkg['CPV'])
    cp = '%s/%s' % (cpv.category, cpv.package)
    self._found_packages.add(cp)
    return cpv.package not in self._packages and cp not in self._packages

  def _UploadPrebuilt(self, package_path, url_suffix):
    """Upload host or board prebuilt files to Google Storage space.

    Args:
      package_path: The path to the packages dir.
      url_suffix: The remote subdirectory where we should upload the packages.
    """
    # Process Packages file, removing duplicates and filtered packages.
    pkg_index = binpkg.GrabLocalPackageIndex(package_path)
    pkg_index.SetUploadLocation(self._binhost_base_url, url_suffix)
    pkg_index.RemoveFilteredPackages(self._ShouldFilterPackage)
    uploads = pkg_index.ResolveDuplicateUploads(self._pkg_indexes)
    unmatched_pkgs = self._packages - self._found_packages
    if unmatched_pkgs:
      logging.warning('unable to match packages: %r' % unmatched_pkgs)

    # Write Packages file.
    pkg_index.header['TTL'] = _BINPKG_TTL
    tmp_packages_file = pkg_index.WriteToNamedTemporaryFile()

    remote_location = '%s/%s' % (self._upload_location.rstrip('/'), url_suffix)
    assert remote_location.startswith('gs://')

    # Build list of files to upload. Manually include the dev-only files but
    # skip them if not present.
    # TODO(deymo): Upload dev-only-extras.tbz2 as dev-only-extras.tar.bz2
    # outside packages/ directory. See crbug.com/448178 for details.
    if os.path.exists(os.path.join(package_path, 'dev-only-extras.tbz2')):
      uploads.append({'CPV': 'dev-only-extras'})
    upload_files = GenerateUploadDict(package_path, remote_location, uploads)
    remote_file = '%s/Packages' % remote_location.rstrip('/')
    upload_files[tmp_packages_file.name] = remote_file

    RemoteUpload(self._gs_context, self._acl, upload_files)

    with tempfile.NamedTemporaryFile(
        prefix='chromite.upload_prebuilts.index.') as index:
      GenerateHtmlIndex(
          [x[len(remote_location) + 1:] for x in upload_files.values()],
          index.name, self._target, self._version, remote_location)
      self._Upload(index.name, '%s/index.html' % remote_location.rstrip('/'))

      link_name = 'Prebuilts[%s]: %s' % (self._target, self._version)
      url = '%s%s/index.html' % (gs.PUBLIC_BASE_HTTPS_URL,
                                 remote_location[len(gs.BASE_GS_URL):])
      logging.PrintBuildbotLink(link_name, url)

  def _UploadSdkTarball(self, board_path, url_suffix, prepackaged,
                        toolchains_overlay_tarballs,
                        toolchains_overlay_upload_path,
                        toolchain_tarballs, toolchain_upload_path):
    """Upload a tarball of the sdk at the specified path to Google Storage.

    Args:
      board_path: The path to the board dir.
      url_suffix: The remote subdirectory where we should upload the packages.
      prepackaged: If given, a tarball that has been packaged outside of this
                   script and should be used.
      toolchains_overlay_tarballs: List of toolchains overlay tarball
          specifications to upload. Items take the form
          "toolchains_spec:/path/to/tarball".
      toolchains_overlay_upload_path: Path template under the bucket to place
          toolchains overlay tarballs.
      toolchain_tarballs: List of toolchain tarballs to upload.
      toolchain_upload_path: Path under the bucket to place toolchain tarballs.
    """
    remote_location = '%s/%s' % (self._upload_location.rstrip('/'), url_suffix)
    assert remote_location.startswith('gs://')
    boardname = os.path.basename(board_path.rstrip('/'))
    # We do not upload non SDK board tarballs,
    assert boardname == constants.CHROOT_BUILDER_BOARD
    assert prepackaged is not None

    version_str = self._version[len('chroot-'):]
    remote_tarfile = toolchain.GetSdkURL(
        for_gsutil=True, suburl='cros-sdk-%s.tar.xz' % (version_str,))
    # For SDK, also upload the manifest which is guaranteed to exist
    # by the builderstage.
    self._Upload(prepackaged + '.Manifest', remote_tarfile + '.Manifest')
    self._Upload(prepackaged, remote_tarfile)

    # Upload SDK toolchains overlays and toolchain tarballs, if given.
    for tarball_list, upload_path, qualifier_name in (
        (toolchains_overlay_tarballs, toolchains_overlay_upload_path,
         'toolchains'),
        (toolchain_tarballs, toolchain_upload_path, 'target')):
      for tarball_spec in tarball_list:
        qualifier_val, local_path = tarball_spec.split(':')
        suburl = upload_path % {qualifier_name: qualifier_val}
        remote_path = toolchain.GetSdkURL(for_gsutil=True, suburl=suburl)
        self._Upload(local_path, remote_path)

    # Finally, also update the pointer to the latest SDK on which polling
    # scripts rely.
    with osutils.TempDir() as tmpdir:
      pointerfile = os.path.join(tmpdir, 'cros-sdk-latest.conf')
      remote_pointerfile = toolchain.GetSdkURL(for_gsutil=True,
                                               suburl='cros-sdk-latest.conf')
      osutils.WriteFile(pointerfile, 'LATEST_SDK="%s"' % version_str)
      self._Upload(pointerfile, remote_pointerfile)

  def _GetTargets(self):
    """Retuns the list of targets to use."""
    targets = self._slave_targets[:]
    if self._target:
      targets.append(self._target)

    return targets

  def SyncHostPrebuilts(self, key, git_sync, sync_binhost_conf):
    """Synchronize host prebuilt files.

    This function will sync both the standard host packages, plus the host
    packages associated with all targets that have been "setup" with the
    current host's chroot. For instance, if this host has been used to build
    x86-generic, it will sync the host packages associated with
    'i686-pc-linux-gnu'. If this host has also been used to build arm-generic,
    it will also sync the host packages associated with
    'armv7a-cros-linux-gnueabi'.

    Args:
      key: The variable key to update in the git file.
      git_sync: If set, update make.conf of target to reference the latest
          prebuilt packages generated here.
      sync_binhost_conf: If set, update binhost config file in
          chromiumos-overlay for the host.
    """
    # Slave boards are listed before the master board so that the master board
    # takes priority (i.e. x86-generic preflight host prebuilts takes priority
    # over preflight host prebuilts from other builders.)
    binhost_urls = []
    for target in self._GetTargets():
      url_suffix = _REL_HOST_PATH % {'version': self._version,
                                     'host_arch': _HOST_ARCH,
                                     'target': target}
      packages_url_suffix = '%s/packages' % url_suffix.rstrip('/')

      if self._target == target and not self._skip_upload:
        # Upload prebuilts.
        package_path = os.path.join(self._build_path, _HOST_PACKAGES_PATH)
        self._UploadPrebuilt(package_path, packages_url_suffix)

      # Record URL where prebuilts were uploaded.
      binhost_urls.append('%s/%s/' % (self._binhost_base_url.rstrip('/'),
                                      packages_url_suffix.rstrip('/')))

    binhost = ' '.join(binhost_urls)
    if git_sync:
      git_file = os.path.join(self._build_path, _PREBUILT_MAKE_CONF[_HOST_ARCH])
      RevGitFile(git_file, {key: binhost}, dryrun=self._dryrun)
    if sync_binhost_conf:
      binhost_conf = os.path.join(
          self._binhost_conf_dir, 'host', '%s-%s.conf' % (_HOST_ARCH, key))
      UpdateBinhostConfFile(binhost_conf, key, binhost)

  def SyncBoardPrebuilts(self, key, git_sync, sync_binhost_conf,
                         upload_board_tarball, prepackaged_board,
                         toolchains_overlay_tarballs,
                         toolchains_overlay_upload_path,
                         toolchain_tarballs, toolchain_upload_path):
    """Synchronize board prebuilt files.

    Args:
      key: The variable key to update in the git file.
      git_sync: If set, update make.conf of target to reference the latest
          prebuilt packages generated here.
      sync_binhost_conf: If set, update binhost config file in
          chromiumos-overlay for the current board.
      upload_board_tarball: Include a tarball of the board in our upload.
      prepackaged_board: A tarball of the board built outside of this script.
      toolchains_overlay_tarballs: List of toolchains overlay tarball
          specifications to upload. Items take the form
          "toolchains_spec:/path/to/tarball".
      toolchains_overlay_upload_path: Path template under the bucket to place
          toolchains overlay tarballs.
      toolchain_tarballs: A list of toolchain tarballs to upload.
      toolchain_upload_path: Path under the bucket to place toolchain tarballs.
    """
    updated_binhosts = set()
    for target in self._GetTargets():
      board_path = os.path.join(self._build_path,
                                _BOARD_PATH % {'board': target.board_variant})
      package_path = os.path.join(board_path, 'packages')
      url_suffix = _REL_BOARD_PATH % {'target': target,
                                      'version': self._version}
      packages_url_suffix = '%s/packages' % url_suffix.rstrip('/')

      # Process the target board differently if it is the main --board.
      if self._target == target and not self._skip_upload:
        # This strips "chroot" prefix because that is sometimes added as the
        # --prepend-version argument (e.g. by chromiumos-sdk bot).
        # TODO(build): Clean it up to be less hard-coded.
        version_str = self._version[len('chroot-'):]

        # Upload board tarballs in the background.
        if upload_board_tarball:
          if toolchain_upload_path:
            toolchain_upload_path %= {'version': version_str}
          if toolchains_overlay_upload_path:
            toolchains_overlay_upload_path %= {'version': version_str}
          tar_process = multiprocessing.Process(
              target=self._UploadSdkTarball,
              args=(board_path, url_suffix, prepackaged_board,
                    toolchains_overlay_tarballs,
                    toolchains_overlay_upload_path, toolchain_tarballs,
                    toolchain_upload_path))
          tar_process.start()

        # Upload prebuilts.
        self._UploadPrebuilt(package_path, packages_url_suffix)

        # Make sure we finished uploading the board tarballs.
        if upload_board_tarball:
          tar_process.join()
          assert tar_process.exitcode == 0

      # Record URL where prebuilts were uploaded.
      url_value = '%s/%s/' % (self._binhost_base_url.rstrip('/'),
                              packages_url_suffix.rstrip('/'))

      if git_sync:
        git_file = DeterminePrebuiltConfFile(self._build_path, target)
        RevGitFile(git_file, {key: url_value}, dryrun=self._dryrun)

      if sync_binhost_conf:
        # Update the binhost configuration file in git.
        binhost_conf = os.path.join(
            self._binhost_conf_dir, 'target', '%s-%s.conf' % (target, key))
        updated_binhosts.add(binhost_conf)
        UpdateBinhostConfFile(binhost_conf, key, url_value)

    if sync_binhost_conf:
      # Clear all old binhosts. The files must be left empty in case anybody
      # is referring to them.
      all_binhosts = set(glob.glob(os.path.join(
          self._binhost_conf_dir, 'target', '*-%s.conf' % key)))
      for binhost_conf in all_binhosts - updated_binhosts:
        UpdateBinhostConfFile(binhost_conf, key, '')


class _AddSlaveBoardAction(argparse.Action):
  """Callback that adds a slave board to the list of slave targets."""
  def __call__(self, parser, namespace, values, option_string=None):
    getattr(namespace, self.dest).append(BuildTarget(values))


class _AddSlaveProfileAction(argparse.Action):
  """Callback that adds a slave profile to the list of slave targets."""
  def __call__(self, parser, namespace, values, option_string=None):
    if not namespace.slave_targets:
      parser.error('Must specify --slave-board before --slave-profile')
    if namespace.slave_targets[-1].profile is not None:
      parser.error('Cannot specify --slave-profile twice for same board')
    namespace.slave_targets[-1].profile = values


def ParseOptions(argv):
  """Returns options given by the user and the target specified.

  Args:
    argv: The args to parse.

  Returns:
    A tuple containing a parsed options object and BuildTarget.
    The target instance is None if no board is specified.
  """
  parser = commandline.ArgumentParser()
  parser.add_argument('-H', '--binhost-base-url', default=_BINHOST_BASE_URL,
                      help='Base URL to use for binhost in make.conf updates')
  parser.add_argument('--previous-binhost-url', action='append', default=[],
                      help='Previous binhost URL')
  parser.add_argument('-b', '--board',
                      help='Board type that was built on this machine')
  parser.add_argument('-B', '--prepackaged-tarball', type='path',
                      help='Board tarball prebuilt outside of this script.')
  parser.add_argument('--toolchains-overlay-tarball',
                      dest='toolchains_overlay_tarballs',
                      action='append', default=[],
                      help='Toolchains overlay tarball specification to '
                           'upload. Takes the form '
                           '"toolchains_spec:/path/to/tarball".')
  parser.add_argument('--toolchains-overlay-upload-path', default='',
                      help='Path template for uploading toolchains overlays.')
  parser.add_argument('--toolchain-tarball', dest='toolchain_tarballs',
                      action='append', default=[],
                      help='Redistributable toolchain tarball.')
  parser.add_argument('--toolchain-upload-path', default='',
                      help='Path to place toolchain tarballs in the sdk tree.')
  parser.add_argument('--profile',
                      help='Profile that was built on this machine')
  parser.add_argument('--slave-board', default=[], action=_AddSlaveBoardAction,
                      dest='slave_targets',
                      help='Board type that was built on a slave machine. To '
                           'add a profile to this board, use --slave-profile.')
  parser.add_argument('--slave-profile', action=_AddSlaveProfileAction,
                      help='Board profile that was built on a slave machine. '
                           'Applies to previous slave board.')
  parser.add_argument('-p', '--build-path', required=True,
                      help='Path to the directory containing the chroot')
  parser.add_argument('--packages', action='append', default=[],
                      help='Only include the specified packages. '
                           '(Default is to include all packages.)')
  parser.add_argument('-s', '--sync-host', default=False, action='store_true',
                      help='Sync host prebuilts')
  parser.add_argument('-g', '--git-sync', default=False, action='store_true',
                      help='Enable git version sync (This commits to a repo.) '
                           'This is used by full builders to commit directly '
                           'to board overlays.')
  parser.add_argument('-u', '--upload',
                      help='Upload location')
  parser.add_argument('-V', '--prepend-version',
                      help='Add an identifier to the front of the version')
  parser.add_argument('-f', '--filters', action='store_true', default=False,
                      help='Turn on filtering of private ebuild packages')
  parser.add_argument('-k', '--key', default='PORTAGE_BINHOST',
                      help='Key to update in make.conf / binhost.conf')
  parser.add_argument('--set-version',
                      help='Specify the version string')
  parser.add_argument('--sync-binhost-conf', default=False, action='store_true',
                      help='Update binhost.conf in chromiumos-overlay or '
                           'chromeos-overlay. Commit the changes, but don\'t '
                           'push them. This is used for preflight binhosts.')
  parser.add_argument('--binhost-conf-dir',
                      help='Directory to commit binhost config with '
                           '--sync-binhost-conf.')
  parser.add_argument('-P', '--private', action='store_true', default=False,
                      help='Mark gs:// uploads as private.')
  parser.add_argument('--skip-upload', action='store_true', default=False,
                      help='Skip upload step.')
  parser.add_argument('--upload-board-tarball', action='store_true',
                      default=False,
                      help='Upload board tarball to Google Storage.')
  parser.add_argument('-n', '--dry-run', dest='dryrun',
                      action='store_true', default=False,
                      help='Don\'t push or upload prebuilts.')

  options = parser.parse_args(argv)
  if not options.upload and not options.skip_upload:
    parser.error('you need to provide an upload location using -u')
  if not options.set_version and options.skip_upload:
    parser.error('If you are using --skip-upload, you must specify a '
                 'version number using --set-version.')

  target = None
  if options.board:
    target = BuildTarget(options.board, options.profile)

  if target in options.slave_targets:
    parser.error('--board/--profile must not also be a slave target.')

  if len(set(options.slave_targets)) != len(options.slave_targets):
    parser.error('--slave-boards must not have duplicates.')

  if options.slave_targets and options.git_sync:
    parser.error('--slave-boards is not compatible with --git-sync')

  if (options.upload_board_tarball and options.skip_upload and
      options.board == 'amd64-host'):
    parser.error('--skip-upload is not compatible with '
                 '--upload-board-tarball and --board=amd64-host')

  if (options.upload_board_tarball and not options.skip_upload and
      not options.upload.startswith('gs://')):
    parser.error('--upload-board-tarball only works with gs:// URLs.\n'
                 '--upload must be a gs:// URL.')

  if options.upload_board_tarball and options.prepackaged_tarball is None:
    parser.error('--upload-board-tarball requires --prepackaged-tarball')

  if options.private:
    if options.sync_host:
      parser.error('--private and --sync-host/-s cannot be specified '
                   'together; we do not support private host prebuilts')

    if not options.upload or not options.upload.startswith('gs://'):
      parser.error('--private is only valid for gs:// URLs; '
                   '--upload must be a gs:// URL.')

    if options.binhost_base_url != _BINHOST_BASE_URL:
      parser.error('when using --private the --binhost-base-url '
                   'is automatically derived.')

  if options.sync_binhost_conf and not options.binhost_conf_dir:
    parser.error('--sync-binhost-conf requires --binhost-conf-dir')

  if (options.toolchains_overlay_tarballs and
      not options.toolchains_overlay_upload_path):
    parser.error('--toolchains-overlay-tarball requires '
                 '--toolchains-overlay-upload-path')

  return options, target


def main(argv):
  # Set umask to a sane value so that files created as root are readable.
  os.umask(0o22)

  options, target = ParseOptions(argv)

  # Calculate a list of Packages index files to compare against. Whenever we
  # upload a package, we check to make sure it's not already stored in one of
  # the packages files we uploaded. This list of packages files might contain
  # both board and host packages.
  pkg_indexes = _GrabAllRemotePackageIndexes(options.previous_binhost_url)

  if options.set_version:
    version = options.set_version
  else:
    version = GetVersion()

  if options.prepend_version:
    version = '%s-%s' % (options.prepend_version, version)

  acl = 'public-read'
  binhost_base_url = options.binhost_base_url

  if options.private:
    binhost_base_url = options.upload
    if target:
      acl = portage_util.FindOverlayFile(_GOOGLESTORAGE_GSUTIL_FILE,
                                         board=target.board_variant,
                                         buildroot=options.build_path)
      if acl is None:
        cros_build_lib.Die('No Google Storage ACL file %s found in %s overlay.',
                           _GOOGLESTORAGE_GSUTIL_FILE, target.board_variant)

  binhost_conf_dir = None
  if options.binhost_conf_dir:
    binhost_conf_dir = os.path.join(options.build_path,
                                    options.binhost_conf_dir)

  uploader = PrebuiltUploader(options.upload, acl, binhost_base_url,
                              pkg_indexes, options.build_path,
                              options.packages, options.skip_upload,
                              binhost_conf_dir, options.dryrun,
                              target, options.slave_targets, version)

  if options.sync_host:
    uploader.SyncHostPrebuilts(options.key, options.git_sync,
                               options.sync_binhost_conf)

  if options.board or options.slave_targets:
    uploader.SyncBoardPrebuilts(options.key, options.git_sync,
                                options.sync_binhost_conf,
                                options.upload_board_tarball,
                                options.prepackaged_tarball,
                                options.toolchains_overlay_tarballs,
                                options.toolchains_overlay_upload_path,
                                options.toolchain_tarballs,
                                options.toolchain_upload_path)
