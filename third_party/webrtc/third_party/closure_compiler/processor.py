# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Process Chrome resources (HTML/CSS/JS) to handle <include> and <if> tags."""

from collections import defaultdict
import re
import os


class LineNumber(object):
  """A simple wrapper to hold line information (e.g. file.js:32)."""
  def __init__(self, source_file, line_number):
    """
    Args:
      source_file: A file path (as a string).
      line_number: The line in |file| (as an integer).
    """
    self.file = source_file
    self.line_number = int(line_number)


class FileCache(object):
  """An in-memory cache to speed up reading the same files over and over.
  
  Usage:
    FileCache.read(path_to_file)
  """

  _cache = defaultdict(str)

  @classmethod
  def read(self, source_file):
    """Read a file and return it as a string.

    Args:
      source_file: a file path (as a string) to read and return the contents.

    Returns:
      The contents of |source_file| (as a string).
    """
    abs_file = os.path.abspath(source_file)
    self._cache[abs_file] = self._cache[abs_file] or open(abs_file, "r").read()
    return self._cache[abs_file]


class Processor(object):
  """Processes resource files, inlining the contents of <include> tags, removing
  <if> tags, and retaining original line info.

  For example

    1: /* blah.js */
    2: <if expr="is_win">
    3: <include src="win.js">
    4: </if>

  would be turned into:

    1: /* blah.js */
    2:
    3: /* win.js */
    4: alert('Ew; Windows.');
    5:
  """

  _IF_TAGS_REG = "</?if[^>]*?>"
  _INCLUDE_REG = "<include[^>]+src=['\"]([^>]*)['\"]>"

  def __init__(self, source_file):
    """
    Args:
      source_file: A file path to process (as a string).
    """
    self.included_files = set()
    self._index = 0
    self._lines = self._get_file(source_file)

    # Can't enumerate(self._lines) here because some lines are re-processed.
    while self._index < len(self._lines):
      current_line = self._lines[self._index]
      match = re.search(self._INCLUDE_REG, current_line[2])
      if match:
        file_dir = os.path.dirname(current_line[0])
        file_name = os.path.abspath(os.path.join(file_dir, match.group(1)))
        if file_name not in self.included_files:
          self._include_file(file_name)
          continue  # Stay on the same line.
        else:
          # Found a duplicate <include>. Ignore and insert a blank line to
          # preserve line numbers.
          self._lines[self._index] = self._lines[self._index][:2] + ("",)
      self._index += 1

    for i, line in enumerate(self._lines):
      self._lines[i] = line[:2] + (re.sub(self._IF_TAGS_REG, "", line[2]),)

    self.contents = "\n".join(l[2] for l in self._lines)

  # Returns a list of tuples in the format: (file, line number, line contents).
  def _get_file(self, source_file):
    lines = FileCache.read(source_file).splitlines()
    return [(source_file, lnum + 1, line) for lnum, line in enumerate(lines)]

  def _include_file(self, source_file):
    self.included_files.add(source_file)
    f = self._get_file(source_file)
    self._lines = self._lines[:self._index] + f + self._lines[self._index + 1:]

  def get_file_from_line(self, line_number):
    """Get the original file and line number for an expanded file's line number.

    Args:
      line_number: A processed file's line number (as an integer or string).
    """
    line_number = int(line_number) - 1
    return LineNumber(self._lines[line_number][0], self._lines[line_number][1])
