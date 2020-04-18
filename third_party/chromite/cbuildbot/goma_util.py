# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module to use goma from buildbot."""

from __future__ import print_function

import collections
import datetime
import getpass
import glob
import json
import os
import shlex
import tempfile

from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import path_util


_GOMA_COMPILER_PROXY_LOG_URL_TEMPLATE = (
    'http://chromium-build-stats.appspot.com/compiler_proxy_log/%s/%s')
_GOMA_NINJA_LOG_URL_TEMPLATE = (
    'http://chromium-build-stats.appspot.com/ninja_log/%s/%s')


class Goma(object):
  """Interface to use goma on bots."""

  # Default environment variables to use goma.
  _DEFAULT_ENV_VARS = {
      # Set MAX_COMPILER_DISABLED_TASKS to let goma enter Burst mode, if
      # there are too many local fallback failures. In the Burst mode, goma
      # tries to use CPU cores as many as possible. Note that, by default,
      # goma runs only a few local fallback tasks in parallel at once.
      # The value is the threashold of the number of local fallback failures
      # to enter the mode.
      # Note that 30 is just heuristically chosen by discussion with goma team.
      #
      # Specifically, this is short-term workaround of the case that all
      # compile processes get local fallback. Otherwise, because goma uses only
      # several processes for local fallback by default, it causes significant
      # slow down of the build.
      # Practically, this happens when toolchain is updated in repository,
      # but prebuilt package is not yet ready. (cf. crbug.com/728971)
      'GOMA_MAX_COMPILER_DISABLED_TASKS': '30',

      # Disable goma soft stickiness.
      # Goma was historically using `soft stickiness cookie` so that uploaded
      # file cache is available as much as possible. However, such sticky
      # requests are cuasing unbalanced server load, and the disadvantage of the
      # unbalanceness cannot be negligible now. According to chrome's
      # experiment, the disadvantage of disabling soft stickiness is negligible,
      # and achieving balanced server load will have more advantage for entire
      # build. (cf. crbug.com/730962)
      # TODO(shinyak): This will be removed after crbug.com/730962 is resolved.
      'GOMA_BACKEND_SOFT_STICKINESS': 'false',

      # Enable DepsCache. DepsCache is a cache that holds a file list that
      # compiler_proxy sends to goma server for each compile. This can
      # reduces a lot of I/O and calculation.
      # This is the base file name under GOMA_CACHE_DIR.
      'GOMA_DEPS_CACHE_FILE': 'goma.deps',
  }

  def __init__(self, goma_dir, goma_client_json, goma_tmp_dir=None,
               stage_name=None):
    """Initializes Goma instance.

    This ensures that |self.goma_log_dir| directory exists (if missing,
    creates it).

    Args:
      goma_dir: Path to the goma directory (outside of chroot).
      goma_client_json: Path to the service account json file to use goma.
        On bots, this must be specified, otherwise raise a ValueError.
        On local, this is optional, and can be set to None.
      goma_tmp_dir: Path to the GOMA_TMP_DIR to be passed to goma programs.
        If given, it is used. If not given, creates a directory under
        /tmp in the chroot, expecting that the directory is removed in the
        next run's clean up phase on bots.
      stage_name: optional name of the currently running stage. E.g.
        "build_packages" or "test_simple_chrome_workflow". If this is set
        deps cache is enabled.

    Raises:
       ValueError if 1) |goma_dir| does not point to a directory, 2)
       on bots, but |goma_client_json| is not given, 3) |goma_client_json|
       is given, but it does not point to a file, or 4) if |goma_tmp_dir| is
       given but it does not point to a directory.
    """
    # Sanity checks of given paths.
    if not os.path.isdir(goma_dir):
      raise ValueError('goma_dir does not point a directory: %s' % (goma_dir,))

    # If this script runs on bot, service account json file needs to be
    # provided, otherwise it cannot access to goma service.
    if cros_build_lib.HostIsCIBuilder() and goma_client_json is None:
      raise ValueError(
          'goma is enabled on bot, but goma_client_json is not provided')

    # If goma_client_json file is provided, it must be an existing file.
    if goma_client_json and not os.path.isfile(goma_client_json):
      raise ValueError(
          'Goma client json file is missing: %s' % (goma_client_json,))

    # If goma_tmp_dir is provided, it must be an existing directory.
    if goma_tmp_dir and not os.path.isdir(goma_tmp_dir):
      raise ValueError(
          'GOMA_TMP_DIR does not point a directory: %s' % (goma_tmp_dir,))

    self.goma_dir = goma_dir
    self.goma_client_json = goma_client_json
    if stage_name:
      self.goma_cache = os.path.join(goma_dir, 'goma_cache', stage_name)
      osutils.SafeMakedirs(self.goma_cache)
    else:
      self.goma_cache = None

    if goma_tmp_dir is None:
      # If |goma_tmp_dir| is not given, create GOMA_TMP_DIR (goma
      # compiler_proxy's working directory), and its log directory.
      # Create unique directory by mkdtemp under chroot's /tmp.
      # Expect this directory is removed in next run's clean up phase.
      goma_tmp_dir = tempfile.mkdtemp(
          prefix='goma_tmp_dir.', dir=path_util.FromChrootPath('/tmp'))
    self.goma_tmp_dir = goma_tmp_dir

    # Create log directory if not exist.
    if not os.path.isdir(self.goma_log_dir):
      os.mkdir(self.goma_log_dir)

  @property
  def goma_log_dir(self):
    """Path to goma's log directory."""
    return os.path.join(self.goma_tmp_dir, 'log_dir')

  def GetExtraEnv(self):
    """Extra env vars set to use goma."""
    result = dict(
        Goma._DEFAULT_ENV_VARS,
        GOMA_DIR=self.goma_dir,
        GOMA_TMP_DIR=self.goma_tmp_dir,
        GLOG_log_dir=self.goma_log_dir)
    if self.goma_client_json:
      result['GOMA_SERVICE_ACCOUNT_JSON_FILE'] = self.goma_client_json
    if self.goma_cache:
      result['GOMA_CACHE_DIR'] = self.goma_cache
    return result

  def GetChrootExtraEnv(self):
    """Extra env vars set to use goma inside chroot."""
    # Note: GOMA_DIR and GOMA_SERVICE_ACCOUNT_JSON_FILE in chroot is hardcoded.
    # Please see also enter_chroot.sh.
    goma_dir = os.path.join('/home', os.environ.get('USER'), 'goma')
    result = dict(
        Goma._DEFAULT_ENV_VARS,
        GOMA_DIR=goma_dir,
        GOMA_TMP_DIR=path_util.ToChrootPath(self.goma_tmp_dir),
        GLOG_log_dir=path_util.ToChrootPath(self.goma_log_dir))
    if self.goma_client_json:
      result['GOMA_SERVICE_ACCOUNT_JSON_FILE'] = (
          '/creds/service_accounts/service-account-goma-client.json')

    if self.goma_cache:
      result['GOMA_CACHE_DIR'] = os.path.join(
          goma_dir, os.path.relpath(self.goma_cache, self.goma_dir))
    return result

  def _RunGomaCtl(self, command):
    goma_ctl = os.path.join(self.goma_dir, 'goma_ctl.py')
    cros_build_lib.RunCommand(
        ['python', goma_ctl, command], extra_env=self.GetExtraEnv())

  def Start(self):
    """Starts goma compiler proxy."""
    self._RunGomaCtl('start')

  def Stop(self):
    """Stops goma compiler proxy."""
    self._RunGomaCtl('stop')

  def UploadLogs(self):
    """Uploads INFO files related to goma.

    Returns:
      URL to the compiler_proxy log visualizer. None if unavailable.
    """
    uploader = GomaLogUploader(self.goma_log_dir)
    return uploader.Upload()


# Note: Public for testing purpose. In real use, please think about using
# Goma.UploadLogs() instead.
class GomaLogUploader(object):
  """Manages to upload goma log files."""

  # The Google Cloud Storage bucket to store logs related to goma.
  _BUCKET = 'chrome-goma-log'

  def __init__(self, goma_log_dir, today=None, dry_run=False):
    """Initializes the uploader.

    Args:
      goma_log_dir: path to the directory containing goma's INFO log files.
      today: datetime.date instance representing today. This is for testing
        purpose, because datetime.date is unpatchable. In real use case,
        this must be None.
      dry_run: If True, no actual upload. This is for testing purpose.
    """
    self._goma_log_dir = goma_log_dir
    logging.info('Goma log directory is: %s', self._goma_log_dir)

    # Set log upload destination.
    if today is None:
      today = datetime.date.today()
    self.dest_path = '%s/%s' % (
        today.strftime('%Y/%m/%d'), cros_build_lib.GetHostName())
    self._remote_dir = 'gs://%s/%s' % (GomaLogUploader._BUCKET, self.dest_path)
    logging.info('Goma log upload destination: %s', self._remote_dir)

    # Build metadata to be annotated to log files.
    # Use OrderedDict for json output stabilization.
    builder_info = json.dumps(collections.OrderedDict([
        ('builder', os.environ.get('BUILDBOT_BUILDERNAME', '')),
        ('master', os.environ.get('BUILDBOT_MASTERNAME', '')),
        ('slave', os.environ.get('BUILDBOT_SLAVENAME', '')),
        ('clobber', bool(os.environ.get('BUILDBOT_CLOBBER'))),
        ('os', 'chromeos'),
    ]))
    logging.info('BuilderInfo: %s', builder_info)
    self._headers = ['x-goog-meta-builderinfo:' + builder_info]

    self._gs_context = gs.GSContext(dry_run=dry_run)

  def Upload(self):
    """Uploads all necessary log files to Google Storage.

    Returns:
      A list of pairs of label and URL of goma log visualizers to be linked
      from the build status page.
    """
    compiler_proxy_subproc_paths = self._UploadInfoFiles(
        'compiler_proxy-subproc')
    # compiler_proxy-subproc.INFO file should be exact one.
    if len(compiler_proxy_subproc_paths) != 1:
      logging.warning('Unexpected compiler_proxy-subproc INFO files: %r',
                      compiler_proxy_subproc_paths)

    compiler_proxy_paths = self._UploadInfoFiles('compiler_proxy')
    # compiler_proxy.INFO file should be exact one.
    if len(compiler_proxy_paths) != 1:
      logging.warning('Unexpected compiler_proxy INFO files: %r',
                      compiler_proxy_paths)
    compiler_proxy_path, uploaded_compiler_proxy_filename = (
        compiler_proxy_paths[0] if compiler_proxy_paths else (None, None))

    self._UploadGomaccInfoFiles()

    uploaded_ninja_log_filename = self._UploadNinjaLog(compiler_proxy_path)

    # Build URL to be linked.
    result = []
    if uploaded_compiler_proxy_filename:
      result.append((
          'Goma compiler_proxy log',
          _GOMA_COMPILER_PROXY_LOG_URL_TEMPLATE % (
              self.dest_path, uploaded_compiler_proxy_filename)))
    if uploaded_ninja_log_filename:
      result.append((
          'Goma ninja_log',
          _GOMA_NINJA_LOG_URL_TEMPLATE % (
              self.dest_path, uploaded_ninja_log_filename)))
    return result

  def _UploadInfoFiles(self, pattern):
    """Uploads INFO files matched with pattern, with gzip'ing.

    Args:
      pattern: matching path pattern.

    Returns:
      A list of uploaded file paths.
    """
    # Find files matched with the pattern in |goma_log_dir|. Sort for
    # stabilization.
    paths = sorted(glob.glob(
        os.path.join(self._goma_log_dir, '%s.*.INFO.*' % pattern)))
    if not paths:
      logging.warning('No glog files matched with: %s', pattern)

    result = []
    for path in paths:
      logging.info('Uploading %s', path)
      uploaded_filename = os.path.basename(path) + '.gz'
      self._gs_context.CopyInto(
          path, self._remote_dir, filename=uploaded_filename,
          auto_compress=True, headers=self._headers)
      result.append((path, uploaded_filename))
    return result

  def _UploadGomaccInfoFiles(self):
    """Uploads gomacc INFO files, with gzip'ing.

    Returns:
      Uploaded file path. If failed, None.
    """

    # Since the number of gomacc logs can be large, we'd like to compress them.
    # Otherwise, upload will take long (> 10 mins).
    # Each gomacc logs file size must be small (around 4KB).

    # Find files matched with the pattern in |goma_log_dir|. Sort for
    # stabilization.
    gomacc_paths = sorted(glob.glob(
        os.path.join(self._goma_log_dir, 'gomacc.*.INFO.*')))
    if not gomacc_paths:
      # gomacc logs won't be made every time.
      # Only when goma compiler_proxy has
      # crashed. So it's usual gomacc logs are not found.
      logging.info('No gomacc logs found')
      return None

    # Taking the first name as uploaded_filename.
    tgz_name = os.path.basename(gomacc_paths[0]) + '.tar.gz'
    tgz_path = os.path.join(self._goma_log_dir, tgz_name)
    cros_build_lib.CreateTarball(target=tgz_path,
                                 cwd=self._goma_log_dir,
                                 compression=cros_build_lib.COMP_GZIP,
                                 inputs=gomacc_paths)
    self._gs_context.CopyInto(tgz_path, self._remote_dir,
                              filename=tgz_name,
                              headers=self._headers)
    return tgz_name

  def _UploadNinjaLog(self, compiler_proxy_path):
    """Uploads .ninja_log file and its related metadata.

    This uploads the .ninja_log file generated by ninja to build Chrome.
    Also, it appends some related metadata at the end of the file following
    '# end of ninja log' marker.

    Args:
      compiler_proxy_path: Path to the compiler proxy, which will be contained
        in the metadata.

    Returns:
      The name of the uploaded file.
    """
    ninja_log_path = os.path.join(self._goma_log_dir, 'ninja_log')
    if not os.path.exists(ninja_log_path):
      logging.warning('ninja_log is not found: %s', ninja_log_path)
      return None
    ninja_log_content = osutils.ReadFile(ninja_log_path)

    try:
      st = os.stat(ninja_log_path)
      ninja_log_mtime = datetime.datetime.fromtimestamp(st.st_mtime)
    except OSError:
      logging.exception('Failed to get timestamp: %s', ninja_log_path)
      return None

    ninja_log_info = self._BuildNinjaInfo(compiler_proxy_path)

    # Append metadata at the end of the log content.
    ninja_log_content += '# end of ninja log\n' + json.dumps(ninja_log_info)

    # Aligned with goma_utils in chromium bot.
    pid = os.getpid()

    upload_ninja_log_path = os.path.join(
        self._goma_log_dir,
        'ninja_log.%s.%s.%s.%d' % (
            getpass.getuser(), cros_build_lib.GetHostName(),
            ninja_log_mtime.strftime('%Y%m%d-%H%M%S'), pid))
    osutils.WriteFile(upload_ninja_log_path, ninja_log_content)
    uploaded_filename = os.path.basename(upload_ninja_log_path) + '.gz'
    self._gs_context.CopyInto(
        upload_ninja_log_path, self._remote_dir, filename=uploaded_filename,
        auto_compress=True, headers=self._headers)
    return uploaded_filename

  def _BuildNinjaInfo(self, compiler_proxy_path):
    """Reads metadata for the ninja run.

    Each metadata should be written into a dedicated file in the log directory.
    Read the info, and build the dict containing metadata.

    Args:
      compiler_proxy_path: Path to the compiler_proxy log file.

    Returns:
      A dict of the metadata.
    """

    info = {'platform': 'chromeos'}

    command_path = os.path.join(self._goma_log_dir, 'ninja_command')
    if os.path.exists(command_path):
      info['cmdline'] = shlex.split(
          osutils.ReadFile(command_path).strip())

    cwd_path = os.path.join(self._goma_log_dir, 'ninja_cwd')
    if os.path.exists(cwd_path):
      info['cwd'] = osutils.ReadFile(cwd_path).strip()

    exit_path = os.path.join(self._goma_log_dir, 'ninja_exit')
    if os.path.exists(exit_path):
      info['exit'] = int(osutils.ReadFile(exit_path).strip())

    env_path = os.path.join(self._goma_log_dir, 'ninja_env')
    if os.path.exists(env_path):
      # env is null-byte separated, and has a trailing null byte.
      content = osutils.ReadFile(env_path).rstrip('\0')
      info['env'] = dict(line.split('=', 1) for line in content.split('\0'))

    if compiler_proxy_path:
      info['compiler_proxy_info'] = os.path.basename(compiler_proxy_path)

    return info
