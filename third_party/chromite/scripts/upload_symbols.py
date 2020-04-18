# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Upload all debug symbols required for crash reporting purposes.

This script need only be used to upload release builds symbols or to debug
crashes on non-release builds (in which case try to only upload the symbols
for those executables involved).
"""

from __future__ import print_function

import hashlib
import httplib
import itertools
import os
import socket
import sys
import textwrap
import tempfile
import time
import urllib2
import urlparse

from chromite.lib import constants

# The isolateserver includes a bunch of third_party python packages that clash
# with chromite's bundled third_party python packages (like oauth2client).
# Since upload_symbols is not imported in to other parts of chromite, and there
# are no deps in third_party we care about, purge the chromite copy.  This way
# we can use isolateserver for deduping.
# TODO: If we ever sort out third_party/ handling and make it per-script opt-in,
# we can purge this logic.
third_party = os.path.join(constants.CHROMITE_DIR, 'third_party')
while True:
  try:
    sys.path.remove(third_party)
  except ValueError:
    break
sys.path.insert(0, os.path.join(third_party, 'swarming.client'))
sys.path.insert(0, os.path.join(third_party, 'upload_symbols'))
del third_party

# Has to be after sys.path manipulation above.
# And our sys.path muckery confuses pylint.
import poster  # pylint: disable=import-error

from chromite.lib import cache
from chromite.lib import commandline
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import retry_stats
from chromite.lib import timeout_util
from chromite.scripts import cros_generate_breakpad_symbols

# Needs to be after chromite imports.
# We don't want to import the general keyring module as that will implicitly
# try to import & connect to a dbus server.  That's a waste of time.
sys.modules['keyring'] = None
# And our sys.path muckery confuses pylint.
import isolateserver  # pylint: disable=import-error


# We need this to run once per process. Do it at module import time as that
# will let us avoid doing it inline at function call time (see UploadSymbolFile)
# as that func might be called by the multiprocessing module which means we'll
# do the opener logic multiple times overall. Plus, if you're importing this
# module, it's a pretty good chance that you're going to need this.
poster.streaminghttp.register_openers()


# URLs used for uploading symbols.
OFFICIAL_UPLOAD_URL = 'http://clients2.google.com/cr/symbol'
STAGING_UPLOAD_URL = 'http://clients2.google.com/cr/staging_symbol'


# The crash server rejects files that are this big.
CRASH_SERVER_FILE_LIMIT = 700 * 1024 * 1024
# Give ourselves a little breathing room from what the server expects.
DEFAULT_FILE_LIMIT = CRASH_SERVER_FILE_LIMIT - (10 * 1024 * 1024)


# The batch limit when talking to the dedup server.  We avoid sending one at a
# time as the round trip overhead will dominate.  Conversely, we avoid sending
# all at once so we can start uploading symbols asap -- the symbol server is a
# bit slow and will take longer than anything else.
DEDUPE_LIMIT = 100

# How long to wait for the server to respond with the results.  Note that the
# larger the limit above, the larger this will need to be.  So we give it ~1
# second per item max.
DEDUPE_TIMEOUT = DEDUPE_LIMIT

# How long to wait for the notification to finish (in seconds).
DEDUPE_NOTIFY_TIMEOUT = 240

# The unique namespace in the dedupe server that only we use.  Helps avoid
# collisions with all the hashed values and unrelated content.
OFFICIAL_DEDUPE_NAMESPACE_TMPL = '%s-upload-symbols'
STAGING_DEDUPE_NAMESPACE_TMPL = '%s-staging' % OFFICIAL_DEDUPE_NAMESPACE_TMPL


# The minimum average rate (in bytes per second) that we expect to maintain
# when uploading symbols.  This has to allow for symbols that are up to
# CRASH_SERVER_FILE_LIMIT in size.
UPLOAD_MIN_RATE = CRASH_SERVER_FILE_LIMIT / (30 * 60)

# The lowest timeout (in seconds) we'll allow.  If the server is overloaded,
# then there might be a delay in setting up the connection, not just with the
# transfer.  So even a small file might need a larger value.
UPLOAD_MIN_TIMEOUT = 5 * 60


# Sleep for 500ms in between uploads to avoid DoS'ing symbol server.
SLEEP_DELAY = 0.5


# Number of seconds to wait before retrying an upload.  The delay will double
# for each subsequent retry of the same symbol file.
INITIAL_RETRY_DELAY = 1

# Allow up to 7 attempts to upload a symbol file (total delay may be
# 1+2+4+8+16+32=63 seconds).
MAX_RETRIES = 6

# Number of total errors, before uploads are no longer attempted.
# This is used to avoid lots of errors causing unreasonable delays.
MAX_TOTAL_ERRORS_FOR_RETRY = 30

# Category to use for collection upload retry stats.
UPLOAD_STATS = 'UPLOAD'


def BatchGenerator(iterator, batch_size):
  """Given an iterator, break into lists of size batch_size.

  The result is a generator, that will only read in as many inputs as needed for
  the current batch. The final result can be smaller than batch_size.
  """
  batch = []
  for i in iterator:
    batch.append(i)
    if len(batch) >= batch_size:
      yield batch
      batch = []

  if batch:
    # if there was anything left in the final batch, yield it.
    yield batch


def IsTarball(path):
  """Guess if this is a tarball based on the filename."""
  parts = path.split('.')
  if len(parts) <= 1:
    return False

  if parts[-1] == 'tar':
    return True

  if parts[-2] == 'tar':
    return parts[-1] in ('bz2', 'gz', 'xz')

  return parts[-1] in ('tbz2', 'tbz', 'tgz', 'txz')


class SymbolFile(object):
  """This class represents the state of a symbol file during processing.

  Properties:
    display_path: Name of symbol file that should be consistent between builds.
    file_name: Transient path of the symbol file.
    header: ReadSymsHeader output. Dict with assorted meta-data.
    status: INITIAL, DUPLICATE, or UPLOADED based on status of processing.
    dedupe_item: None or instance of DedupeItem for this symbol file.
    dedupe_push_state: Opaque value to return to dedupe code for file.
    display_name: Read only friendly (short) file name for logging.
    file_size: Read only size of the symbol file.
  """
  INITIAL = 'initial'
  DUPLICATE = 'duplicate'
  UPLOADED = 'uploaded'
  ERROR = 'error'

  def __init__(self, display_path, file_name):
    """An instance of this class represents a symbol file over time.

    Args:
      display_path: A unique/persistent between builds name to present to the
                    crash server. It is the file name, relative to where it
                    came from (tarball, breakpad dir, etc).
      file_name: A the current location of the symbol file.
    """
    self.display_path = display_path
    self.file_name = file_name
    self.header = cros_generate_breakpad_symbols.ReadSymsHeader(file_name)
    self.status = SymbolFile.INITIAL
    self.dedupe_item = None
    self.dedupe_push_state = None

  @property
  def display_name(self):
    return os.path.basename(self.display_path)

  def FileSize(self):
    return os.path.getsize(self.file_name)


class DedupeItem(isolateserver.BufferItem):
  """Turn a SymbolFile into an isolateserver.Item"""

  ALGO = hashlib.sha1

  def __init__(self, symbol):
    isolateserver.BufferItem.__init__(self, str(symbol.header), self.ALGO)
    self.symbol = symbol


def FindSymbolFiles(tempdir, paths):
  """Locate symbol files in |paths|

  This returns SymbolFile objects that contain file references which are valid
  after this exits. Those files may exist externally, or be created in the
  tempdir (say, when expanding tarballs). The caller must not consider
  SymbolFile's valid after tempdir is cleaned up.

  Args:
    tempdir: Path to use for temporary files.
    paths: A list of input paths to walk. Files are returned w/out any checks.
      Dirs are searched for files that end in ".sym". Urls are fetched and then
      processed. Tarballs are unpacked and walked.

  Yields:
    A SymbolFile for every symbol file found in paths.
  """
  cache_dir = path_util.GetCacheDir()
  common_path = os.path.join(cache_dir, constants.COMMON_CACHE)
  tar_cache = cache.TarballCache(common_path)

  for p in paths:
    # Pylint is confused about members of ParseResult.

    o = urlparse.urlparse(p)
    if o.scheme:  # pylint: disable=E1101
      # Support globs of filenames.
      ctx = gs.GSContext()
      for p in ctx.LS(p):
        logging.info('processing files inside %s', p)
        o = urlparse.urlparse(p)
        key = ('%s%s' % (o.netloc, o.path)).split('/')  # pylint: disable=E1101
        # The common cache will not be LRU, removing the need to hold a read
        # lock on the cached gsutil.
        ref = tar_cache.Lookup(key)
        try:
          ref.SetDefault(p)
        except cros_build_lib.RunCommandError as e:
          logging.warning('ignoring %s\n%s', p, e)
          continue
        for p in FindSymbolFiles(tempdir, [ref.path]):
          yield p

    elif os.path.isdir(p):
      for root, _, files in os.walk(p):
        for f in files:
          if f.endswith('.sym'):
            # If p is '/tmp/foo' and filename is '/tmp/foo/bar/bar.sym',
            # display_path = 'bar/bar.sym'
            filename = os.path.join(root, f)
            yield SymbolFile(display_path=filename[len(p):].lstrip('/'),
                             file_name=filename)

    elif IsTarball(p):
      logging.info('processing files inside %s', p)
      tardir = tempfile.mkdtemp(dir=tempdir)
      cache.Untar(os.path.realpath(p), tardir)
      for p in FindSymbolFiles(tardir, [tardir]):
        yield p

    else:
      yield SymbolFile(display_path=p, file_name=p)


def AdjustSymbolFileSize(symbol, tempdir, file_limit):
  """Examine symbols files for size problems, and reduce if needed.

  If the symbols size is too big, strip out the call frame info.  The CFI
  is unnecessary for 32bit x86 targets where the frame pointer is used (as
  all of ours have) and it accounts for over half the size of the symbols
  uploaded.

  Stripped files will be created inside tempdir, and will be the callers
  responsibility to clean up.

  We also warn, if a symbols file is still too large after stripping.

  Args:
    symbol: SymbolFile instance to be examined and modified as needed..
    tempdir: A temporary directory we can create files in that the caller will
             clean up.
    file_limit: We only strip files which are larger than this limit.

  Returns:
    SymbolFile instance (original or modified as needed)
  """
  file_size = symbol.FileSize()

  if file_limit and symbol.FileSize() > file_limit:
    with tempfile.NamedTemporaryFile(
        prefix='upload_symbols', bufsize=0,
        dir=tempdir, delete=False) as temp_sym_file:

      temp_sym_file.writelines(
          [x for x in open(symbol.file_name, 'rb').readlines()
           if not x.startswith('STACK CFI')]
      )

      original_file_size = file_size
      symbol.file_name = temp_sym_file.name
      file_size = symbol.FileSize()

      logging.warning('stripped CFI for %s reducing size %s > %s',
                      symbol.display_name, original_file_size, file_size)

  # Hopefully the crash server will let it through.  But it probably won't.
  # Not sure what the best answer is in this case.
  if file_size >= CRASH_SERVER_FILE_LIMIT:
    logging.PrintBuildbotStepWarnings()
    logging.warning('upload file %s is awfully large, risking rejection by '
                    'the symbol server (%s > %s)', symbol.display_path,
                    file_size, CRASH_SERVER_FILE_LIMIT)

  return symbol

def OpenDeduplicateConnection(dedupe_namespace):
  """Open a connection to the isolate server for Dedupe use.

  Args:
    dedupe_namespace: String id for the comparison namespace.

  Returns:
    Connection proxy, or None on failure.
  """
  try:
    with timeout_util.Timeout(DEDUPE_TIMEOUT):
      return isolateserver.get_storage_api(constants.ISOLATESERVER,
                                           dedupe_namespace)
  except Exception:
    logging.warning('initializing isolate server connection failed',
                    exc_info=True)
    return None


def FindDuplicates(symbols, dedupe_namespace):
  """Mark symbol files we've already uploaded as duplicates.

  Using the swarming service, ask it to tell us which symbol files we've already
  uploaded in previous runs and/or by other bots.  If the query fails for any
  reason, we'll just upload all symbols.  This is fine as the symbol server will
  do the right thing and this phase is purely an optimization.

  Args:
    symbols: An iterable of SymbolFiles to be uploaded.
    dedupe_namespace: String id for the comparison namespace.

  Yields:
    All SymbolFiles from symbols, but duplicates have status updated to
    DUPLICATE.
  """
  storage_query = OpenDeduplicateConnection(dedupe_namespace)

  # We query isolate in batches, to reduce the number of network queries.
  for batch in BatchGenerator(symbols, DEDUPE_LIMIT):
    query_results = None

    if storage_query:
      # Convert SymbolFiles into DedupeItems.
      items = [DedupeItem(x) for x in batch]
      for item in items:
        item.prepare(DedupeItem.ALGO)

      # Look for duplicates.
      try:
        with timeout_util.Timeout(DEDUPE_TIMEOUT):
          query_results = storage_query.contains(items)
      except Exception:
        logging.warning('talking to dedupe server failed', exc_info=True)
        storage_query = None

    if query_results is not None:
      for b in batch:
        b.status = SymbolFile.DUPLICATE

      # Only the non-duplicates appear in the query_results.
      for item, push_state in query_results.iteritems():
        # Remember the dedupe state, so we can mark the symbol as uploaded
        # later on.
        item.symbol.status = SymbolFile.INITIAL
        item.symbol.dedupe_item = item
        item.symbol.dedupe_push_state = push_state

    # Yield all symbols we haven't shown to be duplicates.
    for b in batch:
      if b.status == SymbolFile.DUPLICATE:
        logging.debug('Found duplicate: %s', b.display_name)
      yield b


def PostForDeduplication(symbols, dedupe_namespace):
  """Send a symbol file to the swarming service

  Notify the isolate service of a successful upload. If the notification fails
  for any reason, we ignore it. We don't care as it just means we'll upload it
  again later on, and the symbol server will handle that graciously.

  Args:
    symbols: An iterable of SymbolFiles to be uploaded.
    dedupe_namespace: String id for the comparison namespace.

  Yields:
    Each symbol from symbols, unmodified.
  """
  storage_query = OpenDeduplicateConnection(dedupe_namespace)

  for s in symbols:
    # If we can talk to isolate, and we uploaded this symbol, and we
    # queried for it's presence before, upload to isolate now.

    if storage_query and s.status == SymbolFile.UPLOADED and s.dedupe_item:
      s.dedupe_item.prepare(DedupeItem.ALGO)
      try:
        with timeout_util.Timeout(DEDUPE_NOTIFY_TIMEOUT):
          storage_query.push(s.dedupe_item, s.dedupe_push_state,
                             s.dedupe_item.content())
          logging.info('sent %s', s.display_name)
      except Exception:
        logging.warning('posting %s to dedupe server failed',
                        os.path.basename(s.display_path), exc_info=True)
        storage_query = None

    yield s


def GetUploadTimeout(symbol):
  """How long to wait for a specific file to upload to the crash server.

  This is a function largely to make unittesting easier.

  Args:
    symbol: A SymbolFile instance.

  Returns:
    Timeout length (in seconds)
  """
  # Scale the timeout based on the filesize.
  return max(symbol.FileSize() / UPLOAD_MIN_RATE, UPLOAD_MIN_TIMEOUT)


def UploadSymbolFile(upload_url, symbol, product_name):
  """Upload a symbol file to the crash server.

  The upload is a multipart/form-data POST with the following parameters:
    code_file: the basename of the module, e.g. "app"
    code_identifier: the module file's identifier
    debug_file: the basename of the debugging file, e.g. "app"
    debug_identifier: the debug file's identifier, usually consisting of
                      the guid and age embedded in the pdb, e.g.
                      "11111111BBBB3333DDDD555555555555F"
    version: the file version of the module, e.g. "1.2.3.4"
    product: HTTP-friendly product name
    os: the operating system that the module was built for
    cpu: the CPU that the module was built for
    symbol_file: the contents of the breakpad-format symbol file

  Args:
    upload_url: The crash URL to POST the |sym_file| to
    symbol: A SymbolFile instance.
    product_name: A string for stats purposes. Usually 'ChromeOS' or 'Android'.
  """
  fields = (
      ('code_file', symbol.header.name),
      ('debug_file', symbol.header.name),
      ('debug_identifier', symbol.header.id.replace('-', '')),
      # The product/version fields are used by the server only for statistic
      # purposes.  They do not impact symbolization, so they're safe to set
      # to any value all the time.
      # In this case, we use it to help see the load our build system is
      # placing on the server.
      # Not sure what to set for the version.  Maybe the git sha1 of this file.
      # Note: the server restricts this to 30 chars.
      #('version', None),
      ('product', product_name),
      ('os', symbol.header.os),
      ('cpu', symbol.header.cpu),
      poster.encode.MultipartParam.from_file('symbol_file', symbol.file_name),
  )

  data, headers = poster.encode.multipart_encode(fields)
  request = urllib2.Request(upload_url, data, headers)
  request.add_header('User-agent', 'chromite.upload_symbols')
  urllib2.urlopen(request, timeout=GetUploadTimeout(symbol))


def PerformSymbolsFileUpload(symbols, upload_url, product_name='ChromeOS'):
  """Upload the symbols to the crash server

  Args:
    symbols: An iterable of SymbolFiles to be uploaded.
    upload_url: URL of crash server to upload too.
    failures: Tracker for total upload failures.
    product_name: A string for stats purposes. Usually 'ChromeOS' or 'Android'.

  Yields:
    Each symbol from symbols, perhaps modified.
  """
  failures = 0

  for s in symbols:
    if (failures < MAX_TOTAL_ERRORS_FOR_RETRY and
        s.status in (SymbolFile.INITIAL, SymbolFile.ERROR)):
      # Keeps us from DoS-ing the symbol server.
      time.sleep(SLEEP_DELAY)
      logging.info('Uploading symbol_file: %s', s.display_path)
      try:
        # This command retries the upload multiple times with growing delays. We
        # only consider the upload a failure if these retries fail.
        def ShouldRetryUpload(exception):
          return isinstance(exception, (urllib2.HTTPError, urllib2.URLError,
                                        httplib.HTTPException, socket.error))

        with cros_build_lib.TimedSection() as timer:
          retry_stats.RetryWithStats(
              UPLOAD_STATS, ShouldRetryUpload, MAX_RETRIES,
              UploadSymbolFile,
              upload_url, s, product_name,
              sleep=INITIAL_RETRY_DELAY,
              log_all_retries=True)
        logging.info('upload of %10i bytes took %s', s.FileSize(), timer.delta)
        s.status = SymbolFile.UPLOADED
      except urllib2.HTTPError as e:
        logging.warning('could not upload: %s: HTTP %s: %s',
                        s.display_name, e.code, e.reason)
        s.status = SymbolFile.ERROR
        failures += 1
      except (urllib2.URLError, httplib.HTTPException, socket.error) as e:
        logging.warning('could not upload: %s: %s %s', s.display_name,
                        type(e).__name__, e)
        s.status = SymbolFile.ERROR
        failures += 1

    # We pass the symbol along, on both success and failure.
    yield s


def ReportResults(symbols, failed_list):
  """Log a summary of the symbol uploading.

  This has the side effect of fully consuming the symbols iterator.

  Args:
    symbols: An iterator of SymbolFiles to be uploaded.
    failed_list: A filename at which to write out a list of our failed uploads.

  Returns:
    The number of symbols not uploaded.
  """
  upload_failures = []
  result_counts = {
      SymbolFile.INITIAL: 0,
      SymbolFile.UPLOADED: 0,
      SymbolFile.DUPLICATE: 0,
      SymbolFile.ERROR: 0,
  }

  for s in symbols:
    result_counts[s.status] += 1
    if s.status in [SymbolFile.INITIAL, SymbolFile.ERROR]:
      upload_failures.append(s)

  # Report retry numbers.
  _, _, retries = retry_stats.CategoryStats(UPLOAD_STATS)
  if retries:
    logging.warning('%d upload retries performed.', retries)

  logging.info('Uploaded %(uploaded)d, Skipped %(duplicate)d duplicates.',
               result_counts)

  if result_counts[SymbolFile.ERROR]:
    logging.PrintBuildbotStepWarnings()
    logging.warning('%d non-recoverable upload errors',
                    result_counts[SymbolFile.ERROR])

  if result_counts[SymbolFile.INITIAL]:
    logging.PrintBuildbotStepWarnings()
    logging.warning('%d upload(s) were skipped because of excessive errors',
                    result_counts[SymbolFile.INITIAL])

  if failed_list is not None:
    with open(failed_list, 'w') as fl:
      for s in upload_failures:
        fl.write('%s\n' % s.display_path)

  return result_counts[SymbolFile.INITIAL] + result_counts[SymbolFile.ERROR]


def UploadSymbols(sym_paths, upload_url, product_name, dedupe_namespace=None,
                  failed_list=None, upload_limit=None, strip_cfi=None):
  """Upload all the generated symbols for |board| to the crash server

  Args:
    sym_paths: Specific symbol files (or dirs of sym files) to upload,
      otherwise search |breakpad_dir|
    upload_url: URL of crash server to upload too.
    product_name: A string for crash server stats purposes.
                  Usually 'ChromeOS' or 'Android'.
    dedupe_namespace: None for no deduping, or string namespace in isolate.
    failed_list: A filename at which to write out a list of our failed uploads.
    upload_limit: Integer listing how many files to upload. None for no limit.
    strip_cfi: File size at which we strip out CFI data. None for no limit.

  Returns:
    The number of errors that were encountered.
  """
  retry_stats.SetupStats()

  # Note: This method looks like each step of processing is performed
  # sequentially for all SymbolFiles, but instead each step is a generator that
  # produces the next iteration only when it's read. This means that (except for
  # some batching) each SymbolFile goes through all of these steps before the
  # next one is processed at all.

  # This is used to hold striped
  with osutils.TempDir(prefix='upload_symbols.') as tempdir:
    symbols = FindSymbolFiles(tempdir, sym_paths)

    # Sort all of our symbols so the largest ones (probably the most important)
    # are processed first.
    symbols = list(symbols)
    symbols.sort(key=lambda s: s.FileSize(), reverse=True)

    if upload_limit is not None:
      # Restrict symbols processed to the limit.
      symbols = itertools.islice(symbols, None, upload_limit)

    # Strip CFI, if needed.
    symbols = (AdjustSymbolFileSize(s, tempdir, strip_cfi) for s in symbols)

    # Skip duplicates.
    if dedupe_namespace:
      symbols = FindDuplicates(symbols, dedupe_namespace)

    # Perform uploads
    symbols = PerformSymbolsFileUpload(symbols, upload_url, product_name)

    # Record for future deduping.
    if dedupe_namespace:
      symbols = PostForDeduplication(symbols, dedupe_namespace)

    # Log the final results, and consume the symbols generator fully.
    failures = ReportResults(symbols, failed_list)

  return failures


def main(argv):
  parser = commandline.ArgumentParser(description=__doc__)

  # TODO: Make sym_paths, breakpad_root, and root exclusive.

  parser.add_argument('sym_paths', type='path_or_uri', nargs='*', default=None,
                      help='symbol file or directory or URL or tarball')
  parser.add_argument('--board', default=None,
                      help='Used to find default breakpad_root.')
  parser.add_argument('--breakpad_root', type='path', default=None,
                      help='full path to the breakpad symbol directory')
  parser.add_argument('--root', type='path', default=None,
                      help='full path to the chroot dir')
  parser.add_argument('--official_build', action='store_true', default=False,
                      help='point to official symbol server')
  parser.add_argument('--server', type=str, default=None,
                      help='URI for custom symbol server')
  parser.add_argument('--regenerate', action='store_true', default=False,
                      help='regenerate all symbols')
  parser.add_argument('--upload-limit', type=int,
                      help='only upload # number of symbols')
  parser.add_argument('--strip_cfi', type=int,
                      default=DEFAULT_FILE_LIMIT,
                      help='strip CFI data for files above this size')
  parser.add_argument('--failed-list', type='path',
                      help='where to save a list of failed symbols')
  parser.add_argument('--dedupe', action='store_true', default=False,
                      help='use the swarming service to avoid re-uploading')
  parser.add_argument('--yes', action='store_true', default=False,
                      help='answer yes to all prompts')
  parser.add_argument('--product_name', type=str, default='ChromeOS',
                      help='Produce Name for breakpad stats.')

  opts = parser.parse_args(argv)
  opts.Freeze()

  # Figure out the symbol files/directories to upload.
  if opts.sym_paths:
    sym_paths = opts.sym_paths
  elif opts.breakpad_root:
    sym_paths = [opts.breakpad_root]
  elif opts.root:
    if not opts.board:
      cros_build_lib.Die('--board must be set if --root is used.')
    breakpad_dir = cros_generate_breakpad_symbols.FindBreakpadDir(opts.board)
    sym_paths = [os.path.join(opts.root, breakpad_dir.lstrip('/'))]
  else:
    cros_build_lib.Die('--sym_paths, --breakpad_root, or --root must be set.')

  if opts.sym_paths or opts.breakpad_root:
    if opts.regenerate:
      cros_build_lib.Die('--regenerate may not be used with specific files, '
                         'or breakpad_root')
  else:
    if opts.board is None:
      cros_build_lib.Die('--board is required')

  # Figure out the dedupe namespace.
  dedupe_namespace = None
  if opts.dedupe:
    if opts.official_build:
      dedupe_namespace = OFFICIAL_DEDUPE_NAMESPACE_TMPL % opts.product_name
    else:
      dedupe_namespace = STAGING_DEDUPE_NAMESPACE_TMPL % opts.product_name

  # Figure out which crash server to upload too.
  upload_url = opts.server
  if not upload_url:
    if opts.official_build:
      upload_url = OFFICIAL_UPLOAD_URL
    else:
      logging.warning('unofficial builds upload to the staging server')
      upload_url = STAGING_UPLOAD_URL

  # Confirm we really want the long upload.
  if not opts.yes:
    prolog = '\n'.join(textwrap.wrap(textwrap.dedent("""
        Uploading symbols for an entire Chromium OS build is really only
        necessary for release builds and in a few cases for developers
        to debug problems.  It will take considerable time to run.  For
        developer debugging purposes, consider instead passing specific
        files to upload.
    """), 80)).strip()
    if not cros_build_lib.BooleanPrompt(
        prompt='Are you sure you want to upload all build symbols',
        default=False, prolog=prolog):
      cros_build_lib.Die('better safe than sorry')

  ret = 0

  # Regenerate symbols from binaries.
  if opts.regenerate:
    ret += cros_generate_breakpad_symbols.GenerateBreakpadSymbols(
        opts.board, breakpad_dir=opts.breakpad_root)

  # Do the upload.
  ret += UploadSymbols(
      sym_paths=sym_paths,
      upload_url=upload_url,
      product_name=opts.product_name,
      dedupe_namespace=dedupe_namespace,
      failed_list=opts.failed_list,
      upload_limit=opts.upload_limit,
      strip_cfi=opts.strip_cfi)

  if ret:
    logging.error('encountered %i problem(s)', ret)
    # Since exit(status) gets masked, clamp it to 1 so we don't inadvertently
    # return 0 in case we are a multiple of the mask.
    return 1
