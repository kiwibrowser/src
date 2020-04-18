# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities to create sysroots."""

from __future__ import print_function

import glob
import multiprocessing
import os

from chromite.cbuildbot import binhost
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import locking
from chromite.lib import osutils
from chromite.lib import portage_util
from chromite.lib import toolchain


class ConfigurationError(Exception):
  """Raised when an invalid configuration is found."""


STANDARD_FIELD_PORTDIR_OVERLAY = 'PORTDIR_OVERLAY'
STANDARD_FIELD_CHOST = 'CHOST'
STANDARD_FIELD_BOARD_OVERLAY = 'BOARD_OVERLAY'
STANDARD_FIELD_BOARD_USE = 'BOARD_USE'
STANDARD_FIELD_ARCH = 'ARCH'

_PORTAGE_WRAPPER_TEMPLATE = """#!/bin/sh

# If we try to use sudo when the sandbox is active, we get ugly warnings that
# just confuse developers.  Disable the sandbox in this case by rexecing.
if [ "${{SANDBOX_ON}}" = "1" ]; then
  SANDBOX_ON=0 exec "$0" "$@"
else
  unset LD_PRELOAD
fi

export CHOST="{chost}"
export PORTAGE_CONFIGROOT="{sysroot}"
export SYSROOT="{sysroot}"
if [ -z "$PORTAGE_USERNAME" ]; then
  export PORTAGE_USERNAME=$(basename "${{HOME}}")
fi
export ROOT="{sysroot}"
exec sudo -E {command} "$@"
"""

_BOARD_WRAPPER_TEMPLATE = """#!/bin/sh
exec {command} --board="{board}" "$@"
"""

_PKGCONFIG_WRAPPER_TEMPLATE = """#!/bin/bash

PKG_CONFIG_LIBDIR=$(printf '%s:' "{sysroot}"/usr/*/pkgconfig)
export PKG_CONFIG_LIBDIR

export PKG_CONFIG_SYSROOT_DIR="{sysroot}"

# Portage will get confused and try to "help" us by exporting this.
# Undo that logic.
unset PKG_CONFIG_PATH

exec pkg-config "$@"
"""

_wrapper_dir = '/usr/local/bin'

_IMPLICIT_SYSROOT_DEPS = 'IMPLICIT_SYSROOT_DEPS'

_CONFIGURATION_PATH = 'etc/make.conf.board_setup'

_CACHE_PATH = 'var/cache/edb/chromeos'

_CHROMIUMOS_OVERLAY = '/usr/local/portage/chromiumos'
_ECLASS_OVERLAY = '/usr/local/portage/eclass-overlay'

_CHROME_BINHOST_SUFFIX = '-LATEST_RELEASE_CHROME_BINHOST.conf'

_INTERNAL_BINHOST_DIR = os.path.join(
    constants.SOURCE_ROOT, 'src/private-overlays/chromeos-partner-overlay/'
    'chromeos/binhost/target')
_EXTERNAL_BINHOST_DIR = os.path.join(
    constants.SOURCE_ROOT, constants.CHROMIUMOS_OVERLAY_DIR,
    'chromeos/binhost/target')

_CHROMEOS_INTERNAL_BOTO_PATH = os.path.join(
    constants.SOURCE_ROOT, 'src', 'private-overlays', 'chromeos-overlay',
    'googlestorage_account.boto')

_ARCH_MAPPING = {
    'amd64': 'amd64-generic',
    'x86': 'x86-generic',
    'arm': 'arm-generic',
    'mips': 'mipsel-o32-generic',
}


def _CreateWrapper(wrapper_path, template, **kwargs):
  """Creates a wrapper from a given template.

  Args:
    wrapper_path: path to the wrapper.
    template: wrapper template.
    kwargs: fields to be set in the template.
  """
  osutils.WriteFile(wrapper_path, template.format(**kwargs), makedirs=True,
                    sudo=True)
  cros_build_lib.SudoRunCommand(['chmod', '+x', wrapper_path], print_cmd=False,
                                redirect_stderr=True)


def _NotEmpty(filepath):
  """Returns True if |filepath| is not empty.

  Args:
    filepath: path to a file.
  """
  return os.path.exists(filepath) and osutils.ReadFile(filepath).strip()


def _DictToKeyValue(dictionary):
  """Formats dictionary in to a key=value string.

  Args:
    dictionary: a python dictionary.
  """
  output = []
  for key in sorted(dictionary.keys()):
    output.append('%s="%s"' % (key, dictionary[key]))

  return '\n'.join(output)


class Sysroot(object):
  """Class that encapsulate the interaction with sysroots."""

  def __init__(self, path):
    self.path = path
    self._config_file = os.path.join(path, _CONFIGURATION_PATH)
    self._cache_file = os.path.join(path, _CACHE_PATH)
    self._cache_file_lock = self._cache_file + '.lock'

  def GetStandardField(self, field):
    """Returns the value of a standard field.

    Args:
      field: Field from the standard configuration file to get.
        One of STANDARD_FIELD_* from above.
    """
    return osutils.SourceEnvironment(self._config_file,
                                     [field], multiline=True).get(field)

  def GetCachedField(self, field):
    """Returns the value of |field| in the sysroot cache file.

    Access to the cache is thread-safe as long as we access it through this
    methods or the bash helper in common.sh.

    Args:
      field: name of the field.
    """
    if not os.path.exists(self._cache_file):
      return None

    with locking.FileLock(
        self._cache_file_lock, locktype=locking.FLOCK,
        world_writable=True).read_lock():
      return osutils.SourceEnvironment(self._cache_file, [field]).get(field)

  def SetCachedField(self, field, value):
    """Sets |field| to |value| in the sysroot cache file.

    Access to the cache is thread-safe as long as we access it through this
    methods or the bash helper in common.sh.

    Args:
      field: name of the field.
      value: value to set. If |value| is None, the field is unset.
    """
    # TODO(bsimonnet): add support for values with quotes and newlines.
    # crbug.com/476764.
    for symbol in '\n`$"\\':
      if value and symbol in value:
        raise ValueError('Cannot use \\n, `, $, \\ or " in cached value.')

    with locking.FileLock(
        self._cache_file_lock, locktype=locking.FLOCK,
        world_writable=True).write_lock():
      lines = []
      if os.path.exists(self._cache_file):
        lines = osutils.ReadFile(self._cache_file).splitlines()

        # Remove the old value for field if it exists.
        lines = [l for l in lines if not l.startswith(field + '=')]

      if value is not None:
        lines.append('%s="%s"' % (field, value))
      osutils.WriteFile(self._cache_file, '\n'.join(lines), sudo=True)

  def _WrapperPath(self, command, friendly_name=None):
    """Returns the path to the wrapper for |command|.

    Args:
      command: command to wrap.
      friendly_name: suffix to add to the command name. If None, the wrapper
        will be created in the sysroot.
    """
    if friendly_name:
      return os.path.join(_wrapper_dir, '%s-%s' % (command, friendly_name))
    return os.path.join(self.path, 'build', 'bin', command)

  def CreateAllWrappers(self, friendly_name=None):
    """Creates all the wrappers.

    Creates all portage tools wrappers, plus wrappers for gdb, cros_workon and
    pkg-config.

    Args:
      friendly_name: if not None, create friendly wrappers with |friendly_name|
        added to the command.
    """
    chost = self.GetStandardField(STANDARD_FIELD_CHOST)
    for cmd in ('ebuild', 'eclean', 'emaint', 'equery', 'portageq', 'qcheck',
                'qdepends', 'qfile', 'qlist', 'qmerge', 'qsize'):
      args = {'sysroot': self.path, 'chost': chost, 'command': cmd}
      if friendly_name:
        _CreateWrapper(self._WrapperPath(cmd, friendly_name),
                       _PORTAGE_WRAPPER_TEMPLATE, **args)
      _CreateWrapper(self._WrapperPath(cmd),
                     _PORTAGE_WRAPPER_TEMPLATE, **args)

    if friendly_name:
      _CreateWrapper(self._WrapperPath('emerge', friendly_name),
                     _PORTAGE_WRAPPER_TEMPLATE, sysroot=self.path, chost=chost,
                     command='emerge --root-deps',
                     source_root=constants.SOURCE_ROOT)

      _CreateWrapper(self._WrapperPath('cros_workon', friendly_name),
                     _BOARD_WRAPPER_TEMPLATE, board=friendly_name,
                     command='cros_workon')
      _CreateWrapper(self._WrapperPath('gdb', friendly_name),
                     _BOARD_WRAPPER_TEMPLATE, board=friendly_name,
                     command='cros_gdb')
      _CreateWrapper(self._WrapperPath('pkg-config', friendly_name),
                     _PKGCONFIG_WRAPPER_TEMPLATE, sysroot=self.path)

    _CreateWrapper(self._WrapperPath('pkg-config'),
                   _PKGCONFIG_WRAPPER_TEMPLATE, sysroot=self.path)
    _CreateWrapper(self._WrapperPath('emerge'), _PORTAGE_WRAPPER_TEMPLATE,
                   sysroot=self.path, chost=chost, command='emerge --root-deps',
                   source_root=constants.SOURCE_ROOT)

    # Create a link to the debug symbols in the chroot so that gdb can detect
    # them.
    debug_symlink = os.path.join('/usr/lib/debug', self.path.lstrip('/'))
    sysroot_debug = os.path.join(self.path, 'usr/lib/debug')
    osutils.SafeMakedirs(os.path.dirname(debug_symlink), sudo=True)
    osutils.SafeMakedirs(sysroot_debug, sudo=True)

    osutils.SafeSymlink(sysroot_debug, debug_symlink, sudo=True)

  def _GenerateConfig(self, toolchains, board_overlays, portdir_overlays,
                      header, **kwargs):
    """Create common config settings for boards and bricks.

    Args:
      toolchains: ToolchainList object to use.
      board_overlays: List of board overlays.
      portdir_overlays: List of portage overlays.
      header: Header comment string; must start with #.
      kwargs: Additional configuration values to set.

    Returns:
      Configuration string.

    Raises:
      ConfigurationError: Could not generate a valid configuration.
    """
    config = {}

    default_toolchains = toolchain.FilterToolchains(toolchains, 'default', True)
    if not default_toolchains:
      raise ConfigurationError('No default toolchain could be found.')
    config['CHOST'] = default_toolchains.keys()[0]
    config['ARCH'] = toolchain.GetArchForTarget(config['CHOST'])

    config['BOARD_OVERLAY'] = '\n'.join(board_overlays)
    config['PORTDIR_OVERLAY'] = '\n'.join(portdir_overlays)

    config['MAKEOPTS'] = '-j%s' % str(multiprocessing.cpu_count())
    config['ROOT'] = self.path + '/'
    config['PKG_CONFIG'] = self._WrapperPath('pkg-config')

    config.update(kwargs)

    return '\n'.join((header, _DictToKeyValue(config)))

  def GenerateBoardConfig(self, board):
    """Generates the configuration for a given board.

    Args:
      board: board name to use to generate the configuration.
    """
    toolchains = toolchain.GetToolchainsForBoard(board)

    # Compute the overlay list.
    portdir_overlays = portage_util.FindOverlays(constants.BOTH_OVERLAYS, board)
    prefix = os.path.join(constants.SOURCE_ROOT, 'src', 'third_party')
    board_overlays = [o for o in portdir_overlays if not o.startswith(prefix)]

    header = "# Created by cros_sysroot_utils from --board=%s." % board
    return self._GenerateConfig(toolchains, board_overlays, portdir_overlays,
                                header, BOARD_USE=board)

  def WriteConfig(self, config):
    """Writes the configuration.

    Args:
      config: configuration to use.
    """
    path = os.path.join(self.path, _CONFIGURATION_PATH)
    osutils.WriteFile(path, config, makedirs=True, sudo=True)

  def GenerateMakeConf(self, accepted_licenses=None):
    """Generates the board specific make.conf.

    Args:
      accepted_licenses: Licenses accepted by portage as a string.

    Returns:
      The make.conf file as a python string.
    """
    config = ["""# AUTO-GENERATED FILE. DO NOT EDIT.

  # Source make.conf from each overlay."""]

    overlay_list = self.GetStandardField(STANDARD_FIELD_BOARD_OVERLAY)
    boto_config = ''
    for overlay in overlay_list.splitlines():
      make_conf = os.path.join(overlay, 'make.conf')
      boto_file = os.path.join(overlay, 'googlestorage_account.boto')
      if os.path.isfile(make_conf):
        config.append('source %s' % make_conf)

      if os.path.isfile(boto_file):
        boto_config = boto_file

    # If there is a boto file in the chromeos internal overlay, use it as it
    # will have access to the most stuff.
    if os.path.isfile(_CHROMEOS_INTERNAL_BOTO_PATH):
      boto_config = _CHROMEOS_INTERNAL_BOTO_PATH

    gs_fetch_binpkg = os.path.join(constants.SOURCE_ROOT, 'chromite', 'bin',
                                   'gs_fetch_binpkg')
    gsutil_cmd = '%s \\"${URI}\\" \\"${DISTDIR}/${FILE}\\"' % gs_fetch_binpkg
    config.append('BOTO_CONFIG="%s"' % boto_config)
    config.append('FETCHCOMMAND_GS="bash -c \'BOTO_CONFIG=%s %s\'"'
                  % (boto_config, gsutil_cmd))
    config.append('RESUMECOMMAND_GS="$FETCHCOMMAND_GS"')

    if accepted_licenses:
      config.append('ACCEPT_LICENSE="%s"' % accepted_licenses)

    return '\n'.join(config)

  def GenerateBinhostConf(self, chrome_only=False, local_only=False):
    """Returns the binhost configuration.

    Args:
      chrome_only: If True, generate only the binhost for chrome.
      local_only: If True, use binary packages from local boards only.
    """
    board = self.GetStandardField(STANDARD_FIELD_BOARD_USE)
    if local_only:
      if not board:
        return ''
      # TODO(bsimonnet): Refactor cros_generate_local_binhosts into a function
      # here and remove the following call.
      local_binhosts = cros_build_lib.RunCommand(
          [os.path.join(constants.CHROMITE_BIN_DIR,
                        'cros_generate_local_binhosts'), '--board=%s' % board],
          print_cmd=False, capture_output=True).output
      return '\n'.join([local_binhosts,
                        'PORTAGE_BINHOST="$LOCAL_BINHOST"'])

    config = []
    chrome_binhost = board and self._ChromeBinhost(board)
    preflight_binhost, preflight_binhost_internal = self._PreflightBinhosts(
        board)

    if chrome_only:
      if chrome_binhost:
        return '\n'.join(['source %s' % chrome_binhost,
                          'PORTAGE_BINHOST="$LATEST_RELEASE_CHROME_BINHOST"'])
      else:
        return ''

    config.append("""
# FULL_BINHOST is populated by the full builders. It is listed first because it
# is the lowest priority binhost. It is better to download packages from the
# preflight binhost because they are fresher packages.
PORTAGE_BINHOST="$FULL_BINHOST"
""")

    if preflight_binhost:
      config.append("""
# PREFLIGHT_BINHOST is populated by the preflight builders. If the same
# package is provided by both the preflight and full binhosts, the package is
# downloaded from the preflight binhost.
source %s
PORTAGE_BINHOST="$PORTAGE_BINHOST $PREFLIGHT_BINHOST"
""" % preflight_binhost)

    if preflight_binhost_internal:
      config.append("""
# The internal PREFLIGHT_BINHOST is populated by the internal preflight
# builders. It takes priority over the public preflight binhost.
source %s
PORTAGE_BINHOST="$PORTAGE_BINHOST $PREFLIGHT_BINHOST"
""" % preflight_binhost_internal)

    if chrome_binhost:
      config.append("""
# LATEST_RELEASE_CHROME_BINHOST provides prebuilts for chromeos-chrome only.
source %s
PORTAGE_BINHOST="$PORTAGE_BINHOST $LATEST_RELEASE_CHROME_BINHOST"
""" % chrome_binhost)

    return '\n'.join(config)

  def _ChromeBinhost(self, board):
    """Gets the latest chrome binhost for |board|.

    Args:
      board: The board to use.
    """
    extra_useflags = os.environ.get('USE', '').split()
    compat_id = binhost.CalculateCompatId(board, extra_useflags)
    internal_config = binhost.PrebuiltMapping.GetFilename(
        constants.SOURCE_ROOT, 'chrome')
    external_config = binhost.PrebuiltMapping.GetFilename(
        constants.SOURCE_ROOT, 'chromium', internal=False)
    binhost_dirs = (_INTERNAL_BINHOST_DIR, _EXTERNAL_BINHOST_DIR)

    if os.path.exists(internal_config):
      pfq_configs = binhost.PrebuiltMapping.Load(internal_config)
    elif os.path.exists(external_config):
      pfq_configs = binhost.PrebuiltMapping.Load(external_config)
    else:
      return None

    for key in pfq_configs.GetPrebuilts(compat_id):
      for binhost_dir in binhost_dirs:
        binhost_file = os.path.join(binhost_dir,
                                    key.board + _CHROME_BINHOST_SUFFIX)
        # Make sure the binhost file is not empty. We sometimes empty the file
        # to force clients to use another binhost.
        if _NotEmpty(binhost_file):
          return binhost_file

    return None

  def _PreflightBinhosts(self, board=None):
    """Returns the preflight binhost to use.

    Args:
      board: Board name.
    """
    prefixes = []
    arch = self.GetStandardField(STANDARD_FIELD_ARCH)
    if arch in _ARCH_MAPPING:
      prefixes.append(_ARCH_MAPPING[arch])

    if board:
      prefixes = [board, board.split('_')[0]] + prefixes

    filenames = ['%s-PREFLIGHT_BINHOST.conf' % p for p in prefixes]

    external = internal = None
    for filename in filenames:
      # The binhost file must exist and not be empty, both for internal and
      # external binhosts.
      # When a builder is deleted and no longer publishes prebuilts, we need
      # developers to pick up the next set of prebuilts. Clearing the binhost
      # files triggers this.
      candidate = os.path.join(_INTERNAL_BINHOST_DIR, filename)
      if not internal and _NotEmpty(candidate):
        internal = candidate

      candidate = os.path.join(_EXTERNAL_BINHOST_DIR, filename)
      if not external and _NotEmpty(candidate):
        external = candidate

    return external, internal

  def CreateSkeleton(self):
    """Creates a sysroot skeleton."""
    needed_dirs = [
        os.path.join(self.path, 'etc', 'portage', 'hooks'),
        os.path.join(self.path, 'etc'),
        os.path.join(self.path, 'etc', 'portage', 'profile'),
        os.path.join('/', 'usr', 'local', 'bin'),
    ]
    for d in needed_dirs:
      osutils.SafeMakedirs(d, sudo=True)

    make_user = os.path.join('/', 'etc', 'make.conf.user')
    link = os.path.join(self.path, 'etc', 'make.conf.user')
    if not os.path.exists(make_user):
      osutils.WriteFile(make_user, '', sudo=True)
    osutils.SafeSymlink(make_user, link, sudo=True)

    # Create links for portage hooks.
    hook_glob = os.path.join(constants.CROSUTILS_DIR, 'hooks', '*')
    for filename in glob.glob(hook_glob):
      linkpath = os.path.join(self.path, 'etc', 'portage', 'hooks',
                              os.path.basename(filename))
      osutils.SafeSymlink(filename, linkpath, sudo=True)

  def _SelectDefaultMakeConf(self):
    """Selects the best make.conf file possible.

    The best make.conf possible is the ARCH-specific make.conf. If it does not
    exist, we use the generic make.conf.
    """
    make_conf = os.path.join(
        constants.SOURCE_ROOT, constants.CHROMIUMOS_OVERLAY_DIR, 'chromeos',
        'config', 'make.conf.generic-target')
    link = os.path.join(self.path, 'etc', 'make.conf')
    osutils.SafeSymlink(make_conf, link, sudo=True)

  def _GenerateProfile(self):
    """Generates the portage profile for this sysroot.

    The generated portage profile depends on the profiles of all used bricks in
    order as well as the general brillo profile for this architecture.
    """
    overlays = self.GetStandardField(STANDARD_FIELD_BOARD_OVERLAY).splitlines()
    profile_list = [os.path.join(o, 'profiles', 'base') for o in overlays]

    # Keep only the profiles that exist.
    profile_list = [p for p in profile_list if os.path.exists(p)]

    # Add the arch specific profile.
    # The profile list is ordered from the lowest to the highest priority. This
    # profile has to go first so that other profiles can override it.
    arch = self.GetStandardField(STANDARD_FIELD_ARCH)
    profile_list.insert(0, 'chromiumos:default/linux/%s/brillo' % arch)

    generated_parent = os.path.join(self.path, 'build', 'generated_profile',
                                    'parent')
    osutils.WriteFile(
        generated_parent, '\n'.join(profile_list), sudo=True, makedirs=True)
    profile_link = os.path.join(self.path, 'etc', 'portage', 'make.profile')
    osutils.SafeMakedirs(os.path.dirname(profile_link), sudo=True)
    osutils.SafeSymlink(os.path.dirname(generated_parent), profile_link,
                        sudo=True)

  def GeneratePortageConfig(self):
    """Generates the portage config.

    This step will:
    * create the portage wrappers.
    * create the symlink to the architecture-specific make.conf
    * generate make.conf.board (binhost, gsutil setup and various portage
      configuration)
    * choose the best portage profile possible.
    """
    self.CreateAllWrappers()
    self._SelectDefaultMakeConf()
    self._GenerateProfile()

    make_conf = self.GenerateMakeConf()
    make_conf_path = os.path.join(self.path, 'etc', 'make.conf.board')
    osutils.WriteFile(make_conf_path, make_conf, sudo=True)

    # Once make.conf.board has been generated, generate the binhost config.
    # We need to do this in two steps as the binhost generation step needs
    # portageq to be available.
    osutils.WriteFile(make_conf_path,
                      '\n'.join([make_conf, self.GenerateBinhostConf()]),
                      sudo=True)

  def UpdateToolchain(self):
    """Updates the toolchain packages.

    This will install both the toolchains and the packages that are implicitly
    needed (gcc-libs, linux-headers).
    """
    cros_build_lib.RunCommand(
        [os.path.join(constants.CROSUTILS_DIR, 'install_toolchain'),
         '--sysroot', self.path])

    if not self.GetCachedField(_IMPLICIT_SYSROOT_DEPS):
      emerge = [os.path.join(constants.CHROMITE_BIN_DIR, 'parallel_emerge'),
                '--sysroot=%s' % self.path]
      cros_build_lib.SudoRunCommand(
          emerge + ['--root-deps=rdeps', '--usepkg', '--getbinpkg', '--select',
                    'gcc-libs', 'linux-headers'])
      self.SetCachedField(_IMPLICIT_SYSROOT_DEPS, 'yes')
