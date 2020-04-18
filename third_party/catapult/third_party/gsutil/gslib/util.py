# -*- coding: utf-8 -*-
# Copyright 2010 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Static data and helper functions."""

from __future__ import absolute_import

import collections
from datetime import timedelta
from datetime import tzinfo
import errno
import locale
import logging
import math
import multiprocessing
import os
import pkgutil
import re
import struct
import sys
import tempfile
import textwrap
import threading
import time
import traceback
import urlparse
import xml.etree.ElementTree as ElementTree

from apitools.base.py import http_wrapper

import boto
from boto import config
import boto.auth
from boto.exception import NoAuthHandlerFound
from boto.gs.connection import GSConnection
from boto.provider import Provider
from boto.pyami.config import BotoConfigLocations

import gslib
from gslib.exception import CommandException
from gslib.storage_url import StorageUrlFromString
from gslib.third_party.storage_apitools import storage_v1_messages as apitools_messages
from gslib.thread_message import RetryableErrorMessage
from gslib.translation_helper import AclTranslation
from gslib.translation_helper import GenerationFromUrlAndString
from gslib.translation_helper import S3_ACL_MARKER_GUID
from gslib.translation_helper import S3_DELETE_MARKER_GUID
from gslib.translation_helper import S3_MARKER_GUIDS

import httplib2
from oauth2client.client import HAS_CRYPTO
from retry_decorator import retry_decorator

# Detect platform types.
PLATFORM = str(sys.platform).lower()
IS_WINDOWS = 'win32' in PLATFORM
IS_CYGWIN = 'cygwin' in PLATFORM
IS_LINUX = 'linux' in PLATFORM
IS_OSX = 'darwin' in PLATFORM

UTF8 = 'utf-8'
WINDOWS_1252 = 'cp1252'

# pylint: disable=g-import-not-at-top
if IS_WINDOWS:
  from ctypes import c_int
  from ctypes import c_uint64
  from ctypes import c_char_p
  from ctypes import c_wchar_p
  from ctypes import windll
  from ctypes import POINTER
  from ctypes import WINFUNCTYPE
  from ctypes import WinError
  IS_CP1252 = locale.getdefaultlocale()[1] == WINDOWS_1252
else:
  IS_CP1252 = False

# pylint: disable=g-import-not-at-top
try:
  # This module doesn't necessarily exist on Windows.
  import resource
  HAS_RESOURCE_MODULE = True
except ImportError, e:
  HAS_RESOURCE_MODULE = False

DEBUGLEVEL_DUMP_REQUESTS = 3
DEBUGLEVEL_DUMP_REQUESTS_AND_PAYLOADS = 4

ONE_KIB = 1024
ONE_MIB = 1024 * 1024
TWO_MIB = 2 * ONE_MIB
EIGHT_MIB = 8 * ONE_MIB
TEN_MIB = 10 * ONE_MIB
DEFAULT_FILE_BUFFER_SIZE = 8 * ONE_KIB
_DEFAULT_LINES = 25
RESUMABLE_THRESHOLD_MIB = 8
RESUMABLE_THRESHOLD_B = RESUMABLE_THRESHOLD_MIB * ONE_MIB

# By default, the timeout for SSL read errors is infinite. This could
# cause gsutil to hang on network disconnect, so pick a more reasonable
# timeout.
SSL_TIMEOUT = 60

# Start with a progress callback every 64 KiB during uploads/downloads (JSON
# API). Callback implementation should back off until it hits the maximum size
# so that callbacks do not create huge amounts of log output.
START_CALLBACK_PER_BYTES = 1024*256
MAX_CALLBACK_PER_BYTES = 1024*1024*100

# Upload/download files in 8 KiB chunks over the HTTP connection.
TRANSFER_BUFFER_SIZE = 1024*8

# Default number of progress callbacks during transfer (XML API).
XML_PROGRESS_CALLBACKS = 10

# Number of objects to request in listing calls.
NUM_OBJECTS_PER_LIST_PAGE = 1000

# For files >= this size, output a message indicating that we're running an
# operation on the file (like hashing or gzipping) so it does not appear to the
# user that the command is hanging.
MIN_SIZE_COMPUTE_LOGGING = 100*1024*1024  # 100 MiB

NO_MAX = sys.maxint

VERSION_MATCHER = re.compile(r'^(?P<maj>\d+)(\.(?P<min>\d+)(?P<suffix>.*))?')

RELEASE_NOTES_URL = 'https://pub.storage.googleapis.com/gsutil_ReleaseNotes.txt'

# Binary exponentiation strings.
_EXP_STRINGS = [
    (0, 'B', 'bit'),
    (10, 'KiB', 'Kibit', 'K'),
    (20, 'MiB', 'Mibit', 'M'),
    (30, 'GiB', 'Gibit', 'G'),
    (40, 'TiB', 'Tibit', 'T'),
    (50, 'PiB', 'Pibit', 'P'),
    (60, 'EiB', 'Eibit', 'E'),
]

_EXP_TEN_STRING = [
    (3, 'k'),
    (6, 'm'),
    (9, 'b'),
    (12, 't'),
    (15, 'q')
]
# Number of seconds to wait before printing a long retry warning message.
LONG_RETRY_WARN_SEC = 10

SECONDS_PER_DAY = 86400L

# Compressed transport encoded uploads buffer chunks of compressed data. When
# running many uploads in parallel, compression may consume more memory than
# available. This restricts the number of compressed transport encoded uploads
# running in parallel such that they don't consume more memory than set here.
MAX_UPLOAD_COMPRESSION_BUFFER_SIZE = 2*1024*1024*1024  # 2 GiB

global manager  # pylint: disable=global-at-module-level
# Single certs file for use across all processes.
configured_certs_file = None
# TODO(KMS): Remove this once we support specifying KMS key name for copy
# operations via the XML API.
is_copying_with_kms_key = False
# Temporary certs file for cleanup upon exit.
temp_certs_file = None


def _GenerateSuffixRegex():
  """Creates a suffix regex for human-readable byte counts."""
  human_bytes_re = r'(?P<num>\d*\.\d+|\d+)\s*(?P<suffix>%s)?'
  suffixes = []
  suffix_to_si = {}
  for i, si in enumerate(_EXP_STRINGS):
    si_suffixes = [s.lower() for s in list(si)[1:]]
    for suffix in si_suffixes:
      suffix_to_si[suffix] = i
    suffixes.extend(si_suffixes)
  human_bytes_re %= '|'.join(suffixes)
  matcher = re.compile(human_bytes_re)
  return suffix_to_si, matcher

SUFFIX_TO_SI, MATCH_HUMAN_BYTES = _GenerateSuffixRegex()

SECONDS_PER_DAY = 3600 * 24

# On Unix-like systems, we will set the maximum number of open files to avoid
# hitting the limit imposed by the OS. This number was obtained experimentally.
MIN_ACCEPTABLE_OPEN_FILES_LIMIT = 1000

GSUTIL_PUB_TARBALL = 'gs://pub/gsutil.tar.gz'

Retry = retry_decorator.retry  # pylint: disable=invalid-name

# Cache the values from this check such that they're available to all callers
# without needing to run all the checks again (some of these, such as calling
# multiprocessing.Manager(), are expensive operations).
cached_multiprocessing_is_available = None
cached_multiprocessing_is_available_stack_trace = None
cached_multiprocessing_is_available_message = None


# This function used to belong inside of update.py. However, it needed to be
# moved here due to compatibility issues with Travis CI, because update.py is
# not included with PyPI installations.
def DisallowUpdateIfDataInGsutilDir(directory=gslib.GSUTIL_DIR):
  """Disallows the update command if files not in the gsutil distro are found.

  This prevents users from losing data if they are in the habit of running
  gsutil from the gsutil directory and leaving data in that directory.

  This will also detect someone attempting to run gsutil update from a git
  repo, since the top-level directory will contain git files and dirs (like
  .git) that are not distributed with gsutil.

  Args:
    directory: The directory to use this functionality on.

  Raises:
    CommandException: if files other than those distributed with gsutil found.
  """
  # Manifest includes recursive-includes of gslib. Directly add
  # those to the list here so we will skip them in os.listdir() loop without
  # having to build deeper handling of the MANIFEST file here. Also include
  # 'third_party', which isn't present in manifest but gets added to the
  # gsutil distro by the gsutil submodule configuration; and the MANIFEST.in
  # and CHANGES.md files.
  manifest_lines = ['MANIFEST.in', 'third_party']

  try:
    with open(os.path.join(directory, 'MANIFEST.in'), 'r') as fp:
      for line in fp:
        if line.startswith('include '):
          manifest_lines.append(line.split()[-1])
        elif re.match(r'recursive-include \w+ \*', line):
          manifest_lines.append(line.split()[1])
  except IOError:
    logging.getLogger().warn('MANIFEST.in not found in %s.\nSkipping user data '
                             'check.\n', directory)
    return

  # Look just at top-level directory. We don't try to catch data dropped into
  # subdirs (like gslib) because that would require deeper parsing of
  # MANFFEST.in, and most users who drop data into gsutil dir do so at the top
  # level directory.
  for filename in os.listdir(directory):
    if (filename.endswith('.pyc') or filename == '__pycache__'
        or filename == '.travis.yml'):
      # Ignore compiled code and travis config.
      continue
    if filename not in manifest_lines:
      raise CommandException('\n'.join(textwrap.wrap(
          'A file (%s) that is not distributed with gsutil was found in '
          'the gsutil directory. The update command cannot run with user '
          'data in the gsutil directory.' %
          os.path.join(gslib.GSUTIL_DIR, filename))))


# This class is necessary to convert timestamps to UTC. By default Python
# datetime objects are timezone unaware. This created problems when interacting
# with cloud object timestamps which are timezone aware. This issue appeared
# when handling the timeCreated metadata attribute. The values returned by the
# service were placed in RFC 3339 format in the storage_v1_messages module. RFC
# 3339 requires a timezone in any timestamp. This caused problems as the
# datetime object elsewhere in the code was timezone unaware and was different
# by exactly one hour. The main problem is because the local system uses
# daylight savings time which consequently adjusted the timestamp ahead by one
# hour.
class UTC(tzinfo):
  """Timezone information class used to convert datetime timestamps to UTC."""

  def utcoffset(self, _):
    """An offset of the number of minutes away from UTC this tzinfo object is.

    Returns:
      A time duration of zero. UTC is zero minutes away from UTC.
    """
    return timedelta(0)

  def tzname(self, _):
    """A method to retrieve the name of this timezone object.

    Returns:
      The name of the timezone (i.e. 'UTC').
    """
    return 'UTC'

  def dst(self, _):
    """A fixed offset to handle daylight savings time (DST).

    Returns:
      A time duration of zero as UTC does not use DST.
    """
    return timedelta(0)


class LazyWrapper(object):
  """Wrapper for lazily instantiated objects."""

  def __init__(self, func):
    """The init method for LazyWrapper.

    Args:
      func: A function (lambda or otherwise) to lazily evaluate.
    """
    self._func = func

  def __int__(self):
    try:
      return int(self._value)
    except AttributeError:
      self._value = self._func()
      return int(self._value)

  def __eq__(self, other):
    try:
      return self._value == other
    except AttributeError:
      self._value = self._func()
      return self._value == other

  def __repr__(self):
    try:
      return str(self._value)
    except AttributeError:
      self._value = self._func()
      return str(self._value)

  def __str__(self):
    try:
      return str(self._value)
    except AttributeError:
      self._value = self._func()
      return str(self._value)

  def __call__(self):
    """The call method for a LazyWrapper object."""
    try:
      return self._value
    except AttributeError:
      self._value = self._func()
      return self._value

  def __len__(self):
    """The len method for a LazyWrapper object."""
    try:
      return len(self._value)
    except AttributeError:
      self.__call__()
      return len(self._value)

  def __iter__(self):
    """The iter method for a LazyWrapper object."""
    try:
      return self._value.__iter__()
    except AttributeError:
      self.__call__()
      return self._value.__iter__()


# Enum class for specifying listing style.
class ListingStyle(object):
  SHORT = 'SHORT'
  LONG = 'LONG'
  LONG_LONG = 'LONG_LONG'


def UsingCrcmodExtension(crcmod):
  return (boto.config.get('GSUtil', 'test_assume_fast_crcmod', None) or
          (getattr(crcmod, 'crcmod', None) and
           getattr(crcmod.crcmod, '_usingExtension', None)))


def ObjectIsGzipEncoded(obj_metadata):
  """Returns true if source apitools Object has gzip content-encoding."""
  return (obj_metadata.contentEncoding and
          obj_metadata.contentEncoding.lower().endswith('gzip'))


def AddAcceptEncodingGzipIfNeeded(headers_dict, compressed_encoding=False):
  if compressed_encoding:
    # If we send accept-encoding: gzip with a range request, the service
    # may respond with the whole object, which would be bad for resuming.
    # So only accept gzip encoding if the object we are downloading has
    # a gzip content encoding.
    # TODO: If we want to support compressive transcoding fully in the client,
    # condition on whether we are requesting the entire range of the object.
    # In this case, we can accept the first bytes of the object compressively
    # transcoded, but we must perform data integrity checking on bytes after
    # they are decompressed on-the-fly, and any connection break must be
    # resumed without compressive transcoding since we cannot specify an
    # offset. We would also need to ensure that hashes for downloaded data
    # from objects stored with content-encoding:gzip continue to be calculated
    # prior to our own on-the-fly decompression so they match the stored hashes.
    headers_dict['accept-encoding'] = 'gzip'


def CheckFreeSpace(path):
  """Return path/drive free space (in bytes)."""
  if IS_WINDOWS:
    try:
      # pylint: disable=invalid-name
      get_disk_free_space_ex = WINFUNCTYPE(c_int, c_wchar_p,
                                           POINTER(c_uint64),
                                           POINTER(c_uint64),
                                           POINTER(c_uint64))
      get_disk_free_space_ex = get_disk_free_space_ex(
          ('GetDiskFreeSpaceExW', windll.kernel32), (
              (1, 'lpszPathName'),
              (2, 'lpFreeUserSpace'),
              (2, 'lpTotalSpace'),
              (2, 'lpFreeSpace'),))
    except AttributeError:
      get_disk_free_space_ex = WINFUNCTYPE(c_int, c_char_p,
                                           POINTER(c_uint64),
                                           POINTER(c_uint64),
                                           POINTER(c_uint64))
      get_disk_free_space_ex = get_disk_free_space_ex(
          ('GetDiskFreeSpaceExA', windll.kernel32), (
              (1, 'lpszPathName'),
              (2, 'lpFreeUserSpace'),
              (2, 'lpTotalSpace'),
              (2, 'lpFreeSpace'),))

    def GetDiskFreeSpaceExErrCheck(result, unused_func, args):
      if not result:
        raise WinError()
      return args[1].value
    get_disk_free_space_ex.errcheck = GetDiskFreeSpaceExErrCheck

    return get_disk_free_space_ex(os.getenv('SystemDrive'))
  else:
    (_, f_frsize, _, _, f_bavail, _, _, _, _, _) = os.statvfs(path)
    return f_frsize * f_bavail


def CreateDirIfNeeded(dir_path, mode=0777):
  """Creates a directory, suppressing already-exists errors."""
  if not os.path.exists(dir_path):
    try:
      # Unfortunately, even though we catch and ignore EEXIST, this call will
      # output a (needless) error message (no way to avoid that in Python).
      os.makedirs(dir_path, mode)
    # Ignore 'already exists' in case user tried to start up several
    # resumable uploads concurrently from a machine where no tracker dir had
    # yet been created.
    except OSError as e:
      if e.errno != errno.EEXIST:
        raise


def GetDiskCounters():
  """Retrieves disk I/O statistics for all disks.

  Adapted from the psutil module's psutil._pslinux.disk_io_counters:
    http://code.google.com/p/psutil/source/browse/trunk/psutil/_pslinux.py

  Originally distributed under under a BSD license.
  Original Copyright (c) 2009, Jay Loden, Dave Daeschler, Giampaolo Rodola.

  Returns:
    A dictionary containing disk names mapped to the disk counters from
    /disk/diskstats.
  """
  # iostat documentation states that sectors are equivalent with blocks and
  # have a size of 512 bytes since 2.4 kernels. This value is needed to
  # calculate the amount of disk I/O in bytes.
  sector_size = 512

  partitions = []
  with open('/proc/partitions', 'r') as f:
    lines = f.readlines()[2:]
    for line in lines:
      _, _, _, name = line.split()
      if name[-1].isdigit():
        partitions.append(name)

  retdict = {}
  with open('/proc/diskstats', 'r') as f:
    for line in f:
      values = line.split()[:11]
      _, _, name, reads, _, rbytes, rtime, writes, _, wbytes, wtime = values
      if name in partitions:
        rbytes = int(rbytes) * sector_size
        wbytes = int(wbytes) * sector_size
        reads = int(reads)
        writes = int(writes)
        rtime = int(rtime)
        wtime = int(wtime)
        retdict[name] = (reads, writes, rbytes, wbytes, rtime, wtime)
  return retdict


def CalculateThroughput(total_bytes_transferred, total_elapsed_time):
  """Calculates throughput and checks for a small total_elapsed_time.

  Args:
    total_bytes_transferred: Total bytes transferred in a period of time.
    total_elapsed_time: The amount of time elapsed in seconds.

  Returns:
    The throughput as a float.
  """
  if total_elapsed_time < 0.01:
    total_elapsed_time = 0.01
  return float(total_bytes_transferred) / float(total_elapsed_time)


def DivideAndCeil(dividend, divisor):
  """Returns ceil(dividend / divisor).

  Takes care to avoid the pitfalls of floating point arithmetic that could
  otherwise yield the wrong result for large numbers.

  Args:
    dividend: Dividend for the operation.
    divisor: Divisor for the operation.

  Returns:
    Quotient.
  """
  quotient = dividend // divisor
  if (dividend % divisor) != 0:
    quotient += 1
  return quotient


def GetGsutilStateDir():
  """Returns the location of the directory for gsutil state files.

  Certain operations, such as cross-process credential sharing and
  resumable transfer tracking, need a known location for state files which
  are created by gsutil as-needed.

  This location should only be used for storing data that is required to be in
  a static location.

  Returns:
    Path to directory for gsutil static state files.
  """
  config_file_dir = config.get(
      'GSUtil', 'state_dir',
      os.path.expanduser(os.path.join('~', '.gsutil')))
  CreateDirIfNeeded(config_file_dir)
  return config_file_dir


def GetGsutilClientIdAndSecret():
  """Returns a tuple of the gsutil OAuth2 client ID and secret.

  Google OAuth2 clients always have a secret, even if the client is an installed
  application/utility such as gsutil.  Of course, in such cases the "secret" is
  actually publicly known; security depends entirely on the secrecy of refresh
  tokens, which effectively become bearer tokens.

  Returns:
    Tuple of strings (client ID, secret).
  """
  if (os.environ.get('CLOUDSDK_WRAPPER') == '1' and
      os.environ.get('CLOUDSDK_CORE_PASS_CREDENTIALS_TO_GSUTIL') == '1'):
    # Cloud SDK installs have a separate client ID / secret.
    return ('32555940559.apps.googleusercontent.com',  # Cloud SDK client ID
            'ZmssLNjJy2998hD4CTg2ejr2')                # Cloud SDK secret

  return ('909320924072.apps.googleusercontent.com',   # gsutil client ID
          'p3RlpR10xMFh9ZXBS/ZNLYUu')                  # gsutil secret


def GetCredentialStoreFilename():
  # As of gsutil 4.29, this changed from 'credstore' to 'credstore2' because
  # of a change to the underlying credential storage format.
  return os.path.join(GetGsutilStateDir(), 'credstore2')


def GetGceCredentialCacheFilename():
  return os.path.join(GetGsutilStateDir(), 'gcecredcache')


def GetTabCompletionLogFilename():
  return os.path.join(GetGsutilStateDir(), 'tab-completion-logs')


def GetTabCompletionCacheFilename():
  tab_completion_dir = os.path.join(GetGsutilStateDir(), 'tab-completion')
  # Limit read permissions on the directory to owner for privacy.
  CreateDirIfNeeded(tab_completion_dir, mode=0700)
  return os.path.join(tab_completion_dir, 'cache')


def GetPrintableExceptionString(exc):
  """Returns a short Unicode string describing the exception."""
  return unicode(exc).encode(UTF8) or str(exc.__class__)


def PrintableStr(input_str):
  return input_str.encode(UTF8) if input_str is not None else None


def PrintTrackerDirDeprecationWarningIfNeeded():
  # TODO: Remove this along with the tracker_dir config value 1 year after
  # 4.6 release date. Use state_dir instead.
  if config.has_option('GSUtil', 'resumable_tracker_dir'):
    sys.stderr.write('Warning: you have set resumable_tracker_dir in your '
                     '.boto configuration file. This configuration option is '
                     'deprecated; please use the state_dir configuration '
                     'option instead.\n')


# Name of file where we keep the timestamp for the last time we checked whether
# a new version of gsutil is available.
PrintTrackerDirDeprecationWarningIfNeeded()
CreateDirIfNeeded(GetGsutilStateDir())
LAST_CHECKED_FOR_GSUTIL_UPDATE_TIMESTAMP_FILE = (
    os.path.join(GetGsutilStateDir(), '.last_software_update_check'))


def HasConfiguredCredentials():
  """Determines if boto credential/config file exists."""
  has_goog_creds = (config.has_option('Credentials', 'gs_access_key_id') and
                    config.has_option('Credentials', 'gs_secret_access_key'))
  has_amzn_creds = (config.has_option('Credentials', 'aws_access_key_id') and
                    config.has_option('Credentials', 'aws_secret_access_key'))
  has_oauth_creds = (
      config.has_option('Credentials', 'gs_oauth2_refresh_token'))
  has_service_account_creds = (
      HAS_CRYPTO and
      config.has_option('Credentials', 'gs_service_client_id') and
      config.has_option('Credentials', 'gs_service_key_file'))

  if (has_goog_creds or has_amzn_creds or has_oauth_creds or
      has_service_account_creds):
    return True

  valid_auth_handler = None
  try:
    valid_auth_handler = boto.auth.get_auth_handler(
        GSConnection.DefaultHost, config, Provider('google'),
        requested_capability=['s3'])
    # Exclude the no-op auth handler as indicating credentials are configured.
    # Note we can't use isinstance() here because the no-op module may not be
    # imported so we can't get a reference to the class type.
    if getattr(getattr(valid_auth_handler, '__class__', None),
               '__name__', None) == 'NoOpAuth':
      valid_auth_handler = None
  except NoAuthHandlerFound:
    pass

  return valid_auth_handler


def ConfigureNoOpAuthIfNeeded():
  """Sets up no-op auth handler if no boto credentials are configured."""
  if not HasConfiguredCredentials():
    if (config.has_option('Credentials', 'gs_service_client_id')
        and not HAS_CRYPTO):
      if os.environ.get('CLOUDSDK_WRAPPER') == '1':
        raise CommandException('\n'.join(textwrap.wrap(
            'Your gsutil is configured with an OAuth2 service account, but '
            'you do not have PyOpenSSL or PyCrypto 2.6 or later installed. '
            'Service account authentication requires one of these libraries; '
            'please reactivate your service account via the gcloud auth '
            'command and ensure any gcloud packages necessary for '
            'service accounts are present.')))
      else:
        raise CommandException('\n'.join(textwrap.wrap(
            'Your gsutil is configured with an OAuth2 service account, but '
            'you do not have PyOpenSSL or PyCrypto 2.6 or later installed. '
            'Service account authentication requires one of these libraries; '
            'please install either of them to proceed, or configure a '
            'different type of credentials with "gsutil config".')))
    else:
      # With no boto config file the user can still access publicly readable
      # buckets and objects.
      from gslib import no_op_auth_plugin  # pylint: disable=unused-variable


def GetConfigFilePaths():
  """Returns a list of the path(s) to the boto config file(s) to be loaded."""
  config_paths = []
  # The only case in which we load multiple boto configurations is
  # when the BOTO_CONFIG environment variable is not set and the
  # BOTO_PATH environment variable is set with multiple path values.
  # Otherwise, we stop when we find the first readable config file.
  # This predicate was taken from the boto.pyami.config module.
  should_look_for_multiple_configs = (
      'BOTO_CONFIG' not in os.environ and
      'BOTO_PATH' in os.environ)
  for path in BotoConfigLocations:
    try:
      with open(path, 'r'):
        config_paths.append(path)
        if not should_look_for_multiple_configs:
          break
    except IOError:
      pass
  return config_paths


def GetBotoConfigFileList():
  """Returns list of boto config files that exist."""
  config_paths = boto.pyami.config.BotoConfigLocations
  if 'AWS_CREDENTIAL_FILE' in os.environ:
    config_paths.append(os.environ['AWS_CREDENTIAL_FILE'])
  return [cfg_path for cfg_path in config_paths if os.path.exists(cfg_path)]


def GetCertsFile():
  return configured_certs_file


def ConfigureCertsFile():
  """Configures and returns the CA Certificates file.

  If one is already configured, use it. Otherwise, use the cert roots
  distributed with gsutil.

  Returns:
    string filename of the certs file to use.
  """
  certs_file = boto.config.get('Boto', 'ca_certificates_file', None)
  # The 'system' keyword indicates to use the system installed certs. Some
  # Linux distributions patch the stack such that the Python SSL
  # infrastructure picks up the system installed certs by default, thus no
  #  action necessary on our part
  if certs_file == 'system':
    return None
  if not certs_file:
    global configured_certs_file, temp_certs_file
    if not configured_certs_file:
      configured_certs_file = os.path.abspath(
          os.path.join(gslib.GSLIB_DIR, 'data', 'cacerts.txt'))
      if not os.path.exists(configured_certs_file):
        # If the file is not present on disk, this means the gslib module
        # doesn't actually exist on disk anywhere. This can happen if it's
        # being imported from a zip file. Unfortunately, we have to copy the
        # certs file to a local temp file on disk because the underlying SSL
        # socket requires it to be a filesystem path.
        certs_data = pkgutil.get_data('gslib', 'data/cacerts.txt')
        if not certs_data:
          raise CommandException('Certificates file not found. Please '
                                 'reinstall gsutil from scratch')
        fd, fname = tempfile.mkstemp(suffix='.txt', prefix='gsutil-cacerts')
        f = os.fdopen(fd, 'w')
        f.write(certs_data)
        f.close()
        temp_certs_file = fname
        configured_certs_file = temp_certs_file
    certs_file = configured_certs_file
  return certs_file


def GetCleanupFiles():
  """Returns a list of temp files to delete (if possible) when program exits."""
  return [temp_certs_file] if temp_certs_file else []


def ProxyInfoFromEnvironmentVar(proxy_env_var):
  """Reads proxy info from the environment and converts to httplib2.ProxyInfo.

  Args:
    proxy_env_var: Environment variable string to read, such as http_proxy or
       https_proxy.

  Returns:
    httplib2.ProxyInfo constructed from the environment string.
  """
  proxy_url = os.environ.get(proxy_env_var)
  if not proxy_url or not proxy_env_var.lower().startswith('http'):
    return httplib2.ProxyInfo(httplib2.socks.PROXY_TYPE_HTTP, None, 0)
  proxy_protocol = proxy_env_var.lower().split('_')[0]
  if not proxy_url.lower().startswith('http'):
    # proxy_info_from_url requires a protocol, which is always http or https.
    proxy_url = proxy_protocol + '://' + proxy_url
  return httplib2.proxy_info_from_url(proxy_url, method=proxy_protocol)


def GetNewHttp(http_class=httplib2.Http, **kwargs):
  """Creates and returns a new httplib2.Http instance.

  Args:
    http_class: Optional custom Http class to use.
    **kwargs: Arguments to pass to http_class constructor.

  Returns:
    An initialized httplib2.Http instance.
  """
  proxy_host = boto.config.get('Boto', 'proxy', None)
  proxy_info = httplib2.ProxyInfo(
      proxy_type=3,
      proxy_host=proxy_host,
      proxy_port=boto.config.getint('Boto', 'proxy_port', 0),
      proxy_user=boto.config.get('Boto', 'proxy_user', None),
      proxy_pass=boto.config.get('Boto', 'proxy_pass', None),
      proxy_rdns=boto.config.get('Boto',
                                 'proxy_rdns',
                                 True if proxy_host else False))

  if not (proxy_info.proxy_host and proxy_info.proxy_port):
    # Fall back to using the environment variable.
    for proxy_env_var in ['http_proxy', 'https_proxy', 'HTTPS_PROXY']:
      if proxy_env_var in os.environ and os.environ[proxy_env_var]:
        proxy_info = ProxyInfoFromEnvironmentVar(proxy_env_var)
        # Assume proxy_rnds is True if a proxy environment variable exists.
        proxy_info.proxy_rdns = boto.config.get('Boto', 'proxy_rdns', True)
        break

  # Some installers don't package a certs file with httplib2, so use the
  # one included with gsutil.
  kwargs['ca_certs'] = GetCertsFile()
  # Use a non-infinite SSL timeout to avoid hangs during network flakiness.
  kwargs['timeout'] = SSL_TIMEOUT
  http = http_class(proxy_info=proxy_info, **kwargs)
  http.disable_ssl_certificate_validation = (not config.getbool(
      'Boto', 'https_validate_certificates'))
  return http


# Retry for 10 minutes with exponential backoff, which corresponds to
# the maximum Downtime Period specified in the GCS SLA
# (https://cloud.google.com/storage/sla)
def GetNumRetries():
  return config.getint('Boto', 'num_retries', 23)


def GetMaxRetryDelay():
  return config.getint('Boto', 'max_retry_delay', 32)


# Resumable downloads and uploads make one HTTP call per chunk (and must be
# in multiples of 256KiB). Overridable for testing.
def GetJsonResumableChunkSize():
  chunk_size = config.getint('GSUtil', 'json_resumable_chunk_size',
                             1024*1024*100L)
  if chunk_size == 0:
    chunk_size = 1024*256L
  elif chunk_size % 1024*256L != 0:
    chunk_size += (1024*256L - (chunk_size % (1024*256L)))
  return chunk_size


def JsonResumableChunkSizeDefined():
  chunk_size_defined = config.get('GSUtil', 'json_resumable_chunk_size',
                                  None)
  return chunk_size_defined is not None


def GetMaxUploadCompressionBufferSize():
  """Get the max amount of memory compressed transport uploads may buffer."""
  return HumanReadableToBytes(
      config.get('GSUtil', 'max_upload_compression_buffer_size', '2GiB'))


def GetMaxConcurrentCompressedUploads():
  """Gets the max concurrent transport compressed uploads allowed in parallel.

  Returns:
    The max number of concurrent transport compressed uploads allowed in
    parallel without exceeding the max_upload_compression_buffer_size.
  """
  upload_chunk_size = GetJsonResumableChunkSize()
  # From apitools compression.py.
  compression_chunk_size = 16777216  # 16MiB
  total_upload_size = (
      upload_chunk_size + compression_chunk_size + 17 +
      5 * (((compression_chunk_size - 1) / 16383) + 1))
  max_concurrent_uploads = (
      GetMaxUploadCompressionBufferSize() / total_upload_size)
  if max_concurrent_uploads <= 0:
    max_concurrent_uploads = 1
  return max_concurrent_uploads


def _RoundToNearestExponent(num):
  i = 0
  while i + 1 < len(_EXP_STRINGS) and num >= (2 ** _EXP_STRINGS[i+1][0]):
    i += 1
  return i, round(float(num) / 2 ** _EXP_STRINGS[i][0], 2)


def MakeHumanReadable(num):
  """Generates human readable string for a number of bytes.

  Args:
    num: The number, in bytes.

  Returns:
    A string form of the number using size abbreviations (KiB, MiB, etc.).
  """
  i, rounded_val = _RoundToNearestExponent(num)
  return '%g %s' % (rounded_val, _EXP_STRINGS[i][1])


def MakeBitsHumanReadable(num):
  """Generates human readable string for a number of bits.

  Args:
    num: The number, in bits.

  Returns:
    A string form of the number using bit size abbreviations (kbit, Mbit, etc.)
  """
  i, rounded_val = _RoundToNearestExponent(num)
  return '%g %s' % (rounded_val, _EXP_STRINGS[i][2])


def HumanReadableToBytes(human_string):
  """Tries to convert a human-readable string to a number of bytes.

  Args:
    human_string: A string supplied by user, e.g. '1M', '3 GiB'.
  Returns:
    An integer containing the number of bytes.
  Raises:
    ValueError: on an invalid string.
  """
  human_string = human_string.lower()
  m = MATCH_HUMAN_BYTES.match(human_string)
  if m:
    num = float(m.group('num'))
    if m.group('suffix'):
      power = _EXP_STRINGS[SUFFIX_TO_SI[m.group('suffix')]][0]
      num *= (2.0 ** power)
    num = int(round(num))
    return num
  raise ValueError('Invalid byte string specified: %s' % human_string)


def DecimalShort(num):
  """Creates a shorter string version for a given number of objects.

  Args:
    num: The number of objects to be shortened.
  Returns:
    shortened string version for this number. It takes the largest
    scale (thousand, million or billion) smaller than the number and divides it
    by that scale, indicated by a suffix with one decimal place. This will thus
    create a string of at most 6 characters, assuming num < 10^18.
    Example: 123456789 => 123.4m
  """
  for divisor_exp, suffix in reversed(_EXP_TEN_STRING):
    if num >= 10**divisor_exp:
      quotient = '%.1lf' % (float(num) / 10**divisor_exp)
      return quotient + suffix
  return str(num)


def PrettyTime(remaining_time):
  """Creates a standard version for a given remaining time in seconds.

  Created over using strftime because strftime seems to be
    more suitable for a datetime object, rather than just a number of
    seconds remaining.
  Args:
    remaining_time: The number of seconds remaining as a float, or a
      string/None value indicating time was not correctly calculated.
  Returns:
    if remaining_time is a valid float, %H:%M:%D time remaining format with
    the nearest integer from remaining_time (%H might be higher than 23).
    Else, it returns the same message it received.
  """
  remaining_time = int(round(remaining_time))
  hours = int(remaining_time / 3600)
  if hours >= 100:
    # Too large to display with precision of minutes and seconds.
    # If over 1000, saying 999+ hours should be enough.
    return '%d+ hrs' % min(hours, 999)
  remaining_time -= (3600 * hours)
  minutes = int(remaining_time / 60)
  remaining_time -= (60 * minutes)
  seconds = int(remaining_time)
  return (str('%02d' % hours) + ':' + str('%02d' % minutes)+':' +
          str('%02d' % seconds))


def HumanReadableWithDecimalPlaces(number, decimal_places=1):
  """Creates a human readable format for bytes with fixed decimal places.

  Args:
    number: The number of bytes.
    decimal_places: The number of decimal places.
  Returns:
    String representing a readable format for number with decimal_places
     decimal places.
  """
  number_format = MakeHumanReadable(number).split()
  num = str(int(round(10**decimal_places * float(number_format[0]))))
  if num == '0':
    number_format[0] = ('0' + (('.' + ('0' * decimal_places)) if decimal_places
                               else ''))
  else:
    num_length = len(num)
    if decimal_places:
      num = (num[:num_length-decimal_places] + '.' +
             num[num_length-decimal_places:])
    number_format[0] = num
  return ' '.join(number_format)


def Percentile(values, percent, key=lambda x: x):
  """Find the percentile of a list of values.

  Taken from: http://code.activestate.com/recipes/511478/

  Args:
    values: a list of numeric values. Note that the values MUST BE already
            sorted.
    percent: a float value from 0.0 to 1.0.
    key: optional key function to compute value from each element of the list
         of values.

  Returns:
    The percentile of the values.
  """
  if not values:
    return None
  k = (len(values) - 1) * percent
  f = math.floor(k)
  c = math.ceil(k)
  if f == c:
    return key(values[int(k)])
  d0 = key(values[int(f)]) * (c - k)
  d1 = key(values[int(c)]) * (k - f)
  return d0 + d1


def RemoveCRLFFromString(input_str):
  r"""Returns the input string with all \n and \r removed."""
  return re.sub(r'[\r\n]', '', input_str)


def UnaryDictToXml(message):
  """Generates XML representation of a nested dict.

  This dict contains exactly one top-level entry and an arbitrary number of
  2nd-level entries, e.g. capturing a WebsiteConfiguration message.

  Args:
    message: The dict encoding the message.

  Returns:
    XML string representation of the input dict.

  Raises:
    Exception: if dict contains more than one top-level entry.
  """
  if len(message) != 1:
    raise Exception('Expected dict of size 1, got size %d' % len(message))

  name, content = message.items()[0]
  element_type = ElementTree.Element(name)
  for element_property, value in sorted(content.items()):
    node = ElementTree.SubElement(element_type, element_property)
    node.text = value
  return ElementTree.tostring(element_type)


def LookUpGsutilVersion(gsutil_api, url_str):
  """Looks up the gsutil version of the specified gsutil tarball URL.

  Version is specified in the metadata field set on that object.

  Args:
    gsutil_api: gsutil Cloud API to use when retrieving gsutil tarball.
    url_str: tarball URL to retrieve (such as 'gs://pub/gsutil.tar.gz').

  Returns:
    Version string if URL is a cloud URL containing x-goog-meta-gsutil-version
    metadata, else None.
  """
  url = StorageUrlFromString(url_str)
  if url.IsCloudUrl():
    obj = gsutil_api.GetObjectMetadata(url.bucket_name, url.object_name,
                                       provider=url.scheme,
                                       fields=['metadata'])
    if obj.metadata and obj.metadata.additionalProperties:
      for prop in obj.metadata.additionalProperties:
        if prop.key == 'gsutil_version':
          return prop.value


class DiscardMessagesQueue(object):
  """Emulates a Cloud API status queue but drops all messages."""

  # pylint: disable=invalid-name, unused-argument
  def put(self, message=None, timeout=None):
    pass
  # pylint: enable=invalid-name, unused-argument


def GetGsutilVersionModifiedTime():
  """Returns unix timestamp of when the VERSION file was last modified."""
  if not gslib.VERSION_FILE:
    return 0
  return int(os.path.getmtime(gslib.VERSION_FILE))


def IsRunningInteractively():
  """Returns True if currently running interactively on a TTY."""
  return sys.stdout.isatty() and sys.stderr.isatty() and sys.stdin.isatty()


def _HttpsValidateCertifcatesEnabled():
  return config.get('Boto', 'https_validate_certificates', True)

CERTIFICATE_VALIDATION_ENABLED = _HttpsValidateCertifcatesEnabled()


def _BotoIsSecure():
  return config.get('Boto', 'is_secure', True)

BOTO_IS_SECURE = _BotoIsSecure()


def ResumableThreshold():
  return config.getint('GSUtil', 'resumable_threshold', EIGHT_MIB)


def CreateCustomMetadata(entries=None, custom_metadata=None):
  """Creates a custom metadata (apitools Object.MetadataValue) object.

  Inserts the key/value pairs in entries.

  Args:
    entries: The dictionary containing key/value pairs to insert into metadata.
    custom_metadata: A pre-existing custom metadata object to add to.

  Returns:
    An apitools Object.MetadataVlue.
  """
  if custom_metadata is None:
    custom_metadata = apitools_messages.Object.MetadataValue(
        additionalProperties=[])
  if entries is None:
    entries = {}
  for key, value in entries.iteritems():
    custom_metadata.additionalProperties.append(
        apitools_messages.Object.MetadataValue.AdditionalProperty(
            key=str(key), value=str(value)))
  return custom_metadata


def GetValueFromObjectCustomMetadata(obj_metadata, search_key,
                                     default_value=None):
  """Filters a specific element out of an object's custom metadata.

  Args:
    obj_metadata: The metadata for an object.
    search_key: The custom metadata key to search for.
    default_value: The default value to use for the key if it cannot be found.

  Returns:
    A tuple indicating if the value could be found in metadata and a value
    corresponding to search_key. The value at the specified key in custom
    metadata or the default value, if the specified key does not exist in the
    customer metadata.
  """
  try:
    value = next((attr.value for attr in
                  obj_metadata.metadata.additionalProperties
                  if attr.key == search_key), None)
    if value is None:
      return False, default_value
    return True, value
  except AttributeError:
    return False, default_value


def InsistAscii(string, message):
  if not all(ord(c) < 128 for c in string):
    raise CommandException(message)


def InsistAsciiHeader(header):
  InsistAscii(header, 'Invalid non-ASCII header (%s).' % header)


def InsistAsciiHeaderValue(header, value):
  InsistAscii(
      value,
      'Invalid non-ASCII value (%s) was provided for header %s.\nOnly ASCII '
      'characters are allowed in headers other than x-goog-meta- and '
      'x-amz-meta- headers' % (value, header))


def IsCustomMetadataHeader(header):
  """Returns true if header (which must be lowercase) is a custom header."""
  return header.startswith('x-goog-meta-') or header.startswith('x-amz-meta-')


# pylint: disable=too-many-statements
def PrintFullInfoAboutObject(bucket_listing_ref, incl_acl=True):
  """Print full info for given object (like what displays for gsutil ls -L).

  Args:
    bucket_listing_ref: BucketListingRef being listed.
                        Must have ref_type OBJECT and a populated root_object
                        with the desired fields.
    incl_acl: True if ACL info should be output.

  Returns:
    Tuple (number of objects, object_length)

  Raises:
    Exception: if calling bug encountered.
  """
  url_str = bucket_listing_ref.url_string
  storage_url = StorageUrlFromString(url_str)
  obj = bucket_listing_ref.root_object

  if (obj.metadata and S3_DELETE_MARKER_GUID in
      obj.metadata.additionalProperties):
    num_bytes = 0
    num_objs = 0
    url_str += '<DeleteMarker>'
  else:
    num_bytes = obj.size
    num_objs = 1

  print '%s:' % url_str.encode(UTF8)
  if obj.timeCreated:
    print MakeMetadataLine(
        'Creation time', obj.timeCreated.strftime('%a, %d %b %Y %H:%M:%S GMT'))
  if obj.updated:
    print MakeMetadataLine(
        'Update time', obj.updated.strftime('%a, %d %b %Y %H:%M:%S GMT'))
  if (obj.timeStorageClassUpdated and
      obj.timeStorageClassUpdated != obj.timeCreated):
    print MakeMetadataLine(
        'Storage class update time',
        obj.timeStorageClassUpdated.strftime('%a, %d %b %Y %H:%M:%S GMT'))
  if obj.storageClass:
    print MakeMetadataLine('Storage class', obj.storageClass)
  if obj.kmsKeyName:
    print MakeMetadataLine('KMS key', obj.kmsKeyName)
  if obj.cacheControl:
    print MakeMetadataLine('Cache-Control', obj.cacheControl)
  if obj.contentDisposition:
    print MakeMetadataLine('Content-Disposition', obj.contentDisposition)
  if obj.contentEncoding:
    print MakeMetadataLine('Content-Encoding', obj.contentEncoding)
  if obj.contentLanguage:
    print MakeMetadataLine('Content-Language', obj.contentLanguage)
  print MakeMetadataLine('Content-Length', obj.size)
  print MakeMetadataLine('Content-Type', obj.contentType)
  if obj.componentCount:
    print MakeMetadataLine('Component-Count', obj.componentCount)
  if obj.timeDeleted:
    print MakeMetadataLine(
        'Archived time',
        obj.timeDeleted.strftime('%a, %d %b %Y %H:%M:%S GMT'))
  marker_props = {}
  if obj.metadata and obj.metadata.additionalProperties:
    non_marker_props = []
    for add_prop in obj.metadata.additionalProperties:
      if add_prop.key not in S3_MARKER_GUIDS:
        non_marker_props.append(add_prop)
      else:
        marker_props[add_prop.key] = add_prop.value
    if non_marker_props:
      print MakeMetadataLine('Metadata', '')
      for ap in non_marker_props:
        print MakeMetadataLine(
            ('%s' % ap.key).encode(UTF8), ('%s' % ap.value).encode(UTF8),
            indent=2)
  if obj.customerEncryption:
    if not obj.crc32c:
      print MakeMetadataLine('Hash (crc32c)', 'encrypted')
    if not obj.md5Hash:
      print MakeMetadataLine('Hash (md5)', 'encrypted')
    print MakeMetadataLine(
        'Encryption algorithm', obj.customerEncryption.encryptionAlgorithm)
    print MakeMetadataLine(
        'Encryption key SHA256', obj.customerEncryption.keySha256)
  if obj.crc32c:
    print MakeMetadataLine('Hash (crc32c)', obj.crc32c)
  if obj.md5Hash:
    print MakeMetadataLine('Hash (md5)', obj.md5Hash)
  print MakeMetadataLine('ETag', obj.etag.strip('"\''))
  if obj.generation:
    generation_str = GenerationFromUrlAndString(storage_url, obj.generation)
    print MakeMetadataLine('Generation', generation_str)
  if obj.metageneration:
    print MakeMetadataLine('Metageneration', obj.metageneration)
  if incl_acl:
    # JSON API won't return acls as part of the response unless we have
    # full control scope
    if obj.acl:
      print MakeMetadataLine('ACL', AclTranslation.JsonFromMessage(obj.acl))
    elif S3_ACL_MARKER_GUID in marker_props:
      print MakeMetadataLine('ACL', marker_props[S3_ACL_MARKER_GUID])
    else:
      print MakeMetadataLine('ACL', 'ACCESS DENIED')
      print MakeMetadataLine(
          'Note', 'You need OWNER permission on the object to read its ACL', 2)
  return (num_objs, num_bytes)


def MakeMetadataLine(label, value, indent=1):
  """Returns a string with a vertically aligned label and value.

  Labels of the same indentation level will start at the same column. Values
  will all start at the same column (unless the combined left-indent and
  label length is excessively long). If a value spans multiple lines,
  indentation will only be applied to the first line. Example output from
  several calls:

      Label1:            Value (default indent of 1 was used)
          Sublabel1:     Value (used indent of 2 here)
      Label2:            Value

  Args:
    label: The label to print in the first column.
    value: The value to print in the second column.
    indent: (4 * indent) spaces will be placed before the label.
  Returns:
    A string with a vertically aligned label and value.
  """
  return '%s%s' % (((' ' * indent * 4) + label + ':').ljust(28), value)


def CompareVersions(first, second):
  """Compares the first and second gsutil version strings.

  For example, 3.33 > 3.7, and 4.1 is a greater major version than 3.33.
  Does not handle multiple periods (e.g. 3.3.4) or complicated suffixes
  (e.g., 3.3RC4 vs. 3.3RC5). A version string with a suffix is treated as
  less than its non-suffix counterpart (e.g. 3.32 > 3.32pre).

  Args:
    first: First gsutil version string.
    second: Second gsutil version string.

  Returns:
    (g, m):
       g is True if first known to be greater than second, else False.
       m is True if first known to be greater by at least 1 major version,
         else False.
  """
  m1 = VERSION_MATCHER.match(str(first))
  m2 = VERSION_MATCHER.match(str(second))

  # If passed strings we don't know how to handle, be conservative.
  if not m1 or not m2:
    return (False, False)

  major_ver1 = int(m1.group('maj'))
  minor_ver1 = int(m1.group('min')) if m1.group('min') else 0
  suffix_ver1 = m1.group('suffix')
  major_ver2 = int(m2.group('maj'))
  minor_ver2 = int(m2.group('min')) if m2.group('min') else 0
  suffix_ver2 = m2.group('suffix')

  if major_ver1 > major_ver2:
    return (True, True)
  elif major_ver1 == major_ver2:
    if minor_ver1 > minor_ver2:
      return (True, False)
    elif minor_ver1 == minor_ver2:
      return (bool(suffix_ver2) and not suffix_ver1, False)
  return (False, False)


def _IncreaseSoftLimitForResource(resource_name, fallback_value):
  """Sets a new soft limit for the maximum number of open files.

  The soft limit is used for this process (and its children), but the
  hard limit is set by the system and cannot be exceeded.

  We will first try to set the soft limit to the hard limit's value; if that
  fails, we will try to set the soft limit to the fallback_value iff this would
  increase the soft limit.

  Args:
    resource_name: Name of the resource to increase the soft limit for.
    fallback_value: Fallback value to be used if we couldn't set the
                    soft value to the hard value (e.g., if the hard value
                    is "unlimited").

  Returns:
    Current soft limit for the resource (after any changes we were able to
    make), or -1 if the resource doesn't exist.
  """

  # Get the value of the resource.
  try:
    (soft_limit, hard_limit) = resource.getrlimit(resource_name)
  except (resource.error, ValueError):
    # The resource wasn't present, so we can't do anything here.
    return -1

  # Try to set the value of the soft limit to the value of the hard limit.
  if hard_limit > soft_limit:  # Some OS's report 0 for "unlimited".
    try:
      resource.setrlimit(resource_name, (hard_limit, hard_limit))
      return hard_limit
    except (resource.error, ValueError):
      # We'll ignore this and try the fallback value.
      pass

  # Try to set the value of the soft limit to the fallback value.
  if soft_limit < fallback_value:
    try:
      resource.setrlimit(resource_name, (fallback_value, hard_limit))
      return fallback_value
    except (resource.error, ValueError):
      # We couldn't change the soft limit, so just report the current
      # value of the soft limit.
      return soft_limit
  else:
    return soft_limit


def GetCloudApiInstance(cls, thread_state=None):
  """Gets a gsutil Cloud API instance.

  Since Cloud API implementations are not guaranteed to be thread-safe, each
  thread needs its own instance. These instances are passed to each thread
  via the thread pool logic in command.

  Args:
    cls: Command class to be used for single-threaded case.
    thread_state: Per thread state from this thread containing a gsutil
                  Cloud API instance.

  Returns:
    gsutil Cloud API instance.
  """
  return thread_state or cls.gsutil_api


def GetFileSize(fp, position_to_eof=False):
  """Returns size of file, optionally leaving fp positioned at EOF."""
  if not position_to_eof:
    cur_pos = fp.tell()
  fp.seek(0, os.SEEK_END)
  cur_file_size = fp.tell()
  if not position_to_eof:
    fp.seek(cur_pos)
  return cur_file_size


def GetStreamFromFileUrl(storage_url, mode='rb'):
  if storage_url.IsStream():
    return sys.stdin
  else:
    return open(storage_url.object_name, mode)


def UrlsAreForSingleProvider(url_args):
  """Tests whether the URLs are all for a single provider.

  Args:
    url_args: Strings to check.

  Returns:
    True if URLs are for single provider, False otherwise.
  """
  provider = None
  url = None
  for url_str in url_args:
    url = StorageUrlFromString(url_str)
    if not provider:
      provider = url.scheme
    elif url.scheme != provider:
      return False
  return provider is not None


def HaveFileUrls(args_to_check):
  """Checks whether args_to_check contain any file URLs.

  Args:
    args_to_check: Command-line argument subset to check.

  Returns:
    True if args_to_check contains any file URLs.
  """
  for url_str in args_to_check:
    storage_url = StorageUrlFromString(url_str)
    if storage_url.IsFileUrl():
      return True
  return False


def HaveProviderUrls(args_to_check):
  """Checks whether args_to_check contains any provider URLs (like 'gs://').

  Args:
    args_to_check: Command-line argument subset to check.

  Returns:
    True if args_to_check contains any provider URLs.
  """
  for url_str in args_to_check:
    storage_url = StorageUrlFromString(url_str)
    if storage_url.IsCloudUrl() and storage_url.IsProvider():
      return True
  return False

# This must be defined at the module level for pickling across processes.
MultiprocessingIsAvailableResult = collections.namedtuple(
    'MultiprocessingIsAvailableResult', ['is_available', 'stack_trace'])


def CheckMultiprocessingAvailableAndInit(logger=None):
  """Checks if multiprocessing is available.

  There are some environments in which there is no way to use multiprocessing
  logic that's built into Python (e.g., if /dev/shm is not available, then
  we can't create semaphores). This simply tries out a few things that will be
  needed to make sure the environment can support the pieces of the
  multiprocessing module that we need.

  If multiprocessing is available, this performs necessary initialization for
  multiprocessing.  See gslib.command.InitializeMultiprocessingVariables for
  an explanation of why this is necessary.

  Args:
    logger: logging.logger to use for debug output.

  Returns:
    (multiprocessing_is_available, stack_trace):
      multiprocessing_is_available: True iff the multiprocessing module is
                                    available for use.
      stack_trace: The stack trace generated by the call we tried that failed.
  """
  # pylint: disable=global-variable-undefined
  global cached_multiprocessing_is_available
  global cached_multiprocessing_check_stack_trace
  global cached_multiprocessing_is_available_message
  if cached_multiprocessing_is_available is not None:
    if logger:
      logger.debug(cached_multiprocessing_check_stack_trace)
      logger.warn(cached_multiprocessing_is_available_message)
    return MultiprocessingIsAvailableResult(
        is_available=cached_multiprocessing_is_available,
        stack_trace=cached_multiprocessing_check_stack_trace)

  if IS_WINDOWS:
    message = """
Multiple processes are not supported on Windows. Operations requesting
parallelism will be executed with multiple threads in a single process only.
"""
    if logger:
      logger.warn(message)
    return MultiprocessingIsAvailableResult(is_available=False,
                                            stack_trace=None)

  stack_trace = None
  multiprocessing_is_available = True
  message = """
You have requested multiple processes for an operation, but the
required functionality of Python\'s multiprocessing module is not available.
Operations requesting parallelism will be executed with multiple threads in a
single process only.
"""
  try:
    # Fails if /dev/shm (or some equivalent thereof) is not available for use
    # (e.g., there's no implementation, or we can't write to it, etc.).
    try:
      multiprocessing.Value('i', 0)
    except:
      message += """
Please ensure that you have write access to both /dev/shm and /run/shm.
"""
      raise  # We'll handle this in one place below.

    global manager  # pylint: disable=global-variable-undefined
    manager = multiprocessing.Manager()

    # Check that the max number of open files is reasonable. Always check this
    # after we're sure that the basic multiprocessing functionality is
    # available, since this won't matter unless that's true.
    limit = -1
    if HAS_RESOURCE_MODULE:
      # Try to set this with both resource names - RLIMIT_NOFILE for most Unix
      # platforms, and RLIMIT_OFILE for BSD. Ignore AttributeError because the
      # "resource" module is not guaranteed to know about these names.
      try:
        limit = max(limit,
                    _IncreaseSoftLimitForResource(
                        resource.RLIMIT_NOFILE,
                        MIN_ACCEPTABLE_OPEN_FILES_LIMIT))
      except AttributeError:
        pass
      try:
        limit = max(limit,
                    _IncreaseSoftLimitForResource(
                        resource.RLIMIT_OFILE, MIN_ACCEPTABLE_OPEN_FILES_LIMIT))
      except AttributeError:
        pass

    if limit < MIN_ACCEPTABLE_OPEN_FILES_LIMIT:
      message += ("""
Your max number of open files, %s, is too low to allow safe multiprocessing.
On Linux you can fix this by adding something like "ulimit -n 10000" to your
~/.bashrc or equivalent file and opening a new terminal.

On MacOS, you may also need to run a command like this once (in addition to the
above instructions), which might require a restart of your system to take
effect:
  launchctl limit maxfiles 10000

Alternatively, edit /etc/launchd.conf with something like:
  limit maxfiles 10000 10000

""" % limit)
      raise Exception('Max number of open files, %s, is too low.' % limit)
  except:  # pylint: disable=bare-except
    stack_trace = traceback.format_exc()
    multiprocessing_is_available = False
    if logger is not None:
      logger.debug(stack_trace)
      logger.warn(message)

  # Set the cached values so that we never need to do this check again.
  cached_multiprocessing_is_available = multiprocessing_is_available
  cached_multiprocessing_check_stack_trace = stack_trace
  cached_multiprocessing_is_available_message = message
  return MultiprocessingIsAvailableResult(
      is_available=cached_multiprocessing_is_available,
      stack_trace=cached_multiprocessing_check_stack_trace)


def CreateLock():
  """Returns either a multiprocessing lock or a threading lock.

  Use Multiprocessing lock iff we have access to the parts of the
  multiprocessing module that are necessary to enable parallelism in operations.

  Returns:
    Multiprocessing or threading lock.
  """
  if CheckMultiprocessingAvailableAndInit().is_available:
    return manager.Lock()
  else:
    return threading.Lock()


def IsCloudSubdirPlaceholder(url, blr=None):
  """Determines if URL is a cloud subdir placeholder.

  This function is needed because GUI tools (like the GCS cloud console) allow
  users to create empty "folders" by creating a placeholder object; and parts
  of gsutil need to treat those placeholder objects specially. For example,
  gsutil rsync needs to avoid downloading those objects because they can cause
  conflicts (see comments in rsync command for details).

  We currently detect two cases:
    - Cloud objects whose name ends with '_$folder$'
    - Cloud objects whose name ends with '/'

  Args:
    url: The URL to be checked.
    blr: BucketListingRef to check, or None if not available.
         If None, size won't be checked.

  Returns:
    True/False.
  """
  if not url.IsCloudUrl():
    return False
  url_str = url.url_string
  if url_str.endswith('_$folder$'):
    return True
  if blr and blr.IsObject():
    size = blr.root_object.size
  else:
    size = 0
  return size == 0 and url_str.endswith('/')


def GetTermLines():
  """Returns number of terminal lines."""
  # fcntl isn't supported in Windows.
  try:
    import fcntl    # pylint: disable=g-import-not-at-top
    import termios  # pylint: disable=g-import-not-at-top
  except ImportError:
    return _DEFAULT_LINES
  def ioctl_GWINSZ(fd):  # pylint: disable=invalid-name
    try:
      return struct.unpack(
          'hh', fcntl.ioctl(fd, termios.TIOCGWINSZ, '1234'))[0]
    except:  # pylint: disable=bare-except
      return 0  # Failure (so will retry on different file descriptor below).
  # Try to find a valid number of lines from termio for stdin, stdout,
  # or stderr, in that order.
  ioc = ioctl_GWINSZ(0) or ioctl_GWINSZ(1) or ioctl_GWINSZ(2)
  if not ioc:
    try:
      fd = os.open(os.ctermid(), os.O_RDONLY)
      ioc = ioctl_GWINSZ(fd)
      os.close(fd)
    except:  # pylint: disable=bare-except
      pass
  if not ioc:
    ioc = os.environ.get('LINES', _DEFAULT_LINES)
  return int(ioc)


def FixWindowsEncodingIfNeeded(input_str):
  """Attempts to detect Windows CP1252 encoding and convert to UTF8.

  Windows doesn't provide a way to set UTF-8 for string encodings; you can set
  the system locale (see
  http://windows.microsoft.com/en-us/windows/change-system-locale#1TC=windows-7)
  but that takes you to a "Change system locale" dropdown that just lists
  languages (e.g., "English (United States)". Instead, we're forced to check if
  a encoding as UTF8 raises an exception and if so, try converting from CP1252
  to Unicode.

  Args:
    input_str: The input string.
  Returns:
    The converted string (or the original, if conversion wasn't needed).
  """
  if IS_CP1252:
    return input_str.decode(WINDOWS_1252).encode(UTF8)
  else:
    return input_str


def LogAndHandleRetries(is_data_transfer=False, status_queue=None):
  """Higher-order function allowing retry handler to access global status queue.

  Args:
    is_data_transfer: If True, disable retries in apitools.
    status_queue: The global status queue.

  Returns:
    A retry function for retryable errors in apitools.
  """
  def WarnAfterManyRetriesHandler(retry_args):
    """Exception handler for http failures in apitools.

    If the user has had to wait several seconds since their first request, print
    a progress message to the terminal to let them know we're still retrying,
    then perform the default retry logic and post a RetryableErrorMessage to the
    global status queue.

    Args:
      retry_args: An apitools ExceptionRetryArgs tuple.
    """
    if retry_args.total_wait_sec >= LONG_RETRY_WARN_SEC:
      logging.info('Retrying request, attempt #%d...', retry_args.num_retries)
    if status_queue:
      status_queue.put(RetryableErrorMessage(
          retry_args.exc, time.time(), num_retries=retry_args.num_retries,
          total_wait_sec=retry_args.total_wait_sec))
    http_wrapper.HandleExceptionsAndRebuildHttpConnections(retry_args)

  def RetriesInDataTransferHandler(retry_args):
    """Exception handler that disables retries in apitools data transfers.

    Post a RetryableErrorMessage to the global status queue. We handle the
    actual retries within the download and upload functions.

    Args:
      retry_args: An apitools ExceptionRetryArgs tuple.
    """
    if status_queue:
      status_queue.put(RetryableErrorMessage(
          retry_args.exc, time.time(), num_retries=retry_args.num_retries,
          total_wait_sec=retry_args.total_wait_sec))
    http_wrapper.RethrowExceptionHandler(retry_args)

  if is_data_transfer:
    return RetriesInDataTransferHandler
  return WarnAfterManyRetriesHandler


def StdinIterator():
  """A generator function that returns lines from stdin."""
  for line in sys.stdin:
    # Strip CRLF.
    yield line.rstrip()


def ConvertRecursiveToFlatWildcard(url_strs):
  """A generator that adds '**' to each url string in url_strs."""
  for url_str in url_strs:
    yield '%s**' % url_str


def NormalizeStorageClass(sc):
  """Returns a normalized form of the given storage class name.

  Converts the given string to uppercase and expands valid abbreviations to
  full storage class names (e.g 'std' would return 'STANDARD'). Note that this
  method does not check if the given storage class is valid.

  Args:
    sc: String representing the storage class's full name or abbreviation.

  Returns:
    A string representing the full name of the given storage class.
  """
  shorthand_to_full_name = {
      'CL': 'COLDLINE',
      'DRA': 'DURABLE_REDUCED_AVAILABILITY',
      'NL': 'NEARLINE',
      'S': 'STANDARD',
      'STD': 'STANDARD'}
  # Use uppercase; storage class argument for the S3 API must be uppercase,
  # and it's case-insensitive for GS APIs.
  sc = sc.upper()
  if sc in shorthand_to_full_name:
    sc = shorthand_to_full_name[sc]
  return sc


def AddQueryParamToUrl(url_str, param_name, param_value):
  """Adds a query parameter to a URL string.

  Appends a query parameter to the query string portion of a url. If a parameter
  with the given name was already present, it is not removed; the new name/value
  pair will be appended to the end of the query string. It is assumed that all
  arguments will be of type `str` (either ASCII or UTF-8 encoded) or `unicode`.

  Note that this method performs no URL-encoding. It is the caller's
  responsibility to ensure proper URL encoding of the entire URL; i.e. if the
  URL is already URL-encoded, you should pass in URL-encoded values for
  param_name and param_value. If the URL is not URL-encoded, you should not pass
  in URL-encoded parameters; instead, you could perform URL-encoding using the
  URL string returned from this function.

  Args:
    url_str: String representing the URL.
    param_name: String key of the query parameter.
    param_value: String value of the query parameter.

  Returns:
    A string representing the modified url, of type `unicode` if the url_str
    argument was a `unicode`, otherwise a `str` encoded in UTF-8.
  """
  url_was_unicode = isinstance(url_str, unicode)
  if isinstance(url_str, unicode):
    url_str = url_str.encode('utf-8')
  if isinstance(param_name, unicode):
    param_name = param_name.encode('utf-8')
  if isinstance(param_value, unicode):
    param_value = param_value.encode('utf-8')
  scheme, netloc, path, query_str, fragment = urlparse.urlsplit(url_str)

  query_params = urlparse.parse_qsl(query_str, keep_blank_values=True)
  query_params.append((param_name, param_value))
  new_query_str = '&'.join(['%s=%s' % (k, v) for (k, v) in query_params])

  new_url = urlparse.urlunsplit((scheme, netloc, path, new_query_str, fragment))
  if url_was_unicode:
    new_url = new_url.decode('utf-8')
  return new_url


class RsyncDiffToApply(object):
  """Class that encapsulates info needed to apply diff for one object."""

  def __init__(self, src_url_str, dst_url_str, src_posix_attrs, diff_action,
               copy_size):
    """Constructor.

    Args:
      src_url_str: The source URL string, or None if diff_action is REMOVE.
      dst_url_str: The destination URL string.
      src_posix_attrs: The source posix_attributes.
      diff_action: _DiffAction to be applied.
      copy_size: The amount of bytes to copy, or None if diff_action is REMOVE.
    """
    self.src_url_str = src_url_str
    self.dst_url_str = dst_url_str
    self.src_posix_attrs = src_posix_attrs
    self.diff_action = diff_action
    self.copy_size = copy_size
