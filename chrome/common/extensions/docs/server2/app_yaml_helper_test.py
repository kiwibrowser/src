#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from app_yaml_helper import AppYamlHelper
from extensions_paths import SERVER2
from host_file_system_provider import HostFileSystemProvider
from mock_file_system import MockFileSystem
from object_store_creator import ObjectStoreCreator
from test_file_system import MoveTo, TestFileSystem
from test_util import DisableLogging

_ExtractVersion, _IsGreater, _GenerateAppYaml = (
    AppYamlHelper.ExtractVersion,
    AppYamlHelper.IsGreater,
    AppYamlHelper.GenerateAppYaml)

class AppYamlHelperTest(unittest.TestCase):
  def testExtractVersion(self):
    def run_test(version):
      self.assertEqual(version, _ExtractVersion(_GenerateAppYaml(version)))
    run_test('0')
    run_test('0-0')
    run_test('0-0-0')
    run_test('1')
    run_test('1-0')
    run_test('1-0-0')
    run_test('1-0-1')
    run_test('1-1-0')
    run_test('1-1-1')
    run_test('2-0-9')
    run_test('2-0-12')
    run_test('2-1')
    run_test('2-1-0')
    run_test('2-11-0')
    run_test('3-1-0')
    run_test('3-1-3')
    run_test('3-12-0')

  def testIsGreater(self):
    def assert_is_greater(lhs, rhs):
      self.assertTrue(_IsGreater(lhs, rhs), '%s is not > %s' % (lhs, rhs))
      self.assertFalse(_IsGreater(rhs, lhs),
                       '%s should not be > %s' % (rhs, lhs))
    assert_is_greater('0-0', '0')
    assert_is_greater('0-0-0', '0')
    assert_is_greater('0-0-0', '0-0')
    assert_is_greater('1', '0')
    assert_is_greater('1', '0-0')
    assert_is_greater('1', '0-0-0')
    assert_is_greater('1-0', '0-0')
    assert_is_greater('1-0-0-0', '0-0-0')
    assert_is_greater('2-0-12', '2-0-9')
    assert_is_greater('2-0-12', '2-0-9-0')
    assert_is_greater('2-0-12-0', '2-0-9')
    assert_is_greater('2-0-12-0', '2-0-9-0')
    assert_is_greater('2-1', '2-0-9')
    assert_is_greater('2-1', '2-0-12')
    assert_is_greater('2-1-0', '2-0-9')
    assert_is_greater('2-1-0', '2-0-12')
    assert_is_greater('3-1-0', '2-1')
    assert_is_greater('3-1-0', '2-1-0')
    assert_is_greater('3-1-0', '2-11-0')
    assert_is_greater('3-1-3', '3-1-0')
    assert_is_greater('3-12-0', '3-1-0')
    assert_is_greater('3-12-0', '3-1-3')
    assert_is_greater('3-12-0', '3-1-3-0')

  @DisableLogging('warning')
  def testInstanceMethods(self):
    test_data = {
      'app.yaml': _GenerateAppYaml('1-0'),
      'app_yaml_helper.py': 'Copyright notice etc'
    }

    updates = []
    # Pass a specific file system at head to the HostFileSystemProvider so that
    # we know it's always going to be backed by a MockFileSystem. The Provider
    # may decide to wrap it in caching etc.
    file_system_at_head = MockFileSystem(
        TestFileSystem(test_data, relative_to=SERVER2))

    def apply_update(update):
      update = MoveTo(SERVER2, update)
      file_system_at_head.Update(update)
      updates.append(update)

    def host_file_system_constructor(branch, commit=None):
      self.assertEqual('master', branch)
      self.assertTrue(commit is not None)
      return MockFileSystem.Create(
          TestFileSystem(test_data, relative_to=SERVER2), updates[:int(commit)])

    object_store_creator = ObjectStoreCreator.ForTest()
    host_file_system_provider = HostFileSystemProvider(
        object_store_creator,
        default_master_instance=file_system_at_head,
        constructor_for_test=host_file_system_constructor)
    helper = AppYamlHelper(object_store_creator, host_file_system_provider)

    def assert_is_up_to_date(version):
      self.assertTrue(helper.IsUpToDate(version),
                      '%s is not up to date' % version)
      self.assertRaises(ValueError,
                        helper.GetFirstRevisionGreaterThan, version)

    self.assertEqual(0, helper.GetFirstRevisionGreaterThan('0-5-0'))
    assert_is_up_to_date('1-0-0')
    assert_is_up_to_date('1-5-0')

    # Revision 1.
    apply_update({
      'app.yaml': _GenerateAppYaml('1-5-0')
    })

    self.assertEqual(0, helper.GetFirstRevisionGreaterThan('0-5-0'))
    self.assertEqual(1, helper.GetFirstRevisionGreaterThan('1-0-0'))
    assert_is_up_to_date('1-5-0')
    assert_is_up_to_date('2-5-0')

    # Revision 2.
    apply_update({
      'app_yaml_helper.py': 'fixed a bug'
    })

    self.assertEqual(0, helper.GetFirstRevisionGreaterThan('0-5-0'))
    self.assertEqual(1, helper.GetFirstRevisionGreaterThan('1-0-0'))
    assert_is_up_to_date('1-5-0')
    assert_is_up_to_date('2-5-0')

    # Revision 3.
    apply_update({
      'app.yaml': _GenerateAppYaml('1-6-0')
    })

    self.assertEqual(0, helper.GetFirstRevisionGreaterThan('0-5-0'))
    self.assertEqual(1, helper.GetFirstRevisionGreaterThan('1-0-0'))
    self.assertEqual(3, helper.GetFirstRevisionGreaterThan('1-5-0'))
    assert_is_up_to_date('2-5-0')

    # Revision 4.
    apply_update({
      'app.yaml': _GenerateAppYaml('1-8-0')
    })
    # Revision 5.
    apply_update({
      'app.yaml': _GenerateAppYaml('2-0-0')
    })
    # Revision 6.
    apply_update({
      'app.yaml': _GenerateAppYaml('2-2-0')
    })
    # Revision 7.
    apply_update({
      'app.yaml': _GenerateAppYaml('2-4-0')
    })
    # Revision 8.
    apply_update({
      'app.yaml': _GenerateAppYaml('2-6-0')
    })

    self.assertEqual(0, helper.GetFirstRevisionGreaterThan('0-5-0'))
    self.assertEqual(1, helper.GetFirstRevisionGreaterThan('1-0-0'))
    self.assertEqual(3, helper.GetFirstRevisionGreaterThan('1-5-0'))
    self.assertEqual(5, helper.GetFirstRevisionGreaterThan('1-8-0'))
    self.assertEqual(6, helper.GetFirstRevisionGreaterThan('2-0-0'))
    self.assertEqual(6, helper.GetFirstRevisionGreaterThan('2-1-0'))
    self.assertEqual(7, helper.GetFirstRevisionGreaterThan('2-2-0'))
    self.assertEqual(7, helper.GetFirstRevisionGreaterThan('2-3-0'))
    self.assertEqual(8, helper.GetFirstRevisionGreaterThan('2-4-0'))
    self.assertEqual(8, helper.GetFirstRevisionGreaterThan('2-5-0'))
    assert_is_up_to_date('2-6-0')
    assert_is_up_to_date('2-7-0')

if __name__ == '__main__':
  unittest.main()
