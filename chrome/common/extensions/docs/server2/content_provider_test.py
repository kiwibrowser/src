#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from cStringIO import StringIO
import json
import unittest
from zipfile import ZipFile

from compiled_file_system import CompiledFileSystem
from content_provider import ContentProvider
from file_system import FileNotFoundError
from object_store_creator import ObjectStoreCreator
from path_canonicalizer import PathCanonicalizer
from test_file_system import TestFileSystem
from third_party.motemplate import Motemplate

_REDIRECTS_JSON = json.dumps({
  'oldfile.html': 'storage.html',
  'index.html': 'https://developers.google.com/chrome',
})


_MARKDOWN_CONTENT = (
  ('# Header 1 #', u'<h1 id="header-1">Header 1</h1>'),
  ('1.  Foo\n', u'<ol>\n<li>Foo</li>\n</ol>'),
  ('![alt text](/path/img.jpg "Title")\n',
      '<p><img alt="alt text" src="/path/img.jpg" title="Title" /></p>'),
  ('* Unordered item 1', u'<ul>\n<li>Unordered item 1</li>\n</ul>')
)

# Test file system data which exercises many different mimetypes.
_TEST_DATA = {
  'dir': {
    'a.txt': 'a.txt content',
    'b.txt': 'b.txt content',
    'c': {
      'd.txt': 'd.txt content',
    },
  },
  'dir2': {
    'dir3': {
      'a.txt': 'a.txt content',
      'b.txt': 'b.txt content',
      'c': {
        'd.txt': 'd.txt content',
      },
    },
  },
  'dir4': {
    'index.html': 'index.html content 1'
  },
  'dir5': {
    'index.html': 'index.html content 2'
  },
  'dir6': {
    'notindex.html': 'notindex.html content'
  },
  'dir7': {
    'index.md': '\n'.join(text[0] for text in _MARKDOWN_CONTENT)
  },
  'dir.txt': 'dir.txt content',
  'dir5.html': 'dir5.html content',
  'img.png': 'img.png content',
  'index.html': 'index.html content',
  'read.txt': 'read.txt content',
  'redirects.json': _REDIRECTS_JSON,
  'noextension': 'noextension content',
  'run.js': 'run.js content',
  'site.css': 'site.css content',
  'storage.html': 'storage.html content',
  'markdown.md': '\n'.join(text[0] for text in _MARKDOWN_CONTENT)
}


class ContentProviderUnittest(unittest.TestCase):
  def setUp(self):
    self._test_file_system = TestFileSystem(_TEST_DATA)
    self._content_provider = self._CreateContentProvider()

  def _CreateContentProvider(self, supports_zip=False):
    object_store_creator = ObjectStoreCreator.ForTest()
    return ContentProvider(
        'foo',
        CompiledFileSystem.Factory(object_store_creator),
        self._test_file_system,
        object_store_creator,
        default_extensions=('.html', '.md'),
        # TODO(kalman): Test supports_templates=False.
        supports_templates=True,
        supports_zip=supports_zip)

  def _assertContent(self, content, content_type, content_and_type):
    # Assert type so that str is differentiated from unicode.
    self.assertEqual(type(content), type(content_and_type.content))
    self.assertEqual(content, content_and_type.content)
    self.assertEqual(content_type, content_and_type.content_type)

  def _assertTemplateContent(self, content, path, version):
    content_and_type = self._content_provider.GetContentAndType(path).Get()
    self.assertEqual(Motemplate, type(content_and_type.content))
    content_and_type.content = content_and_type.content.source
    self._assertContent(content, 'text/html', content_and_type)
    self.assertEqual(version, self._content_provider.GetVersion(path).Get())

  def _assertMarkdownContent(self, content, path, version):
    content_and_type = self._content_provider.GetContentAndType(path).Get()
    content_and_type.content = content_and_type.content.source
    self._assertContent(content, 'text/html', content_and_type)
    self.assertEqual(version, self._content_provider.GetVersion(path).Get())

  def testPlainText(self):
    self._assertContent(
        u'a.txt content', 'text/plain',
        self._content_provider.GetContentAndType('dir/a.txt').Get())
    self._assertContent(
        u'd.txt content', 'text/plain',
        self._content_provider.GetContentAndType('dir/c/d.txt').Get())
    self._assertContent(
        u'read.txt content', 'text/plain',
        self._content_provider.GetContentAndType('read.txt').Get())
    self._assertContent(
        unicode(_REDIRECTS_JSON, 'utf-8'), 'application/json',
        self._content_provider.GetContentAndType('redirects.json').Get())
    self._assertContent(
        u'run.js content', 'application/javascript',
        self._content_provider.GetContentAndType('run.js').Get())
    self._assertContent(
        u'site.css content', 'text/css',
        self._content_provider.GetContentAndType('site.css').Get())

  def testTemplate(self):
    self._assertTemplateContent(u'storage.html content', 'storage.html', '0')
    self._test_file_system.IncrementStat('storage.html')
    self._assertTemplateContent(u'storage.html content', 'storage.html', '1')

  def testImage(self):
    self._assertContent(
        'img.png content', 'image/png',
        self._content_provider.GetContentAndType('img.png').Get())

  def testZipTopLevel(self):
    zip_content_provider = self._CreateContentProvider(supports_zip=True)
    content_and_type = zip_content_provider.GetContentAndType('dir.zip').Get()
    zipfile = ZipFile(StringIO(content_and_type.content))
    content_and_type.content = zipfile.namelist()
    self._assertContent(
        ['dir/a.txt', 'dir/b.txt', 'dir/c/d.txt'], 'application/zip',
        content_and_type)

  def testZip2ndLevel(self):
    zip_content_provider = self._CreateContentProvider(supports_zip=True)
    content_and_type = zip_content_provider.GetContentAndType(
        'dir2/dir3.zip').Get()
    zipfile = ZipFile(StringIO(content_and_type.content))
    content_and_type.content = zipfile.namelist()
    self._assertContent(
        ['dir3/a.txt', 'dir3/b.txt', 'dir3/c/d.txt'], 'application/zip',
        content_and_type)

  def testCanonicalZipPaths(self):
    # Without supports_zip the path is canonicalized as a file.
    self.assertEqual(
        'dir.txt',
        self._content_provider.GetCanonicalPath('dir.zip'))
    self.assertEqual(
        'dir.txt',
        self._content_provider.GetCanonicalPath('diR.zip'))
    # With supports_zip the path is canonicalized as the zip file which
    # corresponds to the canonical directory.
    zip_content_provider = self._CreateContentProvider(supports_zip=True)
    self.assertEqual(
        'dir.zip',
        zip_content_provider.GetCanonicalPath('dir.zip'))
    self.assertEqual(
        'dir.zip',
        zip_content_provider.GetCanonicalPath('diR.zip'))

  def testMarkdown(self):
    expected_content = '\n'.join(text[1] for text in _MARKDOWN_CONTENT)
    self._assertMarkdownContent(expected_content, 'markdown', '0')
    self._test_file_system.IncrementStat('markdown.md')
    self._assertMarkdownContent(expected_content, 'markdown', '1')

  def testNotFound(self):
    self.assertRaises(
        FileNotFoundError,
        self._content_provider.GetContentAndType('oops').Get)

  def testIndexRedirect(self):
    self._assertTemplateContent(u'index.html content', '', '0')
    self._assertTemplateContent(u'index.html content 1', 'dir4', '0')
    self._assertTemplateContent(u'dir5.html content', 'dir5', '0')
    self._assertMarkdownContent(
        '\n'.join(text[1] for text in _MARKDOWN_CONTENT),
        'dir7',
        '0')
    self._assertContent(
        'noextension content', 'text/plain',
        self._content_provider.GetContentAndType('noextension').Get())
    self.assertRaises(
        FileNotFoundError,
        self._content_provider.GetContentAndType('dir6').Get)

  def testRefresh(self):
    # Not entirely sure what to test here, but get some code coverage.
    self._content_provider.Refresh().Get()


if __name__ == '__main__':
  unittest.main()
