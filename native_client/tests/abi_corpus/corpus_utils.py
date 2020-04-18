#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import codecs
import hashlib
import json
import math
import os
import shutil
import struct
import subprocess
import sys
import threading
import time
import zipfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
TESTS_DIR = os.path.dirname(SCRIPT_DIR)
NACL_DIR = os.path.dirname(TESTS_DIR)

class DownloadError(Exception):
  """Indicates a download failed."""
  pass


class FailedTests(Exception):
  """Indicates a test run failed."""
  pass


def GsutilCopySilent(src, dst):
  """Invoke gsutil cp, swallowing the output, with retry.

  Args:
    src: src url.
    dst: dst path.
  """
  env = os.environ.copy()
  env['PATH'] = '/b/build/scripts/slave' + os.pathsep + env['PATH']
  # Retry to compensate for storage flake.
  for attempt in range(3):
    process = subprocess.Popen(
        ['gsutil', 'cp', src, dst],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    process_stdout, process_stderr = process.communicate()
    if process.returncode == 0:
      return
    time.sleep(math.pow(2, attempt + 1) * 5)
  raise DownloadError(
      'Unexpected return code: %s\n'
      '>>> STDOUT\n%s\n'
      '>>> STDERR\n%s\n' % (
        process.returncode, process_stdout, process_stderr))


def DownloadFileFromCorpus(src_path, dst_filename):
  """Download a file from our snapshot.

  Args:
    src_path: datastore relative path to download from.
    dst_filename: destination filename.
  """
  GsutilCopySilent('gs://nativeclient-snaps/%s' % src_path, dst_filename)


def DownloadCorpusList(list_filename):
  """Download list of all files in test corpus.

  Args:
    list_filename: destination filename (kept around for debugging).
  Returns:
    List of files.
  """
  DownloadFileFromCorpus('naclapps.all', list_filename)
  fh = open(list_filename)
  filenames = fh.read().splitlines()
  fh.close()
  return filenames


def Sha1Digest(path):
  """Determine the sha1 hash of a file's contents given its path."""
  m = hashlib.sha1()
  fh = open(path, 'rb')
  m.update(fh.read())
  fh.close()
  return m.hexdigest()


def Hex2Alpha(ch):
  """Convert a hexadecimal digit from 0-9 / a-f to a-p.

  Args:
    ch: a character in 0-9 / a-f.
  Returns:
    A character in a-p.
  """
  if ch >= '0' and ch <= '9':
    return chr(ord(ch) - ord('0') + ord('a'))
  else:
    return chr(ord(ch) + 10)


def ChromeAppIdFromPath(path):
  """Converts a path to the corrisponding chrome app id.

  A stable but semi-undocumented property of unpacked chrome extensions is
  that they are assigned an app-id based on the first 32 characters of the
  sha256 digest of the absolute symlink expanded path of the extension.
  Instead of hexadecimal digits, characters a-p.
  From discussion with webstore team + inspection of extensions code.
  Args:
    path: Path to an unpacked extension.
  Returns:
    A 32 character chrome extension app id.
  """
  hasher = hashlib.sha256()
  hasher.update(os.path.realpath(path))
  hexhash = hasher.hexdigest()[:32]
  return ''.join([Hex2Alpha(ch) for ch in hexhash])


def RunWithTimeout(cmd, timeout):
  """Run a program, capture output, allowing to run up to a timeout.

  Args:
    cmd: List of strings containing command to run.
    timeout: Duration to timeout.
  Returns:
    Tuple of stdout, stderr, returncode.
  """
  process = subprocess.Popen(cmd,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE)
  # Put the read in another thread so the buffer doesn't fill up.
  def GatherOutput(fh, dst):
    dst.append(fh.read())
  # Gather stdout.
  stdout_output = []
  stdout_thread = threading.Thread(
      target=GatherOutput, args=(process.stdout, stdout_output))
  stdout_thread.start()
  # Gather stderr.
  stderr_output = []
  stderr_thread = threading.Thread(
      target=GatherOutput, args=(process.stderr, stderr_output))
  stderr_thread.start()
  # Wait for a small span for the app to load.
  time.sleep(timeout)
  process.kill()
  # Join up.
  process.wait()
  stdout_thread.join()
  stderr_thread.join()
  # Pick out result.
  return stdout_output[0], stderr_output[0], process.returncode


def LoadManifest(app_path):
  manifest_data = codecs.open(os.path.join(app_path, 'manifest.json'),
                             'r', encoding='utf-8').read()
  # Ignore CRs as they confuse json.loads.
  manifest_data = manifest_data.replace('\r', '')
  # Ignore unicode endian markers as they confuse json.loads.
  manifest_data = manifest_data.replace(u'\ufeff', '')
  manifest_data = manifest_data.replace(u'\uffee', '')
  return json.loads(manifest_data)


def CachedPath(cache_dir, filename):
  """Find the full path of a cached file, a cache root relative path.

  Args:
    cache_dir: directory to keep the cache in.
    filename: filename relative to the top of the download url / cache.
  Returns:
    Absolute path of where the file goes in the cache.
  """
  return os.path.join(cache_dir, 'nacl_abi_corpus_cache', filename)


def Sha1FromFilename(filename):
  """Get the expected sha1 of a file path.

  Throughout we use the convention that files are store to a name of the form:
    <path_to_file>/<sha1hex>[.<some_extention>]
  This function extracts the expected sha1.

  Args:
    filename: filename to extract.
  Returns:
    Excepted sha1.
  """
  return os.path.splitext(os.path.basename(filename))[0]


def PrimeCache(cache_dir, filename):
  """Attempt to add a file to the cache directory if its not already there.

  Args:
    cache_dir: directory to keep the cache in.
    filename: filename relative to the top of the download url / cache.
  """
  dpath = CachedPath(cache_dir, filename)
  if (not os.path.exists(dpath) or
      Sha1Digest(dpath) != Sha1FromFilename(filename)):
    # Try to make the directory, fail is ok, let the download fail instead.
    try:
      os.makedirs(os.path.dirname(dpath))
    except OSError:
      pass
    DownloadFileFromCorpus(filename, dpath)


def CopyFromCache(cache_dir, filename, dest_filename):
  """Copy an item from the cache.

  Args:
    cache_dir: directory to keep the cache in.
    filename: filename relative to the top of the download url / cache.
    dest_filename: location to copy the file to.
  """
  dpath = CachedPath(cache_dir, filename)
  shutil.copy(dpath, dest_filename)
  assert Sha1Digest(dest_filename) == Sha1FromFilename(filename)


def ExtractFromCache(cache_dir, source, dest):
  """Extract a crx from the cache.

  Args:
    cache_dir: directory to keep the cache in.
    source: crx file to extract (cache relative).
    dest: location to extract to.
  """
  # We don't want to accidentally extract two extensions on top of each other.
  # Assert that the destination doesn't yet exist.
  assert not os.path.exists(dest)
  dpath = CachedPath(cache_dir, source)
  # The cached location must exist.
  assert os.path.exists(dpath)
  zf = zipfile.ZipFile(dpath, 'r')
  os.makedirs(dest)
  for info in zf.infolist():
    # Skip directories.
    if info.filename.endswith('/'):
      continue
    # Do not support absolute paths or paths containing ..
    if os.path.isabs(info.filename) or '..' in info.filename:
      raise Exception('Unacceptable zip filename %s' % info.filename)
    tpath = os.path.join(dest, info.filename)
    tdir = os.path.dirname(tpath)
    if not os.path.exists(tdir):
      os.makedirs(tdir)
    zf.extract(info, dest)
  zf.close()


def DefaultCacheDirectory():
  """Decide a default cache directory.

  Decide a default cache directory.
  Prefer /b (for the bots)
  Failing that, use scons-out.
  Failing that, use the current users's home dir.
  Returns:
    Default to use for a corpus cache directory.
  """
  default_cache_dir = '/b'
  if not os.path.isdir(default_cache_dir):
    default_cache_dir = os.path.join(NACL_DIR, 'scons-out')
  if not os.path.isdir(default_cache_dir):
    default_cache_dir = os.path.expanduser('~/')
  default_cache_dir = os.path.realpath(default_cache_dir)
  assert os.path.isdir(default_cache_dir)
  assert os.path.realpath('.') != default_cache_dir
  return default_cache_dir


def NexeArchitecture(filename):
  """Decide the architecture of a nexe.

  Args:
    filename: filename of the nexe.
  Returns:
    Architecture string (x86-32 / x86-64) or None.
  """
  fh = open(filename, 'rb')
  head = fh.read(20)
  # Must not be too short.
  if len(head) != 20:
    print 'ERROR - header too short'
    return None
  # Must have ELF header.
  if head[0:4] != '\x7fELF':
    print 'ERROR - no elf header'
    return None
  # Decode e_machine
  machine = struct.unpack('<H', head[18:])[0]
  return {
      3: 'x86-32',
      #40: 'arm',  # TODO(bradnelson): handle arm.
      62: 'x86-64',
  }.get(machine)


class Progress(object):
  def __init__(self, total):
    self.total = total
    self.count = 0
    self.successes = 0
    self.failures = 0
    self.skips = 0
    self.start = time.time()

  def Tally(self):
    if self.count > 0:
      tm = time.time()
      eta = (self.total - self.count) * (tm - self.start) / self.count
      eta_minutes = int(eta // 60)
      eta_seconds = int(eta % 60)
      eta_str = ' (ETA %d:%02d)' % (eta_minutes, eta_seconds)
    else:
      eta_str = ''
    self.count += 1
    print 'Processing %d of %d%s...' % (self.count, self.total, eta_str)

  def Result(self, success):
    if success:
      self.successes += 1
    else:
      self.failures += 1

  def Skip(self):
    self.skips += 1

  def Summary(self, warn_only=False):
    print 'Ran tests on %d of %d items.' % (
        self.successes + self.failures, self.total)
    if self.skips:
      print '%d tests were skipped.' % self.skips
    if self.failures:
      # Our alternate validators don't currently cover everything.
      # For now, don't fail just emit warning (and a tally of failures).
      print '@@@STEP_TEXT@FAILED %d times (%.1f%% are incorrect)@@@' % (
          self.failures, self.failures * 100 / (self.successes + self.failures))
      if warn_only:
        print '@@@STEP_WARNINGS@@@'
      else:
        raise FailedTests('FAILED %d tests' % self.failures)
    else:
      print 'SUCCESS'
