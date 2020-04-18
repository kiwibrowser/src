#!/usr/bin/env python

# Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

import os.path
import sys
import unittest
import check_3pp

SCRIPT_DIR = os.path.realpath(os.path.dirname(os.path.abspath(__file__)))
CHECKOUT_SRC_DIR = os.path.realpath(os.path.join(SCRIPT_DIR, os.pardir,
                                                 os.pardir))
TEST_DATA_DIR = os.path.join(SCRIPT_DIR, 'testdata',
                             'check_third_party_changes')
sys.path.append(CHECKOUT_SRC_DIR)
from presubmit_test_mocks import MockInputApi, MockOutputApi, MockFile


class CheckThirdPartyChangesTest(unittest.TestCase):

  def setUp(self):
    self._input_api = MockInputApi()
    self._output_api = MockOutputApi()

  def testGetChromiumOwnedAddedDeps(self):
    self._input_api.files = [
      MockFile('THIRD_PARTY_CHROMIUM_DEPS.json',
               new_contents=[
                 """
                 {
                   "dependencies": [
                     "foo",
                     "bar",
                     "buzz",
                     "xyz"
                   ]
                 }
                 """
               ],
               old_contents=[
                 """
                 {
                   "dependencies": [
                     "foo",
                     "buzz"
                   ]
                 }
                 """
               ],
               action='M')
    ]
    added_deps = check_3pp.GetChromiumOwnedAddedDeps(self._input_api)
    self.assertEqual(len(added_deps), 2, added_deps)
    self.assertIn('bar', added_deps)
    self.assertIn('xyz', added_deps)

  def testCheckNoNotOwned3ppDepsFire(self):
    self._input_api.presubmit_local_path = os.path.join(TEST_DATA_DIR,
                                                        'not_owned_dep')
    errors = check_3pp.CheckNoNotOwned3ppDeps(self._input_api, self._output_api,
                                              ['webrtc'], ['chromium'])
    self.assertEqual(len(errors), 1)
    self.assertIn('not_owned', errors[0].message)

  def testCheckNoNotOwned3ppDepsNotFire(self):
    self._input_api.presubmit_local_path = os.path.join(TEST_DATA_DIR,
                                                        'not_owned_dep')
    errors = check_3pp.CheckNoNotOwned3ppDeps(self._input_api, self._output_api,
                                              ['webrtc', 'not_owned'],
                                              ['chromium'])
    self.assertEqual(len(errors), 0, errors)

  def testCheckNoBothOwned3ppDepsFire(self):
    errors = check_3pp.CheckNoBothOwned3ppDeps(self._output_api, ['foo', 'bar'],
                                               ['buzz', 'bar'])
    self.assertEqual(len(errors), 1)
    self.assertIn('bar', errors[0].message)

  def testCheckNoBothOwned3ppDepsNotFire(self):
    errors = check_3pp.CheckNoBothOwned3ppDeps(self._output_api, ['foo', 'bar'],
                                               ['buzz'])
    self.assertEqual(len(errors), 0, errors)

  def testCheckNoChangesInAutoImportedDepsFire(self):
    self._input_api.files = [
      MockFile('third_party/chromium/source.js')
    ]
    errors = check_3pp.CheckNoChangesInAutoImportedDeps(self._input_api,
                                                        self._output_api,
                                                        ['webrtc'],
                                                        ['chromium'], [],
                                                        None)
    self.assertEqual(len(errors), 1)
    self.assertIn('chromium', errors[0].message)

  def testCheckNoChangesInAutoImportedDepsNotFire(self):
    self._input_api.files = [
      MockFile('third_party/webrtc/source.js')
    ]
    errors = check_3pp.CheckNoChangesInAutoImportedDeps(self._input_api,
                                                        self._output_api,
                                                        ['webrtc'],
                                                        ['chromium'], [],
                                                        None)
    self.assertEqual(len(errors), 0, errors)

  def testCheckNoChangesInAutoImportedDepsNotFireOnNewlyAdded(self):
    self._input_api.files = [
      MockFile('third_party/chromium/source.js')
    ]
    errors = check_3pp.CheckNoChangesInAutoImportedDeps(self._input_api,
                                                        self._output_api,
                                                        ['webrtc'],
                                                        ['chromium'],
                                                        ['chromium'], None)
    self.assertEqual(len(errors), 0, errors)

  def testCheckNoChangesInAutoImportedDepsNotFireOnSpecialTag(self):
    self._input_api.files = [
      MockFile('third_party/chromium/source.js')
    ]
    self._input_api.change.tags['NO_AUTOIMPORT_DEPS_CHECK'] = 'True'
    errors = check_3pp.CheckNoChangesInAutoImportedDeps(self._input_api,
                                                        self._output_api,
                                                        ['webrtc'],
                                                        ['chromium'],
                                                        [], None)
    self.assertEqual(len(errors), 0, errors)


if __name__ == '__main__':
  unittest.main()
