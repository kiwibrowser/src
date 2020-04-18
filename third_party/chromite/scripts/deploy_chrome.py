# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script that deploys a Chrome build to a device.

The script supports deploying Chrome from these sources:

1. A local build output directory, such as chromium/src/out/[Debug|Release].
2. A Chrome tarball uploaded by a trybot/official-builder to GoogleStorage.
3. A Chrome tarball existing locally.

The script copies the necessary contents of the source location (tarball or
build directory) and rsyncs the contents of the staging directory onto your
device's rootfs.
"""

from __future__ import print_function

import argparse
import collections
import contextlib
import functools
import glob
import multiprocessing
import os
import shlex
import shutil
import time

from chromite.lib import constants
from chromite.lib import failures_lib
from chromite.cli.cros import cros_chrome_sdk
from chromite.lib import chrome_util
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import remote_access as remote
from chromite.lib import timeout_util
from gn_helpers import gn_helpers


KERNEL_A_PARTITION = 2
KERNEL_B_PARTITION = 4

KILL_PROC_MAX_WAIT = 10
POST_KILL_WAIT = 2

MOUNT_RW_COMMAND = 'mount -o remount,rw /'
LSOF_COMMAND = 'lsof %s/chrome'
DBUS_RELOAD_COMMAND = 'killall -HUP dbus-daemon'

_ANDROID_DIR = '/system/chrome'
_ANDROID_DIR_EXTRACT_PATH = 'system/chrome/*'

_CHROME_DIR = '/opt/google/chrome'
_CHROME_DIR_MOUNT = '/mnt/stateful_partition/deploy_rootfs/opt/google/chrome'

_UMOUNT_DIR_IF_MOUNTPOINT_CMD = (
    'if mountpoint -q %(dir)s; then umount %(dir)s; fi')
_BIND_TO_FINAL_DIR_CMD = 'mount --rbind %s %s'
_SET_MOUNT_FLAGS_CMD = 'mount -o remount,exec,suid %s'

DF_COMMAND = 'df -k %s'


def _UrlBaseName(url):
  """Return the last component of the URL."""
  return url.rstrip('/').rpartition('/')[-1]


class DeployFailure(failures_lib.StepFailure):
  """Raised whenever the deploy fails."""


DeviceInfo = collections.namedtuple(
    'DeviceInfo', ['target_dir_size', 'target_fs_free'])


class DeployChrome(object):
  """Wraps the core deployment functionality."""

  def __init__(self, options, tempdir, staging_dir):
    """Initialize the class.

    Args:
      options: options object.
      tempdir: Scratch space for the class.  Caller has responsibility to clean
        it up.
      staging_dir: Directory to stage the files to.
    """
    self.tempdir = tempdir
    self.options = options
    self.staging_dir = staging_dir
    if not self.options.staging_only:
      self.device = remote.RemoteDevice(options.to, port=options.port,
                                        ping=options.ping,
                                        private_key=options.private_key)
    self._target_dir_is_still_readonly = multiprocessing.Event()

    self.copy_paths = chrome_util.GetCopyPaths('chrome')
    self.chrome_dir = _CHROME_DIR

  def _GetRemoteMountFree(self, remote_dir):
    result = self.device.RunCommand(DF_COMMAND % remote_dir)
    line = result.output.splitlines()[1]
    value = line.split()[3]
    multipliers = {
        'G': 1024 * 1024 * 1024,
        'M': 1024 * 1024,
        'K': 1024,
    }
    return int(value.rstrip('GMK')) * multipliers.get(value[-1], 1)

  def _GetRemoteDirSize(self, remote_dir):
    result = self.device.RunCommand('du -ks %s' % remote_dir,
                                    capture_output=True)
    return int(result.output.split()[0])

  def _GetStagingDirSize(self):
    result = cros_build_lib.DebugRunCommand(['du', '-ks', self.staging_dir],
                                            redirect_stdout=True,
                                            capture_output=True)
    return int(result.output.split()[0])

  def _ChromeFileInUse(self):
    result = self.device.RunCommand(LSOF_COMMAND % (self.options.target_dir,),
                                    error_code_ok=True, capture_output=True)
    return result.returncode == 0

  def _Reboot(self):
    # A reboot in developer mode takes a while (and has delays), so the user
    # will have time to read and act on the USB boot instructions below.
    logging.info('Please remember to press Ctrl-U if you are booting from USB.')
    self.device.Reboot()

  def _DisableRootfsVerification(self):
    if not self.options.force:
      logging.error('Detected that the device has rootfs verification enabled.')
      logging.info('This script can automatically remove the rootfs '
                   'verification, which requires that it reboot the device.')
      logging.info('Make sure the device is in developer mode!')
      logging.info('Skip this prompt by specifying --force.')
      if not cros_build_lib.BooleanPrompt('Remove roots verification?', False):
        # Since we stopped Chrome earlier, it's good form to start it up again.
        if self.options.startui:
          logging.info('Starting Chrome...')
          self.device.RunCommand('start ui')
        raise DeployFailure('Need rootfs verification to be disabled. '
                            'Aborting.')

    logging.info('Removing rootfs verification from %s', self.options.to)
    # Running in VM's cause make_dev_ssd's firmware sanity checks to fail.
    # Use --force to bypass the checks.
    cmd = ('/usr/share/vboot/bin/make_dev_ssd.sh --partitions %d '
           '--remove_rootfs_verification --force')
    for partition in (KERNEL_A_PARTITION, KERNEL_B_PARTITION):
      self.device.RunCommand(cmd % partition, error_code_ok=True)

    self._Reboot()

    # Now that the machine has been rebooted, we need to kill Chrome again.
    self._KillProcsIfNeeded()

    # Make sure the rootfs is writable now.
    self._MountRootfsAsWritable(error_code_ok=False)

  def _CheckUiJobStarted(self):
    # status output is in the format:
    # <job_name> <status> ['process' <pid>].
    # <status> is in the format <goal>/<state>.
    try:
      result = self.device.RunCommand('status ui', capture_output=True)
    except cros_build_lib.RunCommandError as e:
      if 'Unknown job' in e.result.error:
        return False
      else:
        raise e

    return result.output.split()[1].split('/')[0] == 'start'

  def _KillProcsIfNeeded(self):
    if self._CheckUiJobStarted():
      logging.info('Shutting down Chrome...')
      self.device.RunCommand('stop ui')

    # Developers sometimes run session_manager manually, in which case we'll
    # need to help shut the chrome processes down.
    try:
      with timeout_util.Timeout(self.options.process_timeout):
        while self._ChromeFileInUse():
          logging.warning('The chrome binary on the device is in use.')
          logging.warning('Killing chrome and session_manager processes...\n')

          self.device.RunCommand("pkill 'chrome|session_manager'",
                                 error_code_ok=True)
          # Wait for processes to actually terminate
          time.sleep(POST_KILL_WAIT)
          logging.info('Rechecking the chrome binary...')
    except timeout_util.TimeoutError:
      msg = ('Could not kill processes after %s seconds.  Please exit any '
             'running chrome processes and try again.'
             % self.options.process_timeout)
      raise DeployFailure(msg)

  def _MountRootfsAsWritable(self, error_code_ok=True):
    """Mounts the rootfs as writable.

    If the command fails, and error_code_ok is True, and the target dir is not
    writable then this function sets self._target_dir_is_still_readonly.

    Args:
      error_code_ok: See remote.RemoteAccess.RemoteSh for details.
    """
    # TODO: Should migrate to use the remount functions in remote_access.
    result = self.device.RunCommand(MOUNT_RW_COMMAND,
                                    error_code_ok=error_code_ok,
                                    capture_output=True)
    if (result.returncode and
        not self.device.IsDirWritable(self.options.target_dir)):
      self._target_dir_is_still_readonly.set()

  def _EnsureTargetDir(self):
    """Ensures that the target directory exists on the remote device."""
    target_dir = self.options.target_dir
    # Any valid /opt directory should already exist so avoid the remote call.
    if os.path.commonprefix([target_dir, '/opt']) == '/opt':
      return
    self.device.RunCommand(['mkdir', '-p', '--mode', '0775', target_dir])

  def _GetDeviceInfo(self):
    """Returns the disk space used and available for the target diectory."""
    steps = [
        functools.partial(self._GetRemoteDirSize, self.options.target_dir),
        functools.partial(self._GetRemoteMountFree, self.options.target_dir)
    ]
    return_values = parallel.RunParallelSteps(steps, return_values=True)
    return DeviceInfo(*return_values)

  def _CheckDeviceFreeSpace(self, device_info):
    """See if target device has enough space for Chrome.

    Args:
      device_info: A DeviceInfo named tuple.
    """
    effective_free = device_info.target_dir_size + device_info.target_fs_free
    staging_size = self._GetStagingDirSize()
    if effective_free < staging_size:
      raise DeployFailure(
          'Not enough free space on the device.  Required: %s MiB, '
          'actual: %s MiB.' % (staging_size / 1024, effective_free / 1024))
    if device_info.target_fs_free < (100 * 1024):
      logging.warning('The device has less than 100MB free.  deploy_chrome may '
                      'hang during the transfer.')

  def _ShouldUseCompression(self):
    """Checks if compression should be used for rsync."""
    if self.options.compress == 'always':
      return True
    elif self.options.compress == 'never':
      return False
    elif self.options.compress == 'auto':
      return not self.device.HasGigabitEthernet()

  def _Deploy(self):
    logging.info('Copying Chrome to %s on device...', self.options.target_dir)
    # CopyToDevice will fall back to scp if rsync is corrupted on stateful.
    # This does not work for deploy.
    if not self.device.HasRsync():
      raise DeployFailure(
          'rsync is not found on the device.\n'
          'Run dev_install on the device to get rsync installed')
    self.device.CopyToDevice('%s/' % os.path.abspath(self.staging_dir),
                             self.options.target_dir,
                             mode='rsync', inplace=True,
                             compress=self._ShouldUseCompression(),
                             debug_level=logging.INFO,
                             verbose=self.options.verbose)

    for p in self.copy_paths:
      if p.mode:
        # Set mode if necessary.
        self.device.RunCommand('chmod %o %s/%s' % (
            p.mode, self.options.target_dir, p.src if not p.dest else p.dest))

    # Send SIGHUP to dbus-daemon to tell it to reload its configs. This won't
    # pick up major changes (bus type, logging, etc.), but all we care about is
    # getting the latest policy from /opt/google/chrome/dbus so that Chrome will
    # be authorized to take ownership of its service names.
    self.device.RunCommand(DBUS_RELOAD_COMMAND, error_code_ok=True)

    if self.options.startui:
      logging.info('Starting UI...')
      self.device.RunCommand('start ui')

  def _CheckConnection(self):
    try:
      logging.info('Testing connection to the device...')
      self.device.RunCommand('true')
    except cros_build_lib.RunCommandError as ex:
      logging.error('Error connecting to the test device.')
      raise DeployFailure(ex)

  def _CheckDeployType(self):
    if self.options.build_dir:
      def BinaryExists(filename):
        """Checks if the passed-in file is present in the build directory."""
        return os.path.exists(os.path.join(self.options.build_dir, filename))

      # Handle non-Chrome deployments.
      if not BinaryExists('chrome'):
        if BinaryExists('app_shell'):
          self.copy_paths = chrome_util.GetCopyPaths('app_shell')

        # TODO(derat): Update _Deploy() and remove this after figuring out how
        # app_shell should be executed.
        self.options.startui = False

  def _PrepareStagingDir(self):
    _PrepareStagingDir(self.options, self.tempdir, self.staging_dir,
                       self.copy_paths, self.chrome_dir)

  def _MountTarget(self):
    logging.info('Mounting Chrome...')

    # Create directory if does not exist
    self.device.RunCommand(['mkdir', '-p', '--mode', '0775',
                            self.options.mount_dir])
    # Umount the existing mount on mount_dir if present first
    self.device.RunCommand(_UMOUNT_DIR_IF_MOUNTPOINT_CMD %
                           {'dir': self.options.mount_dir})
    self.device.RunCommand(_BIND_TO_FINAL_DIR_CMD % (self.options.target_dir,
                                                     self.options.mount_dir))
    # Chrome needs partition to have exec and suid flags set
    self.device.RunCommand(_SET_MOUNT_FLAGS_CMD % (self.options.mount_dir,))

  def Cleanup(self):
    """Clean up RemoteDevice."""
    if not self.options.staging_only:
      self.device.Cleanup()

  def Perform(self):
    self._CheckDeployType()

    # If requested, just do the staging step.
    if self.options.staging_only:
      self._PrepareStagingDir()
      return 0

    # Ensure that the target directory exists before running parallel steps.
    self._EnsureTargetDir()

    # Run setup steps in parallel. If any step fails, RunParallelSteps will
    # stop printing output at that point, and halt any running steps.
    logging.info('Preparing device')
    steps = [self._GetDeviceInfo, self._CheckConnection,
             self._KillProcsIfNeeded, self._MountRootfsAsWritable,
             self._PrepareStagingDir]
    ret = parallel.RunParallelSteps(steps, halt_on_error=True,
                                    return_values=True)
    self._CheckDeviceFreeSpace(ret[0])

    # If we're trying to deploy to a dir which is not writable and we failed
    # to mark the rootfs as writable, try disabling rootfs verification.
    if self._target_dir_is_still_readonly.is_set():
      self._DisableRootfsVerification()

    if self.options.mount_dir is not None:
      self._MountTarget()

    # Actually deploy Chrome to the device.
    self._Deploy()


def ValidateStagingFlags(value):
  """Convert formatted string to dictionary."""
  return chrome_util.ProcessShellFlags(value)


def ValidateGnArgs(value):
  """Convert GN_ARGS-formatted string to dictionary."""
  return gn_helpers.FromGNArgs(value)


def _CreateParser():
  """Create our custom parser."""
  parser = commandline.ArgumentParser(description=__doc__, caching=True)

  # TODO(rcui): Have this use the UI-V2 format of having source and target
  # device be specified as positional arguments.
  parser.add_argument('--force', action='store_true', default=False,
                      help='Skip all prompts (i.e., for disabling of rootfs '
                           'verification).  This may result in the target '
                           'machine being rebooted.')
  sdk_board_env = os.environ.get(cros_chrome_sdk.SDKFetcher.SDK_BOARD_ENV)
  parser.add_argument('--board', default=sdk_board_env,
                      help="The board the Chrome build is targeted for.  When "
                           "in a 'cros chrome-sdk' shell, defaults to the SDK "
                           "board.")
  parser.add_argument('--build-dir', type='path',
                      help='The directory with Chrome build artifacts to '
                           'deploy from. Typically of format '
                           '<chrome_root>/out/Debug. When this option is used, '
                           'the GN_ARGS environment variable must be set.')
  parser.add_argument('--target-dir', type='path',
                      default=None,
                      help='Target directory on device to deploy Chrome into.')
  parser.add_argument('-g', '--gs-path', type='gs_path',
                      help='GS path that contains the chrome to deploy.')
  parser.add_argument('--private-key', type='path', default=None,
                      help='An ssh private key to use when deploying to '
                           'a CrOS device.')
  parser.add_argument('--nostartui', action='store_false', dest='startui',
                      default=True,
                      help="Don't restart the ui daemon after deployment.")
  parser.add_argument('--nostrip', action='store_false', dest='dostrip',
                      default=True,
                      help="Don't strip binaries during deployment.  Warning: "
                           'the resulting binaries will be very large!')
  parser.add_argument('-p', '--port', type=int, default=remote.DEFAULT_SSH_PORT,
                      help='Port of the target device to connect to.')
  parser.add_argument('-t', '--to',
                      help='The IP address of the CrOS device to deploy to.')
  parser.add_argument('-v', '--verbose', action='store_true', default=False,
                      help='Show more debug output.')
  parser.add_argument('--mount-dir', type='path', default=None,
                      help='Deploy Chrome in target directory and bind it '
                           'to the directory specified by this flag.'
                           'Any existing mount on this directory will be '
                           'umounted first.')
  parser.add_argument('--mount', action='store_true', default=False,
                      help='Deploy Chrome to default target directory and bind '
                           'it to the default mount directory.'
                           'Any existing mount on this directory will be '
                           'umounted first.')

  group = parser.add_argument_group('Advanced Options')
  group.add_argument('-l', '--local-pkg-path', type='path',
                     help='Path to local chrome prebuilt package to deploy.')
  group.add_argument('--sloppy', action='store_true', default=False,
                     help='Ignore when mandatory artifacts are missing.')
  group.add_argument('--staging-flags', default=None, type=ValidateStagingFlags,
                     help=('Extra flags to control staging.  Valid flags are - '
                           '%s' % ', '.join(chrome_util.STAGING_FLAGS)))
  # TODO(stevenjb): Remove --strict entirely once removed from the ebuild.
  group.add_argument('--strict', action='store_true', default=False,
                     help='Deprecated. Default behavior is "strict". Use '
                          '--sloppy to omit warnings for missing optional '
                          'files.')
  group.add_argument('--strip-flags', default=None,
                     help="Flags to call the 'strip' binutil tool with.  "
                          "Overrides the default arguments.")
  group.add_argument('--ping', action='store_true', default=False,
                     help='Ping the device before connection attempt.')
  group.add_argument('--process-timeout', type=int,
                     default=KILL_PROC_MAX_WAIT,
                     help='Timeout for process shutdown.')

  group = parser.add_argument_group(
      'Metadata Overrides (Advanced)',
      description='Provide all of these overrides in order to remove '
                  'dependencies on metadata.json existence.')
  group.add_argument('--target-tc', action='store', default=None,
                     help='Override target toolchain name, e.g. '
                          'x86_64-cros-linux-gnu')
  group.add_argument('--toolchain-url', action='store', default=None,
                     help='Override toolchain url format pattern, e.g. '
                          '2014/04/%%(target)s-2014.04.23.220740.tar.xz')

  # DEPRECATED: --gyp-defines is ignored, but retained for backwards
  # compatibility. TODO(stevenjb): Remove once eliminated from the ebuild.
  parser.add_argument('--gyp-defines', default=None, type=ValidateStagingFlags,
                      help=argparse.SUPPRESS)

  # GN_ARGS (args.gn) used to build Chrome. Influences which files are staged
  # when --build-dir is set. Defaults to reading from the GN_ARGS env variable.
  # CURRENLY IGNORED, ADDED FOR FORWARD COMPATABILITY.
  parser.add_argument('--gn-args', default=None, type=ValidateGnArgs,
                      help=argparse.SUPPRESS)

  # Path of an empty directory to stage chrome artifacts to.  Defaults to a
  # temporary directory that is removed when the script finishes. If the path
  # is specified, then it will not be removed.
  parser.add_argument('--staging-dir', type='path', default=None,
                      help=argparse.SUPPRESS)
  # Only prepare the staging directory, and skip deploying to the device.
  parser.add_argument('--staging-only', action='store_true', default=False,
                      help=argparse.SUPPRESS)
  # Path to a binutil 'strip' tool to strip binaries with.  The passed-in path
  # is used as-is, and not normalized.  Used by the Chrome ebuild to skip
  # fetching the SDK toolchain.
  parser.add_argument('--strip-bin', default=None, help=argparse.SUPPRESS)
  parser.add_argument('--compress', action='store', default='auto',
                      choices=('always', 'never', 'auto'),
                      help='Choose the data compression behavior. Default '
                           'is set to "auto", that disables compression if '
                           'the target device has a gigabit ethernet port.')
  return parser


def _ParseCommandLine(argv):
  """Parse args, and run environment-independent checks."""
  parser = _CreateParser()
  options = parser.parse_args(argv)

  if not any([options.gs_path, options.local_pkg_path, options.build_dir]):
    parser.error('Need to specify either --gs-path, --local-pkg-path, or '
                 '--build-dir')
  if options.build_dir and any([options.gs_path, options.local_pkg_path]):
    parser.error('Cannot specify both --build_dir and '
                 '--gs-path/--local-pkg-patch')
  if (not options.board and options.build_dir and options.dostrip and
      not options.strip_bin):
    parser.error('--board is required for stripping.')
  if options.gs_path and options.local_pkg_path:
    parser.error('Cannot specify both --gs-path and --local-pkg-path')
  if not (options.staging_only or options.to):
    parser.error('Need to specify --to')
  if options.staging_flags and not options.build_dir:
    parser.error('--staging-flags require --build-dir to be set.')

  if options.strict:
    logging.warning('--strict is deprecated.')
  if options.gyp_defines:
    logging.warning('--gyp-defines is deprecated.')

  if options.mount or options.mount_dir:
    if not options.target_dir:
      options.target_dir = _CHROME_DIR_MOUNT
  else:
    if not options.target_dir:
      options.target_dir = _CHROME_DIR

  if options.mount and not options.mount_dir:
    options.mount_dir = _CHROME_DIR

  return options


def _PostParseCheck(options):
  """Perform some usage validation (after we've parsed the arguments).

  Args:
    options: The options object returned by the cli parser.
  """
  if options.local_pkg_path and not os.path.isfile(options.local_pkg_path):
    cros_build_lib.Die('%s is not a file.', options.local_pkg_path)

  if not options.gn_args:
    gn_env = os.getenv('GN_ARGS')
    if gn_env is not None:
      options.gn_args = gn_helpers.FromGNArgs(gn_env)
      logging.debug('GN_ARGS taken from environment: %s', options.gn_args)

  if not options.staging_flags:
    use_env = os.getenv('USE')
    if use_env is not None:
      options.staging_flags = ' '.join(set(use_env.split()).intersection(
          chrome_util.STAGING_FLAGS))
      logging.info('Staging flags taken from USE in environment: %s',
                   options.staging_flags)


def _FetchChromePackage(cache_dir, tempdir, gs_path):
  """Get the chrome prebuilt tarball from GS.

  Returns:
    Path to the fetched chrome tarball.
  """
  gs_ctx = gs.GSContext(cache_dir=cache_dir, init_boto=True)
  files = gs_ctx.LS(gs_path)
  files = [found for found in files if
           _UrlBaseName(found).startswith('%s-' % constants.CHROME_PN)]
  if not files:
    raise Exception('No chrome package found at %s' % gs_path)
  elif len(files) > 1:
    # - Users should provide us with a direct link to either a stripped or
    #   unstripped chrome package.
    # - In the case of being provided with an archive directory, where both
    #   stripped and unstripped chrome available, use the stripped chrome
    #   package.
    # - Stripped chrome pkg is chromeos-chrome-<version>.tar.gz
    # - Unstripped chrome pkg is chromeos-chrome-<version>-unstripped.tar.gz.
    files = [f for f in files if not 'unstripped' in f]
    assert len(files) == 1
    logging.warning('Multiple chrome packages found.  Using %s', files[0])

  filename = _UrlBaseName(files[0])
  logging.info('Fetching %s...', filename)
  gs_ctx.Copy(files[0], tempdir, print_cmd=False)
  chrome_path = os.path.join(tempdir, filename)
  assert os.path.exists(chrome_path)
  return chrome_path


@contextlib.contextmanager
def _StripBinContext(options):
  if not options.dostrip:
    yield None
  elif options.strip_bin:
    yield options.strip_bin
  else:
    sdk = cros_chrome_sdk.SDKFetcher(options.cache_dir, options.board)
    components = (sdk.TARGET_TOOLCHAIN_KEY, constants.CHROME_ENV_TAR)
    with sdk.Prepare(components=components, target_tc=options.target_tc,
                     toolchain_url=options.toolchain_url) as ctx:
      env_path = os.path.join(ctx.key_map[constants.CHROME_ENV_TAR].path,
                              constants.CHROME_ENV_FILE)
      strip_bin = osutils.SourceEnvironment(env_path, ['STRIP'])['STRIP']
      strip_bin = os.path.join(ctx.key_map[sdk.TARGET_TOOLCHAIN_KEY].path,
                               'bin', os.path.basename(strip_bin))
      yield strip_bin


def _PrepareStagingDir(options, tempdir, staging_dir, copy_paths=None,
                       chrome_dir=_CHROME_DIR):
  """Place the necessary files in the staging directory.

  The staging directory is the directory used to rsync the build artifacts over
  to the device.  Only the necessary Chrome build artifacts are put into the
  staging directory.
  """
  osutils.SafeMakedirs(staging_dir)
  os.chmod(staging_dir, 0o755)
  if options.build_dir:
    with _StripBinContext(options) as strip_bin:
      strip_flags = (None if options.strip_flags is None else
                     shlex.split(options.strip_flags))
      chrome_util.StageChromeFromBuildDir(
          staging_dir, options.build_dir, strip_bin,
          sloppy=options.sloppy, gn_args=options.gn_args,
          staging_flags=options.staging_flags,
          strip_flags=strip_flags, copy_paths=copy_paths)
  else:
    pkg_path = options.local_pkg_path
    if options.gs_path:
      pkg_path = _FetchChromePackage(options.cache_dir, tempdir,
                                     options.gs_path)

    assert pkg_path
    logging.info('Extracting %s...', pkg_path)
    # Extract only the ./opt/google/chrome contents, directly into the staging
    # dir, collapsing the directory hierarchy.
    if pkg_path[-4:] == '.zip':
      cros_build_lib.DebugRunCommand(
          ['unzip', '-X', pkg_path, _ANDROID_DIR_EXTRACT_PATH, '-d',
           staging_dir])
      for filename in glob.glob(os.path.join(staging_dir, 'system/chrome/*')):
        shutil.move(filename, staging_dir)
      osutils.RmDir(os.path.join(staging_dir, 'system'), ignore_missing=True)
    else:
      cros_build_lib.DebugRunCommand(
          ['tar', '--strip-components', '4', '--extract',
           '--preserve-permissions', '--file', pkg_path, '.%s' % chrome_dir],
          cwd=staging_dir)


def main(argv):
  options = _ParseCommandLine(argv)
  _PostParseCheck(options)

  # Set cros_build_lib debug level to hide RunCommand spew.
  if options.verbose:
    logging.getLogger().setLevel(logging.DEBUG)
  else:
    logging.getLogger().setLevel(logging.INFO)

  with osutils.TempDir(set_global=True) as tempdir:
    staging_dir = options.staging_dir
    if not staging_dir:
      staging_dir = os.path.join(tempdir, 'chrome')

    deploy = DeployChrome(options, tempdir, staging_dir)
    try:
      deploy.Perform()
    except failures_lib.StepFailure as ex:
      raise SystemExit(str(ex).strip())
    deploy.Cleanup()
