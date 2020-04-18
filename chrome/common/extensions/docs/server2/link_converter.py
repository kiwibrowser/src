#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script converts old-style <a> links to API docs to the new $ref links.
# See reference_resolver.py for more info on the format of $ref links.

import optparse
import os
import re

from docs_server_utils import SanitizeAPIName

def _ReadFile(filename):
  with open(filename) as f:
    return f.read()

def _WriteFile(filename, contents):
  with open(filename, 'w') as f:
    f.write(contents)

def _Replace(matches, filename):
  title = matches.group(3)
  if matches.group(2).count('#') != 1:
    return '<a%shref=%s>%s</a>' % (matches.group(1),
                                   matches.group(2),
                                   title)
  clean = (matches.group(2).replace('\\', '')
                           .replace("'", '')
                           .replace('"', '')
                           .replace('/', ''))
  page, link = clean.split('#')
  if not page:
    page = '%s.html' % SanitizeAPIName(filename.rsplit(os.sep, 1)[-1])
  if (not link.startswith('property-') and
      not link.startswith('type-') and
      not link.startswith('method-') and
      not link.startswith('event-')):
    return '<a%shref=%s>%s</a>' % (matches.group(1),
                                   matches.group(2),
                                   title)

  link = re.sub('^(property|type|method|event)-', '', link).replace('-', '.')
  page = page.replace('.html', '.').replace('_', '.')
  if matches.group(1) == ' ':
    padding = ''
  else:
    padding = matches.group(1)
  if link in title:
    return '%s$(ref:%s%s)' % (padding, page, link)
  else:
    return '%s$(ref:%s%s %s)' % (padding, page, link, title)

def _ConvertFile(filename, use_stdout):
  regex = re.compile(r'<a(.*?)href=(.*?)>(.*?)</a>', flags=re.DOTALL)
  contents = _ReadFile(filename)
  contents  = re.sub(regex,
                     lambda m: _Replace(m, filename),
                     contents)
  contents = contents.replace('$(ref:extension.lastError)',
                              '$(ref:runtime.lastError)')
  if use_stdout:
    print contents
  else:
    _WriteFile(filename, contents)

if __name__ == '__main__':
  parser = optparse.OptionParser(
      description='Converts <a> links to $ref links.',
      usage='usage: %prog [option] <directory>')
  parser.add_option('-f', '--file', default='',
      help='Convert links in single file.')
  parser.add_option('-o', '--out', action='store_true', default=False,
      help='Write to stdout.')
  regex = re.compile(r'<a(.*?)href=(.*?)>(.*?)</a>', flags=re.DOTALL)

  opts, argv = parser.parse_args()
  if opts.file:
    _ConvertFile(opts.file, opts.out)
  else:
    if len(argv) != 1:
      parser.print_usage()
      exit(0)
    for root, dirs, files in os.walk(argv[0]):
      for name in files:
        _ConvertFile(os.path.join(root, name), opts.out)
