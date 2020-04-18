# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script manages the installed toolchains in the chroot."""

from __future__ import print_function

import errno
import glob
import hashlib
import json
import os
import re

from chromite.lib import constants
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import toolchain

# Needs to be after chromite imports.
import lddtree

if cros_build_lib.IsInsideChroot():
  # Only import portage after we've checked that we're inside the chroot.
  # Outside may not have portage, in which case the above may not happen.
  # We'll check in main() if the operation needs portage.
  # pylint: disable=F0401
  import portage


EMERGE_CMD = os.path.join(constants.CHROMITE_BIN_DIR, 'parallel_emerge')
PACKAGE_STABLE = '[stable]'

CHROMIUMOS_OVERLAY = '/usr/local/portage/chromiumos'
ECLASS_OVERLAY = '/usr/local/portage/eclass-overlay'
STABLE_OVERLAY = '/usr/local/portage/stable'
CROSSDEV_OVERLAY = '/usr/local/portage/crossdev'


# The exact list of host toolchain packages we care about.  These are the
# packages that bots/devs install only from binpkgs and rely on the SDK bot
# (chromiumos-sdk) to validate+uprev.
#
# We don't use crossdev to manage the host toolchain for us, especially since
# we diverge significantly now (with llvm/clang/etc...), and we don't need or
# want crossdev managing /etc/portage config files for the sdk
HOST_PACKAGES = (
    'dev-lang/go',
    'dev-libs/elfutils',
    'sys-devel/binutils',
    'sys-devel/clang',
    'sys-devel/gcc',
    'sys-devel/lld',
    'sys-devel/llvm',
    'sys-kernel/linux-headers',
    'sys-libs/glibc',
    'sys-libs/libcxx',
    'sys-libs/libcxxabi',
)

# These packages are also installed into the host SDK.  However, they require
# the cross-compilers to be installed first (because they need them to actually
# build), so we have to delay their installation.
HOST_POST_CROSS_PACKAGES = (
    'dev-lang/rust',
    'dev-util/cargo',
    'virtual/target-sdk-post-cross',
)

# New packages that we're in the process of adding to the SDK.  Since the SDK
# bot hasn't had a chance to run yet, there are no binary packages available,
# so we have to list them here and wait.  Once it completes, entries here can
# be removed so they'll end up on bots & dev's systems.
NEW_PACKAGES = (
)

# Enable the Go compiler for these targets.
TARGET_GO_ENABLED = (
    'x86_64-cros-linux-gnu',
    'armv7a-cros-linux-gnueabi',
    'armv7a-cros-linux-gnueabihf',
    'aarch64-cros-linux-gnu',
)
CROSSDEV_GO_ARGS = ['--ex-pkg', 'dev-lang/go']

# Enable llvm's compiler-rt for these targets.
TARGET_COMPILER_RT_ENABLED = (
    'armv7a-cros-linux-gnueabi',
    'armv7a-cros-linux-gnueabihf',
    'aarch64-cros-linux-gnu',
    'armv7m-cros-eabi',
)
CROSSDEV_COMPILER_RT_ARGS = ['--ex-pkg', 'sys-libs/compiler-rt']

TARGET_LLVM_PKGS_ENABLED = (
    'armv7a-cros-linux-gnueabi',
    'armv7a-cros-linux-gnueabihf',
    'aarch64-cros-linux-gnu',
    'x86_64-cros-linux-gnu',
)

LLVM_PKGS_TABLE = {
    'ex_libcxxabi' : ['--ex-pkg', 'sys-libs/libcxxabi'],
    'ex_libcxx' : ['--ex-pkg', 'sys-libs/libcxx'],
}

# Overrides for {gcc,binutils}-config, pick a package with particular suffix.
CONFIG_TARGET_SUFFIXES = {
    'binutils' : {
        'armv6j-cros-linux-gnueabi': '-gold',
        'armv7a-cros-linux-gnueabi': '-gold',
        'armv7a-cros-linux-gnueabihf': '-gold',
        'i686-pc-linux-gnu' : '-gold',
        'x86_64-cros-linux-gnu' : '-gold',
    },
}


class Crossdev(object):
  """Class for interacting with crossdev and caching its output."""

  _CACHE_FILE = os.path.join(CROSSDEV_OVERLAY, '.configured.json')
  _CACHE = {}
  # Packages that needs separate handling, in addition to what we have from
  # crossdev.
  MANUAL_PKGS = {
      'clang': 'sys-devel',
      'lld': 'sys-devel',
      'llvm': 'sys-devel',
      'libcxxabi': 'sys-libs',
      'libcxx': 'sys-libs',
      'elfutils': 'dev-libs',
  }

  @classmethod
  def Load(cls, reconfig):
    """Load crossdev cache from disk.

    We invalidate the cache when crossdev updates or this script changes.
    """
    crossdev_version = GetStablePackageVersion('sys-devel/crossdev', True)
    # If we run the compiled/cached .pyc file, we'll read/hash that when we
    # really always want to track the source .py file.
    script = os.path.abspath(__file__)
    if script.endswith('.pyc'):
      script = script[:-1]
    setup_toolchains_hash = hashlib.md5(osutils.ReadFile(script)).hexdigest()

    cls._CACHE = {
        'crossdev_version': crossdev_version,
        'setup_toolchains_hash': setup_toolchains_hash,
    }

    logging.debug('cache: checking file: %s', cls._CACHE_FILE)
    if reconfig:
      logging.debug('cache: forcing regen due to reconfig')
      return

    try:
      file_data = osutils.ReadFile(cls._CACHE_FILE)
    except IOError as e:
      if e.errno != errno.ENOENT:
        logging.warning('cache: reading failed: %s', e)
        osutils.SafeUnlink(cls._CACHE_FILE)
      return

    try:
      data = json.loads(file_data)
    except ValueError as e:
      logging.warning('cache: ignoring invalid content: %s', e)
      return

    if crossdev_version != data.get('crossdev_version'):
      logging.debug('cache: rebuilding after crossdev upgrade')
    elif setup_toolchains_hash != data.get('setup_toolchains_hash'):
      logging.debug('cache: rebuilding after cros_setup_toolchains change')
    else:
      logging.debug('cache: content is up-to-date!')
      cls._CACHE = data

  @classmethod
  def Save(cls):
    """Store crossdev cache on disk."""
    # Save the cache from the successful run.
    with open(cls._CACHE_FILE, 'w') as f:
      json.dump(cls._CACHE, f)

  @classmethod
  def GetConfig(cls, target):
    """Returns a map of crossdev provided variables about a tuple."""
    CACHE_ATTR = '_target_tuple_map'

    val = cls._CACHE.setdefault(CACHE_ATTR, {})
    if not target in val:
      if target.startswith('host'):
        conf = {
            'crosspkgs': [],
            'target': toolchain.GetHostTuple(),
        }
        if target == 'host':
          packages_list = HOST_PACKAGES
        else:
          packages_list = HOST_POST_CROSS_PACKAGES
        manual_pkgs = dict((pkg, cat) for cat, pkg in
                           [x.split('/') for x in packages_list])
      else:
        # Build the crossdev command.
        cmd = ['crossdev', '--show-target-cfg', '--ex-gdb']
        if target in TARGET_COMPILER_RT_ENABLED:
          cmd.extend(CROSSDEV_COMPILER_RT_ARGS)
        if target in TARGET_GO_ENABLED:
          cmd.extend(CROSSDEV_GO_ARGS)
        if target in TARGET_LLVM_PKGS_ENABLED:
          for pkg in LLVM_PKGS_TABLE:
            cmd.extend(LLVM_PKGS_TABLE[pkg])
        cmd.extend(['-t', target])
        # Catch output of crossdev.
        out = cros_build_lib.RunCommand(
            cmd, print_cmd=False, redirect_stdout=True).output.splitlines()
        # List of tuples split at the first '=', converted into dict.
        conf = dict((k, cros_build_lib.ShellUnquote(v))
                    for k, v in (x.split('=', 1) for x in out))
        conf['crosspkgs'] = conf['crosspkgs'].split()

        manual_pkgs = cls.MANUAL_PKGS

      for pkg, cat in manual_pkgs.items():
        conf[pkg + '_pn'] = pkg
        conf[pkg + '_category'] = cat
        if pkg not in conf['crosspkgs']:
          conf['crosspkgs'].append(pkg)

      val[target] = conf

    return val[target]

  @classmethod
  def UpdateTargets(cls, targets, usepkg, config_only=False):
    """Calls crossdev to initialize a cross target.

    Args:
      targets: The list of targets to initialize using crossdev.
      usepkg: Copies the commandline opts.
      config_only: Just update.
    """
    configured_targets = cls._CACHE.setdefault('configured_targets', [])

    cmdbase = ['crossdev', '--show-fail-log']
    cmdbase.extend(['--env', 'FEATURES=splitdebug'])
    # Pick stable by default, and override as necessary.
    cmdbase.extend(['-P', '--oneshot'])
    if usepkg:
      cmdbase.extend(['-P', '--getbinpkg',
                      '-P', '--usepkgonly',
                      '--without-headers'])

    overlays = ' '.join((CHROMIUMOS_OVERLAY, ECLASS_OVERLAY, STABLE_OVERLAY))
    cmdbase.extend(['--overlays', overlays])
    cmdbase.extend(['--ov-output', CROSSDEV_OVERLAY])

    # Build target by the reversed alphabetical order to make sure
    # armv7a-cros-linux-gnueabihf builds before armv7a-cros-linux-gnueabi
    # because some dependency issue. This can be reverted once we
    # migrated to armv7a-cros-linux-gnueabihf. crbug.com/711369
    for target in sorted(targets, reverse=True):
      if config_only and target in configured_targets:
        continue

      cmd = cmdbase + ['-t', target]

      for pkg in GetTargetPackages(target):
        if pkg == 'gdb':
          # Gdb does not have selectable versions.
          cmd.append('--ex-gdb')
        elif pkg == 'ex_compiler-rt':
          cmd.extend(CROSSDEV_COMPILER_RT_ARGS)
        elif pkg == 'ex_go':
          # Go does not have selectable versions.
          cmd.extend(CROSSDEV_GO_ARGS)
        elif pkg in LLVM_PKGS_TABLE:
          cmd.extend(LLVM_PKGS_TABLE[pkg])
        elif pkg in cls.MANUAL_PKGS:
          pass
        else:
          # The first of the desired versions is the "primary" one.
          version = GetDesiredPackageVersions(target, pkg)[0]
          cmd.extend(['--%s' % pkg, version])

      cmd.extend(targets[target]['crossdev'].split())
      if config_only:
        # In this case we want to just quietly reinit
        cmd.append('--init-target')
        cros_build_lib.RunCommand(cmd, print_cmd=False, redirect_stdout=True)
      else:
        cros_build_lib.RunCommand(cmd)

      configured_targets.append(target)


def GetTargetPackages(target):
  """Returns a list of packages for a given target."""
  conf = Crossdev.GetConfig(target)
  # Undesired packages are denoted by empty ${pkg}_pn variable.
  return [x for x in conf['crosspkgs'] if conf.get(x+'_pn')]


# Portage helper functions:
def GetPortagePackage(target, package):
  """Returns a package name for the given target."""
  conf = Crossdev.GetConfig(target)
  # Portage category:
  if target.startswith('host') or package in Crossdev.MANUAL_PKGS:
    category = conf[package + '_category']
  else:
    category = conf['category']
  # Portage package:
  pn = conf[package + '_pn']
  # Final package name:
  assert category
  assert pn
  return '%s/%s' % (category, pn)


def PortageTrees(root):
  """Return the portage trees for a given root."""
  if root == '/':
    return portage.db['/']
  # The portage logic requires the path always end in a slash.
  root = root.rstrip('/') + '/'
  return portage.create_trees(target_root=root, config_root=root)[root]


def GetInstalledPackageVersions(atom, root='/'):
  """Extracts the list of current versions of a target, package pair.

  Args:
    atom: The atom to operate on (e.g. sys-devel/gcc)
    root: The root to check for installed packages.

  Returns:
    The list of versions of the package currently installed.
  """
  versions = []
  # pylint: disable=E1101
  for pkg in PortageTrees(root)['vartree'].dbapi.match(atom, use_cache=0):
    version = portage.versions.cpv_getversion(pkg)
    versions.append(version)
  return versions


def GetStablePackageVersion(atom, installed, root='/'):
  """Extracts the current stable version for a given package.

  Args:
    atom: The target/package to operate on eg. i686-pc-linux-gnu,gcc
    installed: Whether we want installed packages or ebuilds
    root: The root to use when querying packages.

  Returns:
    A string containing the latest version.
  """
  pkgtype = 'vartree' if installed else 'porttree'
  # pylint: disable=E1101
  cpv = portage.best(PortageTrees(root)[pkgtype].dbapi.match(atom, use_cache=0))
  return portage.versions.cpv_getversion(cpv) if cpv else None


def VersionListToNumeric(target, package, versions, installed, root='/'):
  """Resolves keywords in a given version list for a particular package.

  Resolving means replacing PACKAGE_STABLE with the actual number.

  Args:
    target: The target to operate on (e.g. i686-pc-linux-gnu)
    package: The target/package to operate on (e.g. gcc)
    versions: List of versions to resolve
    installed: Query installed packages
    root: The install root to use; ignored if |installed| is False.

  Returns:
    List of purely numeric versions equivalent to argument
  """
  resolved = []
  atom = GetPortagePackage(target, package)
  if not installed:
    root = '/'
  for version in versions:
    if version == PACKAGE_STABLE:
      resolved.append(GetStablePackageVersion(atom, installed, root=root))
    else:
      resolved.append(version)
  return resolved


def GetDesiredPackageVersions(target, package):
  """Produces the list of desired versions for each target, package pair.

  The first version in the list is implicitly treated as primary, ie.
  the version that will be initialized by crossdev and selected.

  If the version is PACKAGE_STABLE, it really means the current version which
  is emerged by using the package atom with no particular version key.
  Since crossdev unmasks all packages by default, this will actually
  mean 'unstable' in most cases.

  Args:
    target: The target to operate on (e.g. i686-pc-linux-gnu)
    package: The target/package to operate on (e.g. gcc)

  Returns:
    A list composed of either a version string, PACKAGE_STABLE
  """
  if package in GetTargetPackages(target):
    return [PACKAGE_STABLE]
  else:
    return []


def TargetIsInitialized(target):
  """Verifies if the given list of targets has been correctly initialized.

  This determines whether we have to call crossdev while emerging
  toolchain packages or can do it using emerge. Emerge is naturally
  preferred, because all packages can be updated in a single pass.

  Args:
    target: The target to operate on (e.g. i686-pc-linux-gnu)

  Returns:
    True if |target| is completely initialized, else False
  """
  # Check if packages for the given target all have a proper version.
  try:
    for package in GetTargetPackages(target):
      atom = GetPortagePackage(target, package)
      # Do we even want this package && is it initialized?
      if not (GetStablePackageVersion(atom, True) and
              GetStablePackageVersion(atom, False)):
        return False
    return True
  except cros_build_lib.RunCommandError:
    # Fails - The target has likely never been initialized before.
    return False


def RemovePackageMask(target):
  """Removes a package.mask file for the given platform.

  The pre-existing package.mask files can mess with the keywords.

  Args:
    target: The target to operate on (e.g. i686-pc-linux-gnu)
  """
  maskfile = os.path.join('/etc/portage/package.mask', 'cross-' + target)
  osutils.SafeUnlink(maskfile)


# Main functions performing the actual update steps.
def RebuildLibtool(root='/'):
  """Rebuild libtool as needed

  Libtool hardcodes full paths to internal gcc files, so whenever we upgrade
  gcc, libtool will break.  We can't use binary packages either as those will
  most likely be compiled against the previous version of gcc.

  Args:
    root: The install root where we want libtool rebuilt.
  """
  needs_update = False
  with open(os.path.join(root, 'usr/bin/libtool')) as f:
    for line in f:
      # Look for a line like:
      #   sys_lib_search_path_spec="..."
      # It'll be a list of paths and gcc will be one of them.
      if line.startswith('sys_lib_search_path_spec='):
        line = line.rstrip()
        for path in line.split('=', 1)[1].strip('"').split():
          root_path = os.path.join(root, path.lstrip(os.path.sep))
          logging.debug('Libtool: checking %s', root_path)
          if not os.path.exists(root_path):
            logging.info('Rebuilding libtool after gcc upgrade')
            logging.info(' %s', line)
            logging.info(' missing path: %s', path)
            needs_update = True
            break

      if needs_update:
        break

  if needs_update:
    cmd = [EMERGE_CMD, '--oneshot']
    if root != '/':
      cmd.extend(['--sysroot=%s' % root, '--root=%s' % root])
    cmd.append('sys-devel/libtool')
    cros_build_lib.RunCommand(cmd)
  else:
    logging.debug('Libtool is up-to-date; no need to rebuild')


def UpdateTargets(targets, usepkg, root='/'):
  """Determines which packages need update/unmerge and defers to portage.

  Args:
    targets: The list of targets to update
    usepkg: Copies the commandline option
    root: The install root in which we want packages updated.
  """
  # For each target, we do two things. Figure out the list of updates,
  # and figure out the appropriate keywords/masks. Crossdev will initialize
  # these, but they need to be regenerated on every update.
  logging.info('Determining required toolchain updates...')
  mergemap = {}
  for target in targets:
    logging.debug('Updating target %s', target)
    # Record the highest needed version for each target, for masking purposes.
    RemovePackageMask(target)
    for package in GetTargetPackages(target):
      # Portage name for the package
      logging.debug('   Checking package %s', package)
      pkg = GetPortagePackage(target, package)
      current = GetInstalledPackageVersions(pkg, root=root)
      desired = GetDesiredPackageVersions(target, package)
      desired_num = VersionListToNumeric(target, package, desired, False)
      if pkg in NEW_PACKAGES and usepkg:
        # Skip this binary package (for now).
        continue
      mergemap[pkg] = set(desired_num).difference(current)
      logging.debug('      %s -> %s', current, desired_num)

  packages = [pkg for pkg, vers in mergemap.items() if vers]
  if not packages:
    logging.info('Nothing to update!')
    return False

  logging.info('Updating packages:')
  logging.info('%s', packages)

  cmd = [EMERGE_CMD, '--oneshot', '--update']
  if usepkg:
    cmd.extend(['--getbinpkg', '--usepkgonly'])
  if root != '/':
    cmd.extend(['--sysroot=%s' % root, '--root=%s' % root])

  cmd.extend(packages)
  cros_build_lib.RunCommand(cmd)
  return True


def CleanTargets(targets, root='/'):
  """Unmerges old packages that are assumed unnecessary.

  Args:
    targets: The list of targets to clean up.
    root: The install root in which we want packages cleaned up.
  """
  unmergemap = {}
  for target in targets:
    logging.debug('Cleaning target %s', target)
    for package in GetTargetPackages(target):
      logging.debug('   Cleaning package %s', package)
      pkg = GetPortagePackage(target, package)
      current = GetInstalledPackageVersions(pkg, root=root)
      desired = GetDesiredPackageVersions(target, package)
      # NOTE: This refers to installed packages (vartree) rather than the
      # Portage version (porttree and/or bintree) when determining the current
      # version. While this isn't the most accurate thing to do, it is probably
      # a good simple compromise, which should have the desired result of
      # uninstalling everything but the latest installed version. In
      # particular, using the bintree (--usebinpkg) requires a non-trivial
      # binhost sync and is probably more complex than useful.
      desired_num = VersionListToNumeric(target, package, desired, True)
      if not set(desired_num).issubset(current):
        logging.warning('Error detecting stable version for %s, '
                        'skipping clean!', pkg)
        return
      unmergemap[pkg] = set(current).difference(desired_num)

  # Cleaning doesn't care about consistency and rebuilding package.* files.
  packages = []
  for pkg, vers in unmergemap.iteritems():
    packages.extend('=%s-%s' % (pkg, ver) for ver in vers if ver != '9999')

  if packages:
    logging.info('Cleaning packages:')
    logging.info('%s', packages)
    cmd = [EMERGE_CMD, '--unmerge']
    if root != '/':
      cmd.extend(['--sysroot=%s' % root, '--root=%s' % root])
    cmd.extend(packages)
    cros_build_lib.RunCommand(cmd)
  else:
    logging.info('Nothing to clean!')


def SelectActiveToolchains(targets, suffixes, root='/'):
  """Runs gcc-config and binutils-config to select the desired.

  Args:
    targets: The targets to select
    suffixes: Optional target-specific hacks
    root: The root where we want to select toolchain versions.
  """
  for package in ['gcc', 'binutils']:
    for target in targets:
      # See if this package is part of this target.
      if package not in GetTargetPackages(target):
        logging.debug('%s: %s is not used', target, package)
        continue

      # Pick the first version in the numbered list as the selected one.
      desired = GetDesiredPackageVersions(target, package)
      desired_num = VersionListToNumeric(target, package, desired, True,
                                         root=root)
      desired = desired_num[0]
      # *-config does not play revisions, strip them, keep just PV.
      desired = portage.versions.pkgsplit('%s-%s' % (package, desired))[1]

      if target.startswith('host'):
        # *-config is the only tool treating host identically (by tuple).
        target = toolchain.GetHostTuple()

      # And finally, attach target to it.
      desired = '%s-%s' % (target, desired)

      # Target specific hacks
      if package in suffixes:
        if target in suffixes[package]:
          desired += suffixes[package][target]

      extra_env = {'CHOST': target}
      if root != '/':
        extra_env['ROOT'] = root
      cmd = ['%s-config' % package, '-c', target]
      result = cros_build_lib.RunCommand(
          cmd, print_cmd=False, redirect_stdout=True, extra_env=extra_env)
      current = result.output.splitlines()[0]

      # Do not reconfig when the current is live or nothing needs to be done.
      extra_env = {'ROOT': root} if root != '/' else None
      if current != desired and current != '9999':
        cmd = [package + '-config', desired]
        cros_build_lib.RunCommand(cmd, print_cmd=False, extra_env=extra_env)


def ExpandTargets(targets_wanted):
  """Expand any possible toolchain aliases into full targets

  This will expand 'all' and 'sdk' into the respective toolchain tuples.

  Args:
    targets_wanted: The targets specified by the user.

  Returns:
    Dictionary of concrete targets and their toolchain tuples.
  """
  targets_wanted = set(targets_wanted)
  if targets_wanted == set(['boards']):
    # Only pull targets from the included boards.
    return {}

  all_targets = toolchain.GetAllTargets()
  if targets_wanted == set(['all']):
    return all_targets
  if targets_wanted == set(['sdk']):
    # Filter out all the non-sdk toolchains as we don't want to mess
    # with those in all of our builds.
    return toolchain.FilterToolchains(all_targets, 'sdk', True)

  # Verify user input.
  nonexistent = targets_wanted.difference(all_targets)
  if nonexistent:
    raise ValueError('Invalid targets: %s', ','.join(nonexistent))
  return {t: all_targets[t] for t in targets_wanted}


def UpdateToolchains(usepkg, deleteold, hostonly, reconfig,
                     targets_wanted, boards_wanted, root='/'):
  """Performs all steps to create a synchronized toolchain enviroment.

  Args:
    usepkg: Use prebuilt packages
    deleteold: Unmerge deprecated packages
    hostonly: Only setup the host toolchain
    reconfig: Reload crossdev config and reselect toolchains
    targets_wanted: All the targets to update
    boards_wanted: Load targets from these boards
    root: The root in which to install the toolchains.
  """
  targets, crossdev_targets, reconfig_targets = {}, {}, {}
  if not hostonly:
    # For hostonly, we can skip most of the below logic, much of which won't
    # work on bare systems where this is useful.
    targets = ExpandTargets(targets_wanted)

    # Now re-add any targets that might be from this board. This is to
    # allow unofficial boards to declare their own toolchains.
    for board in boards_wanted:
      targets.update(toolchain.GetToolchainsForBoard(board))

    # First check and initialize all cross targets that need to be.
    for target in targets:
      if TargetIsInitialized(target):
        reconfig_targets[target] = targets[target]
      else:
        crossdev_targets[target] = targets[target]
    if crossdev_targets:
      logging.info('The following targets need to be re-initialized:')
      logging.info('%s', crossdev_targets)
      Crossdev.UpdateTargets(crossdev_targets, usepkg)
    # Those that were not initialized may need a config update.
    Crossdev.UpdateTargets(reconfig_targets, usepkg, config_only=True)

    # If we're building a subset of toolchains for a board, we might not have
    # all the tuples that the packages expect.  We don't define the "full" set
    # of tuples currently other than "whatever the full sdk has normally".
    if usepkg or set(('all', 'sdk')) & targets_wanted:
      # Since we have cross-compilers now, we can update these packages.
      targets['host-post-cross'] = {}

  # We want host updated.
  targets['host'] = {}

  # Now update all packages.
  if UpdateTargets(targets, usepkg, root=root) or crossdev_targets or reconfig:
    SelectActiveToolchains(targets, CONFIG_TARGET_SUFFIXES, root=root)

  if deleteold:
    CleanTargets(targets, root=root)

  # Now that we've cleared out old versions, see if we need to rebuild
  # anything.  Can't do this earlier as it might not be broken.
  RebuildLibtool(root=root)


def ShowConfig(name):
  """Show the toolchain tuples used by |name|

  Args:
    name: The board name to query.
  """
  toolchains = toolchain.GetToolchainsForBoard(name)
  # Make sure we display the default toolchain first.
  # Note: Do not use logging here as this is meant to be used by other tools.
  print(','.join(
      toolchain.FilterToolchains(toolchains, 'default', True).keys() +
      toolchain.FilterToolchains(toolchains, 'default', False).keys()))


def GeneratePathWrapper(root, wrappath, path):
  """Generate a shell script to execute another shell script

  Since we can't symlink a wrapped ELF (see GenerateLdsoWrapper) because the
  argv[0] won't be pointing to the correct path, generate a shell script that
  just executes another program with its full path.

  Args:
    root: The root tree to generate scripts inside of
    wrappath: The full path (inside |root|) to create the wrapper
    path: The target program which this wrapper will execute
  """
  replacements = {
      'path': path,
      'relroot': os.path.relpath('/', os.path.dirname(wrappath)),
  }
  wrapper = """#!/bin/sh
base=$(realpath "$0")
basedir=${base%%/*}
exec "${basedir}/%(relroot)s%(path)s" "$@"
""" % replacements
  root_wrapper = root + wrappath
  if os.path.islink(root_wrapper):
    os.unlink(root_wrapper)
  else:
    osutils.SafeMakedirs(os.path.dirname(root_wrapper))
  osutils.WriteFile(root_wrapper, wrapper)
  os.chmod(root_wrapper, 0o755)


def FixClangXXWrapper(root, path):
  """Fix wrapper shell scripts and symlinks for invoking clang++

  In a typical installation, clang++ symlinks to clang, which symlinks to the
  elf executable. The executable distinguishes between clang and clang++ based
  on argv[0].

  When invoked through the LdsoWrapper, argv[0] always contains the path to the
  executable elf file, making clang/clang++ invocations indistinguishable.

  This function detects if the elf executable being wrapped is clang-X.Y, and
  fixes wrappers/symlinks as necessary so that clang++ will work correctly.

  The calling sequence now becomes:
  -) clang++ invocation turns into clang++-3.9 (which is a copy of clang-3.9,
     the Ldsowrapper).
  -) clang++-3.9 uses the Ldso to invoke clang++-3.9.elf, which is a symlink
     to the original clang-3.9 elf.
  -) The difference this time is that inside the elf file execution, $0 is
     set as .../usr/bin/clang++-3.9.elf, which contains 'clang++' in the name.

  Update: Starting since clang 7, the clang and clang++ are symlinks to
  clang-7 binary, not clang-7.0. The pattern match is extended to handle
  both clang-7 and clang-7.0 cases for now. (https://crbug.com/837889)

  Args:
    root: The root tree to generate scripts / symlinks inside of
    path: The target elf for which LdsoWrapper was created
  """
  if re.match(r'/usr/bin/clang-\d+(\.\d+)*$', path):
    logging.info('fixing clang++ invocation for %s', path)
    clangdir = os.path.dirname(root + path)
    clang = os.path.basename(path)
    clangxx = clang.replace('clang', 'clang++')

    # Create a symlink clang++-X.Y.elf to point to clang-X.Y.elf
    os.symlink(clang + '.elf', os.path.join(clangdir, clangxx + '.elf'))

    # Create a hardlink clang++-X.Y pointing to clang-X.Y
    os.link(os.path.join(clangdir, clang), os.path.join(clangdir, clangxx))

    # Adjust the clang++ symlink to point to clang++-X.Y
    os.unlink(os.path.join(clangdir, 'clang++'))
    os.symlink(clangxx, os.path.join(clangdir, 'clang++'))


def FileIsCrosSdkElf(elf):
  """Determine if |elf| is an ELF that we execute in the cros_sdk

  We don't need this to be perfect, just quick.  It makes sure the ELF
  is a 64bit LSB x86_64 ELF.  That is the native type of cros_sdk.

  Args:
    elf: The file to check

  Returns:
    True if we think |elf| is a native ELF
  """
  with open(elf) as f:
    data = f.read(20)
    # Check the magic number, EI_CLASS, EI_DATA, and e_machine.
    return (data[0:4] == '\x7fELF' and
            data[4] == '\x02' and
            data[5] == '\x01' and
            data[18] == '\x3e')


def IsPathPackagable(ptype, path):
  """Should the specified file be included in a toolchain package?

  We only need to handle files as we'll create dirs as we need them.

  Further, trim files that won't be useful:
   - non-english translations (.mo) since it'd require env vars
   - debug files since these are for the host compiler itself
   - info/man pages as they're big, and docs are online, and the
     native docs should work fine for the most part (`man gcc`)

  Args:
    ptype: A string describing the path type (i.e. 'file' or 'dir' or 'sym')
    path: The full path to inspect

  Returns:
    True if we want to include this path in the package
  """
  return not (ptype in ('dir',) or
              path.startswith('/usr/lib/debug/') or
              os.path.splitext(path)[1] == '.mo' or
              ('/man/' in path or '/info/' in path))


def ReadlinkRoot(path, root):
  """Like os.readlink(), but relative to a |root|

  Args:
    path: The symlink to read
    root: The path to use for resolving absolute symlinks

  Returns:
    A fully resolved symlink path
  """
  while os.path.islink(root + path):
    path = os.path.join(os.path.dirname(path), os.readlink(root + path))
  return path


def _GetFilesForTarget(target, root='/'):
  """Locate all the files to package for |target|

  This does not cover ELF dependencies.

  Args:
    target: The toolchain target name
    root: The root path to pull all packages from

  Returns:
    A tuple of a set of all packable paths, and a set of all paths which
    are also native ELFs
  """
  paths = set()
  elfs = set()

  # Find all the files owned by the packages for this target.
  for pkg in GetTargetPackages(target):

    # Skip Go compiler from redistributable packages.
    # The "go" executable has GOROOT=/usr/lib/go/${CTARGET} hardcoded
    # into it. Due to this, the toolchain cannot be unpacked anywhere
    # else and be readily useful. To enable packaging Go, we need to:
    # -) Tweak the wrappers/environment to override GOROOT
    #    automatically based on the unpack location.
    # -) Make sure the ELF dependency checking and wrapping logic
    #    below skips the Go toolchain executables and libraries.
    # -) Make sure the packaging process maintains the relative
    #    timestamps of precompiled standard library packages.
    #    (see dev-lang/go ebuild for details).
    if pkg == 'ex_go':
      continue

    # Use armv7a-cros-linux-gnueabi/compiler-rt for
    # armv7a-cros-linux-gnueabihf/compiler-rt.
    # Currently the armv7a-cros-linux-gnueabi is actually
    # the same as armv7a-cros-linux-gnueabihf with different names.
    # Because of that, for compiler-rt, it generates the same binary in
    # the same location. To avoid the installation conflict, we do not
    # install anything for 'armv7a-cros-linux-gnueabihf'. This would cause
    # problem if other people try to use standalone armv7a-cros-linux-gnueabihf
    # toolchain.
    if 'compiler-rt' in pkg and 'armv7a-cros-linux-gnueabi' in target:
      atom = GetPortagePackage(target, pkg)
      cat, pn = atom.split('/')
      ver = GetInstalledPackageVersions(atom, root=root)[0]
      # pylint: disable=E1101
      dblink = portage.dblink(cat, pn + '-' + ver, myroot=root,
                              settings=portage.settings)
      contents = dblink.getcontents()
      if not contents:
        if 'hf' in target:
          new_target = 'armv7a-cros-linux-gnueabi'
        else:
          new_target = 'armv7a-cros-linux-gnueabihf'
        atom = GetPortagePackage(new_target, pkg)
    else:
      atom = GetPortagePackage(target, pkg)

    cat, pn = atom.split('/')
    ver = GetInstalledPackageVersions(atom, root=root)[0]
    logging.info('packaging %s-%s', atom, ver)

    # pylint: disable=E1101
    dblink = portage.dblink(cat, pn + '-' + ver, myroot=root,
                            settings=portage.settings)
    contents = dblink.getcontents()
    for obj in contents:
      ptype = contents[obj][0]
      if not IsPathPackagable(ptype, obj):
        continue

      if ptype == 'obj':
        # For native ELFs, we need to pull in their dependencies too.
        if FileIsCrosSdkElf(obj):
          elfs.add(obj)
      paths.add(obj)

  return paths, elfs


def _BuildInitialPackageRoot(output_dir, paths, elfs, ldpaths,
                             path_rewrite_func=lambda x: x, root='/'):
  """Link in all packable files and their runtime dependencies

  This also wraps up executable ELFs with helper scripts.

  Args:
    output_dir: The output directory to store files
    paths: All the files to include
    elfs: All the files which are ELFs (a subset of |paths|)
    ldpaths: A dict of static ldpath information
    path_rewrite_func: User callback to rewrite paths in output_dir
    root: The root path to pull all packages/files from
  """
  # Link in all the files.
  sym_paths = {}
  for path in paths:
    new_path = path_rewrite_func(path)
    dst = output_dir + new_path
    osutils.SafeMakedirs(os.path.dirname(dst))

    # Is this a symlink which we have to rewrite or wrap?
    # Delay wrap check until after we have created all paths.
    src = root + path
    if os.path.islink(src):
      tgt = os.readlink(src)
      if os.path.sep in tgt:
        sym_paths[lddtree.normpath(ReadlinkRoot(src, root))] = new_path

        # Rewrite absolute links to relative and then generate the symlink
        # ourselves.  All other symlinks can be hardlinked below.
        if tgt[0] == '/':
          tgt = os.path.relpath(tgt, os.path.dirname(new_path))
          os.symlink(tgt, dst)
          continue

    os.link(src, dst)

  # Locate all the dependencies for all the ELFs.  Stick them all in the
  # top level "lib" dir to make the wrapper simpler.  This exact path does
  # not matter since we execute ldso directly, and we tell the ldso the
  # exact path to search for its libraries.
  libdir = os.path.join(output_dir, 'lib')
  osutils.SafeMakedirs(libdir)
  donelibs = set()
  glibc_re = re.compile(r'/lib(c|pthread)-[0-9.]+\.so$')
  for elf in elfs:
    e = lddtree.ParseELF(elf, root=root, ldpaths=ldpaths)
    interp = e['interp']
    # Do not create wrapper for libc. crbug.com/766827
    if interp and not glibc_re.search(elf):
      # Generate a wrapper if it is executable.
      interp = os.path.join('/lib', os.path.basename(interp))
      lddtree.GenerateLdsoWrapper(output_dir, path_rewrite_func(elf), interp,
                                  libpaths=e['rpath'] + e['runpath'])
      FixClangXXWrapper(output_dir, path_rewrite_func(elf))

      # Wrap any symlinks to the wrapper.
      if elf in sym_paths:
        link = sym_paths[elf]
        GeneratePathWrapper(output_dir, link, elf)

    for lib, lib_data in e['libs'].iteritems():
      if lib in donelibs:
        continue

      src = path = lib_data['path']
      if path is None:
        logging.warning('%s: could not locate %s', elf, lib)
        continue
      donelibs.add(lib)

      # Needed libs are the SONAME, but that is usually a symlink, not a
      # real file.  So link in the target rather than the symlink itself.
      # We have to walk all the possible symlinks (SONAME could point to a
      # symlink which points to a symlink), and we have to handle absolute
      # ourselves (since we have a "root" argument).
      dst = os.path.join(libdir, os.path.basename(path))
      src = ReadlinkRoot(src, root)

      os.link(root + src, dst)


def _EnvdGetVar(envd, var):
  """Given a Gentoo env.d file, extract a var from it

  Args:
    envd: The env.d file to load (may be a glob path)
    var: The var to extract

  Returns:
    The value of |var|
  """
  envds = glob.glob(envd)
  assert len(envds) == 1, '%s: should have exactly 1 env.d file' % envd
  envd = envds[0]
  return cros_build_lib.LoadKeyValueFile(envd)[var]


def _ProcessBinutilsConfig(target, output_dir):
  """Do what binutils-config would have done"""
  binpath = os.path.join('/bin', target + '-')

  # Locate the bin dir holding the gold linker.
  binutils_bin_path = os.path.join(output_dir, 'usr', toolchain.GetHostTuple(),
                                   target, 'binutils-bin')
  globpath = os.path.join(binutils_bin_path, '*-gold')
  srcpath = glob.glob(globpath)
  if not srcpath:
    # Maybe this target doesn't support gold.
    globpath = os.path.join(binutils_bin_path, '*')
    srcpath = glob.glob(globpath)
    assert len(srcpath) == 1, ('%s: matched more than one path (but not *-gold)'
                               % globpath)
    srcpath = srcpath[0]
    ld_path = os.path.join(srcpath, 'ld')
    assert os.path.exists(ld_path), '%s: linker is missing!' % ld_path
    ld_path = os.path.join(srcpath, 'ld.bfd')
    assert os.path.exists(ld_path), '%s: linker is missing!' % ld_path
    ld_path = os.path.join(srcpath, 'ld.gold')
    assert not os.path.exists(ld_path), ('%s: exists, but gold dir does not!'
                                         % ld_path)

    # Nope, no gold support to be found.
    gold_supported = False
    logging.warning('%s: binutils lacks support for the gold linker', target)
  else:
    assert len(srcpath) == 1, '%s: did not match exactly 1 path' % globpath
    srcpath = srcpath[0]

    # Package the binutils-bin directory without the '-gold' suffix
    # if gold is not enabled as the default linker for this target.
    gold_supported = CONFIG_TARGET_SUFFIXES['binutils'].get(target) == '-gold'
    if not gold_supported:
      srcpath = srcpath[:-len('-gold')]
      ld_path = os.path.join(srcpath, 'ld')
      assert os.path.exists(ld_path), '%s: linker is missing!' % ld_path

  srcpath = srcpath[len(output_dir):]
  gccpath = os.path.join('/usr', 'libexec', 'gcc')
  for prog in os.listdir(output_dir + srcpath):
    # Skip binaries already wrapped.
    if not prog.endswith('.real'):
      GeneratePathWrapper(output_dir, binpath + prog,
                          os.path.join(srcpath, prog))
      GeneratePathWrapper(output_dir, os.path.join(gccpath, prog),
                          os.path.join(srcpath, prog))

  libpath = os.path.join('/usr', toolchain.GetHostTuple(), target, 'lib')
  envd = os.path.join(output_dir, 'etc', 'env.d', 'binutils', '*')
  if gold_supported:
    envd += '-gold'
  else:
    # If gold is not enabled as the default linker and 2 env.d
    # files exist, pick the one without the '-gold' suffix.
    envds = sorted(glob.glob(envd))
    if len(envds) == 2 and envds[1] == envds[0] + '-gold':
      envd = envds[0]
  srcpath = _EnvdGetVar(envd, 'LIBPATH')
  os.symlink(os.path.relpath(srcpath, os.path.dirname(libpath)),
             output_dir + libpath)


def _ProcessGccConfig(target, output_dir):
  """Do what gcc-config would have done"""
  binpath = '/bin'
  envd = os.path.join(output_dir, 'etc', 'env.d', 'gcc', '*')
  srcpath = _EnvdGetVar(envd, 'GCC_PATH')
  for prog in os.listdir(output_dir + srcpath):
    # Skip binaries already wrapped.
    if (not prog.endswith('.real') and
        not prog.endswith('.elf') and
        prog.startswith(target)):
      GeneratePathWrapper(output_dir, os.path.join(binpath, prog),
                          os.path.join(srcpath, prog))
  return srcpath


def _ProcessSysrootWrappers(_target, output_dir, srcpath):
  """Remove chroot-specific things from our sysroot wrappers"""
  # Disable ccache since we know it won't work outside of chroot.
  for sysroot_wrapper in glob.glob(os.path.join(
      output_dir + srcpath, 'sysroot_wrapper*')):
    contents = osutils.ReadFile(sysroot_wrapper).splitlines()

    # In order to optimize startup time in the chroot we run python a little
    # differently there.  Put it back to the more portable way here.
    # See http://crbug.com/773138 for some details.
    if contents[0] == '#!/usr/bin/python2 -S':
      contents[0] = '#!/usr/bin/env python2'

    for num in xrange(len(contents)):
      if '@CCACHE_DEFAULT@' in contents[num]:
        assert 'True' in contents[num]
        contents[num] = contents[num].replace('True', 'False')
        break
    # Can't update the wrapper in place since it's a hardlink to a file in /.
    os.unlink(sysroot_wrapper)
    osutils.WriteFile(sysroot_wrapper, '\n'.join(contents))
    os.chmod(sysroot_wrapper, 0o755)


def _CreateMainLibDir(target, output_dir):
  """Create some lib dirs so that compiler can get the right Gcc paths"""
  osutils.SafeMakedirs(os.path.join(output_dir, 'usr', target, 'lib'))
  osutils.SafeMakedirs(os.path.join(output_dir, 'usr', target, 'usr/lib'))


def _ProcessDistroCleanups(target, output_dir):
  """Clean up the tree and remove all distro-specific requirements

  Args:
    target: The toolchain target name
    output_dir: The output directory to clean up
  """
  _ProcessBinutilsConfig(target, output_dir)
  gcc_path = _ProcessGccConfig(target, output_dir)
  _ProcessSysrootWrappers(target, output_dir, gcc_path)
  _CreateMainLibDir(target, output_dir)

  osutils.RmDir(os.path.join(output_dir, 'etc'))


def CreatePackagableRoot(target, output_dir, ldpaths, root='/'):
  """Setup a tree from the packages for the specified target

  This populates a path with all the files from toolchain packages so that
  a tarball can easily be generated from the result.

  Args:
    target: The target to create a packagable root from
    output_dir: The output directory to place all the files
    ldpaths: A dict of static ldpath information
    root: The root path to pull all packages/files from
  """
  # Find all the files owned by the packages for this target.
  paths, elfs = _GetFilesForTarget(target, root=root)

  # Link in all the package's files, any ELF dependencies, and wrap any
  # executable ELFs with helper scripts.
  def MoveUsrBinToBin(path):
    """Move /usr/bin to /bin so people can just use that toplevel dir

    Note we do not apply this to clang - there is correlation between clang's
    search path for libraries / inclusion and its installation path.
    """
    if path.startswith('/usr/bin/') and path.find('clang') == -1:
      return path[4:]
    return path
  _BuildInitialPackageRoot(output_dir, paths, elfs, ldpaths,
                           path_rewrite_func=MoveUsrBinToBin, root=root)

  # The packages, when part of the normal distro, have helper scripts
  # that setup paths and such.  Since we are making this standalone, we
  # need to preprocess all that ourselves.
  _ProcessDistroCleanups(target, output_dir)


def CreatePackages(targets_wanted, output_dir, root='/'):
  """Create redistributable cross-compiler packages for the specified targets

  This creates toolchain packages that should be usable in conjunction with
  a downloaded sysroot (created elsewhere).

  Tarballs (one per target) will be created in $PWD.

  Args:
    targets_wanted: The targets to package up.
    output_dir: The directory to put the packages in.
    root: The root path to pull all packages/files from.
  """
  logging.info('Writing tarballs to %s', output_dir)
  osutils.SafeMakedirs(output_dir)
  ldpaths = lddtree.LoadLdpaths(root)
  targets = ExpandTargets(targets_wanted)

  with osutils.TempDir(prefix='create-packages') as tempdir:
    logging.debug('Using tempdir: %s', tempdir)

    # We have to split the root generation from the compression stages.  This is
    # because we hardlink in all the files (to avoid overhead of reading/writing
    # the copies multiple times).  But tar gets angry if a file's hardlink count
    # changes from when it starts reading a file to when it finishes.
    with parallel.BackgroundTaskRunner(CreatePackagableRoot) as queue:
      for target in targets:
        output_target_dir = os.path.join(tempdir, target)
        queue.put([target, output_target_dir, ldpaths, root])

    # Build the tarball.
    with parallel.BackgroundTaskRunner(cros_build_lib.CreateTarball) as queue:
      for target in targets:
        tar_file = os.path.join(output_dir, target + '.tar.xz')
        queue.put([tar_file, os.path.join(tempdir, target)])


def GetParser():
  """Return a command line parser."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('-u', '--nousepkg',
                      action='store_false', dest='usepkg', default=True,
                      help='Use prebuilt packages if possible')
  parser.add_argument('-d', '--deleteold',
                      action='store_true', dest='deleteold', default=False,
                      help='Unmerge deprecated packages')
  parser.add_argument('-t', '--targets',
                      dest='targets', default='sdk',
                      help="Comma separated list of tuples. Special keywords "
                           "'host', 'sdk', 'boards', and 'all' are "
                           "allowed. Defaults to 'sdk'.")
  parser.add_argument('--include-boards', default='', metavar='BOARDS',
                      help='Comma separated list of boards whose toolchains we '
                           'will always include. Default: none')
  parser.add_argument('--hostonly',
                      dest='hostonly', default=False, action='store_true',
                      help='Only setup the host toolchain. '
                           'Useful for bootstrapping chroot')
  parser.add_argument('--show-board-cfg', '--show-cfg',
                      dest='cfg_name', default=None,
                      help='Board  to list toolchains tuples for')
  parser.add_argument('--show-packages', default=None,
                      help='List all packages the specified target uses')
  parser.add_argument('--create-packages',
                      action='store_true', default=False,
                      help='Build redistributable packages')
  parser.add_argument('--output-dir', default=os.getcwd(), type='path',
                      help='Output directory')
  parser.add_argument('--reconfig', default=False, action='store_true',
                      help='Reload crossdev config and reselect toolchains')
  parser.add_argument('--sysroot', type='path',
                      help='The sysroot in which to install the toolchains')
  return parser


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)
  options.Freeze()

  # Figure out what we're supposed to do and reject conflicting options.
  conflicting_options = (
      options.cfg_name,
      options.show_packages,
      options.create_packages,
  )
  if sum(bool(x) for x in conflicting_options) > 1:
    parser.error('conflicting options: create-packages & show-packages & '
                 'show-board-cfg')

  targets_wanted = set(options.targets.split(','))
  boards_wanted = (set(options.include_boards.split(','))
                   if options.include_boards else set())

  if options.cfg_name:
    ShowConfig(options.cfg_name)
  elif options.show_packages is not None:
    cros_build_lib.AssertInsideChroot()
    target = options.show_packages
    Crossdev.Load(False)
    for package in GetTargetPackages(target):
      print(GetPortagePackage(target, package))
  elif options.create_packages:
    cros_build_lib.AssertInsideChroot()
    Crossdev.Load(False)
    CreatePackages(targets_wanted, options.output_dir)
  else:
    cros_build_lib.AssertInsideChroot()
    # This has to be always run as root.
    if os.geteuid() != 0:
      cros_build_lib.Die('this script must be run as root')

    Crossdev.Load(options.reconfig)
    root = options.sysroot or '/'
    UpdateToolchains(options.usepkg, options.deleteold, options.hostonly,
                     options.reconfig, targets_wanted, boards_wanted,
                     root=root)
    Crossdev.Save()

  return 0
