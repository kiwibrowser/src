#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import os
import subprocess
import sys
import tempfile

sys.path.append(os.path.join(os.path.dirname(__file__), '../../'))
import pynacl.file_tools

import corpus_errors
import corpus_utils


class UnexpectedValidateResult(Exception):
  pass


def ValidateNexe(options, path, src_path, expect_pass):
  """Run the validator on a nexe, check if the result is expected.

  Args:
    options: bag of options.
    path: path to the nexe.
    src_path: path to nexe on server.
    expect_pass: boolean indicating if the nexe is expected to validate.
  Returns:
    True if validation matches expectations.
  """
  process = subprocess.Popen(
      [options.validator, path],
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE)
  process_stdout, process_stderr = process.communicate()
  # Check if result is what we expect.
  did_pass = (process.returncode == 0)
  if expect_pass != did_pass:
    if options.verbose:
      print '-' * 70
      print 'Validating: %s' % path
      print 'From: %s' % src_path
      print 'Size: %d' % os.path.getsize(path)
      print 'SHA1: %s' % corpus_utils.Sha1FromFilename(src_path)
      print 'Validator: %s' % options.validator
      print 'Unexpected return code: %s' % process.returncode
      print '>>> STDOUT'
      print process_stdout
      print '>>> STDERR'
      print process_stderr
      print '-' * 70
    else:
      print 'Unexpected return code %d on sha1: %s' % (
          process.returncode, corpus_utils.Sha1FromFilename(src_path))
    return False
  return True


def TestValidators(options, work_dir):
  """Test x86 validators on current snapshot.

  Args:
    options: bag of options.
    work_dir: directory to operate in.
  """
  nexe_filename = os.path.join(work_dir, 'test.nexe')
  list_filename = os.path.join(work_dir, 'naclapps.all')
  filenames = corpus_utils.DownloadCorpusList(list_filename)
  filenames = [f for f in filenames if f.endswith('.nexe') or f.endswith('.so')]
  progress = corpus_utils.Progress(len(filenames))
  for filename in filenames:
    progress.Tally()
    corpus_utils.PrimeCache(options.cache_dir, filename)
    # Stop here if downloading only.
    if options.download_only:
      continue
    # Skip if not the right architecture.
    architecture = corpus_utils.NexeArchitecture(
        corpus_utils.CachedPath(options.cache_dir, filename))
    if architecture != options.architecture:
      continue
    if corpus_errors.NexeShouldBeSkipped(filename):
      print 'Skipped, sha1: %s' % corpus_utils.Sha1FromFilename(filename)
      progress.Skip()
      continue
    # Validate a copy in case validator is mutating.
    corpus_utils.CopyFromCache(options.cache_dir, filename, nexe_filename)
    try:
      result = ValidateNexe(
          options, nexe_filename, filename,
          corpus_errors.NexeShouldValidate(filename))
      progress.Result(result)
      if not result and not options.keep_going:
        break
    finally:
      try:
        os.remove(nexe_filename)
      except OSError:
        print 'ERROR - unable to remove %s' % nexe_filename
  progress.Summary(warn_only=options.warn_only)


def Main():
  parser = optparse.OptionParser()
  parser.add_option(
      '--download-only', dest='download_only',
      default=False, action='store_true',
      help='download to cache without running the tests')
  parser.add_option(
      '-k', '--keep-going', dest='keep_going',
      default=False, action='store_true',
      help='keep going on failure')
  parser.add_option(
      '--validator', dest='validator',
      help='location of validator executable')
  parser.add_option(
      '--arch', dest='architecture',
      help='architecture of validator')
  parser.add_option(
      '--warn-only', dest='warn_only', default=False,
      action='store_true', help='only warn on failure')
  parser.add_option(
      '-v', '--verbose', dest='verbose', default=False, action='store_true',
      help='run in verbose mode')
  parser.add_option(
      '--cache-dir', dest='cache_dir',
      default=corpus_utils.DefaultCacheDirectory(),
      help='directory to cache downloads in')
  options, args = parser.parse_args()
  if args:
    parser.error('unused arguments')
  if not options.download_only:
    if not options.validator:
      parser.error('no validator specified')
    if not options.architecture:
      parser.error('no architecture specified')

  work_dir = tempfile.mkdtemp(suffix='validate_nexes', prefix='tmp')
  try:
    TestValidators(options, work_dir)
  finally:
    pynacl.file_tools.RemoveDirectoryIfPresent(work_dir)


if __name__ == '__main__':
  Main()
