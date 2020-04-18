#!/usr/bin/env python
# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import os
import sys
import unittest
import StringIO

ROOT_DIR = os.path.dirname(os.path.abspath(os.path.join(
    __file__.decode(sys.getfilesystemencoding()),
    os.pardir, os.pardir, os.pardir)))
sys.path.insert(0, ROOT_DIR)

from libs.logdog import streamname


class StreamNameTestCase(unittest.TestCase):

  def testInvalidStreamNamesRaiseValueError(self):
    for name in (
        '',
        'a' * (streamname._MAX_STREAM_NAME_LENGTH+1),
        ' s p a c e s ',
        '-hyphen',
        'stream/path/+/not/name',
    ):
      with self.assertRaises(ValueError):
        streamname.validate_stream_name(name)

  def testValidStreamNamesDoNotRaise(self):
    for name in (
        'a',
        'a' * (streamname._MAX_STREAM_NAME_LENGTH),
        'foo/bar',
        'f123/four/five-_.:',
    ):
      raised = False
      try:
        streamname.validate_stream_name(name)
      except ValueError:
        raised = True
      self.assertFalse(raised, "Stream name '%s' raised ValueError" % (name,))

  def testNormalize(self):
    for name, normalized in (
        ('', 'PFX'),
        ('_invalid_start_char', 'PFX_invalid_start_char'),
        ('valid_stream_name.1:2-3', 'valid_stream_name.1:2-3'),
        ('some stream (with stuff)', 'some_stream__with_stuff_'),
        ('_invalid/st!ream/name entry', 'PFX_invalid/st_ream/name_entry'),
        ('     ', 'PFX_____'),
    ):
      self.assertEqual(streamname.normalize(name, prefix='PFX'), normalized)

    # Assert that an empty stream name with no prefix will raise a ValueError.
    self.assertRaises(ValueError, streamname.normalize, '')

    # Assert that a stream name with an invalid starting character and no prefix
    # will raise a ValueError.
    self.assertRaises(ValueError, streamname.normalize, '_invalid_start_char')


class StreamPathTestCase(unittest.TestCase):

  def testParseValidPath(self):
    for path in (
        'foo/+/bar',
        'foo/bar/+/baz',
    ):
      prefix, name = path.split('/+/')
      parsed = streamname.StreamPath.parse(path)
      self.assertEqual(
          parsed,
          streamname.StreamPath(prefix=prefix, name=name))
      self.assertEqual(str(parsed), path)

  def testParseInvalidValidPathRaisesValueError(self):
    for path in (
        '',
        'foo/+',
        'foo/+/',
        '+/bar',
        '/+/bar',
        'foo/bar',
        '!!!invalid!!!/+/bar',
        'foo/+/!!!invalid!!!',
    ):
      with self.assertRaises(ValueError):
        streamname.StreamPath.parse(path)

  def testLogDogViewerUrl(self):
    for project, path, url in (
        ('test', streamname.StreamPath(prefix='foo', name='bar/baz'),
         'https://example.appspot.com/v/?s=test%2Ffoo%2F%2B%2Fbar%2Fbaz'),

        ('test', streamname.StreamPath(prefix='foo', name='bar/**'),
         'https://example.appspot.com/v/?s=test%2Ffoo%2F%2B%2Fbar%2F%2A%2A'),

        ('test', streamname.StreamPath(prefix='**', name='**'),
         'https://example.appspot.com/v/?s=test%2F%2A%2A%2F%2B%2F%2A%2A'),
    ):
      self.assertEqual(
          streamname.get_logdog_viewer_url('example.appspot.com', project,
                                           path),
          url)

    # Multiple streams.
    self.assertEqual(
        streamname.get_logdog_viewer_url('example.appspot.com', 'test',
            streamname.StreamPath(prefix='foo', name='bar/baz'),
            streamname.StreamPath(prefix='qux', name='**'),
        ),
        ('https://example.appspot.com/v/?s=test%2Ffoo%2F%2B%2Fbar%2Fbaz&'
         's=test%2Fqux%2F%2B%2F%2A%2A'))


if __name__ == '__main__':
  unittest.main()
