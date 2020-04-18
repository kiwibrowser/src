# -*- coding: utf-8 -*-
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing methods and classes to interact with a devserver instance.
"""

from __future__ import print_function

import multiprocessing
import os
import socket
import shutil
import sys
import tempfile
import httplib
import urllib2
import urlparse

from chromite.lib import constants
from chromite.cli import command
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import remote_access
from chromite.lib import timeout_util


DEFAULT_PORT = 8080

DEVSERVER_PKG_DIR = os.path.join(constants.SOURCE_ROOT, 'src/platform/dev')
DEFAULT_STATIC_DIR = path_util.FromChrootPath(
    os.path.join(constants.SOURCE_ROOT, 'src', 'platform', 'dev', 'static'))

XBUDDY_REMOTE = 'remote'
XBUDDY_LOCAL = 'local'

ROOTFS_FILENAME = 'update.gz'
STATEFUL_FILENAME = 'stateful.tgz'


class ImagePathError(Exception):
  """Raised when the provided path can't be resolved to an image."""


def ConvertTranslatedPath(original_path, translated_path):
  """Converts a translated xbuddy path to an xbuddy path.

  Devserver/xbuddy does not accept requests with translated xbuddy
  path (build-id/version/image-name). This function converts such a
  translated path to an xbuddy path that is suitable to used in
  devserver requests.

  Args:
    original_path: the xbuddy path before translation.
      (e.g., remote/peppy/latest-canary).
    translated_path: the translated xbuddy path
      (e.g., peppy-release/R36-5760.0.0).

  Returns:
    A xbuddy path uniquely identifies a build and can be used in devserver
      requests: {local|remote}/build-id/version/image_type
  """
  chunks = translated_path.split(os.path.sep)
  chunks[-1] = constants.IMAGE_NAME_TO_TYPE[chunks[-1]]

  if GetXbuddyPath(original_path).startswith(XBUDDY_REMOTE):
    chunks = [XBUDDY_REMOTE] + chunks
  else:
    chunks = [XBUDDY_LOCAL] + chunks

  return os.path.sep.join(chunks)


def GetXbuddyPath(path):
  """A helper function to parse an xbuddy path.

  Args:
    path: Either a path without no scheme or an xbuddy://path/for/xbuddy

  Returns:
    path/for/xbuddy if |path| is xbuddy://path/for/xbuddy; otherwise,
    returns |path|.

  Raises:
    ValueError if |path| uses any scheme other than xbuddy://.
  """
  parsed = urlparse.urlparse(path)

  # pylint: disable=E1101
  if parsed.scheme == 'xbuddy':
    return '%s%s' % (parsed.netloc, parsed.path)
  elif parsed.scheme == '':
    logging.debug('Assuming %s is an xbuddy path.', path)
    return path
  else:
    raise ValueError('Do not support scheme %s.', parsed.scheme)


def GetImagePathWithXbuddy(path, board, version=None,
                           static_dir=DEFAULT_STATIC_DIR, lookup_only=False):
  """Gets image path and resolved XBuddy path using xbuddy.

  Ask xbuddy to translate |path|, and if necessary, download and stage the
  image, then return a translated path to the image. Also returns the resolved
  XBuddy path, which may be useful for subsequent calls in case the argument is
  an alias.

  Args:
    path: The xbuddy path.
    board: The default board to use if board is not specified in |path|.
    version: The default version to use if one is not specified in |path|.
    static_dir: Static directory to stage the image in.
    lookup_only: Caller only wants to translate the path not download the
      artifact.

  Returns:
    A tuple consisting of a translated path to the image
    (build-id/version/image_name) as well as the fully resolved XBuddy path (in
    the case where |path| is an XBuddy alias).
  """
  # Since xbuddy often wants to use gsutil from $PATH, make sure our local copy
  # shows up first.
  upath = os.environ['PATH'].split(os.pathsep)
  upath.insert(0, os.path.dirname(gs.GSContext.GetDefaultGSUtilBin()))
  os.environ['PATH'] = os.pathsep.join(upath)

  # Import xbuddy for translating, downloading and staging the image.
  if not os.path.exists(DEVSERVER_PKG_DIR):
    raise Exception('Cannot find xbuddy module. Devserver package directory '
                    'does not exist: %s' % DEVSERVER_PKG_DIR)
  sys.path.append(DEVSERVER_PKG_DIR)
  # pylint: disable=import-error
  import xbuddy
  import cherrypy

  # If we are using the progress bar, quiet the logging output of cherrypy.
  if command.UseProgressBar():
    if (hasattr(cherrypy.log, 'access_log') and
        hasattr(cherrypy.log, 'error_log')):
      cherrypy.log.access_log.setLevel(logging.NOTICE)
      cherrypy.log.error_log.setLevel(logging.NOTICE)
    else:
      cherrypy.config.update({'server.log_to_screen': False})

  xb = xbuddy.XBuddy(static_dir=static_dir, board=board, version=version,
                     log_screen=False)
  path_list = GetXbuddyPath(path).rsplit(os.path.sep)
  try:
    if lookup_only:
      build_id, file_name = xb.Translate(path_list)
    else:
      build_id, file_name = xb.Get(path_list)

    resolved_path, _ = xb.LookupAlias(os.path.sep.join(path_list))
    return os.path.join(build_id, file_name), resolved_path
  except xbuddy.XBuddyException as e:
    logging.error('Locating image "%s" failed. The path might not be valid or '
                  'the image might not exist.', path)
    raise ImagePathError('Cannot locate image %s: %s' % (path, e))


def GenerateXbuddyRequest(path, req_type):
  """Generate an xbuddy request used to retreive payloads.

  This function generates a xbuddy request based on |path| and
  |req_type|, which can be used to query the devserver. For request
  type 'image' ('update'), the devserver will repond with a URL
  pointing to the folder where the image (update payloads) is stored.

  Args:
    path: An xbuddy path (with or without xbuddy://).
    req_type: xbuddy request type ('update', 'image', or 'translate').

  Returns:
    A xbuddy request.
  """
  if req_type == 'update':
    return 'xbuddy/%s?for_update=true&return_dir=true' % GetXbuddyPath(path)
  elif req_type == 'image':
    return 'xbuddy/%s?return_dir=true' % GetXbuddyPath(path)
  elif req_type == 'translate':
    return 'xbuddy_translate/%s' % GetXbuddyPath(path)
  else:
    raise ValueError('Does not support xbuddy request type %s' % req_type)


def TranslatedPathToLocalPath(translated_path, static_dir):
  """Convert the translated path to a local path to the image file.

  Args:
    translated_path: the translated xbuddy path
      (e.g., peppy-release/R36-5760.0.0/chromiumos_image).
    static_dir: The static directory used by the devserver.

  Returns:
    A local path to the image file.
  """
  real_path = osutils.ExpandPath(os.path.join(static_dir, translated_path))

  if os.path.exists(real_path):
    return real_path
  else:
    return path_util.FromChrootPath(real_path)


def GetUpdatePayloadsFromLocalPath(path, payload_dir,
                                   src_image_to_delta=None,
                                   static_dir=DEFAULT_STATIC_DIR):
  """Generates update payloads from a local image path.

  This function wraps around ConvertLocalPathToXbuddy and GetUpdatePayloads,
  managing the creation and destruction of the necessary temporary directories
  required by this process.

  Args:
    path: Path to an image.
    payload_dir: The directory to store the payloads. On failure, the devserver
                 log will be copied to |payload_dir|.
    src_image_to_delta: Image used as the base to generate the delta payloads.
    static_dir: Devserver static dir to use.
  """

  with cros_build_lib.ContextManagerStack() as stack:
    image_tempdir = stack.Add(
        osutils.TempDir,
        base_dir=path_util.FromChrootPath('/tmp'),
        prefix='dev_server_wrapper_local_image', sudo_rm=True)
    static_tempdir = stack.Add(osutils.TempDir,
                               base_dir=static_dir,
                               prefix='local_image', sudo_rm=True)
    xbuddy_path = ConvertLocalPathToXbuddyPath(path, image_tempdir,
                                               static_tempdir, static_dir)
    GetUpdatePayloads(xbuddy_path, payload_dir,
                      src_image_to_delta=src_image_to_delta,
                      static_dir=static_dir)


def ConvertLocalPathToXbuddyPath(path, image_tempdir, static_tempdir,
                                 static_dir=DEFAULT_STATIC_DIR):
  """Converts |path| to an xbuddy path.

  This function copies the image into a temprary directory in chroot
  and creates a symlink in static_dir for devserver/xbuddy to
  access.

  Note that the temporary directories need to be cleaned up by the caller
  once they are no longer needed.

  Args:
    path: Path to an image.
    image_tempdir: osutils.TempDir instance to copy the image into. The
                   directory must be located within the chroot.
    static_tempdir: osutils.TempDir instance to be symlinked to by the static
                    directory.
    static_dir: Static directory to create the symlink in.

  Returns:
    The xbuddy path for |path|
  """
  tempdir_path = image_tempdir.tempdir
  logging.info('Copying image to temporary directory %s', tempdir_path)
  # Devserver only knows the image names listed in IMAGE_TYPE_TO_NAME.
  # Rename the image to chromiumos_test_image.bin when copying.
  TEMP_IMAGE_TYPE = 'test'
  shutil.copy(path,
              os.path.join(tempdir_path,
                           constants.IMAGE_TYPE_TO_NAME[TEMP_IMAGE_TYPE]))
  chroot_path = path_util.ToChrootPath(tempdir_path)
  # Create and link static_dir/local_imagexxxx/link to the image
  # folder, so that xbuddy/devserver can understand the path.
  relative_dir = os.path.join(os.path.basename(static_tempdir.tempdir), 'link')
  symlink_path = os.path.join(static_dir, relative_dir)
  logging.info('Creating a symlink %s -> %s', symlink_path, chroot_path)
  os.symlink(chroot_path, symlink_path)
  return os.path.join(relative_dir, TEMP_IMAGE_TYPE)


def GetUpdatePayloads(path, payload_dir, board=None,
                      src_image_to_delta=None, timeout=60 * 15,
                      static_dir=DEFAULT_STATIC_DIR):
  """Launch devserver to get the update payloads.

  Args:
    path: The xbuddy path.
    payload_dir: The directory to store the payloads. On failure, the devserver
                 log will be copied to |payload_dir|.
    board: The default board to use when |path| is None.
    src_image_to_delta: Image used as the base to generate the delta payloads.
    timeout: Timeout for launching devserver (seconds).
    static_dir: Devserver static dir to use.
  """
  ds = DevServerWrapper(static_dir=static_dir, src_image=src_image_to_delta,
                        board=board)
  req = GenerateXbuddyRequest(path, 'update')
  logging.info('Starting local devserver to generate/serve payloads...')
  try:
    ds.Start()
    url = ds.OpenURL(ds.GetURL(sub_dir=req), timeout=timeout)
    ds.DownloadFile(os.path.join(url, ROOTFS_FILENAME), payload_dir)
    ds.DownloadFile(os.path.join(url, STATEFUL_FILENAME), payload_dir)
  except DevServerException:
    logging.warning(ds.TailLog() or 'No devserver log is available.')
    raise
  else:
    logging.debug(ds.TailLog() or 'No devserver log is available.')
  finally:
    ds.Stop()
    if os.path.exists(ds.log_file):
      shutil.copyfile(ds.log_file,
                      os.path.join(payload_dir, 'local_devserver.log'))
    else:
      logging.warning('Could not find %s', ds.log_file)


def GenerateUpdateId(target, src, key, for_vm):
  """Returns a simple representation id of |target| and |src| paths.

  Args:
    target: Target image of the update payloads.
    src: Base image to of the delta update payloads.
    key: Private key used to sign the payloads.
    for_vm: Whether the update payloads are to be used in a VM .
  """
  update_id = target
  if src:
    update_id = '->'.join([src, update_id])

  if key:
    update_id = '+'.join([update_id, key])

  if not for_vm:
    update_id = '+'.join([update_id, 'patched_kernel'])

  return update_id


class DevServerException(Exception):
  """Base exception class of devserver errors."""


class DevServerStartupError(DevServerException):
  """Thrown when the devserver fails to start up."""


class DevServerStopError(DevServerException):
  """Thrown when the devserver fails to stop."""


class DevServerResponseError(DevServerException):
  """Thrown when the devserver responds with an error."""


class DevServerConnectionError(DevServerException):
  """Thrown when unable to connect to devserver."""


class DevServerWrapper(multiprocessing.Process):
  """A Simple wrapper around a dev server instance."""

  # Wait up to 15 minutes for the dev server to start. It can take a
  # while to start when generating payloads in parallel.
  DEV_SERVER_TIMEOUT = 900
  KILL_TIMEOUT = 10

  def __init__(self, static_dir=None, port=None, log_dir=None, src_image=None,
               board=None):
    """Initialize a DevServerWrapper instance.

    Args:
      static_dir: The static directory to be used by the devserver.
      port: The port to used by the devserver.
      log_dir: Directory to store the log files.
      src_image: The path to the image to be used as the base to
        generate delta payloads.
      board: Override board to pass to the devserver for xbuddy pathing.
    """
    super(DevServerWrapper, self).__init__()
    self.devserver_bin = 'start_devserver'
    # Set port if it is given. Otherwise, devserver will start at any
    # available port.
    self.port = None if not port else port
    self.src_image = src_image
    self.board = board
    self.tempdir = None
    self.log_dir = log_dir
    if not self.log_dir:
      self.tempdir = osutils.TempDir(
          base_dir=path_util.FromChrootPath('/tmp'),
          prefix='devserver_wrapper',
          sudo_rm=True)
      self.log_dir = self.tempdir.tempdir
    self.static_dir = static_dir
    self.log_file = os.path.join(self.log_dir, 'dev_server.log')
    self.port_file = os.path.join(self.log_dir, 'dev_server.port')
    self._pid_file = self._GetPIDFilePath()
    self._pid = None

  @classmethod
  def DownloadFile(cls, url, dest):
    """Download the file from the URL to a local path."""
    if os.path.isdir(dest):
      dest = os.path.join(dest, os.path.basename(url))

    logging.info('Downloading %s to %s', url, dest)
    osutils.WriteFile(dest, DevServerWrapper.OpenURL(url), mode='wb')

  def GetURL(self, sub_dir=None):
    """Returns the URL of this devserver instance."""
    return self.GetDevServerURL(port=self.port, sub_dir=sub_dir)

  @classmethod
  def GetDevServerURL(cls, ip=None, port=None, sub_dir=None):
    """Returns the dev server url.

    Args:
      ip: IP address of the devserver. If not set, use the IP
        address of this machine.
      port: Port number of devserver.
      sub_dir: The subdirectory of the devserver url.
    """
    ip = cros_build_lib.GetIPv4Address() if not ip else ip
    # If port number is not given, assume 8080 for backward
    # compatibility.
    port = DEFAULT_PORT if not port else port
    url = 'http://%(ip)s:%(port)s' % {'ip': ip, 'port': str(port)}
    if sub_dir:
      url += '/' + sub_dir

    return url


  @classmethod
  def OpenURL(cls, url, ignore_url_error=False, timeout=60):
    """Returns the HTTP response of a URL."""
    logging.debug('Retrieving %s', url)
    try:
      res = urllib2.urlopen(url, timeout=timeout)
    except (urllib2.HTTPError, httplib.HTTPException) as e:
      logging.error('Devserver responded with HTTP error (%s)', e)
      raise DevServerResponseError(e)
    except (urllib2.URLError, socket.timeout) as e:
      if not ignore_url_error:
        logging.error('Cannot connect to devserver (%s)', e)
        raise DevServerConnectionError(e)
    else:
      return res.read()

  @classmethod
  def WipeStaticDirectory(cls, static_dir):
    """Cleans up |static_dir|.

    Args:
      static_dir: path to the static directory of the devserver instance.
    """
    # Wipe the payload cache.
    cls.WipePayloadCache(static_dir=static_dir)
    logging.info('Cleaning up directory %s', static_dir)
    osutils.RmDir(static_dir, ignore_missing=True, sudo=True)

  @classmethod
  def WipePayloadCache(cls, devserver_bin='start_devserver', static_dir=None):
    """Cleans up devserver cache of payloads.

    Args:
      devserver_bin: path to the devserver binary.
      static_dir: path to use as the static directory of the devserver instance.
    """
    logging.info('Cleaning up previously generated payloads.')
    cmd = [devserver_bin, '--clear_cache', '--exit']
    if static_dir:
      cmd.append('--static_dir=%s' % path_util.ToChrootPath(static_dir))

    cros_build_lib.SudoRunCommand(
        cmd, enter_chroot=True, print_cmd=False, combine_stdout_stderr=True,
        redirect_stdout=True, redirect_stderr=True, cwd=constants.SOURCE_ROOT)

  def _ReadPortNumber(self):
    """Read port number from file."""
    if not self.is_alive():
      raise DevServerStartupError('Devserver is dead and has no port number')

    try:
      timeout_util.WaitForReturnTrue(os.path.exists,
                                     func_args=[self.port_file],
                                     timeout=self.DEV_SERVER_TIMEOUT,
                                     period=5)
    except timeout_util.TimeoutError:
      self.terminate()
      raise DevServerStartupError('Timeout (%s) waiting for devserver '
                                  'port_file' % self.DEV_SERVER_TIMEOUT)

    self.port = int(osutils.ReadFile(self.port_file).strip())

  def IsReady(self):
    """Check if devserver is up and running."""
    if not self.is_alive():
      raise DevServerStartupError('Devserver is not ready because it died')

    url = os.path.join('http://%s:%d' % (remote_access.LOCALHOST_IP, self.port),
                       'check_health')
    if self.OpenURL(url, ignore_url_error=True, timeout=2):
      return True

    return False

  def _GetPIDFilePath(self):
    """Returns pid file path."""
    return tempfile.NamedTemporaryFile(prefix='devserver_wrapper',
                                       dir=self.log_dir,
                                       delete=False).name

  def _GetPID(self):
    """Returns the pid read from the pid file."""
    # Pid file was passed into the chroot.
    return osutils.ReadFile(self._pid_file).rstrip()

  def _WaitUntilStarted(self):
    """Wait until the devserver has started."""
    if not self.port:
      self._ReadPortNumber()

    try:
      timeout_util.WaitForReturnTrue(self.IsReady,
                                     timeout=self.DEV_SERVER_TIMEOUT,
                                     period=5)
    except timeout_util.TimeoutError:
      self.terminate()
      raise DevServerStartupError('Devserver did not start')

  def run(self):
    """Kicks off devserver in a separate process and waits for it to finish."""
    # Truncate the log file if it already exists.
    if os.path.exists(self.log_file):
      osutils.SafeUnlink(self.log_file, sudo=True)

    path_resolver = path_util.ChrootPathResolver()

    port = self.port if self.port else 0
    cmd = [self.devserver_bin,
           '--pidfile', path_resolver.ToChroot(self._pid_file),
           '--logfile', path_resolver.ToChroot(self.log_file),
           '--port=%d' % port,
           '--critical_update']

    if not self.port:
      cmd.append('--portfile=%s' % path_resolver.ToChroot(self.port_file))

    if self.static_dir:
      cmd.append(
          '--static_dir=%s' % path_resolver.ToChroot(self.static_dir))

    if self.src_image:
      cmd.append('--src_image=%s' % path_resolver.ToChroot(self.src_image))

    if self.board:
      cmd.append('--board=%s' % self.board)

    chroot_args = ['--no-ns-pid']
    result = self._RunCommand(
        cmd, enter_chroot=True, chroot_args=chroot_args,
        cwd=constants.SOURCE_ROOT, error_code_ok=True,
        redirect_stdout=True, combine_stdout_stderr=True)
    if result.returncode != 0:
      msg = ('Devserver failed to start!\n'
             '--- Start output from the devserver startup command ---\n'
             '%s'
             '--- End output from the devserver startup command ---' %
             result.output)
      logging.error(msg)

  def Start(self):
    """Starts a background devserver and waits for it to start.

    Starts a background devserver and waits for it to start. Will only return
    once devserver has started and running pid has been read.
    """
    self.start()
    self._WaitUntilStarted()
    self._pid = self._GetPID()

  def Stop(self):
    """Kills the devserver instance with SIGTERM and SIGKILL if SIGTERM fails"""
    if not self._pid:
      logging.debug('No devserver running.')
      return

    logging.debug('Stopping devserver instance with pid %s', self._pid)
    if self.is_alive():
      self._RunCommand(['kill', self._pid], error_code_ok=True)
    else:
      logging.debug('Devserver not running')
      return

    self.join(self.KILL_TIMEOUT)
    if self.is_alive():
      logging.warning('Devserver is unstoppable. Killing with SIGKILL')
      try:
        self._RunCommand(['kill', '-9', self._pid])
      except cros_build_lib.RunCommandError as e:
        raise DevServerStopError('Unable to stop devserver: %s' % e)

  def PrintLog(self):
    """Print devserver output to stdout."""
    print(self.TailLog(num_lines='+1'))

  def TailLog(self, num_lines=50):
    """Returns the most recent |num_lines| lines of the devserver log file."""
    fname = self.log_file
    # We use self._RunCommand here to check the existence of the log
    # file, so it works for RemoteDevserverWrapper as well.
    if self._RunCommand(
        ['test', '-f', fname], error_code_ok=True).returncode == 0:
      result = self._RunCommand(['tail', '-n', str(num_lines), fname],
                                capture_output=True)
      output = '--- Start output from %s ---' % fname
      output += result.output
      output += '--- End output from %s ---' % fname
      return output

  def _RunCommand(self, *args, **kwargs):
    """Runs a shell commmand."""
    kwargs.setdefault('debug_level', logging.DEBUG)
    return cros_build_lib.SudoRunCommand(*args, **kwargs)


class RemoteDevServerWrapper(DevServerWrapper):
  """A wrapper of a devserver on a remote device.

  Devserver wrapper for RemoteDevice. This wrapper kills all existing
  running devserver instances before startup, thus allowing one
  devserver running at a time.

  We assume there is no chroot on the device, thus we do not launch
  devserver inside chroot.
  """

  # Shorter timeout because the remote devserver instance does not
  # need to generate payloads.
  DEV_SERVER_TIMEOUT = 30
  KILL_TIMEOUT = 10
  PID_FILE_PATH = '/tmp/devserver_wrapper.pid'

  CHERRYPY_ERROR_MSG = """
Your device does not have cherrypy package installed; cherrypy is
necessary for launching devserver on the device. Your device may be
running an older image (<R33-4986.0.0), where cherrypy is not
installed by default.

You can fix this with one of the following three options:
  1. Update the device to a newer image with a USB stick.
  2. Run 'cros deploy device cherrypy' to install cherrpy.
  3. Run cros flash with --no-rootfs-update to update only the stateful
     parition to a newer image (with the risk that the rootfs/stateful version
    mismatch may cause some problems).
  """

  def __init__(self, remote_device, devserver_bin, host_log, **kwargs):
    """Initializes a RemoteDevserverPortal object with the remote device.

    Args:
      remote_device: A RemoteDevice object.
      devserver_bin: The path to the devserver script on the device.
      host_log: boolean whether to start the devserver with host_log enabled.
      **kwargs: See DevServerWrapper documentation.
    """
    super(RemoteDevServerWrapper, self).__init__(**kwargs)
    self.device = remote_device
    self.devserver_bin = devserver_bin
    self.hostname = remote_device.hostname
    self.host_log = host_log

  def _GetPID(self):
    """Returns the pid read from pid file."""
    result = self._RunCommand(['cat', self._pid_file])
    return result.output

  def _GetPIDFilePath(self):
    """Returns the pid filename"""
    return self.PID_FILE_PATH

  def _RunCommand(self, *args, **kwargs):
    """Runs a remote shell command.

    Args:
      *args: See RemoteAccess.RemoteDevice documentation.
      **kwargs: See RemoteAccess.RemoteDevice documentation.
    """
    kwargs.setdefault('debug_level', logging.DEBUG)
    return self.device.RunCommand(*args, **kwargs)

  def _ReadPortNumber(self):
    """Read port number from file."""
    if not self.is_alive():
      raise DevServerStartupError('Devserver not alive '
                                  'therefore no port number')

    def PortFileExists():
      result = self._RunCommand(['test', '-f', self.port_file],
                                error_code_ok=True)
      return result.returncode == 0

    try:
      timeout_util.WaitForReturnTrue(PortFileExists,
                                     timeout=self.DEV_SERVER_TIMEOUT,
                                     period=5)
    except timeout_util.TimeoutError:
      self.terminate()
      raise DevServerStartupError('Timeout (%s) waiting for remote devserver'
                                  ' port_file' % self.DEV_SERVER_TIMEOUT)

    self.port = int(self._RunCommand(
        ['cat', self.port_file], capture_output=True).output.strip())

  def IsReady(self):
    """Returns True if devserver is ready to accept requests."""
    if not self.is_alive():
      raise DevServerStartupError('Devserver not alive therefore not ready')

    url = os.path.join('http://127.0.0.1:%d' % self.port, 'check_health')
    # Running wget through ssh because the port on the device is not
    # accessible by default.
    result = self.device.RunCommand(
        ['curl', url, '-o', '/dev/null'], error_code_ok=True)
    return result.returncode == 0

  def run(self):
    """Launches a devserver process on the device."""
    self._RunCommand(['cat', '/dev/null', '>|', self.log_file])

    port = self.port if self.port else 0
    cmd = ['python2', self.devserver_bin,
           '--logfile=%s' % self.log_file,
           '--pidfile', self._pid_file,
           '--port=%d' % port,
           '--critical_update']

    if not self.port:
      cmd.append('--portfile=%s' % self.port_file)

    if self.static_dir:
      cmd.append('--static_dir=%s' % self.static_dir)

    if self.host_log:
      cmd.append('--host_log')

    logging.info('Starting devserver on %s', self.hostname)
    result = self._RunCommand(cmd, error_code_ok=True, redirect_stdout=True,
                              combine_stdout_stderr=True)
    if result.returncode != 0:
      msg = (('Remote devserver failed to start!\n'
              '--- Start output from the devserver startup command ---\n'
              '%s'
              '--- End output from the devserver startup command ---') %
             (result.output))
      logging.error(msg)
      if 'ImportError: No module named cherrypy' in result.output:
        logging.error(self.CHERRYPY_ERROR_MSG)

  def GetURL(self, sub_dir=None):
    """Returns the URL of this devserver instance."""
    return self.GetDevServerURL(ip=self.hostname, port=self.port,
                                sub_dir=sub_dir)

  @classmethod
  def WipePayloadCache(cls, devserver_bin='start_devserver', static_dir=None):
    """Cleans up devserver cache of payloads."""
    raise NotImplementedError()

  @classmethod
  def WipeStaticDirectory(cls, static_dir):
    """Cleans up |static_dir|."""
    raise NotImplementedError()

  def GetDevServerHostLogURL(self, ip=None, port=None, host=None):
    """Returns the dev server host log url.

    Args:
      ip: IP address of the devserver.
      port: Port number of devserver.
      host: The host to get the hostlog for.
    """
    if self.is_alive():
      host_log = 'api/hostlog?ip=%s' % host
      devserver_host_log = self.GetDevServerURL(ip=ip, port=port,
                                                sub_dir=host_log)
      logging.debug('Host Log URL: %s' % devserver_host_log)
      return devserver_host_log
    else:
      logging.error('Cannot get hostlog URL. Devserver not alive.')
      raise DevServerException('Cannot get hostlog URL. Devserver not alive.')
