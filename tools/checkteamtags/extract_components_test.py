# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from collections import OrderedDict
import json
import os
import sys
import unittest

from StringIO import StringIO

import extract_components
from owners_file_tags_test import mock_file_tree

SRC = os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir)
sys.path.append(os.path.join(SRC, 'third_party', 'pymock'))

import mock


class ExtractComponentsTest(unittest.TestCase):
  def setUp(self):
    super(ExtractComponentsTest, self).setUp()
    self.maxDiff = None

  def testBaseCase(self):
    with mock.patch('extract_components.scrape_owners', return_value={
        '.': {},
        'dummydir1': {
            'team': 'dummy-team@chromium.org',
            'component': 'Dummy>Component',
        },
        'dummydir2': {
            'team': 'other-dummy-team@chromium.org',
            'component': 'Components>Component2',
        },
        'dummydir1/innerdir1': {
            'team': 'dummy-specialist-team@chromium.org',
            'component': 'Dummy>Component>Subcomponent'
        }
    }):
      saved_output = StringIO()
      with mock.patch('sys.stdout', saved_output):
        error_code = extract_components.main(['%prog', 'src'])
      self.assertEqual(0, error_code)
      result_minus_readme = json.loads(saved_output.getvalue())
      del result_minus_readme['AAA-README']
      self.assertEqual(result_minus_readme, {
          'component-to-team': {
              'Components>Component2': 'other-dummy-team@chromium.org',
              'Dummy>Component': 'dummy-team@chromium.org',
              'Dummy>Component>Subcomponent':
                  'dummy-specialist-team@chromium.org'
          },
          'dir-to-component': {
              'dummydir1': 'Dummy>Component',
              'dummydir1/innerdir1':
                  'Dummy>Component>Subcomponent',
              'dummydir2': 'Components>Component2'
          }})

  def testOsTagBreaksDuplication(self):
    with mock.patch('extract_components.scrape_owners', return_value={
        '.': {},
        'dummydir1': {
            'team': 'dummy-team@chromium.org',
            'component': 'Dummy>Component',
        },
        'dummydir2': {
            'team': 'mac-dummy-team@chromium.org',
            'component': 'Dummy>Component',
            'os': 'Mac'
        },
        'dummydir1/innerdir1': {
            'team': 'dummy-specialist-team@chromium.org',
            'component': 'Dummy>Component>Subcomponent'
        }
    }):
      saved_output = StringIO()
      with mock.patch('sys.stdout', saved_output):
        error_code = extract_components.main(['%prog', 'src'])
      self.assertEqual(0, error_code)
      result_minus_readme = json.loads(saved_output.getvalue())
      del result_minus_readme['AAA-README']
      self.assertEqual(result_minus_readme, {
          'component-to-team': {
              'Dummy>Component': 'dummy-team@chromium.org',
              'Dummy>Component(Mac)': 'mac-dummy-team@chromium.org',
              'Dummy>Component>Subcomponent':
                  'dummy-specialist-team@chromium.org'
          },
          'dir-to-component': {
              'dummydir1': 'Dummy>Component',
              'dummydir1/innerdir1': 'Dummy>Component>Subcomponent',
              'dummydir2': 'Dummy>Component(Mac)'
          }})

  def testMultipleTeamsOneComponent(self):
    with mock.patch('extract_components.scrape_owners', return_value={
        '.': {},
        'dummydir1': {
            'team': 'dummy-team@chromium.org',
            'component': 'Dummy>Component',
        },
        'dummydir2': {
            'team': 'other-dummy-team@chromium.org',
            'component': 'Dummy>Component',
        },
        'dummydir1/innerdir1': {
            'team': 'dummy-specialist-team@chromium.org',
            'component': 'Dummy>Component>Subcomponent'
        }
    }):
      saved_output = StringIO()
      with mock.patch('sys.stdout', saved_output):
        error_code = extract_components.main(['%prog', '-w', 'src'])
      self.assertNotEqual(0, error_code)
      output = saved_output.getvalue()
      self.assertIn('has more than one team assigned to it', output)
      self.assertIn('Not writing to file', output)

  def testVerbose(self):
    with mock.patch('extract_components.scrape_owners', return_value={
        '.': {},
        'dummydir1': {
            'team': 'dummy-team@chromium.org',
            'component': 'Dummy>Component',
        },
        'dummydir2': {
            'team': 'other-dummy-team@chromium.org',
            'component': 'Dummy>Component',
        },
        'dummydir1/innerdir1': {
            'team': 'dummy-specialist-team@chromium.org',
            'component': 'Dummy>Component>Subcomponent'
        }
    }):
      saved_output = StringIO()
      with mock.patch('sys.stdout', saved_output):
        extract_components.main(['%prog', '-v', 'src'])
      output = saved_output.getvalue()
      self.assertIn('./OWNERS has no COMPONENT tag', output)

  def testCoverage(self):
    with mock.patch('extract_components.scrape_owners', return_value={
        '.': {},
        'dummydir1': {
            'team': 'dummy-team@chromium.org',
            'component': 'Dummy>Component',
        },
        'dummydir2': {
            'component': 'Dummy>Component',
        },
        'dummydir1/innerdir1': {
            'team': 'dummy-specialist-team@chromium.org',
            'component': 'Dummy>Component>Subcomponent'
        }
    }):
      saved_output = StringIO()
      with mock.patch('sys.stdout', saved_output):
        extract_components.main(['%prog', '-s 2', 'src'])
      output = saved_output.getvalue()
      self.assertIn('4 OWNERS files in total.', output)
      self.assertIn('3 (75.00%) OWNERS files have COMPONENT', output)
      self.assertIn('2 (50.00%) OWNERS files have TEAM and COMPONENT', output)

  def testCompleteCoverage(self):
    with mock.patch('extract_components.scrape_owners', return_value={
        '.': {},
        'dummydir1': {
            'team': 'dummy-team@chromium.org',
            'component': 'Dummy>Component',
        },
        'dummydir2': {
            'component': 'Dummy>Component',
        },
        'dummydir1/innerdir1': {
            'team': 'dummy-specialist-team@chromium.org',
            'component': 'Dummy>Component>Subcomponent'
        }
    }):
      saved_output = StringIO()
      with mock.patch('sys.stdout', saved_output):
        extract_components.main(['%prog', '-c', ''])
      output = saved_output.getvalue()
      self.assertIn('4 OWNERS files in total.', output)
      self.assertIn('3 (75.00%) OWNERS files have COMPONENT', output)
      self.assertIn('2 (50.00%) OWNERS files have TEAM and COMPONENT', output)
      self.assertIn('1 OWNERS files at depth 0', output)
      self.assertIn('2 OWNERS files at depth 1', output)
      self.assertIn('1 OWNERS files at depth 2', output)

  def testIncludesSubdirectoriesWithNoOwnersFileOrNoComponentTag(self):
    # We use OrderedDict here to guarantee that mocked version of os.walk
    # returns directories in the specified order (top-down).
    with mock_file_tree(OrderedDict([
        ('chromium/src', 'boss@chromium.org\n'),
        ('chromium/src/dir1', 'dummy@chromium.org\n'
                              '# TEAM: dummy-team@chromium.org\n'
                              '# COMPONENT: Dummy>Component'),
        ('chromium/src/dir2', 'dummy2@chromium.org\n'
                              '# TEAM: other-dummy-team@chromium.org\n'
                              '# COMPONENT: Dummy>Component2'),
        ('chromium/src/dir1/subdir', 'dummy@chromium.org'),
        ('chromium/src/dir2/subdir', None),
        ('third_party/WebKit/LayoutTests/foo',
             '# TEAM: dummy-team-3@chromium.org\n'),
        ('third_party/WebKit/LayoutTests/bar',
             '# TEAM: dummy-team-3@chromium.org\n'
             '# COMPONENT: Dummy>Component3\n'),
    ])):
      # TODO(robertocn): remove this testcase and the auxiliary function
      # mock_file_tree (that has been moved to owners_file_tags_test) when
      # sergiyb's scripts are using the data directly from scrape_owners.
      self.maxDiff = None  # This helps to see assertDictEqual errors in full.
      saved_output = StringIO()
      with mock.patch('sys.stdout', saved_output):
        error_code = extract_components.main(['%prog', '--include-subdirs', ''])
      self.assertEqual(0, error_code)
      result_minus_readme = json.loads(saved_output.getvalue())
      del result_minus_readme['AAA-README']
      self.assertDictEqual(result_minus_readme, {
          u'component-to-team': {
              u'Dummy>Component': u'dummy-team@chromium.org',
              u'Dummy>Component2': u'other-dummy-team@chromium.org',
              u'Dummy>Component3': u'dummy-team-3@chromium.org',
          },
          u'dir-to-component': {
              u'chromium/src/dir1': u'Dummy>Component',
              u'chromium/src/dir1/subdir': u'Dummy>Component',
              u'chromium/src/dir2': u'Dummy>Component2',
              u'chromium/src/dir2/subdir': u'Dummy>Component2',
              u'third_party/WebKit/LayoutTests/bar': u'Dummy>Component3',
          },
          u'dir-to-team': {
              u'third_party/WebKit/LayoutTests/foo':
                  u'dummy-team-3@chromium.org',
          }})

  def testDisplayFile(self):
    with mock.patch('extract_components.scrape_owners', return_value={
        '.': {},
        'dummydir1': {
            'team': 'dummy-team@chromium.org',
            'component': 'Dummy>Component',
        },
        'dummydir1/innerdir1': {
            'team': 'dummy-specialist-team@chromium.org',
            'component': 'Dummy>Component>Subcomponent'
        }
    }):
      saved_output = StringIO()
      with mock.patch('sys.stdout', saved_output):
        extract_components.main(['%prog', '-m 2', 'src'])
      output = saved_output.getvalue()
      self.assertIn('OWNERS files that have missing team and component '
                    'by depth:', output)
      self.assertIn('at depth 0', output)
      self.assertIn('[\'./OWNERS\']', output)
