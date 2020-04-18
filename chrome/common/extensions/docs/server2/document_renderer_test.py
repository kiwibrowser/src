#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from document_renderer import DocumentRenderer
from server_instance import ServerInstance
from test_file_system import TestFileSystem
from test_data.canned_data import CANNED_TEST_FILE_SYSTEM_DATA


class DocumentRendererUnittest(unittest.TestCase):
  def setUp(self):
    self._renderer = ServerInstance.ForTest(
        TestFileSystem(CANNED_TEST_FILE_SYSTEM_DATA)).document_renderer
    self._path = 'apps/some/path/to/document.html'

  def _Render(self, document, render_title=False):
    return self._renderer.Render(document,
                                 self._path,
                                 render_title=render_title)

  def testNothingToSubstitute(self):
    document = 'hello world'

    text, warnings = self._Render(document)
    self.assertEqual(document, text)
    self.assertEqual([], warnings)

    text, warnings = self._Render(document, render_title=True)
    self.assertEqual(document, text)
    self.assertEqual(['Expected a title'], warnings)

  def testTitles(self):
    document = '<h1>title</h1> then $(title) then another $(title)'

    text, warnings = self._Render(document)
    self.assertEqual(document, text)
    self.assertEqual(['Found unexpected title "title"'], warnings)

    text, warnings = self._Render(document, render_title=True)
    self.assertEqual('<h1>title</h1> then title then another $(title)', text)
    self.assertEqual([], warnings)

  def testTocs(self):
    document = ('here is a toc $(table_of_contents) '
                'and another $(table_of_contents)')
    expected_document = ('here is a toc <table-of-contents> and another '
                         '$(table_of_contents)')

    text, warnings = self._Render(document)
    self.assertEqual(expected_document, text)
    self.assertEqual([], warnings)

    text, warnings = self._Render(document, render_title=True)
    self.assertEqual(expected_document, text)
    self.assertEqual(['Expected a title'], warnings)

  def testRefs(self):
    # The references in this and subsequent tests won't actually be resolved
    document = 'A ref $(ref:baz.baz_e1) here, $(ref:foo.foo_t3 ref title) there'
    expected_document = ''.join([
      'A ref <a href=/apps/#type-baz_e1>baz.baz_e1</a> here, ',
      '<a href=/apps/#type-foo_t3>ref title</a> there'
    ])

    text, warnings = self._Render(document)
    self.assertEqual(expected_document, text)
    self.assertEqual([], warnings)

    text, warnings = self._Render(document, render_title=True)
    self.assertEqual(expected_document, text)
    self.assertEqual(['Expected a title'], warnings)

  def testTitleAndToc(self):
    document = '<h1>title</h1> $(title) and $(table_of_contents)'

    text, warnings = self._Render(document)
    self.assertEqual('<h1>title</h1> $(title) and <table-of-contents>', text)
    self.assertEqual(['Found unexpected title "title"'], warnings)

    text, warnings = self._Render(document, render_title=True)
    self.assertEqual('<h1>title</h1> title and <table-of-contents>', text)
    self.assertEqual([], warnings)

  def testRefInTitle(self):
    document = '<h1>$(ref:baz.baz_e1 title)</h1> A $(title) was here'
    href = '/apps/#type-baz_e1'
    expected_document_no_title = ''.join([
      '<h1><a href=%s>title</a></h1> A $(title) was here' % href
    ])

    expected_document = ''.join([
      '<h1><a href=%s>title</a></h1> A title was here' % href
    ])

    text, warnings = self._Render(document)
    self.assertEqual(expected_document_no_title, text)
    self.assertEqual([('Found unexpected title "title"')], warnings)

    text, warnings = self._Render(document, render_title=True)
    self.assertEqual(expected_document, text)
    self.assertEqual([], warnings)

  def testRefSplitAcrossLines(self):
    document = 'Hello, $(ref:baz.baz_e1 world). A $(ref:foo.foo_t3\n link)'
    expected_document = ''.join([
      'Hello, <a href=/apps/#type-baz_e1>world</a>. ',
      'A <a href=/apps/#type-foo_t3>link</a>'
    ])


    text, warnings = self._Render(document)
    self.assertEqual(expected_document, text)
    self.assertEqual([], warnings)

    text, warnings = self._Render(document, render_title=True)
    self.assertEqual(expected_document, text)
    self.assertEqual(['Expected a title'], warnings)

  def testInvalidRef(self):
    # DocumentRenderer attempts to detect unclosed $(ref:...) tags by limiting
    # how far it looks ahead. Lorem Ipsum should be long enough to trigger that.
    _LOREM_IPSUM = (
        'Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do '
        'eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim '
        'ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut '
        'aliquip ex ea commodo consequat. Duis aute irure dolor in '
        'reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla '
        'pariatur. Excepteur sint occaecat cupidatat non proident, sunt in '
        'culpa qui officia deserunt mollit anim id est laborum.')
    document = ''.join([
      'An invalid $(ref:foo.foo_t3 a title ',
      _LOREM_IPSUM,
     '$(ref:baz.baz_e1) here'
    ])
    expected_document = ''.join([
      'An invalid $(ref:foo.foo_t3 a title ',
      _LOREM_IPSUM,
      '<a href=/apps/#type-baz_e1>baz.baz_e1</a> here'
    ])

    text, warnings = self._Render(document)
    self.assertEqual(expected_document, text)
    self.assertEqual([], warnings)

    text, warnings = self._Render(document, render_title=True)
    self.assertEqual(expected_document, text)
    self.assertEqual(['Expected a title'], warnings)


if __name__ == '__main__':
  unittest.main()
