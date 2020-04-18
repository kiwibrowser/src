# -*- coding: utf-8 -*-
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Common Google Storage interface library."""

from __future__ import print_function

import base64
import errno
import os
import re

from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import osutils
from chromite.lib.paygen import filelib
from chromite.lib.paygen import utils


PROTOCOL = 'gs'
RETRY_ATTEMPTS = 2
GS_LS_STATUS_RE = re.compile(r'status=(\d+)')

# Gsutil is filled in by "FindGsUtil" on first invocation.
GSUTIL = None


def FindGsUtil():
  """Find which gsutil executuable to use.

  This may download and cache the command if needed, and will return the
  version pinned by chromite for general use. Will cache the result after
  the first call.

  This function is multi-process safe, but NOT THREAD SAFE. If you need
  to use gsutil functionality in threads, call this function at least
  once before creating the threads. That way the value will be safely
  pre-cached.

  Returns:
    Full path to the gsutil command to use.
  """
  # TODO(dgarrett): This is a hack. Merge chromite and crostools to fix.

  global GSUTIL  # pylint: disable=global-statement
  if GSUTIL is None:
    GSUTIL = gs.GSContext.GetDefaultGSUtilBin()

  return GSUTIL


class GsutilError(Exception):
  """Base exception for errors where gsutil cannot be used for any reason."""


class GsutilMissingError(GsutilError):
  """Returned when the gsutil utility is missing from PATH."""

  def __init__(self, msg='The gsutil utility must be installed.'):
    GsutilError.__init__(self, msg)


class GSLibError(Exception):
  """Raised when gsutil command runs but gives an error."""


class CopyFail(GSLibError):
  """Raised if Copy fails in any way."""


class MoveFail(GSLibError):
  """Raised if Move fails in any way."""


class RemoveFail(GSLibError):
  """Raised if Remove fails in any way."""


class CatFail(GSLibError):
  """Raised if Cat fails in any way."""


class StatFail(GSLibError):
  """Raised if Stat fails in any way."""


class URIError(GSLibError):
  """Raised when URI does not behave as expected."""


class ValidateGsutilFailure(GSLibError):
  """We are unable to validate that gsutil is working correctly."""


def RetryGSLib(func):
  """Decorator to retry function calls that throw an exception.

  If the decorated method throws a GSLibError exception, the exception
  will be thrown away and the function will be run again until all retries
  are exhausted. On the final attempt, the exception will be thrown normally.

  Three attempts in total will be made to run the function (one more
  than RETRY_ATTEMPTS).

  @RetryGSLib
  def MyFunctionHere(): pass
  """
  def RetryHandler(*args, **kwargs):
    """Retry func with given args/kwargs RETRY_ATTEMPTS times."""
    warning_msgs = []
    for i in xrange(0, RETRY_ATTEMPTS + 1):
      try:
        return func(*args, **kwargs)
      except GSLibError as ex:
        # On the last try just pass the exception on up.
        if i >= RETRY_ATTEMPTS:
          raise

        error_msg = str(ex)
        RESUMABLE_ERROR_MESSAGE = (
            gs.GSContext.RESUMABLE_DOWNLOAD_ERROR,
            gs.GSContext.RESUMABLE_UPLOAD_ERROR,
            'ResumableUploadException',
            'ResumableDownloadException',
            'ssl.SSLError: The read operation timed out',
        )
        if (func.__name__ == 'Copy' and
            any(x in error_msg for x in RESUMABLE_ERROR_MESSAGE)):
          logging.info(
              'Resumable download/upload exception occured for %s', args[1])
          # Pass the dest_path to get the tracker filename.
          tracker_filenames = gs.GSContext.GetTrackerFilenames(args[1])
          # This part of the code is copied from chromite.lib.gs with
          # slight modifications. This is a temporary solution until
          # we can deprecate crostools.lib.gslib (crbug.com/322740).
          logging.info('Potential list of tracker files: %s',
                       tracker_filenames)
          for tracker_filename in tracker_filenames:
            tracker_file_path = os.path.join(
                gs.GSContext.DEFAULT_GSUTIL_TRACKER_DIR,
                tracker_filename)
            if os.path.exists(tracker_file_path):
              logging.info('Deleting gsutil tracker file %s before retrying.',
                           tracker_file_path)
              logging.info('The content of the tracker file: %s',
                           osutils.ReadFile(tracker_file_path))
              osutils.SafeUnlink(tracker_file_path)
        else:
          if 'AccessDeniedException' in str(ex) or 'NoSuchKey' in str(ex):
            raise

        # Record a warning message to be issued if a retry actually helps.
        warning_msgs.append('Try %d failed with error message:\n%s' %
                            (i + 1, ex))
      else:
        # If the func succeeded, then log any accumulated warning messages.
        if warning_msgs:
          logging.warning('Failed %s %d times before success:\n%s',
                          func.__name__, len(warning_msgs),
                          '\n'.join(warning_msgs))

  RetryHandler.__module__ = func.__module__
  RetryHandler.__name__ = func.__name__
  RetryHandler.__doc__ = func.__doc__
  return RetryHandler


def RunGsutilCommand(args,
                     redirect_stdout=True,
                     redirect_stderr=True,
                     failed_exception=GSLibError,
                     generation=None,
                     headers=None,
                     get_headers_from_stdout=False,
                     **kwargs):
  """Run gsutil with given args through RunCommand with given options.

  Generally this method is intended for use within this module, see the various
  command-specific wrappers provided for convenience.  However, it can be called
  directly if 'gsutil' needs to be called in specific way.

  A few of the options for RunCommand have their default values switched for
  this function.  Those options are called out explicitly as options here, while
  addition RunCommand options can be used through extra_run_command_opts.

  Args:
    args: List of arguments to use with 'gsutil'.
    redirect_stdout: Boolean option passed directly to RunCommand.
    redirect_stderr: Boolean option passed directly to RunCommand.
    failed_exception: Exception class to raise if CommandFailedException is
      caught.  It should be GSLibError or a subclass.
    generation: Only run the specified command if the generation matches.
       (See "Conditional Updates Using Object Versioning" in the gsutil docs.)
    headers: Fill in this dictionary with header values captured from stderr.
    get_headers_from_stdout: Whether header information is to be parsed from
      stdout (default: stderr).
    kwargs: Additional options to pass directly to RunCommand, beyond the
      explicit ones above.  See RunCommand itself.

  Returns:
    Anything that RunCommand returns, which should be a CommandResult object.

  Raises:
    GsutilMissingError is the gsutil utility cannot be found.
    GSLibError (or whatever is in failed_exception) if RunCommand failed (and
      error_code_ok was not True).
  """
  # The -d flag causes gsutil to dump various metadata, including user
  # credentials.  We therefore don't allow users to pass it in directly.
  assert '-d' not in args, 'Cannot pass in the -d flag directly'

  gsutil = FindGsUtil()

  if generation is not None:
    args = ['-h', 'x-goog-if-generation-match:%s' % generation] + args
  if headers is not None:
    args.insert(0, '-d')
    assert redirect_stderr
  cmd = [gsutil] + args
  run_opts = {
      'redirect_stdout': redirect_stdout,
      'redirect_stderr': redirect_stderr,
  }
  run_opts.update(kwargs)

  try:
    result = cros_build_lib.RunCommand(cmd, **run_opts)
  except OSError as e:
    if e.errno == errno.ENOENT:
      raise GsutilMissingError()
    raise
  except cros_build_lib.RunCommandError as e:
    # If headers is set, we have to hide the output here because it may contain
    # credentials that we don't want to show in buildbot logs.
    raise failed_exception('%r failed' % cmd if headers else e.result.error)

  if headers is not None and result is not None:
    assert redirect_stdout if get_headers_from_stdout else redirect_stderr
    # Parse headers that look like this:
    # header: x-goog-generation: 1359148994758000
    # header: x-goog-metageneration: 1
    headers_source = result.output if get_headers_from_stdout else result.error
    for line in headers_source.splitlines():
      if line.startswith('header: '):
        header, _, value = line.partition(': ')[-1].partition(': ')
        headers[header.replace('x-goog-', '')] = value

    # Strip out stderr entirely to avoid showing credentials in logs; for
    # commands that dump credentials to stdout, clobber that as well.
    result.error = '<stripped>'
    if get_headers_from_stdout:
      result.output = '<stripped>'

  return result


def ValidateGsutilWorking(bucket):
  """Validate that gsutil is working correctly.

  There is a failure mode for gsutil in which all operations fail, and this
  is indistinguishable from all gsutil ls operations matching nothing. We
  check that there is at least one file in the root of the bucket.

  Args:
    bucket: bucket we are about to test.

  Raises:
    ValidateGsutilFailure: If we are unable to find any files in the bucket.
  """
  url = 'gs://%s/' % bucket
  if not List(url):
    raise ValidateGsutilFailure('Unable to find anything in: %s' % url)


@RetryGSLib
def MD5Sum(gs_uri):
  """Read the gsutil md5 sum from etag and gsutil ls -L.

  Note that because this relies on 'gsutil ls -L' it suffers from the
  eventual consistency issue, meaning this function could fail to find
  the MD5 value for a recently created file in Google Storage.

  Args:
    gs_uri: An absolute Google Storage URI that refers directly to an object.
      No globs are supported.

  Returns:
    A string that is an md5sum, or None if no object found.

  Raises:
    GSLibError if the gsutil command fails.  If there is no object at that path
    that is not considered a failure.
  """
  gs_md5_regex = re.compile(r'.*?Hash \(md5\):\s+(.*)', re.IGNORECASE)
  args = ['ls', '-L', gs_uri]

  result = RunGsutilCommand(args, error_code_ok=True)

  # If object was not found then output is completely empty.
  if not result.output:
    return None

  for line in result.output.splitlines():
    match = gs_md5_regex.match(line)
    if match:
      # gsutil now prints the MD5 sum in base64, but we want it in hex.
      return base64.b16encode(base64.b64decode(match.group(1))).lower()

  # This means there was some actual failure in the command.
  raise GSLibError('Unable to determine MD5Sum for %r' % gs_uri)


@RetryGSLib
def Cmp(path1, path2):
  """Return True if paths hold identical files, according to MD5 sum.

  Note that this function relies on MD5Sum, which means it also can only
  promise eventual consistency.  A recently uploaded file in Google Storage
  may behave badly in this comparison function.

  If either file is missing then always return False.

  Args:
    path1: URI to a file.  Local paths also supported.
    path2: URI to a file.  Local paths also supported.

  Returns:
    True if files are the same, False otherwise.
  """
  md5_1 = MD5Sum(path1) if IsGsURI(path1) else filelib.MD5Sum(path1)
  if not md5_1:
    return False

  md5_2 = MD5Sum(path2) if IsGsURI(path2) else filelib.MD5Sum(path2)

  return md5_1 == md5_2


@RetryGSLib
def Copy(src_path, dest_path, acl=None, **kwargs):
  """Run gsutil cp src_path dest_path supporting GS globs.

  e.g.
  gsutil cp /etc/* gs://etc/ where /etc/* is src_path with a glob and
  gs://etc is dest_path.

  This assumes that the src or dest path already exist.

  Args:
    src_path: The src of the path to copy, either a /unix/path or gs:// uri.
    dest_path: The dest of the path to copy, either a /unix/path or gs:// uri.
    acl: an ACL argument (predefined name or XML file) to pass to gsutil
    kwargs: Additional options to pass directly to RunGsutilCommand, beyond the
      explicit ones above.  See RunGsutilCommand itself.

  Raises:
    CopyFail: If the copy fails for any reason.
  """
  args = ['cp']
  if acl:
    args += ['-a', acl]
  args += [src_path, dest_path]
  RunGsutilCommand(args, failed_exception=CopyFail, **kwargs)


@RetryGSLib
def Move(src_path, dest_path, **kwargs):
  """Run gsutil mv src_path dest_path supporting GS globs.

  Note that the created time is changed to now for the moved object(s).

  Args:
    src_path: The src of the path to move, either a /unix/path or gs:// uri.
    dest_path: The dest of the path to move, either a /unix/path or gs:// uri.
    kwargs: Additional options to pass directly to RunGsutilCommand, beyond the
      explicit ones above.  See RunGsutilCommand itself.

  Raises:
    MoveFail: If the move fails for any reason.
  """
  args = ['mv', src_path, dest_path]
  RunGsutilCommand(args, failed_exception=MoveFail, **kwargs)


@RetryGSLib
def Remove(*paths, **kwargs):  # pylint: disable=docstring-misnamed-args
  """Run gsutil rm on path supporting GS globs.

  Args:
    paths: Local path or gs URI, or list of same.
    ignore_no_match: If True, then do not complain if anything was not
      removed because no URI match was found.  Like rm -f.  Defaults to False.
    recurse: Remove recursively starting at path.  Same as rm -R.  Defaults
      to False.
    kwargs: Additional options to pass directly to RunGsutilCommand, beyond the
      explicit ones above.  See RunGsutilCommand itself.

  Raises:
    RemoveFail: If the remove fails for any reason.
  """
  ignore_no_match = kwargs.pop('ignore_no_match', False)
  recurse = kwargs.pop('recurse', False)

  args = ['rm']

  if recurse:
    args.append('-R')

  args.extend(paths)

  try:
    RunGsutilCommand(args, failed_exception=RemoveFail, **kwargs)
  except RemoveFail as e:
    should_raise = True
    msg = str(e.args[0])

    # Sometimes Google Storage glitches and complains about failing to remove a
    # specific revision of the file.  It ends up getting removed anyway, but it
    # throws a NotFoundException.
    if (ignore_no_match and (('No URLs matched' in msg) or
                             ('NotFoundException:' in msg))):
      should_raise = False

    if should_raise:
      raise

def RemoveDirContents(gs_dir_uri):
  """Remove all contents of a directory.

  Args:
    gs_dir_uri: directory to delete contents of.
  """
  Remove(os.path.join(gs_dir_uri, '**'), ignore_no_match=True)


def CreateWithContents(gs_uri, contents, **kwargs):
  """Creates the specified file with specified contents.

  Args:
    gs_uri: The URI of a file on Google Storage.
    contents: Contents to write to the file.
    kwargs: Additional options to pass directly to RunGsutilCommand, beyond the
      explicit ones above.  See RunGsutilCommand itself.

  Raises:
    CopyFail: If it fails for any reason.
  """
  with utils.CreateTempFileWithContents(contents) as content_file:
    Copy(content_file.name, gs_uri, **kwargs)


@RetryGSLib
def Cat(gs_uri, **kwargs):
  """Return the contents of a file at the given GS URI

  Args:
    gs_uri: The URI of a file on Google Storage.
    kwargs: Additional options to pass directly to RunGsutilCommand, beyond the
      explicit ones above.  See RunGsutilCommand itself.

  Raises:
    CatFail: If the cat fails for any reason.
  """
  args = ['cat', gs_uri]
  result = RunGsutilCommand(args, failed_exception=CatFail, **kwargs)
  return result.output


def Stat(gs_uri, **kwargs):
  """Stats a file at the given GS URI (returns nothing).

  Args:
    gs_uri: The URI of a file on Google Storage.
    kwargs: Additional options to pass directly to RunGsutilCommand, beyond the
      explicit ones above.  See RunGsutilCommand itself.

  Raises:
    StatFail: If the stat fails for any reason.
  """
  args = ['stat', gs_uri]
  # IMPORTANT! With stat, header information is dumped to standard output,
  # rather than standard error, as with other gsutil commands. Hence,
  # get_headers_from_stdout must be True to ensure both correct parsing of
  # output and stripping of sensitive information.
  RunGsutilCommand(args, failed_exception=StatFail,
                   get_headers_from_stdout=True, **kwargs)


def IsGsURI(path):
  """Returns true if the path begins with gs://

  Args:
    path: An absolute Google Storage URI.

  Returns:
    True if path is really a google storage uri that begins with gs://
    False otherwise.
  """
  return path and path.startswith(PROTOCOL + '://')


# TODO(mtennant): Rename this "Size" for consistency.
@RetryGSLib
def FileSize(gs_uri, **kwargs):
  """Return the size of the given gsutil file in bytes.

  Args:
    gs_uri: Google Storage URI (beginning with 'gs://') pointing
      directly to a single file.
    kwargs: Additional options to pass directly to RunGsutilCommand, beyond the
      explicit ones above.  See RunGsutilCommand itself.

  Returns:
    Size of file in bytes.

  Raises:
    URIError: Raised when URI is unknown to Google Storage or when
      URI matches more than one file.
  """
  headers = {}
  try:
    Stat(gs_uri, headers=headers, **kwargs)
  except StatFail as e:
    raise URIError('Unable to stat file at URI %r: %s' % (gs_uri, e))

  size_str = headers.get('stored-content-length')
  if size_str is None:
    raise URIError('Failed to get size of %r' % gs_uri)

  return int(size_str)


def Exists(gs_uri, **kwargs):
  """Return True if object exists at given GS URI.

  Args:
    gs_uri: Google Storage URI.  Must be a fully-specified URI with
      no glob expression.  Even if a glob expression matches this
      method will return False.
    kwargs: Additional options to pass directly to RunGsutilCommand, beyond the
      explicit ones above.  See RunGsutilCommand itself.

  Returns:
    True if gs_uri points to an existing object, and False otherwise.
  """
  try:
    Stat(gs_uri, **kwargs)
  except StatFail:
    return False

  return True


@RetryGSLib
def List(root_uri, recurse=False, filepattern=None, sort=False):
  """Return list of file and directory paths under given root URI.

  Args:
    root_uri: e.g. gs://foo/bar
    recurse: Look in subdirectories, as well
    filepattern: glob pattern to match against basename of path
    sort: If True then do a default sort on paths

  Returns:
    List of GS URIs to paths that matched
  """
  gs_uri = root_uri
  if recurse:
    # In gs file patterns '**' absorbs any number of directory names,
    # including none.
    gs_uri = gs_uri.rstrip('/') + '/**'

  # Now match the filename itself at the end of the URI.
  if filepattern:
    gs_uri = gs_uri.rstrip('/') + '/' + filepattern

  args = ['ls', gs_uri]

  try:
    result = RunGsutilCommand(args)
    paths = [path for path in result.output.splitlines() if path]

    if sort:
      paths = sorted(paths)

    return paths

  except GSLibError as e:
    # The ls command will fail under normal operation if there was just
    # nothing to be found. That shows up like this to stderr:
    # CommandException: One or more URLs matched no objects.
    if 'CommandException: One or more URLs matched no objects.' not in str(e):
      raise

  # Otherwise, assume a normal error.
  # TODO(mtennant): It would be more functionally correct to return this
  # if and only if the error is identified as a "file not found" error.
  # We simply have to determine how to do that reliably.
  return []


def ListFiles(root_uri, recurse=False, filepattern=None, sort=False):
  """Return list of file paths under given root URI.

  Directories are intentionally excluded.

  Args:
    root_uri: e.g. gs://foo/bar
    recurse: Look for files in subdirectories, as well
    filepattern: glob pattern to match against basename of file
    sort: If True then do a default sort on paths

  Returns:
    List of GS URIs to files that matched
  """
  paths = List(root_uri, recurse=recurse, filepattern=filepattern, sort=sort)

  # Directory paths should be excluded from output, per ListFiles guarantee.
  return [path for path in paths if not path.endswith('/')]
