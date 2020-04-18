# -*- coding: utf-8 -*-
# Copyright 2013 Google Inc. All Rights Reserved.
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
"""Base test case class for unit and integration tests."""

from __future__ import absolute_import

from functools import wraps
import os.path
import random
import re
import shutil
import tempfile

import boto
from gslib.posix_util import NA_ID
from gslib.posix_util import NA_MODE
import gslib.tests.util as util
from gslib.tests.util import unittest
from gslib.util import UTF8
from six import string_types

MAX_BUCKET_LENGTH = 63


def NotParallelizable(func):
  """Wrapper function for cases that are not parallelizable."""
  @wraps(func)
  def ParallelAnnotatedFunc(*args, **kwargs):
    return func(*args, **kwargs)
  ParallelAnnotatedFunc.is_parallelizable = False
  return ParallelAnnotatedFunc


def RequiresIsolation(func):
  """Wrapper function for cases that require running in a separate process."""
  @wraps(func)
  def RequiresIsolationFunc(*args, **kwargs):
    return func(*args, **kwargs)
  RequiresIsolationFunc.requires_isolation = True
  return RequiresIsolationFunc


class GsUtilTestCase(unittest.TestCase):
  """Base test case class for unit and integration tests."""

  def setUp(self):
    if util.RUN_S3_TESTS:
      self.test_api = 'XML'
      self.default_provider = 's3'
      self.provider_custom_meta = 'amz'
    else:
      self.test_api = boto.config.get('GSUtil', 'prefer_api', 'JSON').upper()
      self.default_provider = 'gs'
      self.provider_custom_meta = 'goog'
    self.tempdirs = []

  def tearDown(self):
    while self.tempdirs:
      tmpdir = self.tempdirs.pop()
      shutil.rmtree(tmpdir, ignore_errors=True)

  def assertNumLines(self, text, numlines):
    self.assertEqual(text.count('\n'), numlines)

  def GetTestMethodName(self):
    if isinstance(self._testMethodName, unicode):
      return self._testMethodName.encode(UTF8)
    return self._testMethodName

  def MakeRandomTestString(self):
    """Creates a random string of hex characters 8 characters long."""
    return '%08x' % random.randrange(256**4)

  def MakeTempName(self, kind, prefix=''):
    """Creates a temporary name that is most-likely unique.

    Args:
      kind: A string indicating what kind of test name this is.
      prefix: Prefix string to be used in the temporary name.

    Returns:
      The temporary name.
    """
    name = '%sgsutil-test-%s-%s' % (prefix, self.GetTestMethodName(), kind)
    name = name[:MAX_BUCKET_LENGTH-9]
    name = '%s-%s' % (name, self.MakeRandomTestString())
    return name

  # TODO: Convert tests to use this for object names.
  def MakeTempUnicodeName(self, kind, prefix=''):
    return self.MakeTempName(kind, prefix=prefix) + '材'

  def CreateTempDir(self, test_files=0, contents=None):
    """Creates a temporary directory on disk.

    The directory and all of its contents will be deleted after the test.

    Args:
      test_files: The number of test files to place in the directory or a list
                  of test file names.
      contents: The contents for each generated test file.

    Returns:
      The path to the new temporary directory.
    """
    tmpdir = tempfile.mkdtemp(prefix=self.MakeTempName('directory'))
    self.tempdirs.append(tmpdir)
    try:
      iter(test_files)
    except TypeError:
      test_files = [self.MakeTempName('file') for _ in range(test_files)]
    for i, name in enumerate(test_files):
      contents_file = contents
      if contents_file is None:
        contents_file = 'test %d' % i
      self.CreateTempFile(tmpdir=tmpdir, file_name=name, contents=contents_file)
    return tmpdir

  def CreateTempFifo(self, tmpdir=None, file_name=None):
    """Creates a temporary fifo file on disk. Should not be used on Windows.

    Args:
      tmpdir: The temporary directory to place the file in. If not specified, a
          new temporary directory is created.
      file_name: The name to use for the file. If not specified, a temporary
          test file name is constructed. This can also be a tuple, where
          ('dir', 'foo') means to create a file named 'foo' inside a
          subdirectory named 'dir'.

    Returns:
      The path to the new temporary fifo.
    """
    tmpdir = tmpdir or self.CreateTempDir()
    file_name = file_name or self.MakeTempName('fifo')
    if isinstance(file_name, basestring):
      fpath = os.path.join(tmpdir, file_name)
    else:
      fpath = os.path.join(tmpdir, *file_name)
    os.mkfifo(fpath)
    return fpath

  def CreateTempFile(self, tmpdir=None, contents=None, file_name=None,
                     mtime=None, mode=NA_MODE, uid=NA_ID, gid=NA_ID):
    """Creates a temporary file on disk.

    Note: if mode, uid, or gid are present, they must be validated by
    ValidateFilePermissionAccess and ValidatePOSIXMode before calling this
    function.

    Args:
      tmpdir: The temporary directory to place the file in. If not specified, a
              new temporary directory is created.
      contents: The contents to write to the file. If not specified, a test
                string is constructed and written to the file.
      file_name: The name to use for the file. If not specified, a temporary
                 test file name is constructed. This can also be a tuple, where
                 ('dir', 'foo') means to create a file named 'foo' inside a
                 subdirectory named 'dir'.
      mtime: The modification time of the file in POSIX time (seconds since
             UTC 1970-01-01). If not specified, this defaults to the current
             system time.
      mode: The POSIX mode for the file. Must be a base-8 3-digit integer
            represented as a string.
      uid: A POSIX user ID.
      gid: A POSIX group ID.

    Returns:
      The path to the new temporary file.
    """
    tmpdir = tmpdir or self.CreateTempDir()
    file_name = file_name or self.MakeTempName('file')
    if isinstance(file_name, basestring):
      fpath = os.path.join(tmpdir, file_name)
    else:
      fpath = os.path.join(tmpdir, *file_name)
    if not os.path.isdir(os.path.dirname(fpath)):
      os.makedirs(os.path.dirname(fpath))

    with open(fpath, 'wb') as f:
      contents = (contents if contents is not None
                  else self.MakeTempName('contents'))
      f.write(contents)
    if mtime is not None:
      # Set the atime and mtime to be the same.
      os.utime(fpath, (mtime, mtime))
    if uid != NA_ID or gid != NA_ID:
      os.chown(fpath, uid, gid)
    if mode != NA_MODE:
      os.chmod(fpath, int(mode, 8))
    return fpath

  def assertRegexpMatchesWithFlags(self, text, pattern, msg=None, flags=0):
    """Like assertRegexpMatches, but allows specifying additional re flags.

    Args:
      text: The text in which to search for pattern.
      pattern: The pattern to search for; should be either a string or compiled
          regex returned from re.compile().
      msg: The message to be displayed if pattern is not found in text. The
          values for pattern and text will be included after this message.
      flags: Additional flags from the re module to be included when compiling
          pattern. If pattern is a regex that was compiled with existing flags,
          these, flags will be added via a bitwise-or.
    """
    if isinstance(pattern, string_types):
      pattern = re.compile(pattern, flags=flags)
    else:  # It's most likely an already-compiled pattern.
      pattern = re.compile(pattern.pattern, flags=pattern.flags | flags)
    if not pattern.search(text):
      failure_msg = msg or 'Regex didn\'t match'
      failure_msg = '%s: %r not found in %r' % (
          failure_msg, pattern.pattern, text)
      raise self.failureException(failure_msg)
