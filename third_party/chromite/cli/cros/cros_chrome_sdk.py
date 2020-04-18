# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The cros chrome-sdk command for the simple chrome workflow."""

from __future__ import print_function

import argparse
import base64
import collections
import contextlib
import glob
import json
import os

from chromite.cli import command
from chromite.lib import cache
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gob_util
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.cbuildbot import archive_lib
from chromite.lib import config_lib
from chromite.lib import constants
from gn_helpers import gn_helpers


COMMAND_NAME = 'chrome-sdk'
CUSTOM_VERSION = 'custom'


def Log(*args, **kwargs):
  """Conditional logging.

  Args:
    silent: If set to True, then logs with level DEBUG.  logs with level INFO
      otherwise.  Defaults to False.
  """
  silent = kwargs.pop('silent', False)
  level = logging.DEBUG if silent else logging.INFO
  logging.log(level, *args, **kwargs)


class NoChromiumSrcDir(Exception):
  """Error thrown when no chromium src dir is found."""

  def __init__(self, path):
    Exception.__init__(self, 'No chromium src dir found in: ' % (path))

class MissingLKGMFile(Exception):
  """Error thrown when we cannot get the version from CHROMEOS_LKGM."""

  def __init__(self, path):
    Exception.__init__(self, 'Cannot parse CHROMEOS_LKGM file: %s' % (path))

class MissingSDK(Exception):
  """Error thrown when we cannot find an SDK."""

  def __init__(self, board, version=None):
    msg = 'Cannot find SDK for %r' % (board,)
    if version is not None:
      msg += ' with version %s' % (version,)
    Exception.__init__(self, msg)


class SDKFetcher(object):
  """Functionality for fetching an SDK environment.

  For the version of ChromeOS specified, the class downloads and caches
  SDK components.
  """
  SDK_BOARD_ENV = '%SDK_BOARD'
  SDK_PATH_ENV = '%SDK_PATH'
  SDK_VERSION_ENV = '%SDK_VERSION'

  SDKContext = collections.namedtuple(
      'SDKContext', ['version', 'target_tc', 'key_map'])

  TARBALL_CACHE = 'tarballs'
  MISC_CACHE = 'misc'

  TARGET_TOOLCHAIN_KEY = 'target_toolchain'
  QEMU_BIN_KEY = 'app-emulation/qemu-2.6.0-r3.tbz2'
  PREBUILT_CONF_PATH = ('chromiumos/overlays/board-overlays.git/+/'
                        'master/overlay-amd64-host/prebuilt.conf')

  CANARIES_PER_DAY = 3
  DAYS_TO_CONSIDER = 14
  VERSIONS_TO_CONSIDER = DAYS_TO_CONSIDER * CANARIES_PER_DAY

  def __init__(self, cache_dir, board, clear_cache=False, chrome_src=None,
               sdk_path=None, toolchain_path=None, silent=False,
               use_external_config=None):
    """Initialize the class.

    Args:
      cache_dir: The toplevel cache dir to use.
      board: The board to manage the SDK for.
      clear_cache: Clears the sdk cache during __init__.
      chrome_src: The location of the chrome checkout.  If unspecified, the
        cwd is presumed to be within a chrome checkout.
      sdk_path: The path (whether a local directory or a gs:// path) to fetch
        SDK components from.
      toolchain_path: The path (whether a local directory or a gs:// path) to
        fetch toolchain components from.
      silent: If set, the fetcher prints less output.
      use_external_config: When identifying the configuration for a board,
        force usage of the external configuration if both external and internal
        are available.
    """
    site_config = config_lib.GetConfig()

    self.cache_base = os.path.join(cache_dir, COMMAND_NAME)
    if clear_cache:
      logging.warning('Clearing the SDK cache.')
      osutils.RmDir(self.cache_base, ignore_missing=True)
    self.tarball_cache = cache.TarballCache(
        os.path.join(self.cache_base, self.TARBALL_CACHE))
    self.misc_cache = cache.DiskCache(
        os.path.join(self.cache_base, self.MISC_CACHE))
    self.board = board
    self.config = site_config.FindCanonicalConfigForBoard(
        board, allow_internal=not use_external_config)
    self.gs_base = archive_lib.GetBaseUploadURI(self.config)
    self.clear_cache = clear_cache
    self.chrome_src = chrome_src
    self.sdk_path = sdk_path
    self.toolchain_path = toolchain_path
    self.silent = silent

    # For external configs, there is no need to run 'gsutil config', because
    # the necessary files are all accessible to anonymous users.
    internal = self.config['internal']
    self.gs_ctx = gs.GSContext(cache_dir=cache_dir, init_boto=internal)

    if self.sdk_path is None:
      self.sdk_path = os.environ.get(self.SDK_PATH_ENV)

    if self.toolchain_path is None:
      self.toolchain_path = 'gs://%s' % constants.SDK_GS_BUCKET

  def _UpdateTarball(self, url, ref):
    """Worker function to fetch tarballs"""
    with osutils.TempDir(base_dir=self.tarball_cache.staging_dir) as tempdir:
      local_path = os.path.join(tempdir, os.path.basename(url))
      Log('SDK: Fetching %s', url, silent=self.silent)
      self.gs_ctx.Copy(url, tempdir, debug_level=logging.DEBUG)
      ref.SetDefault(local_path, lock=True)

  def _GetMetadata(self, version):
    """Return metadata (in the form of a dict) for a given version."""
    raw_json = None
    version_base = self._GetVersionGSBase(version)
    with self.misc_cache.Lookup(
        self._GetCacheKeyForComponent(version, constants.METADATA_JSON)) as ref:
      if ref.Exists(lock=True):
        raw_json = osutils.ReadFile(ref.path)
      else:
        metadata_path = os.path.join(version_base, constants.METADATA_JSON)
        partial_metadata_path = os.path.join(version_base,
                                             constants.PARTIAL_METADATA_JSON)
        try:
          raw_json = self.gs_ctx.Cat(metadata_path,
                                     debug_level=logging.DEBUG)
        except gs.GSNoSuchKey:
          logging.info('Could not read %s, falling back to %s',
                       metadata_path, partial_metadata_path)
          raw_json = self.gs_ctx.Cat(partial_metadata_path,
                                     debug_level=logging.DEBUG)

        ref.AssignText(raw_json)

    return json.loads(raw_json)

  @staticmethod
  def GetChromeLKGM(chrome_src_dir=None):
    """Get ChromeOS LKGM checked into the Chrome tree.

    Args:
      chrome_src_dir: chrome source directory.

    Returns:
      Version number in format '10171.0.0'.
    """
    if not chrome_src_dir:
      chrome_src_dir = path_util.DetermineCheckout(os.getcwd()).chrome_src_dir
    if not chrome_src_dir:
      return None
    lkgm_file = os.path.join(chrome_src_dir, constants.PATH_TO_CHROME_LKGM)
    version = osutils.ReadFile(lkgm_file).rstrip()
    logging.debug('Read LKGM version from %s: %s', lkgm_file, version)
    return version

  @staticmethod
  def _GetQemuBinPath():
    """Get prebuilt QEMU binary path from google storage."""
    contents_b64 = gob_util.FetchUrl(
        constants.EXTERNAL_GOB_HOST,
        '%s?format=TEXT' % SDKFetcher.PREBUILT_CONF_PATH)
    binhost, path = base64.b64decode(contents_b64.read()).strip().split('=')
    if binhost != 'FULL_BINHOST' or not path:
      return None
    return path.strip('"')

  def _GetFullVersionFromStorage(self, version_file):
    """Cat |version_file| in google storage.

    Args:
      version_file: google storage path of the version file.

    Returns:
      Version number in the format 'R30-3929.0.0' or None.
    """
    try:
       # If the version doesn't exist in google storage,
       # which isn't unlikely, don't waste time on retries.
      full_version = self.gs_ctx.Cat(version_file, retries=0)
      assert full_version.startswith('R')
      return full_version
    except (gs.GSNoSuchKey, gs.GSCommandError):
      return None

  def _GetFullVersionFromRecentLatest(self, version):
    """Gets the full version number from a recent LATEST- file.

    If LATEST-{version} does not exist, we need to look for a recent
    LATEST- file to get a valid full version from.

    Args:
      version: The version number to look backwards from. If version is not a
      canary version (ending in .0.0), returns None.

    Returns:
      Version number in the format 'R30-3929.0.0' or None.
    """

    # If version does not end in .0.0 it is not a canary so fail.
    if not version.endswith('.0.0'):
      return None
    version_base = int(version.split('.')[0])
    version_base_min = version_base - self.VERSIONS_TO_CONSIDER

    for v in xrange(version_base - 1, version_base_min, -1):
      version_file = '%s/LATEST-%d.0.0' % (self.gs_base, v)
      logging.info('Trying: %s', version_file)
      full_version = self._GetFullVersionFromStorage(version_file)
      if full_version is not None:
        logging.warning(
            'Using cros version from most recent LATEST file: %s -> %s',
            version_file, full_version)
        return full_version
    logging.warning('No recent LATEST file found from %d.0.0 to %d.0.0: ',
                    version_base_min, version_base)
    return None

  def _GetFullVersionFromLatest(self, version):
    """Gets the full version number from the LATEST-{version} file.

    Args:
      version: The version number or branch to look at.

    Returns:
      Version number in the format 'R30-3929.0.0' or None.
    """
    version_file = '%s/LATEST-%s' % (self.gs_base, version)
    full_version = self._GetFullVersionFromStorage(version_file)
    if full_version is None:
      logging.warning('No LATEST file matching SDK version %s', version)
      return self._GetFullVersionFromRecentLatest(version)
    return full_version

  def GetDefaultVersion(self):
    """Get the default SDK version to use.

    If we are in an existing SDK shell, the default version will just be
    the current version. Otherwise, we will try to calculate the
    appropriate version to use based on the checkout.
    """
    if os.environ.get(self.SDK_BOARD_ENV) == self.board:
      sdk_version = os.environ.get(self.SDK_VERSION_ENV)
      if sdk_version is not None:
        return sdk_version

    with self.misc_cache.Lookup((self.board, 'latest')) as ref:
      if ref.Exists(lock=True):
        version = osutils.ReadFile(ref.path).strip()
        # Deal with the old version format.
        if version.startswith('R'):
          version = version.split('-')[1]
        return version
      else:
        return None

  def _SetDefaultVersion(self, version):
    """Set the new default version."""
    with self.misc_cache.Lookup((self.board, 'latest')) as ref:
      ref.AssignText(version)

  def UpdateDefaultVersion(self):
    """Update the version that we default to using.

    Returns:
      A tuple of the form (version, updated), where |version| is the
      version number in the format '3929.0.0', and |updated| indicates
      whether the version was indeed updated.
    """
    checkout_dir = self.chrome_src if self.chrome_src else os.getcwd()
    checkout = path_util.DetermineCheckout(checkout_dir)
    current = self.GetDefaultVersion() or '0'

    if not checkout.chrome_src_dir:
      raise NoChromiumSrcDir(checkout_dir)

    target = self.GetChromeLKGM(checkout.chrome_src_dir)
    if target is None:
      raise MissingLKGMFile(checkout.chrome_src_dir)

    self._SetDefaultVersion(target)
    return target, target != current

  def GetFullVersion(self, version):
    """Add the release branch and build number to a ChromeOS platform version.

    This will specify where you can get the latest build for the given version
    for the current board.

    Args:
      version: A ChromeOS platform number of the form XXXX.XX.XX, i.e.,
        3918.0.0.

    Returns:
      The version with release branch and build number added, as needed. E.g.
      R28-3918.0.0-b1234.
    """
    assert not version.startswith('R')

    with self.misc_cache.Lookup(('full-version', self.board, version)) as ref:
      if ref.Exists(lock=True):
        return osutils.ReadFile(ref.path).strip()
      else:
        full_version = self._GetFullVersionFromLatest(version)

        if full_version is None:
          raise MissingSDK(self.board, version)

        ref.AssignText(full_version)
        return full_version

  def _GetVersionGSBase(self, version):
    """The base path of the SDK for a particular version."""
    if self.sdk_path is not None:
      return self.sdk_path

    full_version = self.GetFullVersion(version)
    return os.path.join(self.gs_base, full_version)

  def _GetCacheKeyForComponent(self, version, component):
    """Builds the cache key tuple for an SDK component."""
    version_section = version
    if self.sdk_path is not None:
      version_section = self.sdk_path.replace('/', '__').replace(':', '__')
    return (self.board, version_section, component)

  @contextlib.contextmanager
  def Prepare(self, components, version=None, target_tc=None,
              toolchain_url=None):
    """Ensures the components of an SDK exist and are read-locked.

    For a given SDK version, pulls down missing components, and provides a
    context where the components are read-locked, which prevents the cache from
    deleting them during its purge operations.

    If both target_tc and toolchain_url arguments are provided, then this
    does not download metadata.json for the given version. Otherwise, this
    function requires metadata.json for the given version to exist.

    Args:
      gs_ctx: GSContext object.
      components: A list of specific components(tarballs) to prepare.
      version: The version to prepare.  If not set, uses the version returned by
        GetDefaultVersion().  If there is no default version set (this is the
        first time we are being executed), then we update the default version.
      target_tc: Target toolchain name to use, e.g. x86_64-cros-linux-gnu
      toolchain_url: Format pattern for path to fetch toolchain from,
        e.g. 2014/04/%(target)s-2014.04.23.220740.tar.xz

    Yields:
      An SDKFetcher.SDKContext namedtuple object.  The attributes of the
      object are:
        version: The version that was prepared.
        target_tc: Target toolchain name.
        key_map: Dictionary that contains CacheReference objects for the SDK
          artifacts, indexed by cache key.
    """
    if version is None and self.sdk_path is None:
      version = self.GetDefaultVersion()
      if version is None:
        version, _ = self.UpdateDefaultVersion()
    components = list(components)

    key_map = {}
    fetch_urls = {}

    if not target_tc or not toolchain_url:
      metadata = self._GetMetadata(version)
      target_tc = target_tc or metadata['toolchain-tuple'][0]
      toolchain_url = toolchain_url or metadata['toolchain-url']

    # Fetch toolchains from separate location.
    if self.TARGET_TOOLCHAIN_KEY in components:
      fetch_urls[self.TARGET_TOOLCHAIN_KEY] = os.path.join(
          self.toolchain_path, toolchain_url % {'target': target_tc})
      components.remove(self.TARGET_TOOLCHAIN_KEY)

    # Also fetch QEMU binary if VM_IMAGE_TAR is specified.
    if constants.VM_IMAGE_TAR in components:
      qemu_bin_path = self._GetQemuBinPath()
      if qemu_bin_path:
        fetch_urls[self.QEMU_BIN_KEY] = os.path.join(
            qemu_bin_path, self.QEMU_BIN_KEY)
      else:
        logging.warning('Failed to find a QEMU binary to download.')

    version_base = self._GetVersionGSBase(version)
    fetch_urls.update((t, os.path.join(version_base, t)) for t in components)
    try:
      for key, url in fetch_urls.iteritems():
        cache_key = self._GetCacheKeyForComponent(version, key)
        ref = self.tarball_cache.Lookup(cache_key)
        key_map[key] = ref
        ref.Acquire()
        if not ref.Exists(lock=True):
          # TODO(rcui): Parallelize this.  Requires acquiring locks *before*
          # generating worker processes; therefore the functionality needs to
          # be moved into the DiskCache class itself -
          # i.e.,DiskCache.ParallelSetDefault().
          try:
            self._UpdateTarball(url, ref)
          except gs.GSNoSuchKey:
            if key == constants.VM_IMAGE_TAR:
              logging.warning(
                  'No VM available for board %s. '
                  'Please try a different board, e.g. amd64-generic.',
                  self.board)
            else:
              raise

      ctx_version = version
      if self.sdk_path is not None:
        ctx_version = CUSTOM_VERSION
      yield self.SDKContext(ctx_version, target_tc, key_map)
    finally:
      # TODO(rcui): Move to using cros_build_lib.ContextManagerStack()
      cros_build_lib.SafeRun([ref.Release for ref in key_map.itervalues()])


class GomaError(Exception):
  """Indicates error with setting up Goma."""


@command.CommandDecorator(COMMAND_NAME)
class ChromeSDKCommand(command.CliCommand):
  """Set up an environment for building Chrome on Chrome OS.

  Pulls down SDK components for building and testing Chrome for Chrome OS,
  sets up the environment for building Chrome, and runs a command in the
  environment, starting a bash session if no command is specified.

  The bash session environment is set up by a user-configurable rc file located
  at ~/.chromite/chrome_sdk.bashrc.
  """

  # Note, this URL is not accessible outside of corp.
  _GOMA_URL = ('https://clients5.google.com/cxx-compiler-service/'
               'download/goma_ctl.py')

  _CHROME_CLANG_DIR = 'third_party/llvm-build/Release+Asserts/bin'
  _HOST_BINUTILS_DIR = 'third_party/binutils/Linux_x64/Release/bin/'

  EBUILD_ENV = (
      # Compiler tools.
      'CXX',
      'CC',
      'AR',
      'AS',
      'LD',
      'RANLIB',

      # Compiler flags.
      'CFLAGS',
      'CXXFLAGS',
      'CPPFLAGS',
      'LDFLAGS',

      # Misc settings.
      'GN_ARGS',
      'GOLD_SET',
      'USE',
  )

  SDK_GOMA_PORT_ENV = 'SDK_GOMA_PORT'
  SDK_GOMA_DIR_ENV = 'SDK_GOMA_DIR'

  GOMACC_PORT_CMD = ['./gomacc', 'port']
  FETCH_GOMA_CMD = ['wget', _GOMA_URL]

  # Override base class property to use cache related commandline options.
  use_caching_options = True

  @staticmethod
  def ValidateVersion(version):
    if version.startswith('R') or len(version.split('.')) != 3:
      raise argparse.ArgumentTypeError(
          '--version should be in the format 3912.0.0')
    return version

  @classmethod
  def AddParser(cls, parser):
    super(ChromeSDKCommand, cls).AddParser(parser)
    parser.add_argument(
        '--board', required=True, help='The board SDK to use.')
    parser.add_argument(
        '--bashrc', type='path',
        default=constants.CHROME_SDK_BASHRC,
        help='A bashrc file used to set up the SDK shell environment. '
             'Defaults to %s.' % constants.CHROME_SDK_BASHRC)
    parser.add_argument(
        '--chroot', type='path',
        help='Path to a ChromeOS chroot to use. If set, '
             '<chroot>/build/<board> will be used as the sysroot that Chrome '
             'is built against. If chromeos-chrome was built, the build '
             'environment from the chroot will also be used. The version shown '
             'in the SDK shell prompt will have an asterisk prepended to it.')
    parser.add_argument(
        '--chrome-src', type='path',
        help='Specifies the location of a Chrome src/ directory.  Required if '
             'running with --clang if not running from a Chrome checkout.')
    parser.add_argument(
        '--clang', action='store_true', default=False,
        help='Sets up the environment for building with clang. For all '
             'boards, except X86-32, clang is the default.')
    parser.add_argument(
        '--cwd', type='path',
        help='Specifies a directory to switch to after setting up the SDK '
             'shell.  Defaults to the current directory.')
    parser.add_argument(
        '--internal', action='store_true', default=False,
        help='Sets up SDK for building official (internal) Chrome '
             'Chrome, rather than Chromium.')
    parser.add_argument(
        '--component', action='store_true', default=False,
        help='Deprecated and ignored. Set is_component_build=true in args.gn '
             'instead.')
    parser.add_argument(
        '--fastbuild', action='store_true', default=False,
        help='Deprecated and ignored. Set symbol_level=1 in args.gn instead.')
    parser.add_argument(
        '--use-external-config', action='store_true', default=False,
        help='Use the external configuration for the specified board, even if '
             'an internal configuration is avalable.')
    parser.add_argument(
        '--sdk-path', type='local_or_gs_path',
        help='Provides a path, whether a local directory or a gs:// path, to '
             'pull SDK components from.')
    parser.add_argument(
        '--toolchain-path', type='local_or_gs_path',
        help='Provides a path, whether a local directory or a gs:// path, to '
             'pull toolchain components from.')
    parser.add_argument(
        '--gn-extra-args',
        help='Provides extra args to "gn gen". Uses the same format as '
             'gn gen, e.g. "foo = true bar = 1".')
    parser.add_argument(
        '--gn-gen', action='store_true', default=True, dest='gn_gen',
        help='Run "gn gen" if args.gn is stale.')
    parser.add_argument(
        '--nogn-gen', action='store_false', dest='gn_gen',
        help='Do not run "gn gen", warns if args.gn is stale.')
    parser.add_argument(
        '--nogoma', action='store_false', default=True, dest='goma',
        help='Disables Goma in the shell by removing it from the PATH and '
             'set use_goma=false to GN_ARGS.')
    parser.add_argument(
        '--nostart-goma', action='store_false', default=True, dest='start_goma',
        help='Skip starting goma and hope somebody else starts goma later.')
    parser.add_argument(
        '--gomadir', type='path',
        help='Use the goma installation at the specified PATH.')
    parser.add_argument(
        '--version', default=None, type=cls.ValidateVersion,
        help="Specify version of SDK to use, in the format '3912.0.0'.  "
             "Defaults to determining version based on the type of checkout "
             "(Chrome or ChromeOS) you are executing from.")
    parser.add_argument(
        'cmd', nargs='*', default=None,
        help='The command to execute in the SDK environment.  Defaults to '
             'starting a bash shell.')
    parser.add_argument(
        '--download-vm', action='store_true', default=False,
        help='Additionally downloads a VM image from cloud storage.')

    parser.caching_group.add_argument(
        '--clear-sdk-cache', action='store_true',
        default=False,
        help='Removes everything in the SDK cache before starting.')

    group = parser.add_argument_group(
        'Metadata Overrides (Advanced)',
        description='Provide all of these overrides in order to remove '
                    'dependencies on metadata.json existence.')
    group.add_argument(
        '--target-tc', action='store', default=None,
        help='Override target toolchain name, e.g. x86_64-cros-linux-gnu')
    group.add_argument(
        '--toolchain-url', action='store', default=None,
        help='Override toolchain url format pattern, e.g. '
             '2014/04/%%(target)s-2014.04.23.220740.tar.xz')

  def __init__(self, options):
    super(ChromeSDKCommand, self).__init__(options)
    self.board = options.board
    # Lazy initialized.
    self.sdk = None
    # Initialized later based on options passed in.
    self.silent = True

  @staticmethod
  def _PS1Prefix(board, version, chroot=None):
    """Returns a string describing the sdk environment for use in PS1."""
    chroot_star = '*' if chroot else ''
    return '(sdk %s %s%s)' % (board, chroot_star, version)

  @staticmethod
  def _CreatePS1(board, version, chroot=None):
    """Returns PS1 string that sets commandline and xterm window caption.

    If a chroot path is set, then indicate we are using the sysroot from there
    instead of the stock sysroot by prepending an asterisk to the version.

    Args:
      board: The SDK board.
      version: The SDK version.
      chroot: The path to the chroot, if set.
    """
    current_ps1 = cros_build_lib.RunCommand(
        ['bash', '-l', '-c', 'echo "$PS1"'], print_cmd=False,
        capture_output=True).output.splitlines()
    if current_ps1:
      current_ps1 = current_ps1[-1]
    if not current_ps1:
      # Something went wrong, so use a fallback value.
      current_ps1 = r'\u@\h \w $ '
    ps1_prefix = ChromeSDKCommand._PS1Prefix(board, version, chroot)
    return '%s %s' % (ps1_prefix, current_ps1)

  def _FixGoldPath(self, var_contents, toolchain_path):
    """Point to the gold linker in the toolchain tarball.

    Accepts an already set environment variable in the form of '<cmd>
    -B<gold_path>', and overrides the gold_path to the correct path in the
    extracted toolchain tarball.

    Args:
      var_contents: The contents of the environment variable.
      toolchain_path: Path to the extracted toolchain tarball contents.

    Returns:
      Environment string that has correct gold path.
    """
    cmd, _, gold_path = var_contents.partition(' -B')
    gold_path = os.path.join(toolchain_path, gold_path.lstrip('/'))
    return '%s -B%s' % (cmd, gold_path)

  def _StripGnArgs(self, gn_args_dict):
    """Strip GN args set by developers and not by the chrome ebuild.

    Accepts a dictionary of GN args and strips out args that should be ignored
    when comparing two sets of GN args for the purpose of identifying GN args
    that are generated by the chromeos-chrome ebuild and may have changed.

    Returns a new dictionary including only the GN args to compare.
    """
    args_to_ignore = (
        'dcheck_always_on',
        'ffmpeg_branding',
        'is_component_build',
        'is_debug',
        'optimize_webui',
        'proprietary_codecs',
        'symbol_level',
    )
    return dict((k, v) for k, v in gn_args_dict.items()
                if k not in args_to_ignore)

  def _UpdateGnArgsIfStale(self, out_dir, build_label, gn_args, board):
    """Runs 'gn gen' if gn args are stale or logs a warning."""
    gn_args_file_path = os.path.join(
        self.options.chrome_src, out_dir, build_label, 'args.gn')

    if not self._StaleGnArgs(gn_args, gn_args_file_path):
      return

    if not self.options.gn_gen:
      logging.warning('To update gn args run:')
      logging.warning('gn gen out_$SDK_BOARD/Release --args="$GN_ARGS"')
      return

    logging.warning('Running gn gen')
    cros_build_lib.RunCommand(
        ['gn', 'gen', 'out_%s/Release' % board,
         '--args=%s' % gn_helpers.ToGNString(gn_args)],
        print_cmd=logging.getLogger().isEnabledFor(logging.DEBUG),
        cwd=self.options.chrome_src)

  def _StaleGnArgs(self, gn_args, gn_args_file_path):
    """Returns True if args.gn needs to be updated."""
    if not os.path.exists(gn_args_file_path):
      logging.warning('No args.gn file: %s', gn_args_file_path)
      return True

    new_gn_args = self._StripGnArgs(gn_args)
    old_gn_args = self._StripGnArgs(
        gn_helpers.FromGNArgs(osutils.ReadFile(gn_args_file_path)))
    if new_gn_args == old_gn_args:
      return False

    logging.warning('Stale args.gn file: %s', gn_args_file_path)
    self._LogArgsDiff(old_gn_args, new_gn_args)
    return True

  def _LogArgsDiff(self, cur_args, new_args):
    """Logs the differences between |cur_args| and |new_args|."""
    cur_keys = set(cur_args.keys())
    new_keys = set(new_args.keys())

    for k in new_keys - cur_keys:
      logging.info('MISSING ARG: %s = %s', k, new_args[k])

    for k in cur_keys - new_keys:
      logging.info('EXTRA ARG: %s = %s', k, cur_args[k])

    for k in new_keys & cur_keys:
      v_cur = cur_args[k]
      v_new = new_args[k]
      if v_cur != v_new:
        logging.info('MISMATCHED ARG: %s: %s != %s', k, v_cur, v_new)

  def _SetupTCEnvironment(self, sdk_ctx, options, env, gn_is_clang):
    """Sets up toolchain-related environment variables."""
    target_tc_path = sdk_ctx.key_map[self.sdk.TARGET_TOOLCHAIN_KEY].path
    tc_bin_path = os.path.join(target_tc_path, 'bin')
    env['PATH'] = '%s:%s' % (tc_bin_path, os.environ['PATH'])

    for var in ('CXX', 'CC', 'LD'):
      env[var] = self._FixGoldPath(env[var], target_tc_path)

    chrome_clang_path = os.path.join(options.chrome_src, self._CHROME_CLANG_DIR)

    # Either we are forcing the use of clang through options or GN
    # args say we should be using clang.
    if options.clang or gn_is_clang:
      clang_prepend_flags = ['-Wno-unknown-warning-option']
      # crbug.com/686903
      clang_append_flags = ['-Wno-inline-asm']

      env['CC'] = ' '.join([sdk_ctx.target_tc + '-clang'] +
                           env['CC'].split()[1:] + clang_prepend_flags)
      env['CXX'] = ' '.join([sdk_ctx.target_tc + '-clang++'] +
                            env['CXX'].split()[1:] + clang_prepend_flags)
      env['CFLAGS'] = ' '.join(env['CFLAGS'].split() + clang_append_flags)
      env['CXXFLAGS'] = ' '.join(env['CXXFLAGS'].split() + clang_append_flags)
      env['LD'] = env['CXX']

    # For host compiler, we use the compiler that comes with Chrome
    # instead of the target compiler.
    env['CC_host'] = os.path.join(chrome_clang_path, 'clang')
    env['CXX_host'] = os.path.join(chrome_clang_path, 'clang++')
    env['LD_host'] = env['CXX_host']

    binutils_path = os.path.join(options.chrome_src, self._HOST_BINUTILS_DIR)
    env['AR_host'] = os.path.join(binutils_path, 'ar')

  def _SetupEnvironment(self, board, sdk_ctx, options, goma_dir=None,
                        goma_port=None):
    """Sets environment variables to export to the SDK shell."""
    if options.chroot:
      sysroot = os.path.join(options.chroot, 'build', board)
      if not os.path.isdir(sysroot) and not options.cmd:
        logging.warning("Because --chroot is set, expected a sysroot to be at "
                        "%s, but couldn't find one.", sysroot)
    else:
      sysroot = sdk_ctx.key_map[constants.CHROME_SYSROOT_TAR].path

    environment = os.path.join(sdk_ctx.key_map[constants.CHROME_ENV_TAR].path,
                               'environment')
    if options.chroot:
      # Override with the environment from the chroot if available (i.e.
      # build_packages or emerge chromeos-chrome has been run for |board|).
      env_path = os.path.join(sysroot, 'var', 'db', 'pkg', 'chromeos-base',
                              'chromeos-chrome-*')
      env_glob = glob.glob(env_path)
      if len(env_glob) != 1:
        logging.warning('Multiple Chrome versions in %s. This can be resolved'
                        ' by running "eclean-$BOARD -d packages". Using'
                        ' environment from: %s', env_path, environment)
      elif not os.path.isdir(env_glob[0]):
        logging.warning('Environment path not found: %s. Using enviroment from:'
                        ' %s.', env_path, environment)
      else:
        chroot_env_file = os.path.join(env_glob[0], 'environment.bz2')
        if os.path.isfile(chroot_env_file):
          # Log a warning here since this is new behavior that is not obvious.
          logging.notice('Environment fetched from: %s', chroot_env_file)
          # Uncompress enviornment.bz2 to pass to osutils.SourceEnvironment.
          chroot_cache = os.path.join(options.cache_dir, COMMAND_NAME, 'chroot')
          osutils.SafeMakedirs(chroot_cache)
          environment = os.path.join(chroot_cache, 'environment_%s' % board)
          cros_build_lib.UncompressFile(chroot_env_file, environment)

    env = osutils.SourceEnvironment(environment, self.EBUILD_ENV)
    gn_args = gn_helpers.FromGNArgs(env['GN_ARGS'])
    self._SetupTCEnvironment(sdk_ctx, options, env, gn_args['is_clang'])

    # Add managed components to the PATH.
    env['PATH'] = '%s:%s' % (constants.CHROMITE_BIN_DIR, env['PATH'])
    env['PATH'] = '%s:%s' % (os.path.dirname(self.sdk.gs_ctx.gsutil_bin),
                             env['PATH'])

    # Export internally referenced variables.
    os.environ[self.sdk.SDK_BOARD_ENV] = board
    if options.sdk_path:
      os.environ[self.sdk.SDK_PATH_ENV] = options.sdk_path
    os.environ[self.sdk.SDK_VERSION_ENV] = sdk_ctx.version

    # Add board and sdk version as gn args so that tests can bind them in
    # test wrappers generated at compile time.
    gn_args['cros_board'] = board
    gn_args['cros_sdk_version'] = sdk_ctx.version

    # Export the board/version info in a more accessible way, so developers can
    # reference them in their chrome_sdk.bashrc files, as well as within the
    # chrome-sdk shell.
    for var in [self.sdk.SDK_VERSION_ENV, self.sdk.SDK_BOARD_ENV]:
      env[var.lstrip('%')] = os.environ[var]

    # Export Goma information.
    if goma_dir:
      env[self.SDK_GOMA_DIR_ENV] = goma_dir
    if goma_port:
      env[self.SDK_GOMA_PORT_ENV] = goma_port

    # SYSROOT is necessary for Goma and the sysroot wrapper.
    env['SYSROOT'] = sysroot

    # Deprecated options warnings. TODO(stevenjb): Eliminate these entirely
    # once removed from any builders.
    if options.component:
      logging.warning('--component is deprecated, ignoring')
    if options.fastbuild:
      logging.warning('--fastbuild is deprecated, ignoring')

    gn_args['target_sysroot'] = sysroot
    gn_args.pop('pkg_config', None)
    # pkg_config only affects the target and comes from the sysroot.
    # host_pkg_config is used for programs compiled for use later in the build.
    gn_args['host_pkg_config'] = 'pkg-config'
    if options.clang:
      gn_args['is_clang'] = True
    if options.internal:
      gn_args['is_chrome_branded'] = True
      gn_args['is_official_build'] = True
    else:
      gn_args.pop('is_chrome_branded', None)
      gn_args.pop('is_official_build', None)
      gn_args.pop('internal_gles2_conform_tests', None)

    # For SimpleChrome, we use the binutils that comes bundled within Chrome.
    # We should not use the binutils from the host system.
    gn_args['linux_use_bundled_binutils'] = True

    # Need to reset these after the env vars have been fixed by
    # _SetupTCEnvironment.
    gn_args['cros_host_is_clang'] = True
    # v8 snapshot is built on the host, so we need to set this.
    # See crosbug/618346.
    gn_args['cros_v8_snapshot_is_clang'] = True
    #
    gn_args['cros_target_cc'] = env['CC']
    gn_args['cros_target_cxx'] = env['CXX']
    gn_args['cros_target_ld'] = env['LD']
    gn_args['cros_target_extra_cflags'] = env.get('CFLAGS', '')
    gn_args['cros_target_extra_cxxflags'] = env.get('CXXFLAGS', '')
    gn_args['cros_host_cc'] = env['CC_host']
    gn_args['cros_host_cxx'] = env['CXX_host']
    gn_args['cros_host_ld'] = env['LD_host']
    gn_args['cros_host_ar'] = env['AR_host']
    gn_args['cros_v8_snapshot_cc'] = env['CC_host']
    gn_args['cros_v8_snapshot_cxx'] = env['CXX_host']
    gn_args['cros_v8_snapshot_ld'] = env['LD_host']
    gn_args['cros_v8_snapshot_ar'] = env['AR_host']
    # No need to adjust CFLAGS and CXXFLAGS for GN since the only
    # adjustment made in _SetupTCEnvironment is for split debug which
    # is done with 'use_debug_fission'.

    # Enable goma if requested.
    if goma_dir:
      gn_args['use_goma'] = True
      gn_args['goma_dir'] = goma_dir
    elif not options.goma:
      # If --nogoma option is explicitly set, disable goma, even if it is
      # used in the original GN_ARGS.
      gn_args['use_goma'] = False

    gn_args.pop('internal_khronos_glcts_tests', None)  # crbug.com/588080

    # Disable ThinLTO and CFI for simplechrome. Tryjob machines do not have
    # enough file descriptors to use. crbug.com/789607
    if 'use_thin_lto' in gn_args:
      gn_args['use_thin_lto'] = False
    if 'is_cfi' in gn_args:
      gn_args['is_cfi'] = False
    if 'use_cfi_cast' in gn_args:
      gn_args['use_cfi_cast'] = False
    # We need to remove the flag below from cros_target_extra_ldflags.
    # The format of ld flags is something like
    # '-Wl,-O1 -Wl,-O2 -Wl,--as-needed -stdlib=libc++'
    extra_thinlto_flag = '-Wl,-plugin-opt,-import-instr-limit=30'
    extra_ldflags = gn_args.get('cros_target_extra_ldflags', '')
    if extra_thinlto_flag in extra_ldflags:
      gn_args['cros_target_extra_ldflags'] = extra_ldflags.replace(
          extra_thinlto_flag, '')

    # We removed webcore debug symbols on release builds on arm.
    # See crbug.com/792999. However, we want to keep the symbols
    # for simplechrome builds.
    # TODO: remove the 'remove_webcore_debug_symbols' once we
    # change the ebuild file.
    gn_args['remove_webcore_debug_symbols'] = False
    gn_args['blink_symbol_level'] = -1

    if options.gn_extra_args:
      gn_args.update(gn_helpers.FromGNArgs(options.gn_extra_args))

    gn_args_env = gn_helpers.ToGNString(gn_args)
    env['GN_ARGS'] = gn_args_env

    # PS1 sets the command line prompt and xterm window caption.
    full_version = sdk_ctx.version
    if full_version != CUSTOM_VERSION:
      full_version = self.sdk.GetFullVersion(sdk_ctx.version)
    env['PS1'] = self._CreatePS1(self.board, full_version,
                                 chroot=options.chroot)

    # Set the useful part of PS1 for users with a custom PROMPT_COMMAND.
    env['CROS_PS1_PREFIX'] = self._PS1Prefix(self.board, full_version,
                                             chroot=options.chroot)

    out_dir = 'out_%s' % self.board
    env['builddir_name'] = out_dir

    build_label = 'Release'

    # This is used by landmines.py to prevent collisions when building both
    # chromeos and android from shared source.
    # For context, see crbug.com/407417
    env['CHROMIUM_OUT_DIR'] = os.path.join(options.chrome_src, out_dir)

    self._UpdateGnArgsIfStale(out_dir, build_label, gn_args, env['SDK_BOARD'])

    return env

  @staticmethod
  def _VerifyGoma(user_rc):
    """Verify that the user has no goma installations set up in user_rc.

    If the user does have a goma installation set up, verify that it's for
    ChromeOS.

    Args:
      user_rc: User-supplied rc file.
    """
    user_env = osutils.SourceEnvironment(user_rc, ['PATH'])
    goma_ctl = osutils.Which('goma_ctl.py', user_env.get('PATH'))
    if goma_ctl is not None:
      logging.warning(
          '%s is adding Goma to the PATH.  Using that Goma instead of the '
          'managed Goma install.', user_rc)

  @staticmethod
  def _VerifyChromiteBin(user_rc):
    """Verify that the user has not set a chromite bin/ dir in user_rc.

    Args:
      user_rc: User-supplied rc file.
    """
    user_env = osutils.SourceEnvironment(user_rc, ['PATH'])
    chromite_bin = osutils.Which('parallel_emerge', user_env.get('PATH'))
    if chromite_bin is not None:
      logging.warning(
          '%s is adding chromite/bin to the PATH.  Remove it from the PATH to '
          'use the the default Chromite.', user_rc)

  @contextlib.contextmanager
  def _GetRCFile(self, env, user_rc):
    """Returns path to dynamically created bashrc file.

    The bashrc file sets the environment variables contained in |env|, as well
    as sources the user-editable chrome_sdk.bashrc file in the user's home
    directory.  That rc file is created if it doesn't already exist.

    Args:
      env: A dictionary of environment variables that will be set by the rc
        file.
      user_rc: User-supplied rc file.
    """
    if not os.path.exists(user_rc):
      osutils.Touch(user_rc, makedirs=True)

    self._VerifyGoma(user_rc)
    self._VerifyChromiteBin(user_rc)

    # We need a temporary rc file to 'wrap' the user configuration file,
    # because running with '--rcfile' causes bash to ignore bash special
    # variables passed through subprocess.Popen, such as PS1.  So we set them
    # here.
    #
    # Having a wrapper rc file will also allow us to inject bash functions into
    # the environment, not just variables.
    with osutils.TempDir() as tempdir:
      # Only source the user's ~/.bashrc if running in interactive mode.
      contents = [
          '[[ -e ~/.bashrc && $- == *i* ]] && . ~/.bashrc\n',
      ]

      for key, value in env.iteritems():
        contents.append("export %s='%s'\n" % (key, value))
      contents.append('. "%s"\n' % user_rc)

      rc_file = os.path.join(tempdir, 'rcfile')
      osutils.WriteFile(rc_file, contents)
      yield rc_file

  def _GomaPort(self, goma_dir):
    """Returns current active Goma port."""
    port = cros_build_lib.RunCommand(
        self.GOMACC_PORT_CMD, cwd=goma_dir, debug_level=logging.DEBUG,
        error_code_ok=True, capture_output=True).output.strip()
    return port

  def _FetchGoma(self):
    """Fetch, install, and start Goma, using cached version if it exists.

    Returns:
      A tuple (dir, port) containing the path to the cached goma/ dir and the
      Goma port.
    """
    common_path = os.path.join(self.options.cache_dir, constants.COMMON_CACHE)
    common_cache = cache.DiskCache(common_path)

    goma_dir = self.options.gomadir
    if not goma_dir:
      ref = common_cache.Lookup(('goma', '2'))
      if not ref.Exists():
        Log('Installing Goma.', silent=self.silent)
        with osutils.TempDir() as tempdir:
          goma_dir = os.path.join(tempdir, 'goma')
          os.mkdir(goma_dir)
          result = cros_build_lib.DebugRunCommand(
              self.FETCH_GOMA_CMD, cwd=goma_dir, error_code_ok=True)
          if result.returncode:
            raise GomaError('Failed to fetch Goma')
         # Update to latest version of goma. We choose the outside-chroot
         # version ('goobuntu') over the chroot version ('chromeos') by
         # supplying input='1' to the following prompt:
         #
         # What is your platform?
         #  1. Goobuntu  2. Precise (32bit)  3. Lucid (32bit)  4. Debian
         #  5. Chrome OS  6. MacOS ? -->
          cros_build_lib.DebugRunCommand(
              ['python2', 'goma_ctl.py', 'update'], cwd=goma_dir, input='1\n')
          ref.SetDefault(goma_dir)
      goma_dir = ref.path

    port = None
    if self.options.start_goma:
      Log('Starting Goma.', silent=self.silent)
      cros_build_lib.DebugRunCommand(
          ['python2', 'goma_ctl.py', 'ensure_start'], cwd=goma_dir)
      port = self._GomaPort(goma_dir)
      Log('Goma is started on port %s', port, silent=self.silent)
      if not port:
        raise GomaError('No Goma port detected')

    return goma_dir, port

  def Run(self):
    """Perform the command."""
    if os.environ.get(SDKFetcher.SDK_VERSION_ENV) is not None:
      cros_build_lib.Die('Already in an SDK shell.')

    src_path = self.options.chrome_src or os.getcwd()
    checkout = path_util.DetermineCheckout(src_path)
    if not checkout.chrome_src_dir:
      cros_build_lib.Die('Chrome checkout not found at %s', src_path)
    self.options.chrome_src = checkout.chrome_src_dir

    if self.options.clang and not self.options.chrome_src:
      cros_build_lib.Die('--clang requires --chrome-src to be set.')

    if self.options.version and self.options.sdk_path:
      cros_build_lib.Die('Cannot specify both --version and --sdk-path.')

    self.silent = bool(self.options.cmd)
    # Lazy initialize because SDKFetcher creates a GSContext() object in its
    # constructor, which may block on user input.
    self.sdk = SDKFetcher(self.options.cache_dir, self.options.board,
                          clear_cache=self.options.clear_sdk_cache,
                          chrome_src=self.options.chrome_src,
                          sdk_path=self.options.sdk_path,
                          toolchain_path=self.options.toolchain_path,
                          silent=self.silent,
                          use_external_config=self.options.use_external_config)

    prepare_version = self.options.version
    if not prepare_version and not self.options.sdk_path:
      prepare_version, _ = self.sdk.UpdateDefaultVersion()

    components = [self.sdk.TARGET_TOOLCHAIN_KEY, constants.CHROME_ENV_TAR]
    if not self.options.chroot:
      components.append(constants.CHROME_SYSROOT_TAR)
    if self.options.download_vm:
      components.append(constants.VM_IMAGE_TAR)

    goma_dir = None
    goma_port = None
    if self.options.goma:
      try:
        goma_dir, goma_port = self._FetchGoma()
      except GomaError as e:
        logging.error('Goma: %s.  Bypass by running with --nogoma.', e)

    with self.sdk.Prepare(components, version=prepare_version,
                          target_tc=self.options.target_tc,
                          toolchain_url=self.options.toolchain_url) as ctx:
      env = self._SetupEnvironment(self.options.board, ctx, self.options,
                                   goma_dir=goma_dir, goma_port=goma_port)

      with self._GetRCFile(env, self.options.bashrc) as rcfile:
        bash_cmd = ['/bin/bash']

        extra_env = None
        if not self.options.cmd:
          bash_cmd.extend(['--rcfile', rcfile, '-i'])
        else:
          # The '"$@"' expands out to the properly quoted positional args
          # coming after the '--'.
          bash_cmd.extend(['-c', '"$@"', '--'])
          bash_cmd.extend(self.options.cmd)
          # When run in noninteractive mode, bash sources the rc file set in
          # BASH_ENV, and ignores the --rcfile flag.
          extra_env = {'BASH_ENV': rcfile}

        # Bash behaves differently when it detects that it's being launched by
        # sshd - it ignores the BASH_ENV variable.  So prevent ssh-related
        # environment variables from being passed through.
        os.environ.pop('SSH_CLIENT', None)
        os.environ.pop('SSH_CONNECTION', None)
        os.environ.pop('SSH_TTY', None)

        cmd_result = cros_build_lib.RunCommand(
            bash_cmd, print_cmd=False, debug_level=logging.CRITICAL,
            error_code_ok=True, extra_env=extra_env, cwd=self.options.cwd)
        if self.options.cmd:
          return cmd_result.returncode
