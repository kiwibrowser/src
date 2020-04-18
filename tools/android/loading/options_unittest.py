# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import unittest

import options


class OptionsTestCase(unittest.TestCase):
  def testExtract(self):
    args = ['--A', 'foo', '--devtools_port', '2000', '--B=20',
            '--no_sandbox', '--C', '30', 'baz']
    opts = options.Options()
    opts.ExtractArgs(args)
    self.assertEqual(['--A', 'foo', '--B=20', '--C', '30', 'baz'], args)
    self.assertEqual(2000, opts.devtools_port)
    self.assertTrue(opts.no_sandbox)

  def testParent(self):
    opts = options.Options()
    parser = argparse.ArgumentParser(parents=[opts.GetParentParser()])
    parser.add_argument('--foo', type=int)
    parsed_args = parser.parse_args(['--foo=4', '--devtools_port', '2000'])
    self.assertEqual(4, parsed_args.foo)
    opts.SetParsedArgs(parsed_args)
    self.assertEqual(2000, opts.devtools_port)


if __name__ == '__main__':
  unittest.main()
