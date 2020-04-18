#!/usr/bin/env python
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Integration test for breakpad in content shell.

This test checks that content shell and breakpad are correctly hooked up, as
well as that the tools can symbolize a stack trace."""


import glob
import json
import optparse
import os
import shutil
import subprocess
import sys
import tempfile
import time


CONCURRENT_TASKS=4

def run_test(options, crash_dir, additional_arguments = []):
  global failure
  global breakpad_tools_dir

  print "# Run content_shell and make it crash."
  cmd = [options.binary,
         '--run-web-tests',
         'chrome://crash',
         '--enable-crash-reporter',
         '--crash-dumps-dir=%s' % crash_dir]
  cmd += additional_arguments
  if options.verbose:
    print ' '.join(cmd)
  failure = 'Failed to run content_shell.'
  if options.verbose:
    subprocess.check_call(cmd)
  else:
    # On Windows, using os.devnull can cause check_call to never return,
    # so use a temporary file for the output.
    with tempfile.TemporaryFile() as tmpfile:
      subprocess.check_call(cmd, stdout=tmpfile, stderr=tmpfile)

  print "# Retrieve crash dump."
  dmp_dir = crash_dir
  # TODO(crbug.com/782923): This test should not reach directly into the
  # Crashpad database, but instead should use crashpad_database_util.
  if sys.platform == 'darwin':
    dmp_dir = os.path.join(dmp_dir, 'pending')
  elif sys.platform == 'win32':
    dmp_dir = os.path.join(dmp_dir, 'reports')
  dmp_files = glob.glob(os.path.join(dmp_dir, '*.dmp'))
  failure = 'Expected 1 crash dump, found %d.' % len(dmp_files)
  if len(dmp_files) != 1:
    raise Exception(failure)
  dmp_file = dmp_files[0]

  if sys.platform not in ('darwin', 'win32'):
    minidump = os.path.join(crash_dir, 'minidump')
    dmp_to_minidump = os.path.join(breakpad_tools_dir, 'dmp2minidump.py')
    cmd = [dmp_to_minidump, dmp_file, minidump]
    if options.verbose:
      print ' '.join(cmd)
    failure = 'Failed to run dmp_to_minidump.'
    subprocess.check_call(cmd)
  else:
    minidump = dmp_file

  print "# Symbolize crash dump."
  if sys.platform == 'win32':
    cdb_exe = os.path.join(options.build_dir, 'cdb', 'cdb.exe')
    cmd = [cdb_exe, '-y', options.build_dir, '-c', '.lines;.excr;k30;q',
           '-z', dmp_file]
    if options.verbose:
      print ' '.join(cmd)
    failure = 'Failed to run cdb.exe.'
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    stack = proc.communicate()[0]
  else:
    minidump_stackwalk = os.path.join(options.build_dir, 'minidump_stackwalk')
    global symbols_dir
    cmd = [minidump_stackwalk, minidump, symbols_dir]
    if options.verbose:
      print ' '.join(cmd)
    failure = 'Failed to run minidump_stackwalk.'
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    stack = proc.communicate()[0]

  # Check whether the stack contains a CrashIntentionally symbol.
  found_symbol = 'CrashIntentionally' in stack

  os.remove(dmp_file)

  if options.no_symbols:
    if found_symbol:
      if options.verbose:
        print stack
      failure = 'Found unexpected reference to CrashIntentionally in stack'
      raise Exception(failure)
  else:
    if not found_symbol:
      if options.verbose:
        print stack
      failure = 'Could not find reference to CrashIntentionally in stack.'
      raise Exception(failure)

def main():
  global failure
  global breakpad_tools_dir

  parser = optparse.OptionParser()
  parser.add_option('', '--build-dir', default='',
                    help='The build output directory.')
  parser.add_option('', '--binary', default='',
                    help='The path of the binary to generate symbols for.')
  parser.add_option('', '--no-symbols', default=False, action='store_true',
                    help='Symbols are not expected to work.')
  parser.add_option('-j', '--jobs', default=CONCURRENT_TASKS, action='store',
                    type='int', help='Number of parallel tasks to run.')
  parser.add_option('-v', '--verbose', action='store_true',
                    help='Print verbose status output.')
  parser.add_option('', '--json', default='',
                    help='Path to JSON output.')

  (options, _) = parser.parse_args()

  if not options.build_dir:
    print "Required option --build-dir missing."
    return 1

  if not options.binary:
    print "Required option --binary missing."
    return 1

  if not os.access(options.binary, os.X_OK):
    print "Cannot find %s." % options.binary
    return 1

  failure = ''

  # Create a temporary directory to store the crash dumps and symbols in.
  crash_dir = tempfile.mkdtemp()

  crash_service = None

  try:
    if sys.platform != 'win32':
      print "# Generate symbols."
      global symbols_dir
      breakpad_tools_dir = os.path.join(
          os.path.dirname(__file__), '..', '..', '..',
          'components', 'crash', 'content', 'tools')
      generate_symbols = os.path.join(
          breakpad_tools_dir, 'generate_breakpad_symbols.py')
      symbols_dir = os.path.join(crash_dir, 'symbols')
      cmd = [generate_symbols,
             '--build-dir=%s' % options.build_dir,
             '--binary=%s' % options.binary,
             '--symbols-dir=%s' % symbols_dir,
             '--jobs=%d' % options.jobs]
      if options.verbose:
        cmd.append('--verbose')
        print ' '.join(cmd)
      failure = 'Failed to run generate_breakpad_symbols.py.'
      subprocess.check_call(cmd)

    print "# Running test without trap handler."
    run_test(options, crash_dir);
    print "# Running test with trap handler."
    run_test(options, crash_dir,
             additional_arguments =
               ['--enable-features=WebAssemblyTrapHandler']);

  except:
    print "FAIL: %s" % failure
    if options.json:
      with open(options.json, 'w') as json_file:
        json.dump([failure], json_file)

    return 1

  else:
    print "PASS: Breakpad integration test ran successfully."
    if options.json:
      with open(options.json, 'w') as json_file:
        json.dump([], json_file)
    return 0

  finally:
    if crash_service:
      crash_service.terminate()
      crash_service.wait()
    try:
      shutil.rmtree(crash_dir)
    except:
      print 'Failed to delete temp directory "%s".' % crash_dir


if '__main__' == __name__:
  sys.exit(main())
