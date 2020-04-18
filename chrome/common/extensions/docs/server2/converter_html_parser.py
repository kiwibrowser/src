# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from HTMLParser import HTMLParser
from StringIO import StringIO

class _ConverterHTMLParser(HTMLParser):
  def __init__(self, io):
    HTMLParser.__init__(self)
    self._io = io
    self._tag_stack = []

  def handle_starttag(self, tag, attrs):
    attrs_dict = dict(attrs)
    self._tag_stack.append({'tag': tag})
    class_attr = dict(attrs).get('class', None)
    if class_attr is not None:
      if class_attr == 'doc-family extensions':
        self._io.write('{{^is_apps}}\n')
        self._tag_stack[-1]['close'] = True
      if class_attr == 'doc-family apps':
        self._io.write('{{?is_apps}}\n')
        self._tag_stack[-1]['close'] = True
    self._io.write(self.get_starttag_text())

  def handle_startendtag(self, tag, attrs):
    self._io.write(self.get_starttag_text())

  def handle_endtag(self, tag):
    self._io.write('</' + tag + '>')
    if len(self._tag_stack) == 0:
      return
    if self._tag_stack[-1]['tag'] == tag:
      if self._tag_stack[-1].get('close', False):
        self._io.write('\n{{/is_apps}}')
      self._tag_stack.pop()

  def handle_data(self, data):
    self._io.write(data)

  def handle_comment(self, data):
    self._io.write('<!--' + data + '-->')

  def handle_entityref(self, name):
    self._io.write('&' + name + ';')

  def handle_charref(self, name):
    self._io.write('&#' + name + ';')

  def handle_decl(self, data):
    self._io.write('<!' + data + '>')

def HandleDocFamily(html):
  output = StringIO()
  parser = _ConverterHTMLParser(output)
  parser.feed(html)
  return output.getvalue()
