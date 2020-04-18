#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Parse the report output of the llvm test suite or regression tests,
   filter out known failures, and check for new failures

pnacl/scripts/parse_llvm_test_report.py [options]+ reportfile

"""

import csv
import logging
import optparse
import os
import sys
import StringIO

# exclude these tests
EXCLUDES = {}

def ParseCommandLine(argv):
  parser = optparse.OptionParser(prog=argv[0])
  parser.add_option('-x', '--exclude', action='append', dest='excludes',
                    default=[],
                    help='Add list of excluded tests (expected fails)')
  parser.add_option('-c', '--check-excludes', action='store_true',
                    default=False, dest='check_excludes',
                    help='Report tests which unexpectedly pass')
  parser.add_option('-v', '--verbose', action='store_true',
                    default=False, dest='verbose',
                    help='Print compilation/run logs of failing tests')
  parser.add_option('-p', '--build-path', dest='buildpath',
                    help='Path to test-suite build directory')
  parser.add_option('-a', '--attribute', dest='attributes', action='append',
                    default=[],
                    help='Add attribute of test configuration (e.g. arch)')
  parser.add_option('-t', '--testsuite', action='store_true', dest='testsuite',
                    default=False)
  parser.add_option('-l', '--lit', action='store_true', dest='lit',
                    default=False)

  options, args = parser.parse_args(argv[1:])
  return options, args

def Fatal(text):
  print >> sys.stderr, text
  sys.exit(1)

def IsFullname(name):
  return name.find('/') != -1

def GetShortname(fullname):
  return fullname.split('/')[-1]

def ParseTestsuiteCSV(filecontents):
  ''' Parse a CSV file output by llvm testsuite with a record for each test.
      returns 2 dictionaries:
      1) a mapping from the short name of the test (without the path) to
       a list of full pathnames that match it. It contains all the tests.
      2) a mapping of all test failures, mapping full test path to the type
       of failure (compile or exec)
  '''
  alltests = {}
  failures = {}
  reader = csv.DictReader(StringIO.StringIO(filecontents))

  testcount = 0
  for row in reader:
    testcount += 1
    fullname = row['Program']
    shortname = GetShortname(fullname)
    fullnames = alltests.get(shortname, [])
    fullnames.append(fullname)
    alltests[shortname] = fullnames

    if row['CC'] == '*':
      failures[fullname] = 'compile'
    elif row['Exec'] == '*':
      failures[fullname] = 'exec'

  logging.info('%d tests, %d failures', testcount, len(failures))
  return alltests, failures

def ParseLit(filecontents):
  ''' Parse the output of the LLVM regression test runner (lit/make check).
      returns a dictionary mapping test name to the type of failure
      (Clang, LLVM, LLVMUnit, etc)
  '''
  alltests = {}
  failures = {}
  testcount = 0
  for line in filecontents.splitlines():
    l = line.split()
    if len(l) < 4:
      continue
    if l[0] in ('PASS:', 'FAIL:', 'XFAIL:', 'XPASS:', 'UNSUPPORTED:'):
      testcount += 1
      fullname = ''.join(l[1:4])
      shortname = GetShortname(fullname)
      fullnames = alltests.get(shortname, [])
      fullnames.append(fullname)
      alltests[shortname] = fullnames
    if l[0] in ('FAIL:', 'XPASS:'):
      failures[fullname] = l[1]
  logging.info('%d tests, %d failures', testcount, len(failures))
  return alltests, failures

def ParseExcludeFile(filename, config_attributes,
                     check_test_names=False, alltests=None):
  ''' Parse a list of excludes (known test failures). Excludes can be specified
      by shortname (e.g. fbench) or by full path
      (e.g. SingleSource/Benchmarks/Misc/fbench) but if there is more than
      one test with the same shortname, the full name must be given.
      Errors are reported if an exclude does not match exactly one test
      in alltests, or if there are duplicate excludes.

      Returns:
        Number of failures in the exclusion file.
  '''
  errors = 0
  f = open(filename)
  for line in f:
    line = line.strip()
    if not line: continue
    if line.startswith('#'): continue
    tokens = line.split()
    if len(tokens) > 1:
      testname = tokens[0]
      attributes = set(tokens[1].split(','))
      if not attributes.issubset(config_attributes):
        continue
    else:
      testname = line
    if testname in EXCLUDES:
      logging.error('Duplicate exclude: %s', line)
      errors += 1
    if IsFullname(testname):
      shortname = GetShortname(testname)
      if shortname not in alltests or testname not in alltests[shortname]:
        logging.error('Exclude %s not found in list of tests', line)
        errors += 1
      fullname = testname
    else:
      # short name is specified
      shortname = testname
      if shortname not in alltests:
        logging.error('Exclude %s not found in list of tests', shortname)
        errors += 1
      if len(alltests[shortname]) > 1:
        logging.error('Exclude %s matches more than one test: %s. ' +
                      'Specify full name in exclude file.',
                      shortname, str(alltests[shortname]))
        errors += 1
      fullname = alltests[shortname][0]

    if fullname in EXCLUDES:
      logging.error('Duplicate exclude %s', fullname)
      errors += 1

    EXCLUDES[fullname] = filename
  f.close()
  logging.info('Parsed %s: now %d total excludes', filename, len(EXCLUDES))
  return errors

def DumpFileContents(name):
  error = not os.path.exists(name)
  logging.debug(name)
  try:
    logging.debug(open(name, 'rb').read())
  except IOError:
    error = True
  if error:
    logging.error("Couldn't open file: %s", name)
    # Make the bots go red
    logging.error('@@@STEP_FAILURE@@@')

def PrintTestsuiteCompilationResult(path, test):
  ''' Print the compilation and run results for the specified test in the
      LLVM testsuite.
      These results are left in several different log files by the testsuite
      driver, and are different for MultiSource/SingleSource tests
  '''
  logging.debug('RESULTS for %s', test)
  testpath = os.path.join(path, test)
  testdir, testname = os.path.split(testpath)
  outputdir = os.path.join(testdir, 'Output')

  logging.debug('COMPILE phase')
  logging.debug('OBJECT file phase')
  if test.startswith('MultiSource'):
    for f in os.listdir(outputdir):
      if f.endswith('llvm.o.compile'):
        DumpFileContents(os.path.join(outputdir, f))
  elif test.startswith('SingleSource'):
    DumpFileContents(os.path.join(outputdir, testname + '.llvm.o.compile'))
  else:
    Fatal('ERROR: unrecognized test type ' + test)

  logging.debug('PEXE generation phase')
  DumpFileContents(os.path.join(outputdir,
                                testname + '.nonfinal.pexe.compile'))

  logging.debug('PEXE finalization phase')
  DumpFileContents(os.path.join(outputdir, testname + '.final.pexe.finalize'))

  logging.debug('TRANSLATION phase')
  DumpFileContents(os.path.join(outputdir, testname + '.nexe.translate'))

  logging.debug('EXECUTION phase')
  logging.debug('native output:')
  DumpFileContents(os.path.join(outputdir, testname + '.out-nat'))
  logging.debug('pnacl output:')
  DumpFileContents(os.path.join(outputdir, testname + '.out-pnacl'))

def main(argv):
  options, args = ParseCommandLine(argv)

  if len(args) != 1:
    Fatal('Must specify filename to parse')
  filename = args[0]
  return Report(vars(options), filename=filename)


def Report(options, filename=None, filecontents=None):
  loglevel = logging.INFO
  if options['verbose']:
    loglevel = logging.DEBUG
  logging.basicConfig(level=loglevel, format='%(message)s')

  if not (filename or filecontents):
    Fatal('ERROR: must specify filename or filecontents')

  failures = {}
  logging.debug('Full test results:')

  if not filecontents:
    with open(filename, 'rb') as f:
      filecontents = f.read();
  # get the set of tests and failures
  if options['testsuite']:
    if options['verbose'] and options['buildpath'] is None:
      Fatal('ERROR: must specify build path if verbose output is desired')
    alltests, failures = ParseTestsuiteCSV(filecontents)
    check_test_names = True
  elif options['lit']:
    alltests, failures = ParseLit(filecontents)
    check_test_names = True
  else:
    Fatal('Must specify either testsuite (-t) or lit (-l) output format')

  # get the set of excludes
  exclusion_failures = 0
  for f in options['excludes']:
    exclusion_failures += ParseExcludeFile(f, set(options['attributes']),
                                           check_test_names=check_test_names,
                                           alltests=alltests)

  # Regardless of the verbose option, do a dry run of
  # PrintTestsuiteCompilationResult so we can catch errors when intermediate
  # filenames in the compilation pipeline change.
  # E.g. https://code.google.com/p/nativeclient/issues/detail?id=3659
  if len(alltests) and options['testsuite']:
    logging.disable(logging.INFO)
    PrintTestsuiteCompilationResult(options['buildpath'],
                                    alltests.values()[0][0])
    logging.disable(logging.NOTSET)

  # intersect them and check for unexpected fails/passes
  unexpected_failures = 0
  unexpected_passes = 0
  for tests in alltests.itervalues():
    for test in tests:
      if test in failures:
        if test not in EXCLUDES:
          unexpected_failures += 1
          logging.info('[  FAILED  ] %s: %s failure', test, failures[test])
          if options['testsuite']:
            PrintTestsuiteCompilationResult(options['buildpath'], test)
      elif test in EXCLUDES:
        unexpected_passes += 1
        logging.info('%s: unexpected success', test)

  logging.info('%d unexpected failures %d unexpected passes',
               unexpected_failures, unexpected_passes)
  if exclusion_failures:
    logging.info('%d problems in known_failures exclusion files',
                 exclusion_failures)

  if options['check_excludes']:
    return unexpected_failures + unexpected_passes + exclusion_failures > 0
  return unexpected_failures + exclusion_failures > 0

if __name__ == '__main__':
  sys.exit(main(sys.argv))
