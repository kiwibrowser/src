# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import copy
import logging
import sys
import unittest

import mock

from infra_libs.logs import logs

class InfraFilterTest(unittest.TestCase):

  def test_infrafilter_adds_correct_fields(self):
    record = logging.makeLogRecord({})
    infrafilter = logs.InfraFilter('US/Pacific')
    infrafilter.filter(record)
    self.assertTrue(hasattr(record, "severity"))
    self.assertTrue(hasattr(record, "iso8601"))

  def test_infraformatter_adds_full_module_name(self):
    record = logging.makeLogRecord({})
    infrafilter = logs.InfraFilter('US/Pacific')
    infrafilter.filter(record)
    self.assertEqual('infra_libs.logs.test.logs_test', record.fullModuleName)

  def test_filters_by_module_name(self):
    record = logging.makeLogRecord({})
    infrafilter = logs.InfraFilter(
        'US/Pacific', module_name_blacklist=r'^other\.module')
    self.assertTrue(infrafilter.filter(record))

    infrafilter = logs.InfraFilter(
        'US/Pacific',
        module_name_blacklist=r'^infra_libs\.logs\.test\.logs_test$')
    self.assertFalse(infrafilter.filter(record))


class DefaultProgramNameTest(unittest.TestCase):

  def setUp(self):
    self.old_argv = copy.copy(sys.argv)
    self.old_package = sys.modules['__main__'].__package__

  def tearDown(self):
    sys.argv = self.old_argv
    sys.modules['__main__'].__package__ = self.old_package

  def test_from_argv(self):
    sys.argv = ['/foo/bar/program', '--args']
    self.assertEquals('program', logs.default_program_name())

  def test_from_main_package(self):
    sys.argv = ['/foo/bar/__main__.py', '--args']
    sys.modules['__main__'].__package__ = 'foo.bar.program'
    self.assertEquals('program', logs.default_program_name())

  def test_from_argv_when_main_package_is_none(self):
    sys.argv = ['/foo/bar/__main__.py', '--args']
    sys.modules['__main__'].__package__ = None
    self.assertEquals('__main__.py', logs.default_program_name())
