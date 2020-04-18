#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test resources processing, i.e. <if> and <include> tag handling."""

import unittest
from processor import FileCache, Processor, LineNumber


class ProcessorTest(unittest.TestCase):
  """Test <include> tag processing logic."""

  def __init__(self, *args, **kwargs):
    unittest.TestCase.__init__(self, *args, **kwargs)
    self.maxDiff = None

  def setUp(self):
    FileCache._cache["/debug.js"] = """
// Copyright 2002 Older Chromium Author dudes.
function debug(msg) { if (window.DEBUG) alert(msg); }
""".strip()

    FileCache._cache["/global.js"] = """
// Copyright 2014 Old Chromium Author dudes.
<include src="/debug.js">
var global = 'type checking!';
""".strip()

    FileCache._cache["/checked.js"] = """
// Copyright 2028 Future Chromium Author dudes.
/**
 * @fileoverview Coolest app ever.
 * @author Douglas Crockford (douglas@crockford.com)
 */
<include src="/global.js">
debug(global);
// Here continues checked.js, a swell file.
""".strip()

    FileCache._cache["/double-debug.js"] = """
<include src="/debug.js">
<include src="/debug.js">
""".strip()

    self._processor = Processor("/checked.js")

  def testInline(self):
    self.assertMultiLineEqual("""
// Copyright 2028 Future Chromium Author dudes.
/**
 * @fileoverview Coolest app ever.
 * @author Douglas Crockford (douglas@crockford.com)
 */
// Copyright 2014 Old Chromium Author dudes.
// Copyright 2002 Older Chromium Author dudes.
function debug(msg) { if (window.DEBUG) alert(msg); }
var global = 'type checking!';
debug(global);
// Here continues checked.js, a swell file.
""".strip(), self._processor.contents)

  def assertLineNumber(self, abs_line, expected_line):
    actual_line = self._processor.get_file_from_line(abs_line)
    self.assertEqual(expected_line.file, actual_line.file)
    self.assertEqual(expected_line.line_number, actual_line.line_number)

  def testGetFileFromLine(self):
    """Verify that inlined files retain their original line info."""
    self.assertLineNumber(1, LineNumber("/checked.js", 1))
    self.assertLineNumber(5, LineNumber("/checked.js", 5))
    self.assertLineNumber(6, LineNumber("/global.js", 1))
    self.assertLineNumber(7, LineNumber("/debug.js", 1))
    self.assertLineNumber(8, LineNumber("/debug.js", 2))
    self.assertLineNumber(9, LineNumber("/global.js", 3))
    self.assertLineNumber(10, LineNumber("/checked.js", 7))
    self.assertLineNumber(11, LineNumber("/checked.js", 8))

  def testIncludedFiles(self):
    """Verify that files are tracked correctly as they're inlined."""
    self.assertEquals(set(["/global.js", "/debug.js"]),
                      self._processor.included_files)

  def testDoubleIncludedSkipped(self):
    """Verify that doubly included files are skipped."""
    processor = Processor("/double-debug.js")
    self.assertEquals(set(["/debug.js"]), processor.included_files)
    self.assertEquals(FileCache.read("/debug.js") + "\n", processor.contents)

class IfStrippingTest(unittest.TestCase):
  """Test that the contents of XML <if> blocks are stripped."""

  def __init__(self, *args, **kwargs):
    unittest.TestCase.__init__(self, *args, **kwargs)
    self.maxDiff = None

  def setUp(self):
    FileCache._cache["/century.js"] = """
  function getCurrentCentury() {
<if expr="netscape_os">
    alert("Oh wow!");
    return "XX";
</if>
    return "XXI";
  }
""".strip()

    self.processor_ = Processor("/century.js")

  def testIfStripping(self):
    self.assertMultiLineEqual("""
  function getCurrentCentury() {

    alert("Oh wow!");
    return "XX";

    return "XXI";
  }
""".strip(), self.processor_.contents)


if __name__ == '__main__':
  unittest.main()
