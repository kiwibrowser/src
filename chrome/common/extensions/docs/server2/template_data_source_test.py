#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import unittest

from extensions_paths import SERVER2
from server_instance import ServerInstance
from template_data_source import TemplateDataSource
from test_util import DisableLogging, ReadFile
from third_party.motemplate import Motemplate

def _ReadFile(*path):
  return ReadFile(SERVER2, 'test_data', 'template_data_source', *path)

def _CreateTestDataSource(base_dir):
  '''TemplateDataSource is not instantiated directly, rather, its methods
  are invoked through a subclass of it, which has as its only data the
  directory in which TemplateDataSource methods should act on. Thus, we test
  TemplateDataSource indirectly through the TestDataSource class
  '''
  return TestDataSource(ServerInstance.ForLocal(),
                        '%stest_data/template_data_source/%s/' %
                        (SERVER2, base_dir))


class TestDataSource(TemplateDataSource):
  '''Provides a subclass we can use to test the TemplateDataSource methods
  '''
  def __init__(self, server_instance, base_dir):
    type(self)._BASE = base_dir
    TemplateDataSource.__init__(self, server_instance)


class TemplateDataSourceTest(unittest.TestCase):

  def testSimple(self):
    test_data_source = _CreateTestDataSource('simple')
    template_a1 = Motemplate(_ReadFile('simple', 'test1.html'))
    context = [{}, {'templates': {}}]
    self.assertEqual(
        template_a1.Render(*context).text,
        test_data_source.get('test1').Render(*context).text)
    template_a2 = Motemplate(_ReadFile('simple', 'test2.html'))
    self.assertEqual(
        template_a2.Render(*context).text,
        test_data_source.get('test2').Render(*context).text)

  @DisableLogging('warning')
  def testNotFound(self):
    test_data_source = _CreateTestDataSource('simple')
    self.assertEqual(None, test_data_source.get('junk'))

  @DisableLogging('warning')
  def testPartials(self):
    test_data_source = _CreateTestDataSource('partials')
    context = json.loads(_ReadFile('partials', 'input.json'))
    self.assertEqual(
        _ReadFile('partials', 'test_expected.html'),
        test_data_source.get('test_tmpl').Render(
            context, test_data_source).text)


if __name__ == '__main__':
  unittest.main()
