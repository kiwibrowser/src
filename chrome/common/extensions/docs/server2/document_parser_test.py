#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from document_parser import ParseDocument, RemoveTitle


_WHOLE_DOCUMENT = '''
Preamble before heading.

<h1 id='main' class='header'>Main header</h1>
Some intro to the content.

<h2 id='banana' class='header' title=''>Bananas</h2>
Something about bananas.

<h2 id='orange' title='hello'>Oranges</h2>
Something about oranges.

<h3 id='valencia'>Valencia Oranges</h3>
A description of valencia oranges.

<h3 id='seville'>Seville Oranges</h3>
A description of seville oranges.

<h2>Grapefruit</h3>
Grapefruit closed a h2 with a h3. This should be a warning.

<h1 id='not-main'>Not the main header</h1>
But it should still show up in the TOC as though it were an h2.

<h2>Not <h3>a banana</h2>
The embedded h3 should be ignored.

<h4>It's a h4</h4>
h4 are part of the document structure, but this is not inside a h3.

<h3>Plantains</h3>
Now I'm just getting lazy.

<h4>Another h4</h4>
This h4 is inside a h3 so will show up.

<h5>Header 5</h5>
Header 5s are not parsed.
'''


_WHOLE_DOCUMENT_WITHOUT_TITLE = '''
Preamble before heading.


Some intro to the content.

<h2 id='banana' class='header' title=''>Bananas</h2>
Something about bananas.

<h2 id='orange' title='hello'>Oranges</h2>
Something about oranges.

<h3 id='valencia'>Valencia Oranges</h3>
A description of valencia oranges.

<h3 id='seville'>Seville Oranges</h3>
A description of seville oranges.

<h2>Grapefruit</h3>
Grapefruit closed a h2 with a h3. This should be a warning.

<h1 id='not-main'>Not the main header</h1>
But it should still show up in the TOC as though it were an h2.

<h2>Not <h3>a banana</h2>
The embedded h3 should be ignored.

<h4>It's a h4</h4>
h4 are part of the document structure, but this is not inside a h3.

<h3>Plantains</h3>
Now I'm just getting lazy.

<h4>Another h4</h4>
This h4 is inside a h3 so will show up.

<h5>Header 5</h5>
Header 5s are not parsed.
'''


class DocumentParserUnittest(unittest.TestCase):

  def testEmptyDocument(self):
    self.assertEqual(('', 'No opening <h1> was found'), RemoveTitle(''))

    result = ParseDocument('')
    self.assertEqual(None, result.title)
    self.assertEqual(None, result.title_attributes)
    self.assertEqual([], result.sections)
    self.assertEqual([], result.warnings)

    result = ParseDocument('', expect_title=True)
    self.assertEqual('', result.title)
    self.assertEqual({}, result.title_attributes)
    self.assertEqual([], result.sections)
    self.assertEqual(['Expected a title'], result.warnings)

  def testRemoveTitle(self):
    no_closing_tag = '<h1>No closing tag'
    self.assertEqual((no_closing_tag, 'No closing </h1> was found'),
                     RemoveTitle(no_closing_tag))

    no_opening_tag = 'No opening tag</h1>'
    self.assertEqual((no_opening_tag, 'No opening <h1> was found'),
                     RemoveTitle(no_opening_tag))

    tags_wrong_order = '</h1>Tags in wrong order<h1>'
    self.assertEqual((tags_wrong_order, 'The </h1> appeared before the <h1>'),
                     RemoveTitle(tags_wrong_order))

    multiple_titles = '<h1>First header</h1> and <h1>Second header</h1>'
    self.assertEqual((' and <h1>Second header</h1>', None),
                     RemoveTitle(multiple_titles))

    upper_case = '<H1>Upper case header tag</H1> hi'
    self.assertEqual((' hi', None), RemoveTitle(upper_case))
    mixed_case = '<H1>Mixed case header tag</h1> hi'
    self.assertEqual((' hi', None), RemoveTitle(mixed_case))

  def testOnlyTitleDocument(self):
    document = '<h1 id="header">heading</h1>'
    self.assertEqual(('', None), RemoveTitle(document))

    result = ParseDocument(document)
    self.assertEqual(None, result.title)
    self.assertEqual(None, result.title_attributes)
    self.assertEqual([], result.sections)
    self.assertEqual(['Found unexpected title "heading"'], result.warnings)

    result = ParseDocument(document, expect_title=True)
    self.assertEqual('heading', result.title)
    self.assertEqual({'id': 'header'}, result.title_attributes)
    self.assertEqual([], result.sections)
    self.assertEqual([], result.warnings)

  def testWholeDocument(self):
    self.assertEqual((_WHOLE_DOCUMENT_WITHOUT_TITLE, None),
                     RemoveTitle(_WHOLE_DOCUMENT))
    result = ParseDocument(_WHOLE_DOCUMENT, expect_title=True)
    self.assertEqual('Main header', result.title)
    self.assertEqual({'id': 'main', 'class': 'header'}, result.title_attributes)
    self.assertEqual([
      'Found closing </h3> while processing a <h2> (line 19, column 15)',
      'Found multiple <h1> tags. Subsequent <h1> tags will be classified as '
          '<h2> for the purpose of the structure (line 22, column 1)',
      'Found <h3> in the middle of processing a <h2> (line 25, column 9)',
      # TODO(kalman): Re-enable this warning once the reference pages have
      # their references fixed.
      #'Found <h4> without any preceding <h3> (line 28, column 1)',
    ], result.warnings)

    # The non-trivial table of contents assertions...
    self.assertEqual(1, len(result.sections))
    entries = result.sections[0].structure

    self.assertEqual(4, len(entries), entries)
    entry0, entry1, entry2, entry3 = entries

    self.assertEqual('hello', entry0.name)
    self.assertEqual({'id': 'orange'}, entry0.attributes)
    self.assertEqual(2, len(entry0.entries))
    entry0_0, entry0_1 = entry0.entries

    self.assertEqual('Valencia Oranges', entry0_0.name)
    self.assertEqual({'id': 'valencia'}, entry0_0.attributes)
    self.assertEqual([], entry0_0.entries)
    self.assertEqual('Seville Oranges', entry0_1.name)
    self.assertEqual({'id': 'seville'}, entry0_1.attributes)
    self.assertEqual([], entry0_1.entries)

    self.assertEqual('Grapefruit', entry1.name)
    self.assertEqual({}, entry1.attributes)
    self.assertEqual([], entry1.entries)

    self.assertEqual('Not the main header', entry2.name)
    self.assertEqual({'id': 'not-main'}, entry2.attributes)
    self.assertEqual([], entry2.entries)

    self.assertEqual('Not a banana', entry3.name)
    self.assertEqual({}, entry3.attributes)
    self.assertEqual(2, len(entry3.entries))
    entry3_1, entry3_2 = entry3.entries

    self.assertEqual('It\'s a h4', entry3_1.name)
    self.assertEqual({}, entry3_1.attributes)
    self.assertEqual([], entry3_1.entries)

    self.assertEqual('Plantains', entry3_2.name)
    self.assertEqual({}, entry3_2.attributes)
    self.assertEqual(1, len(entry3_2.entries))
    entry3_2_1, = entry3_2.entries

    self.assertEqual('Another h4', entry3_2_1.name)
    self.assertEqual({}, entry3_2_1.attributes)
    self.assertEqual([], entry3_2_1.entries)

  def testSingleExplicitSection(self):
    def test(document):
      result = ParseDocument(document, expect_title=True)
      self.assertEqual([], result.warnings)
      self.assertEqual('Header', result.title)
      self.assertEqual(1, len(result.sections))
      section0, = result.sections
      entry0, = section0.structure
      self.assertEqual('An inner header', entry0.name)
    # A single section, one with the title inside the section, the other out.
    test('<h1>Header</h1>'
         '<section>'
         'Just a single section here.'
         '<h2>An inner header</h2>'
         '</section>')
    test('<section>'
         'Another single section here.'
         '<h1>Header</h1>'
         '<h2>An inner header</h2>'
         '</section>')

  def testMultipleSections(self):
    result = ParseDocument(
        '<h1>Header</h1>'
        '<h2>First header</h2>'
        'This content outside a section is the first section.'
        '<section>'
        'Second section'
        '<h2>Second header</h2>'
        '</section>'
        '<section>'
        'Third section'
        '<h2>Third header</h2>'
        '</section>',
        expect_title=True)
    self.assertEqual([], result.warnings)
    self.assertEqual('Header', result.title)
    self.assertEqual(3, len(result.sections))
    section0, section1, section2 = result.sections
    def assert_single_header(section, name):
      self.assertEqual(1, len(section.structure))
      self.assertEqual(name, section.structure[0].name)
    assert_single_header(section0, 'First header')
    assert_single_header(section1, 'Second header')
    assert_single_header(section2, 'Third header')


if __name__ == '__main__':
  unittest.main()
