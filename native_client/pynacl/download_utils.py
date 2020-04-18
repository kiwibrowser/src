#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A library to assist automatically downloading files.

This library is used by scripts that download tarballs, zipfiles, etc. as part
of the build process.
"""

import hashlib
import os.path
import re
import sys
import urllib2

import http_download

SOURCE_STAMP = 'SOURCE_URL'
HASH_STAMP = 'SOURCE_SHA1'

class HashError(Exception):
  def __init__(self, download_url, expected_hash, actual_hash):
    self.download_url = download_url
    self.expected_hash = expected_hash
    self.actual_hash = actual_hash

  def __str__(self):
    return 'Got hash "%s" but expected hash "%s" for "%s"' % (
        self.actual_hash, self.expected_hash, self.download_url)

def EnsureFileCanBeWritten(filename):
  directory = os.path.dirname(filename)
  if not os.path.exists(directory):
    os.makedirs(directory)


def WriteData(filename, data):
  EnsureFileCanBeWritten(filename)
  f = open(filename, 'wb')
  f.write(data)
  f.close()


def WriteDataFromStream(filename, stream, chunk_size, verbose=True):
  EnsureFileCanBeWritten(filename)
  dst = open(filename, 'wb')
  try:
    while True:
      data = stream.read(chunk_size)
      if len(data) == 0:
        break
      dst.write(data)
      if verbose:
        # Indicate that we're still writing.
        sys.stdout.write('.')
        sys.stdout.flush()
  finally:
    if verbose:
      sys.stdout.write('\n')
    dst.close()


def DoesStampMatch(stampfile, expected, index):
  try:
    f = open(stampfile, 'r')
    stamp = f.read()
    f.close()
    if stamp.split('\n')[index] == expected:
      return 'already up-to-date.'
    elif stamp.startswith('manual'):
      return 'manual override.'
    return False
  except IOError:
    return False


def WriteStamp(stampfile, data):
  EnsureFileCanBeWritten(stampfile)
  f = open(stampfile, 'w')
  f.write(data)
  f.close()


def StampIsCurrent(path, stamp_name, stamp_contents, min_time=None, index=0):
  stampfile = os.path.join(path, stamp_name)

  stampmatch = DoesStampMatch(stampfile, stamp_contents, index)

  # If toolchain was downloaded and/or created manually then keep it untouched
  if stampmatch == 'manual override.':
    return stampmatch

  # Check if the stampfile is older than the minimum last mod time
  if min_time:
    try:
      stamp_time = os.stat(stampfile).st_mtime
      if stamp_time <= min_time:
        return False
    except OSError:
      return False

  return stampmatch


def WriteSourceStamp(path, url):
  stampfile = os.path.join(path, SOURCE_STAMP)
  WriteStamp(stampfile, url)


def WriteHashStamp(path, hash_val):
  hash_stampfile = os.path.join(path, HASH_STAMP)
  WriteStamp(hash_stampfile, hash_val)


def _HashFileHandle(fh):
  """sha1 of a file like object.

  Arguments:
    fh: file handle like object to hash.
  Returns:
    sha1 as a string.
  """
  hasher = hashlib.sha1()
  try:
    while True:
      data = fh.read(4096)
      if not data:
        break
      hasher.update(data)
  finally:
    fh.close()
  return hasher.hexdigest()


def HashFile(filename):
  """sha1 a file on disk.

  Arguments:
    filename: filename to hash.
  Returns:
    sha1 as a string.
  """
  fh = open(filename, 'rb')
  return _HashFileHandle(fh)


def HashUrlByDownloading(url):
  """sha1 the data at an url.

  Arguments:
    url: url to download from.
  Returns:
    sha1 of the data at the url.
  """
  try:
    fh = urllib2.urlopen(url)
  except:
    sys.stderr.write('Failed fetching URL: %s\n' % url)
    raise
  return _HashFileHandle(fh)


# Attempts to get the SHA1 hash of a file given a URL by looking for
# an adjacent file with a ".sha1hash" suffix.  This saves having to
# download a large tarball just to get its hash.  Otherwise, we fall
# back to downloading the main file.
def HashUrl(url):
  hash_url = '%s.sha1hash' % url
  try:
    fh = urllib2.urlopen(hash_url)
    data = fh.read(100)
    fh.close()
  except urllib2.HTTPError, exn:
    if exn.code == 404:
      return HashUrlByDownloading(url)
    raise
  else:
    if not re.match('[0-9a-f]{40}\n?$', data):
      raise AssertionError('Bad SHA1 hash file: %r' % data)
    return data.strip()


def SyncURL(url, filename=None, stamp_dir=None, min_time=None,
            hash_val=None, keep=False, verbose=False, stamp_index=0):
  """Synchronize a destination file with a URL

  if the URL does not match the URL stamp, then we must re-download it.

  Arugments:
    url: the url which will to compare against and download
    filename: the file to create on download
    path: the download path
    stamp_dir: the filename containing the URL stamp to check against
    hash_val: if set, the expected hash which must be matched
    verbose: prints out status as it runs
    stamp_index: index within the stamp file to check.
  Returns:
    True if the file is replaced
    False if the file is not replaced
  Exception:
    HashError: if the hash does not match
  """

  assert url and filename

  # If we are not keeping the tarball, or we already have it, we can
  # skip downloading it for this reason. If we are keeping it,
  # it must exist.
  if keep:
    tarball_ok = os.path.isfile(filename)
  else:
    tarball_ok = True

  # If we don't need the tarball and the stamp_file matches the url, then
  # we must be up to date.  If the URL differs but the recorded hash matches
  # the one we'll insist the tarball has, then that's good enough too.
  # TODO(mcgrathr): Download the .sha1sum file first to compare with
  # the cached hash, in case --file-hash options weren't used.
  if tarball_ok and stamp_dir is not None:
    if StampIsCurrent(stamp_dir, SOURCE_STAMP, url, min_time):
      if verbose:
        print '%s is already up to date.' % filename
      return False
    if (hash_val is not None and
        StampIsCurrent(stamp_dir, HASH_STAMP, hash_val, min_time, stamp_index)):
      if verbose:
        print '%s is identical to the up to date file.' % filename
      return False

  if (os.path.isfile(filename)
      and hash_val is not None
      and hash_val == HashFile(filename)):
    return True

  if verbose:
    print 'Updating %s\n\tfrom %s.' % (filename, url)
  EnsureFileCanBeWritten(filename)
  http_download.HttpDownload(url, filename)

  if hash_val:
    tar_hash = HashFile(filename)
    if hash_val != tar_hash:
      raise HashError(actual_hash=tar_hash, expected_hash=hash_val,
                      download_url=url)

  return True
