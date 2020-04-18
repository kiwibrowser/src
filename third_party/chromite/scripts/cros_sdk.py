# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Manage SDK chroots.

This script is used for manipulating local chroot environments; creating,
deleting, downloading, etc.  If given --enter (or no args), it defaults
to an interactive bash shell within the chroot.

If given args those are passed to the chroot environment, and executed.
"""

from __future__ import print_function

import argparse
import glob
import os
import pwd
import random
import re
import resource
import sys
import urlparse

from chromite.lib import constants
from chromite.lib import cgroups
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import cros_sdk_lib
from chromite.lib import locking
from chromite.lib import namespaces
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import process_util
from chromite.lib import retry_util
from chromite.lib import toolchain

cros_build_lib.STRICT_SUDO = True


COMPRESSION_PREFERENCE = ('xz', 'bz2')

# TODO(zbehan): Remove the dependency on these, reimplement them in python
MAKE_CHROOT = [os.path.join(constants.SOURCE_ROOT,
                            'src/scripts/sdk_lib/make_chroot.sh')]
ENTER_CHROOT = [os.path.join(constants.SOURCE_ROOT,
                             'src/scripts/sdk_lib/enter_chroot.sh')]

# Proxy simulator configuration.
PROXY_HOST_IP = '192.168.240.1'
PROXY_PORT = 8080
PROXY_GUEST_IP = '192.168.240.2'
PROXY_NETMASK = 30
PROXY_VETH_PREFIX = 'veth'
PROXY_CONNECT_PORTS = (80, 443, 9418)
PROXY_APACHE_FALLBACK_USERS = ('www-data', 'apache', 'nobody')
PROXY_APACHE_MPMS = ('event', 'worker', 'prefork')
PROXY_APACHE_FALLBACK_PATH = ':'.join(
    '/usr/lib/apache2/mpm-%s' % mpm for mpm in PROXY_APACHE_MPMS
)
PROXY_APACHE_MODULE_GLOBS = ('/usr/lib*/apache2/modules', '/usr/lib*/apache2')

# We need these tools to run. Very common tools (tar,..) are omitted.
NEEDED_TOOLS = ('curl', 'xz')

# Tools needed for --proxy-sim only.
PROXY_NEEDED_TOOLS = ('ip',)

# Tools needed when use_image is true (the default).
IMAGE_NEEDED_TOOLS = ('losetup', 'lvchange', 'lvcreate', 'lvs', 'mke2fs',
                      'pvscan', 'thin_check', 'vgchange', 'vgcreate', 'vgs')

# As space is used inside the chroot, the empty space in chroot.img is
# allocated.  Deleting files inside the chroot doesn't automatically return the
# used space to the OS.  Over time, this tends to make the sparse chroot.img
# less sparse even if the chroot contents don't currently need much space.  We
# can recover most of this unused space with fstrim, but that takes too much
# time to run it every time. Instead, check the used space against the image
# size after mounting the chroot and only call fstrim if it looks like we could
# recover at least this many GiB.
MAX_UNUSED_IMAGE_GBS = 20


def GetArchStageTarballs(version):
  """Returns the URL for a given arch/version"""
  extension = {'bz2':'tbz2', 'xz':'tar.xz'}
  return [toolchain.GetSdkURL(suburl='cros-sdk-%s.%s'
                              % (version, extension[compressor]))
          for compressor in COMPRESSION_PREFERENCE]


def GetStage3Urls(version):
  return [toolchain.GetSdkURL(suburl='stage3-amd64-%s.tar.%s' % (version, ext))
          for ext in COMPRESSION_PREFERENCE]


def GetToolchainsOverlayUrls(version, toolchains):
  """Returns the URL(s) for a toolchains SDK overlay.

  Args:
    version: The SDK version used, e.g. 2015.05.27.145939. We use the year and
        month components to point to a subdirectory on the SDK bucket where
        overlays are stored (.../2015/05/ in this case).
    toolchains: Iterable of toolchain target strings (e.g. 'i686-pc-linux-gnu').

  Returns:
    List of alternative download URLs for an SDK overlay tarball that contains
    the given toolchains.
  """
  toolchains_desc = '-'.join(sorted(toolchains))
  suburl_template = os.path.join(
      *(version.split('.')[:2] +
        ['cros-sdk-overlay-toolchains-%s-%s.tar.%%s' %
         (toolchains_desc, version)]))
  return [toolchain.GetSdkURL(suburl=suburl_template % ext)
          for ext in COMPRESSION_PREFERENCE]


def FetchRemoteTarballs(storage_dir, urls, desc, allow_none=False):
  """Fetches a tarball given by url, and place it in |storage_dir|.

  Args:
    storage_dir: Path where to save the tarball.
    urls: List of URLs to try to download. Download will stop on first success.
    desc: A string describing what tarball we're downloading (for logging).
    allow_none: Don't fail if none of the URLs worked.

  Returns:
    Full path to the downloaded file, or None if |allow_none| and no URL worked.

  Raises:
    ValueError: If |allow_none| is False and none of the URLs worked.
  """

  # Note we track content length ourselves since certain versions of curl
  # fail if asked to resume a complete file.
  # pylint: disable=C0301,W0631
  # https://sourceforge.net/tracker/?func=detail&atid=100976&aid=3482927&group_id=976
  logging.notice('Downloading %s tarball...', desc)
  status_re = re.compile(r'^HTTP/[0-9]+(\.[0-9]+)? 200')
  for url in urls:
    # http://www.logilab.org/ticket/8766
    # pylint: disable=E1101
    parsed = urlparse.urlparse(url)
    tarball_name = os.path.basename(parsed.path)
    if parsed.scheme in ('', 'file'):
      if os.path.exists(parsed.path):
        return parsed.path
      continue
    content_length = 0
    logging.debug('Attempting download from %s', url)
    result = retry_util.RunCurl(
        ['-I', url], print_cmd=False, debug_level=logging.NOTICE,
        capture_output=True)
    successful = False
    for header in result.output.splitlines():
      # We must walk the output to find the 200 code for use cases where
      # a proxy is involved and may have pushed down the actual header.
      if status_re.match(header):
        successful = True
      elif header.lower().startswith('content-length:'):
        content_length = int(header.split(':', 1)[-1].strip())
        if successful:
          break
    if successful:
      break
  else:
    if allow_none:
      return None
    raise ValueError('No valid URLs found!')

  tarball_dest = os.path.join(storage_dir, tarball_name)
  current_size = 0
  if os.path.exists(tarball_dest):
    current_size = os.path.getsize(tarball_dest)
    if current_size > content_length:
      osutils.SafeUnlink(tarball_dest)
      current_size = 0

  if current_size < content_length:
    retry_util.RunCurl(
        ['--fail', '-L', '-y', '30', '-C', '-', '--output', tarball_dest, url],
        print_cmd=False, debug_level=logging.NOTICE)

  # Cleanup old tarballs now since we've successfull fetched; only cleanup
  # the tarballs for our prefix, or unknown ones. This gets a bit tricky
  # because we might have partial overlap between known prefixes.
  my_prefix = tarball_name.rsplit('-', 1)[0] + '-'
  all_prefixes = ('stage3-amd64-', 'cros-sdk-', 'cros-sdk-overlay-')
  ignored_prefixes = [prefix for prefix in all_prefixes if prefix != my_prefix]
  for filename in os.listdir(storage_dir):
    if (filename == tarball_name or
        any([(filename.startswith(p) and
              not (len(my_prefix) > len(p) and filename.startswith(my_prefix)))
             for p in ignored_prefixes])):
      continue
    logging.info('Cleaning up old tarball: %s', filename)
    osutils.SafeUnlink(os.path.join(storage_dir, filename))

  return tarball_dest


def CreateChroot(chroot_path, sdk_tarball, toolchains_overlay_tarball,
                 cache_dir, nousepkg=False):
  """Creates a new chroot from a given SDK.

  Args:
    chroot_path: Path where the new chroot will be created.
    sdk_tarball: Path to a downloaded Gentoo Stage3 or Chromium OS SDK tarball.
    toolchains_overlay_tarball: Optional path to a second tarball that will be
        unpacked into the chroot on top of the SDK tarball.
    cache_dir: Path to a directory that will be used for caching portage files,
        etc.
    nousepkg: If True, pass --nousepkg to cros_setup_toolchains inside the
        chroot.
  """

  cmd = MAKE_CHROOT + ['--stage3_path', sdk_tarball,
                       '--chroot', chroot_path,
                       '--cache_dir', cache_dir]

  if toolchains_overlay_tarball:
    cmd.extend(['--toolchains_overlay_path', toolchains_overlay_tarball])

  if nousepkg:
    cmd.append('--nousepkg')

  logging.notice('Creating chroot. This may take a few minutes...')
  try:
    cros_build_lib.RunCommand(cmd, print_cmd=False)
  except cros_build_lib.RunCommandError:
    raise SystemExit('Running %r failed!' % cmd)


def DeleteChroot(chroot_path):
  """Deletes an existing chroot"""
  cmd = MAKE_CHROOT + ['--chroot', chroot_path,
                       '--delete']
  try:
    logging.notice('Deleting chroot.')
    cros_build_lib.RunCommand(cmd, print_cmd=False)
  except cros_build_lib.RunCommandError:
    raise SystemExit('Running %r failed!' % cmd)


def EnterChroot(chroot_path, cache_dir, chrome_root, chrome_root_mount,
                workspace, goma_dir, goma_client_json, working_dir,
                additional_args):
  """Enters an existing SDK chroot"""
  st = os.statvfs(os.path.join(chroot_path, 'usr', 'bin', 'sudo'))
  # The os.ST_NOSUID constant wasn't added until python-3.2.
  if st.f_flag & 0x2:
    cros_build_lib.Die('chroot cannot be in a nosuid mount')

  cmd = ENTER_CHROOT + ['--chroot', chroot_path, '--cache_dir', cache_dir]
  if chrome_root:
    cmd.extend(['--chrome_root', chrome_root])
  if chrome_root_mount:
    cmd.extend(['--chrome_root_mount', chrome_root_mount])
  if workspace:
    cmd.extend(['--workspace_root', workspace])
  if goma_dir:
    cmd.extend(['--goma_dir', goma_dir])
  if goma_client_json:
    cmd.extend(['--goma_client_json', goma_client_json])
  if working_dir is not None:
    cmd.extend(['--working_dir', working_dir])

  if len(additional_args) > 0:
    cmd.append('--')
    cmd.extend(additional_args)

  # ThinLTO opens lots of files at the same time.
  resource.setrlimit(resource.RLIMIT_NOFILE, (32768, 32768))
  ret = cros_build_lib.RunCommand(cmd, print_cmd=False, error_code_ok=True,
                                  mute_output=False)
  # If we were in interactive mode, ignore the exit code; it'll be whatever
  # they last ran w/in the chroot and won't matter to us one way or another.
  # Note this does allow chroot entrance to fail and be ignored during
  # interactive; this is however a rare case and the user will immediately
  # see it (nor will they be checking the exit code manually).
  if ret.returncode != 0 and additional_args:
    raise SystemExit(ret.returncode)


def _ImageFileForChroot(chroot):
  """Find the image file that should be associated with |chroot|.

  This function does not check if the image exists; it simply returns the
  filename that would be used.

  Args:
    chroot: Path to the chroot.

  Returns:
    Path to an image file that would be associated with chroot.
  """
  return chroot.rstrip('/') + '.img'


def CreateChrootSnapshot(snapshot_name, chroot_vg, chroot_lv):
  """Create a snapshot for the specified chroot VG/LV.

  Args:
    snapshot_name: The name of the new snapshot.
    chroot_vg: The name of the VG containing the origin LV.
    chroot_lv: The name of the origin LV.

  Returns:
    True if the snapshot was created, or False if a snapshot with the same
    name already exists.

  Raises:
    SystemExit: The lvcreate command failed.
  """
  if snapshot_name in ListChrootSnapshots(chroot_vg, chroot_lv):
    logging.error('Cannot create snapshot %s: A volume with that name already '
                  'exists.', snapshot_name)
    return False

  cmd = ['lvcreate', '-s', '--name', snapshot_name, '%s/%s' % (
      chroot_vg, chroot_lv)]
  try:
    logging.notice('Creating snapshot %s from %s in VG %s.', snapshot_name,
                   chroot_lv, chroot_vg)
    cros_build_lib.RunCommand(cmd, print_cmd=False, capture_output=True)
    return True
  except cros_build_lib.RunCommandError:
    raise SystemExit('Running %r failed!' % cmd)


def DeleteChrootSnapshot(snapshot_name, chroot_vg, chroot_lv):
  """Delete the named snapshot from the specified chroot VG.

  If the requested snapshot is not found, nothing happens.  The main chroot LV
  and internal thinpool LV cannot be deleted with this function.

  Args:
    snapshot_name: The name of the snapshot to delete.
    chroot_vg: The name of the VG containing the origin LV.
    chroot_lv: The name of the origin LV.

  Raises:
    SystemExit: The lvremove command failed.
  """
  if snapshot_name in (cros_sdk_lib.CHROOT_LV_NAME,
                       cros_sdk_lib.CHROOT_THINPOOL_NAME):
    logging.error('Cannot remove LV %s as a snapshot.  Use cros_sdk --delete '
                  'if you want to remove the whole chroot.', snapshot_name)
    return

  if snapshot_name not in ListChrootSnapshots(chroot_vg, chroot_lv):
    return

  cmd = ['lvremove', '-f', '%s/%s' % (chroot_vg, snapshot_name)]
  try:
    logging.notice('Deleting snapshot %s in VG %s.', snapshot_name, chroot_vg)
    cros_build_lib.RunCommand(cmd, print_cmd=False, capture_output=True)
  except cros_build_lib.RunCommandError:
    raise SystemExit('Running %r failed!' % cmd)


def RestoreChrootSnapshot(snapshot_name, chroot_vg, chroot_lv):
  """Restore the chroot to an existing snapshot.

  This is done by renaming the original |chroot_lv| LV to a temporary name,
  renaming the snapshot named |snapshot_name| to |chroot_lv|, and deleting the
  now unused LV.  If an error occurs, attempts to rename the original snapshot
  back to |chroot_lv| to leave the chroot unchanged.

  The chroot must be unmounted before calling this function, and will be left
  unmounted after this function returns.

  Args:
    snapshot_name: The name of the snapshot to restore.  This snapshot will no
        longer be accessible at its original name after this function finishes.
    chroot_vg: The VG containing the chroot LV and snapshot LV.
    chroot_lv: The name of the original chroot LV.

  Returns:
    True if the chroot was restored to the requested snapshot, or False if
    the snapshot wasn't found or isn't valid.

  Raises:
    SystemExit: Any of the LVM commands failed.
  """
  valid_snapshots = ListChrootSnapshots(chroot_vg, chroot_lv)
  if (snapshot_name in (cros_sdk_lib.CHROOT_LV_NAME,
                        cros_sdk_lib.CHROOT_THINPOOL_NAME) or
      snapshot_name not in valid_snapshots):
    logging.error('Chroot cannot be restored to %s.  Valid snapshots: %s',
                  snapshot_name, ', '.join(valid_snapshots))
    return False

  backup_chroot_name = 'chroot-bak-%d' % random.randint(0, 1000)
  cmd = ['lvrename', chroot_vg, chroot_lv, backup_chroot_name]
  try:
    cros_build_lib.RunCommand(cmd, print_cmd=False, capture_output=True)
  except cros_build_lib.RunCommandError:
    raise SystemExit('Running %r failed!' % cmd)

  cmd = ['lvrename', chroot_vg, snapshot_name, chroot_lv]
  try:
    cros_build_lib.RunCommand(cmd, print_cmd=False, capture_output=True)
  except cros_build_lib.RunCommandError:
    cmd = ['lvrename', chroot_vg, backup_chroot_name, chroot_lv]
    try:
      cros_build_lib.RunCommand(cmd, print_cmd=False, capture_output=True)
    except cros_build_lib.RunCommandError:
      raise SystemExit('Failed to rename %s to chroot and failed to restore '
                       '%s back to chroot.  Failed command: %r' %
                       (snapshot_name, backup_chroot_name, cmd))
    raise SystemExit('Failed to rename %s to chroot.  Original chroot LV has '
                     'been restored.  Failed command: %r' %
                     (snapshot_name, cmd))

  # Some versions of LVM set snapshots to be skipped at auto-activate time.
  # Other versions don't have this flag at all.  We run lvchange to try
  # disabling auto-skip and activating the volume, but ignore errors.  Versions
  # that don't have the flag should be auto-activated.
  chroot_lv_path = '%s/%s' % (chroot_vg, chroot_lv)
  cmd = ['lvchange', '-kn', chroot_lv_path]
  cros_build_lib.RunCommand(cmd, print_cmd=False, capture_output=True,
                            error_code_ok=True)

  # Activate the LV in case the lvchange above was needed.  Activating an LV
  # that is already active shouldn't do anything, so this is safe to run even if
  # the -kn wasn't needed.
  cmd = ['lvchange', '-ay', chroot_lv_path]
  cros_build_lib.RunCommand(cmd, print_cmd=False, capture_output=True)

  cmd = ['lvremove', '-f', '%s/%s' % (chroot_vg, backup_chroot_name)]
  try:
    cros_build_lib.RunCommand(cmd, print_cmd=False, capture_output=True)
  except cros_build_lib.RunCommandError:
    raise SystemExit('Failed to remove backup LV %s/%s.  Failed command: %r' %
                     (chroot_vg, backup_chroot_name, cmd))

  return True


def ListChrootSnapshots(chroot_vg, chroot_lv):
  """Return all snapshots in |chroot_vg| regardless of origin volume.

  Args:
    chroot_vg: The name of the VG containing the chroot.
    chroot_lv: The name of the chroot LV.

  Returns:
    A (possibly-empty) list of snapshot LVs found in |chroot_vg|.

  Raises:
    SystemExit: The lvs command failed.
  """
  if not chroot_vg or not chroot_lv:
    return []

  cmd = ['lvs', '-o', 'lv_name,pool_lv,lv_attr', '-O', 'lv_name',
         '--noheadings', '--separator', '\t', chroot_vg]
  try:
    result = cros_build_lib.RunCommand(cmd, print_cmd=False,
                                       redirect_stdout=True)
  except cros_build_lib.RunCommandError:
    raise SystemExit('Running %r failed!' % cmd)

  # Once the thin origin volume has been deleted, there's no way to tell a
  # snapshot apart from any other volume.  Since this VG is created and managed
  # by cros_sdk, we'll assume that all volumes that share the same thin pool are
  # valid snapshots.
  snapshots = []
  snapshot_attrs = re.compile(r'^V.....t.{2,}')  # Matches a thin volume.
  for line in result.output.splitlines():
    lv_name, pool_lv, lv_attr = line.lstrip().split('\t')
    if (lv_name == chroot_lv or
        lv_name == cros_sdk_lib.CHROOT_THINPOOL_NAME or
        pool_lv != cros_sdk_lib.CHROOT_THINPOOL_NAME or
        not snapshot_attrs.match(lv_attr)):
      continue
    snapshots.append(lv_name)
  return snapshots


def _FindSubmounts(*args):
  """Find all mounts matching each of the paths in |args| and any submounts.

  Returns:
    A list of all matching mounts in the order found in /proc/mounts.
  """
  mounts = []
  paths = [p.rstrip('/') for p in args]
  for mtab in osutils.IterateMountPoints():
    for path in paths:
      if mtab.destination == path or mtab.destination.startswith(path + '/'):
        mounts.append(mtab.destination)
        break

  return mounts


def _SudoCommand():
  """Get the 'sudo' command, along with all needed environment variables."""

  # Pass in the ENVIRONMENT_WHITELIST and ENV_PASSTHRU variables so that
  # scripts in the chroot know what variables to pass through.
  cmd = ['sudo']
  for key in constants.CHROOT_ENVIRONMENT_WHITELIST + constants.ENV_PASSTHRU:
    value = os.environ.get(key)
    if value is not None:
      cmd += ['%s=%s' % (key, value)]

  # Pass in the path to the depot_tools so that users can access them from
  # within the chroot.
  cmd += ['DEPOT_TOOLS=%s' % constants.DEPOT_TOOLS_DIR]

  return cmd


def _ReportMissing(missing):
  """Report missing utilities, then exit.

  Args:
    missing: List of missing utilities, as returned by
             osutils.FindMissingBinaries.  If non-empty, will not return.
  """

  if missing:
    raise SystemExit(
        'The tool(s) %s were not found.\n'
        'Please install the appropriate package in your host.\n'
        'Example(ubuntu):\n'
        '  sudo apt-get install <packagename>'
        % ', '.join(missing))


def _ProxySimSetup(options):
  """Set up proxy simulator, and return only in the child environment.

  TODO: Ideally, this should support multiple concurrent invocations of
  cros_sdk --proxy-sim; currently, such invocations will conflict with each
  other due to the veth device names and IP addresses.  Either this code would
  need to generate fresh, unused names for all of these before forking, or it
  would need to support multiple concurrent cros_sdk invocations sharing one
  proxy and allowing it to exit when unused (without counting on any local
  service-management infrastructure on the host).
  """

  may_need_mpm = False
  apache_bin = osutils.Which('apache2')
  if apache_bin is None:
    apache_bin = osutils.Which('apache2', PROXY_APACHE_FALLBACK_PATH)
    if apache_bin is None:
      _ReportMissing(('apache2',))
  else:
    may_need_mpm = True

  # Module names and .so names included for ease of grepping.
  apache_modules = [('proxy_module', 'mod_proxy.so'),
                    ('proxy_connect_module', 'mod_proxy_connect.so'),
                    ('proxy_http_module', 'mod_proxy_http.so'),
                    ('proxy_ftp_module', 'mod_proxy_ftp.so')]

  # Find the apache module directory, and make sure it has the modules we need.
  module_dirs = {}
  for g in PROXY_APACHE_MODULE_GLOBS:
    for mod, so in apache_modules:
      for f in glob.glob(os.path.join(g, so)):
        module_dirs.setdefault(os.path.dirname(f), []).append(so)
  for apache_module_path, modules_found in module_dirs.iteritems():
    if len(modules_found) == len(apache_modules):
      break
  else:
    # Appease cros lint, which doesn't understand that this else block will not
    # fall through to the subsequent code which relies on apache_module_path.
    apache_module_path = None
    raise SystemExit(
        'Could not find apache module path containing all required modules: %s'
        % ', '.join(so for mod, so in apache_modules))

  def check_add_module(name):
    so = 'mod_%s.so' % name
    if os.access(os.path.join(apache_module_path, so), os.F_OK):
      mod = '%s_module' % name
      apache_modules.append((mod, so))
      return True
    return False

  check_add_module('authz_core')
  if may_need_mpm:
    for mpm in PROXY_APACHE_MPMS:
      if check_add_module('mpm_%s' % mpm):
        break

  veth_host = '%s-host' % PROXY_VETH_PREFIX
  veth_guest = '%s-guest' % PROXY_VETH_PREFIX

  # Set up locks to sync the net namespace setup.  We need the child to create
  # the net ns first, and then have the parent assign the guest end of the veth
  # interface to the child's new network namespace & bring up the proxy.  Only
  # then can the child move forward and rely on the network being up.
  ns_create_lock = locking.PipeLock()
  ns_setup_lock = locking.PipeLock()

  pid = os.fork()
  if not pid:
    # Create our new isolated net namespace.
    namespaces.Unshare(namespaces.CLONE_NEWNET)

    # Signal the parent the ns is ready to be configured.
    ns_create_lock.Post()
    del ns_create_lock

    # Wait for the parent to finish setting up the ns/proxy.
    ns_setup_lock.Wait()
    del ns_setup_lock

    # Set up child side of the network.
    commands = (
        ('ip', 'link', 'set', 'up', 'lo'),
        ('ip', 'address', 'add',
         '%s/%u' % (PROXY_GUEST_IP, PROXY_NETMASK),
         'dev', veth_guest),
        ('ip', 'link', 'set', veth_guest, 'up'),
    )
    try:
      for cmd in commands:
        cros_build_lib.RunCommand(cmd, print_cmd=False)
    except cros_build_lib.RunCommandError:
      raise SystemExit('Running %r failed!' % (cmd,))

    proxy_url = 'http://%s:%u' % (PROXY_HOST_IP, PROXY_PORT)
    for proto in ('http', 'https', 'ftp'):
      os.environ[proto + '_proxy'] = proxy_url
    for v in ('all_proxy', 'RSYNC_PROXY', 'no_proxy'):
      os.environ.pop(v, None)
    return

  # Set up parent side of the network.
  uid = int(os.environ.get('SUDO_UID', '0'))
  gid = int(os.environ.get('SUDO_GID', '0'))
  if uid == 0 or gid == 0:
    for username in PROXY_APACHE_FALLBACK_USERS:
      try:
        pwnam = pwd.getpwnam(username)
        uid, gid = pwnam.pw_uid, pwnam.pw_gid
        break
      except KeyError:
        continue
    if uid == 0 or gid == 0:
      raise SystemExit('Could not find a non-root user to run Apache as')

  chroot_parent, chroot_base = os.path.split(options.chroot)
  pid_file = os.path.join(chroot_parent, '.%s-apache-proxy.pid' % chroot_base)
  log_file = os.path.join(chroot_parent, '.%s-apache-proxy.log' % chroot_base)

  # Wait for the child to create the net ns.
  ns_create_lock.Wait()
  del ns_create_lock

  apache_directives = [
      'User #%u' % uid,
      'Group #%u' % gid,
      'PidFile %s' % pid_file,
      'ErrorLog %s' % log_file,
      'Listen %s:%u' % (PROXY_HOST_IP, PROXY_PORT),
      'ServerName %s' % PROXY_HOST_IP,
      'ProxyRequests On',
      'AllowCONNECT %s' % ' '.join(map(str, PROXY_CONNECT_PORTS)),
  ] + [
      'LoadModule %s %s' % (mod, os.path.join(apache_module_path, so))
      for (mod, so) in apache_modules
  ]
  commands = (
      ('ip', 'link', 'add', 'name', veth_host,
       'type', 'veth', 'peer', 'name', veth_guest),
      ('ip', 'address', 'add',
       '%s/%u' % (PROXY_HOST_IP, PROXY_NETMASK),
       'dev', veth_host),
      ('ip', 'link', 'set', veth_host, 'up'),
      ([apache_bin, '-f', '/dev/null'] +
       [arg for d in apache_directives for arg in ('-C', d)]),
      ('ip', 'link', 'set', veth_guest, 'netns', str(pid)),
  )
  cmd = None # Make cros lint happy.
  try:
    for cmd in commands:
      cros_build_lib.RunCommand(cmd, print_cmd=False)
  except cros_build_lib.RunCommandError:
    # Clean up existing interfaces, if any.
    cmd_cleanup = ('ip', 'link', 'del', veth_host)
    try:
      cros_build_lib.RunCommand(cmd_cleanup, print_cmd=False)
    except cros_build_lib.RunCommandError:
      logging.error('running %r failed', cmd_cleanup)
    raise SystemExit('Running %r failed!' % (cmd,))

  # Signal the child that the net ns/proxy is fully configured now.
  ns_setup_lock.Post()
  del ns_setup_lock

  process_util.ExitAsStatus(os.waitpid(pid, 0)[1])


def _ReExecuteIfNeeded(argv):
  """Re-execute cros_sdk as root.

  Also unshare the mount namespace so as to ensure that processes outside
  the chroot can't mess with our mounts.
  """
  if os.geteuid() != 0:
    cmd = _SudoCommand() + ['--'] + argv
    os.execvp(cmd[0], cmd)
  else:
    # We must set up the cgroups mounts before we enter our own namespace.
    # This way it is a shared resource in the root mount namespace.
    cgroups.Cgroup.InitSystem()


def _CreateParser(sdk_latest_version, bootstrap_latest_version):
  """Generate and return the parser with all the options."""
  usage = ('usage: %(prog)s [options] '
           '[VAR1=val1 ... VAR2=val2] [--] [command [args]]')
  parser = commandline.ArgumentParser(usage=usage, description=__doc__,
                                      caching=True)

  # Global options.
  default_chroot = os.path.join(constants.SOURCE_ROOT,
                                constants.DEFAULT_CHROOT_DIR)
  parser.add_argument(
      '--chroot', dest='chroot', default=default_chroot, type='path',
      help=('SDK chroot dir name [%s]' % constants.DEFAULT_CHROOT_DIR))
  parser.add_argument('--nouse-image', dest='use_image', action='store_false',
                      default=True,
                      help='Do not mount the chroot on a loopback image; '
                           'instead, create it directly in a directory.')

  parser.add_argument('--chrome_root', type='path',
                      help='Mount this chrome root into the SDK chroot')
  parser.add_argument('--chrome_root_mount', type='path',
                      help='Mount chrome into this path inside SDK chroot')
  parser.add_argument('--nousepkg', action='store_true', default=False,
                      help='Do not use binary packages when creating a chroot.')
  parser.add_argument('-u', '--url', dest='sdk_url',
                      help='Use sdk tarball located at this url. Use file:// '
                           'for local files.')
  parser.add_argument('--sdk-version',
                      help=('Use this sdk version.  For prebuilt, current is %r'
                            ', for bootstrapping it is %r.'
                            % (sdk_latest_version, bootstrap_latest_version)))
  parser.add_argument('--workspace',
                      help='Workspace directory to mount into the chroot.')
  parser.add_argument('--goma_dir', type='path',
                      help='Goma installed directory to mount into the chroot.')
  parser.add_argument('--goma_client_json', type='path',
                      help='Service account json file to use goma on bot. '
                           'Mounted into the chroot.')

  # Use type=str instead of type='path' to prevent the given path from being
  # transfered to absolute path automatically.
  parser.add_argument('--working-dir', type=str,
                      help='Run the command in specific working directory in '
                           'chroot.  If the given directory is a relative '
                           'path, this program will transfer the path to '
                           'the corresponding one inside chroot.')

  parser.add_argument('commands', nargs=argparse.REMAINDER)

  # SDK overlay tarball options (mutually exclusive).
  group = parser.add_mutually_exclusive_group()
  group.add_argument('--toolchains',
                     help=('Comma-separated list of toolchains we expect to be '
                           'using on the chroot. Used for downloading a '
                           'corresponding SDK toolchains group (if one is '
                           'found), which may speed up chroot initialization '
                           'when building for the first time. Otherwise this '
                           'has no effect and will not restrict the chroot in '
                           'any way. Ignored if using --bootstrap.'))
  group.add_argument('--board',
                     help=('The board we intend to be building in the chroot. '
                           'Used for deriving the list of required toolchains '
                           '(see --toolchains).'))

  # Commands.
  group = parser.add_argument_group('Commands')
  group.add_argument(
      '--enter', action='store_true', default=False,
      help='Enter the SDK chroot.  Implies --create.')
  group.add_argument(
      '--create', action='store_true', default=False,
      help='Create the chroot only if it does not already exist.  '
      'Implies --download.')
  group.add_argument(
      '--bootstrap', action='store_true', default=False,
      help='Build everything from scratch, including the sdk.  '
      'Use this only if you need to validate a change '
      'that affects SDK creation itself (toolchain and '
      'build are typically the only folk who need this).  '
      'Note this will quite heavily slow down the build.  '
      'This option implies --create --nousepkg.')
  group.add_argument(
      '-r', '--replace', action='store_true', default=False,
      help='Replace an existing SDK chroot.  Basically an alias '
      'for --delete --create.')
  group.add_argument(
      '--delete', action='store_true', default=False,
      help='Delete the current SDK chroot if it exists.')
  group.add_argument(
      '--download', action='store_true', default=False,
      help='Download the sdk.')
  group.add_argument(
      '--snapshot-create', metavar='SNAPSHOT_NAME',
      help='Create a snapshot of the chroot.  Requires that the chroot was '
           'created without the --nouse-image option.')
  group.add_argument(
      '--snapshot-restore', metavar='SNAPSHOT_NAME',
      help='Restore the chroot to a previously created snapshot.')
  group.add_argument(
      '--snapshot-delete', metavar='SNAPSHOT_NAME',
      help='Delete a previously created snapshot.  Deleting a snapshot that '
           'does not exist is not an error.')
  group.add_argument(
      '--snapshot-list', action='store_true', default=False,
      help='List existing snapshots of the chroot and exit.')
  commands = group

  # Namespace options.
  group = parser.add_argument_group('Namespaces')
  group.add_argument('--proxy-sim', action='store_true', default=False,
                     help='Simulate a restrictive network requiring an outbound'
                          ' proxy.')
  group.add_argument('--no-ns-pid', dest='ns_pid',
                     default=True, action='store_false',
                     help='Do not create a new PID namespace.')

  # Internal options.
  group = parser.add_argument_group(
      'Internal Chromium OS Build Team Options',
      'Caution: these are for meant for the Chromium OS build team only')
  group.add_argument('--buildbot-log-version', default=False,
                     action='store_true',
                     help='Log SDK version for buildbot consumption')

  return parser, commands


def main(argv):
  conf = cros_build_lib.LoadKeyValueFile(
      os.path.join(constants.SOURCE_ROOT, constants.SDK_VERSION_FILE),
      ignore_missing=True)
  sdk_latest_version = conf.get('SDK_LATEST_VERSION', '<unknown>')
  bootstrap_latest_version = conf.get('BOOTSTRAP_LATEST_VERSION', '<unknown>')
  parser, commands = _CreateParser(sdk_latest_version, bootstrap_latest_version)
  options = parser.parse_args(argv)
  chroot_command = options.commands

  # Some sanity checks first, before we ask for sudo credentials.
  cros_build_lib.AssertOutsideChroot()

  host = os.uname()[4]
  if host != 'x86_64':
    cros_build_lib.Die(
        "cros_sdk is currently only supported on x86_64; you're running"
        " %s.  Please find a x86_64 machine." % (host,))

  _ReportMissing(osutils.FindMissingBinaries(NEEDED_TOOLS))
  if options.proxy_sim:
    _ReportMissing(osutils.FindMissingBinaries(PROXY_NEEDED_TOOLS))
  missing_image_tools = osutils.FindMissingBinaries(IMAGE_NEEDED_TOOLS)

  if (sdk_latest_version == '<unknown>' or
      bootstrap_latest_version == '<unknown>'):
    cros_build_lib.Die(
        'No SDK version was found. '
        'Are you in a Chromium source tree instead of Chromium OS?\n\n'
        'Please change to a directory inside your Chromium OS source tree\n'
        'and retry.  If you need to setup a Chromium OS source tree, see\n'
        '  http://www.chromium.org/chromium-os/developer-guide')

  any_snapshot_operation = (options.snapshot_create or options.snapshot_restore
                            or options.snapshot_delete or options.snapshot_list)
  if any_snapshot_operation and not options.use_image:
    cros_build_lib.Die('Snapshot operations are not compatible with '
                       '--nouse-image.')

  if (options.snapshot_delete and options.snapshot_delete ==
      options.snapshot_restore):
    parser.error('Cannot --snapshot_delete the same snapshot you are '
                 'restoring with --snapshot_restore.')

  _ReExecuteIfNeeded([sys.argv[0]] + argv)

  lock_path = os.path.dirname(options.chroot)
  lock_path = os.path.join(
      lock_path, '.%s_lock' % os.path.basename(options.chroot).lstrip('.'))

  # Expand out the aliases...
  if options.replace:
    options.delete = options.create = True

  if options.bootstrap:
    options.create = True

  # If a command is not given, default to enter.
  # pylint: disable=protected-access
  # This _group_actions access sucks, but upstream decided to not include an
  # alternative to optparse's option_list, and this is what they recommend.
  options.enter |= not any(getattr(options, x.dest)
                           for x in commands._group_actions)
  # pylint: enable=protected-access
  options.enter |= bool(chroot_command)

  if (options.delete and not options.create and
      (options.enter or any_snapshot_operation)):
    parser.error("Trying to enter or snapshot the chroot when --delete "
                 "was specified makes no sense.")

  if options.working_dir is not None and not os.path.isabs(options.working_dir):
    options.working_dir = path_util.ToChrootPath(options.working_dir)

  # Clean up potential leftovers from previous interrupted builds.
  # TODO(bmgordon): Remove this at the end of 2017.  That should be long enough
  # to get rid of them all.
  chroot_build_path = options.chroot + '.build'
  if options.use_image and os.path.exists(chroot_build_path):
    try:
      with cgroups.SimpleContainChildren('cros_sdk'):
        with locking.FileLock(lock_path, 'chroot lock') as lock:
          logging.notice('Cleaning up leftover build directory %s',
                         chroot_build_path)
          lock.write_lock()
          osutils.UmountTree(chroot_build_path)
          osutils.RmDir(chroot_build_path)
    except cros_build_lib.RunCommandError as e:
      logging.warning('Unable to remove %s: %s', chroot_build_path, e)

  # Discern if we need to create the chroot.
  chroot_ver_file = os.path.join(options.chroot, 'etc', 'cros_chroot_version')
  chroot_exists = os.path.exists(chroot_ver_file)
  if (options.use_image and not chroot_exists and not options.delete and
      not missing_image_tools and
      os.path.exists(_ImageFileForChroot(options.chroot))):
    # Try to re-mount an existing image in case the user has rebooted.
    with cgroups.SimpleContainChildren('cros_sdk'):
      with locking.FileLock(lock_path, 'chroot lock') as lock:
        logging.debug('Checking if existing chroot image can be mounted.')
        lock.write_lock()
        cros_sdk_lib.MountChroot(options.chroot, create=False)
        chroot_exists = os.path.exists(chroot_ver_file)
        if chroot_exists:
          logging.notice('Mounted existing image %s on chroot',
                         _ImageFileForChroot(options.chroot))
  if (options.create or options.enter or options.snapshot_create or
      options.snapshot_restore):
    # Only create if it's being wiped, or if it doesn't exist.
    if not options.delete and chroot_exists:
      options.create = False
    else:
      options.download = True

  # Finally, flip create if necessary.
  if options.enter or options.snapshot_create:
    options.create |= not chroot_exists

  # Anything that needs to manipulate the main chroot mount or communicate with
  # LVM needs to be done here before we enter the new namespaces.

  # If deleting, do it regardless of the use_image flag so that a
  # previously-created loopback chroot can also be cleaned up.
  # TODO(bmgordon): See if the DeleteChroot call below can be removed in
  # favor of this block.
  chroot_deleted = False
  if options.delete:
    with cgroups.SimpleContainChildren('cros_sdk'):
      with locking.FileLock(lock_path, 'chroot lock') as lock:
        lock.write_lock()
        if missing_image_tools:
          logging.notice('Unmounting chroot.')
          osutils.UmountTree(options.chroot)
        else:
          logging.notice('Deleting chroot.')
          cros_sdk_lib.CleanupChrootMount(options.chroot, delete_image=True)
          osutils.RmDir(options.chroot, ignore_missing=True)
          chroot_deleted = True

  # Make sure the main chroot mount is visible.  Contents will be filled in
  # below if needed.
  if options.create and options.use_image:
    if missing_image_tools:
      raise SystemExit(
          '''The tool(s) %s were not found.
Please make sure the lvm2 and thin-provisioning-tools packages
are installed on your host.
Example(ubuntu):
  sudo apt-get install lvm2 thin-provisioning-tools

If you want to run without lvm2, pass --nouse-image (chroot
snapshots will be unavailable).''' % ', '.join(missing_image_tools))

    logging.debug('Making sure chroot image is mounted.')
    with cgroups.SimpleContainChildren('cros_sdk'):
      with locking.FileLock(lock_path, 'chroot lock') as lock:
        lock.write_lock()
        if not cros_sdk_lib.MountChroot(options.chroot, create=True):
          cros_build_lib.Die('Unable to mount %s on chroot',
                             _ImageFileForChroot(options.chroot))
        logging.notice('Mounted %s on chroot',
                       _ImageFileForChroot(options.chroot))

  # Snapshot operations will always need the VG/LV, but other actions won't.
  if any_snapshot_operation:
    with cgroups.SimpleContainChildren('cros_sdk'):
      with locking.FileLock(lock_path, 'chroot lock') as lock:
        chroot_vg, chroot_lv = cros_sdk_lib.FindChrootMountSource(
            options.chroot)
        if not chroot_vg or not chroot_lv:
          cros_build_lib.Die('Unable to find VG/LV for chroot %s',
                             options.chroot)

        # Delete snapshot before creating a new one.  This allows the user to
        # throw out old state, create a new snapshot, and enter the chroot in a
        # single call to cros_sdk.  Since restore involves deleting, also do it
        # before creating.
        if options.snapshot_restore:
          lock.write_lock()
          valid_snapshots = ListChrootSnapshots(chroot_vg, chroot_lv)
          if options.snapshot_restore not in valid_snapshots:
            cros_build_lib.Die('%s is not a valid snapshot to restore to. '
                               'Valid snapshots: %s', options.snapshot_restore,
                               ', '.join(valid_snapshots))
          osutils.UmountTree(options.chroot)
          if not RestoreChrootSnapshot(options.snapshot_restore, chroot_vg,
                                       chroot_lv):
            cros_build_lib.Die('Unable to restore chroot to snapshot.')
          if not cros_sdk_lib.MountChroot(options.chroot, create=False):
            cros_build_lib.Die('Unable to mount restored snapshot onto chroot.')

        # Use a read lock for snapshot delete and create even though they modify
        # the filesystem, because they don't modify the mounted chroot itself.
        # The underlying LVM commands take their own locks, so conflicting
        # concurrent operations here may crash cros_sdk, but won't corrupt the
        # chroot image.  This tradeoff seems worth it to allow snapshot
        # operations on chroots that have a process inside.
        if options.snapshot_delete:
          lock.read_lock()
          DeleteChrootSnapshot(options.snapshot_delete, chroot_vg, chroot_lv)

        if options.snapshot_create:
          lock.read_lock()
          if not CreateChrootSnapshot(options.snapshot_create, chroot_vg,
                                      chroot_lv):
            cros_build_lib.Die('Unable to create snapshot.')

  img_path = _ImageFileForChroot(options.chroot)
  if (options.use_image and os.path.exists(options.chroot) and
      os.path.exists(img_path)):
    img_stat = os.stat(img_path)
    img_used_bytes = img_stat.st_blocks * 512

    mount_stat = os.statvfs(options.chroot)
    mount_used_bytes = mount_stat.f_frsize * (mount_stat.f_blocks -
                                              mount_stat.f_bfree)

    extra_gbs = (img_used_bytes - mount_used_bytes) / 2**30
    if extra_gbs > MAX_UNUSED_IMAGE_GBS:
      logging.notice('%s is using %s GiB more than needed.  Running '
                     'fstrim.', img_path, extra_gbs)
      cmd = ['fstrim', options.chroot]
      try:
        cros_build_lib.RunCommand(cmd, print_cmd=False)
      except cros_build_lib.RunCommandError as e:
        logging.warning('Running fstrim failed. Consider running fstrim on '
                        'your chroot manually.\nError: %s', e)

  # Enter a new set of namespaces.  Everything after here cannot directly affect
  # the hosts's mounts or alter LVM volumes.
  namespaces.SimpleUnshare()
  if options.ns_pid:
    first_pid = namespaces.CreatePidNs()
  else:
    first_pid = None

  if options.snapshot_list:
    for snap in ListChrootSnapshots(chroot_vg, chroot_lv):
      print(snap)
    sys.exit(0)

  if not options.sdk_version:
    sdk_version = (bootstrap_latest_version if options.bootstrap
                   else sdk_latest_version)
  else:
    sdk_version = options.sdk_version
  if options.buildbot_log_version:
    logging.PrintBuildbotStepText(sdk_version)

  # Based on selections, determine the tarball to fetch.
  if options.download:
    if options.sdk_url:
      urls = [options.sdk_url]
    elif options.bootstrap:
      urls = GetStage3Urls(sdk_version)
    else:
      urls = GetArchStageTarballs(sdk_version)

  # Get URLs for the toolchains overlay, if one is to be used.
  toolchains_overlay_urls = None
  if not options.bootstrap:
    toolchains = None
    if options.toolchains:
      toolchains = options.toolchains.split(',')
    elif options.board:
      toolchains = toolchain.GetToolchainsForBoard(options.board).keys()

    if toolchains:
      toolchains_overlay_urls = GetToolchainsOverlayUrls(sdk_version,
                                                         toolchains)

  with cgroups.SimpleContainChildren('cros_sdk', pid=first_pid):
    with locking.FileLock(lock_path, 'chroot lock') as lock:
      toolchains_overlay_tarball = None

      if options.proxy_sim:
        _ProxySimSetup(options)

      if (options.delete and not chroot_deleted and
          (os.path.exists(options.chroot) or
           os.path.exists(_ImageFileForChroot(options.chroot)))):
        lock.write_lock()
        DeleteChroot(options.chroot)

      sdk_cache = os.path.join(options.cache_dir, 'sdks')
      distfiles_cache = os.path.join(options.cache_dir, 'distfiles')
      osutils.SafeMakedirsNonRoot(options.cache_dir)

      for target in (sdk_cache, distfiles_cache):
        src = os.path.join(constants.SOURCE_ROOT, os.path.basename(target))
        if not os.path.exists(src):
          osutils.SafeMakedirsNonRoot(target)
          continue
        lock.write_lock(
            "Upgrade to %r needed but chroot is locked; please exit "
            "all instances so this upgrade can finish." % src)
        if not os.path.exists(src):
          # Note that while waiting for the write lock, src may've vanished;
          # it's a rare race during the upgrade process that's a byproduct
          # of us avoiding taking a write lock to do the src check.  If we
          # took a write lock for that check, it would effectively limit
          # all cros_sdk for a chroot to a single instance.
          osutils.SafeMakedirsNonRoot(target)
        elif not os.path.exists(target):
          # Upgrade occurred, but a reversion, or something whacky
          # occurred writing to the old location.  Wipe and continue.
          os.rename(src, target)
        else:
          # Upgrade occurred once already, but either a reversion or
          # some before/after separate cros_sdk usage is at play.
          # Wipe and continue.
          osutils.RmDir(src)

      if options.download:
        lock.write_lock()
        sdk_tarball = FetchRemoteTarballs(
            sdk_cache, urls, 'stage3' if options.bootstrap else 'SDK')
        if toolchains_overlay_urls:
          toolchains_overlay_tarball = FetchRemoteTarballs(
              sdk_cache, toolchains_overlay_urls, 'SDK toolchains overlay',
              allow_none=True)

      if options.create:
        lock.write_lock()
        CreateChroot(options.chroot, sdk_tarball, toolchains_overlay_tarball,
                     options.cache_dir,
                     nousepkg=(options.bootstrap or options.nousepkg))

      if options.enter:
        lock.read_lock()
        EnterChroot(options.chroot, options.cache_dir, options.chrome_root,
                    options.chrome_root_mount, options.workspace,
                    options.goma_dir, options.goma_client_json,
                    options.working_dir, chroot_command)
