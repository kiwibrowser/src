#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Verify perf_expectations.json can be loaded using simplejson.

perf_expectations.json is a JSON-formatted file.  This script verifies
that simplejson can load it correctly.  It should catch most common
formatting problems.
"""

import subprocess
import sys
import os
import unittest
import re

simplejson = None

def OnTestsLoad():
  old_path = sys.path
  script_path = os.path.dirname(sys.argv[0])
  load_path = None
  global simplejson

  # This test script should be stored in src/tools/perf_expectations/.  That
  # directory will most commonly live in 2 locations:
  #
  #   - a regular Chromium checkout, in which case src/third_party
  #     is where to look for simplejson
  #
  #   - a buildbot checkout, in which case .../pylibs is where
  #     to look for simplejson
  #
  # Locate and install the correct path based on what we can find.
  #
  for path in ('../../../third_party', '../../../../../pylibs'):
    path = os.path.join(script_path, path)
    if os.path.exists(path) and os.path.isdir(path):
      load_path = os.path.abspath(path)
      break

  if load_path is None:
    msg = "%s expects to live within a Chromium checkout" % sys.argv[0]
    raise Exception, "Error locating simplejson load path (%s)" % msg

  # Try importing simplejson once.  If this succeeds, we found it and will
  # load it again later properly.  Fail if we cannot load it.
  sys.path.append(load_path)
  try:
    import simplejson as Simplejson
    simplejson = Simplejson
  except ImportError, e:
    msg = "%s expects to live within a Chromium checkout" % sys.argv[0]
    raise Exception, "Error trying to import simplejson from %s (%s)" % \
                     (load_path, msg)
  finally:
    sys.path = old_path
  return True

def LoadJsonFile(filename):
  f = open(filename, 'r')
  try:
    data = simplejson.load(f)
  except ValueError, e:
    f.seek(0)
    print "Error reading %s:\n%s" % (filename,
                                     f.read()[:50]+'...')
    raise e
  f.close()
  return data

OnTestsLoad()

CONFIG_JSON = os.path.join(os.path.dirname(sys.argv[0]),
                           '../chromium_perf_expectations.cfg')
MAKE_EXPECTATIONS = os.path.join(os.path.dirname(sys.argv[0]),
                                 '../make_expectations.py')
PERF_EXPECTATIONS = os.path.join(os.path.dirname(sys.argv[0]),
                                 '../perf_expectations.json')


class PerfExpectationsUnittest(unittest.TestCase):
  def testPerfExpectations(self):
    # Test data is dictionary.
    perf_data = LoadJsonFile(PERF_EXPECTATIONS)
    if not isinstance(perf_data, dict):
      raise Exception('perf expectations is not a dict')

    # Test the 'load' key.
    if not 'load' in perf_data:
      raise Exception("perf expectations is missing a load key")
    if not isinstance(perf_data['load'], bool):
      raise Exception("perf expectations load key has non-bool value")

    # Test all key values are dictionaries.
    bad_keys = []
    for key in perf_data:
      if key == 'load':
        continue
      if not isinstance(perf_data[key], dict):
        bad_keys.append(key)
    if len(bad_keys) > 0:
      msg = "perf expectations keys have non-dict values"
      raise Exception("%s: %s" % (msg, bad_keys))

    # Test all key values have delta and var keys.
    for key in perf_data:
      if key == 'load':
        continue

      # First check if regress/improve is in the key's data.
      if 'regress' in perf_data[key]:
        if 'improve' not in perf_data[key]:
          bad_keys.append(key)
        if (not isinstance(perf_data[key]['regress'], int) and
            not isinstance(perf_data[key]['regress'], float)):
          bad_keys.append(key)
        if (not isinstance(perf_data[key]['improve'], int) and
            not isinstance(perf_data[key]['improve'], float)):
          bad_keys.append(key)
      else:
        # Otherwise check if delta/var is in the key's data.
        if 'delta' not in perf_data[key] or 'var' not in perf_data[key]:
          bad_keys.append(key)
        if (not isinstance(perf_data[key]['delta'], int) and
            not isinstance(perf_data[key]['delta'], float)):
          bad_keys.append(key)
        if (not isinstance(perf_data[key]['var'], int) and
            not isinstance(perf_data[key]['var'], float)):
          bad_keys.append(key)

    if len(bad_keys) > 0:
      msg = "perf expectations key values missing or invalid delta/var"
      raise Exception("%s: %s" % (msg, bad_keys))

    # Test all keys have the correct format.
    for key in perf_data:
      if key == 'load':
        continue
      # tools/buildbot/scripts/master/log_parser.py should have a matching
      # regular expression.
      if not re.match(r"^([\w\.-]+)/([\w\.-]+)/([\w\.-]+)/([\w\.-]+)$", key):
        bad_keys.append(key)
    if len(bad_keys) > 0:
      msg = "perf expectations keys in bad format, expected a/b/c/d"
      raise Exception("%s: %s" % (msg, bad_keys))

  def testNoUpdatesNeeded(self):
    p = subprocess.Popen([MAKE_EXPECTATIONS, '-s'], stdout=subprocess.PIPE)
    p.wait();
    self.assertEqual(p.returncode, 0,
        msg='Update expectations first by running ./make_expectations.py')

  def testConfigFile(self):
    # Test that the config file can be parsed as JSON.
    config = LoadJsonFile(CONFIG_JSON)
    # Require the following keys.
    if 'base_url' not in config:
      raise Exception('base_url not specified in config file')
    if 'perf_file' not in config:
      raise Exception('perf_file not specified in config file')


if __name__ == '__main__':
  unittest.main()
