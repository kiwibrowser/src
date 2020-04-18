# coding=utf8
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Utility functions to handle patches."""

import posixpath
import os
import re


class UnsupportedPatchFormat(Exception):
  def __init__(self, filename, status):
    super(UnsupportedPatchFormat, self).__init__(filename, status)
    self.filename = filename
    self.status = status

  def __str__(self):
    out = 'Can\'t process patch for file %s.' % self.filename
    if self.status:
      out += '\n%s' % self.status
    return out


class FilePatchBase(object):
  """Defines a single file being modified.

  '/' is always used instead of os.sep for consistency.
  """
  is_delete = False
  is_binary = False
  is_new = False

  def __init__(self, filename):
    assert self.__class__ is not FilePatchBase
    self.filename = self._process_filename(filename)
    # Set when the file is copied or moved.
    self.source_filename = None

  @property
  def filename_utf8(self):
    return self.filename.encode('utf-8')

  @property
  def source_filename_utf8(self):
    if self.source_filename is not None:
      return self.source_filename.encode('utf-8')

  @staticmethod
  def _process_filename(filename):
    filename = filename.replace('\\', '/')
    # Blacklist a few characters for simplicity.
    for i in ('$', '..', '\'', '"', '<', '>', ':', '|', '?', '*'):
      if i in filename:
        raise UnsupportedPatchFormat(
            filename, 'Can\'t use \'%s\' in filename.' % i)
    if filename.startswith('/'):
      raise UnsupportedPatchFormat(
          filename, 'Filename can\'t start with \'/\'.')
    if filename == 'CON':
      raise UnsupportedPatchFormat(
          filename, 'Filename can\'t be \'CON\'.')
    if re.match('COM\d', filename):
      raise UnsupportedPatchFormat(
          filename, 'Filename can\'t be \'%s\'.' % filename)
    return filename

  def set_relpath(self, relpath):
    if not relpath:
      return
    relpath = relpath.replace('\\', '/')
    if relpath[0] == '/':
      self._fail('Relative path starts with %s' % relpath[0])
    self.filename = self._process_filename(
        posixpath.join(relpath, self.filename))
    if self.source_filename:
      self.source_filename = self._process_filename(
          posixpath.join(relpath, self.source_filename))

  def _fail(self, msg):
    """Shortcut function to raise UnsupportedPatchFormat."""
    raise UnsupportedPatchFormat(self.filename, msg)

  def __str__(self):
    # Use a status-like board.
    out = ''
    if self.is_binary:
      out += 'B'
    else:
      out += ' '
    if self.is_delete:
      out += 'D'
    else:
      out += ' '
    if self.is_new:
      out += 'N'
    else:
      out += ' '
    if self.source_filename:
      out += 'R'
    else:
      out += ' '
    out += '  '
    if self.source_filename:
      out += '%s->' % self.source_filename_utf8
    return out + self.filename_utf8

  def dump(self):
    """Dumps itself in a verbose way to help diagnosing."""
    return str(self)


class FilePatchDelete(FilePatchBase):
  """Deletes a file."""
  is_delete = True

  def __init__(self, filename, is_binary):
    super(FilePatchDelete, self).__init__(filename)
    self.is_binary = is_binary


class FilePatchBinary(FilePatchBase):
  """Content of a new binary file."""
  is_binary = True

  def __init__(self, filename, data, svn_properties, is_new):
    super(FilePatchBinary, self).__init__(filename)
    self.data = data
    self.svn_properties = svn_properties or []
    self.is_new = is_new

  def get(self):
    return self.data

  def __str__(self):
    return str(super(FilePatchBinary, self)) + ' %d bytes' % len(self.data)


class Hunk(object):
  """Parsed hunk data container."""

  def __init__(self, start_src, lines_src, start_dst, lines_dst):
    self.start_src = start_src
    self.lines_src = lines_src
    self.start_dst = start_dst
    self.lines_dst = lines_dst
    self.variation = self.lines_dst - self.lines_src
    self.text = []

  def __repr__(self):
    return '%s<(%d, %d) to (%d, %d)>' % (
        self.__class__.__name__,
        self.start_src, self.lines_src, self.start_dst, self.lines_dst)


class FilePatchDiff(FilePatchBase):
  """Patch for a single file."""

  def __init__(self, filename, diff, svn_properties):
    super(FilePatchDiff, self).__init__(filename)
    if not diff:
      self._fail('File doesn\'t have a diff.')
    self.diff_header, self.diff_hunks = self._split_header(diff)
    self.svn_properties = svn_properties or []
    self.is_git_diff = self._is_git_diff_header(self.diff_header)
    self.patchlevel = 0
    if self.is_git_diff:
      self._verify_git_header()
    else:
      self._verify_svn_header()
    self.hunks = self._split_hunks()
    if self.source_filename and not self.is_new:
      self._fail('If source_filename is set, is_new must be also be set')

  def get(self, for_git):
    if for_git or not self.source_filename:
      return self.diff_header + self.diff_hunks
    else:
      # patch is stupid. It patches the source_filename instead so get rid of
      # any source_filename reference if needed.
      return (
          self.diff_header.replace(
              self.source_filename_utf8, self.filename_utf8) +
          self.diff_hunks)

  def set_relpath(self, relpath):
    old_filename = self.filename_utf8
    old_source_filename = self.source_filename_utf8 or self.filename_utf8
    super(FilePatchDiff, self).set_relpath(relpath)
    # Update the header too.
    filename = self.filename_utf8
    source_filename = self.source_filename_utf8 or self.filename_utf8
    lines = self.diff_header.splitlines(True)
    for i, line in enumerate(lines):
      if line.startswith('diff --git'):
        lines[i] = line.replace(
            'a/' + old_source_filename, source_filename).replace(
                'b/' + old_filename, filename)
      elif re.match(r'^\w+ from .+$', line) or line.startswith('---'):
        lines[i] = line.replace(old_source_filename, source_filename)
      elif re.match(r'^\w+ to .+$', line) or line.startswith('+++'):
        lines[i] = line.replace(old_filename, filename)
    self.diff_header = ''.join(lines)

  def _split_header(self, diff):
    """Splits a diff in two: the header and the hunks."""
    header = []
    hunks = diff.splitlines(True)
    while hunks:
      header.append(hunks.pop(0))
      if header[-1].startswith('--- '):
        break
    else:
      # Some diff may not have a ---/+++ set like a git rename with no change or
      # a svn diff with only property change.
      pass

    if hunks:
      if not hunks[0].startswith('+++ '):
        self._fail('Inconsistent header')
      header.append(hunks.pop(0))
      if hunks:
        if not hunks[0].startswith('@@ '):
          self._fail('Inconsistent hunk header')

    # Mangle any \\ in the header to /.
    header_lines = ('Index:', 'diff', 'copy', 'rename', '+++', '---')
    basename = os.path.basename(self.filename_utf8)
    for i in xrange(len(header)):
      if (header[i].split(' ', 1)[0] in header_lines or
          header[i].endswith(basename)):
        header[i] = header[i].replace('\\', '/')
    return ''.join(header), ''.join(hunks)

  @staticmethod
  def _is_git_diff_header(diff_header):
    """Returns True if the diff for a single files was generated with git."""
    # Delete: http://codereview.chromium.org/download/issue6368055_22_29.diff
    # Rename partial change:
    # http://codereview.chromium.org/download/issue6250123_3013_6010.diff
    # Rename no change:
    # http://codereview.chromium.org/download/issue6287022_3001_4010.diff
    return any(l.startswith('diff --git') for l in diff_header.splitlines())

  def _split_hunks(self):
    """Splits the hunks and does verification."""
    hunks = []
    for line in self.diff_hunks.splitlines(True):
      if line.startswith('@@'):
        match = re.match(r'^@@ -([\d,]+) \+([\d,]+) @@.*$', line)
        # File add will result in "-0,0 +1" but file deletion will result in
        # "-1,N +0,0" where N is the number of lines deleted. That's from diff
        # and svn diff. git diff doesn't exhibit this behavior.
        # svn diff for a single line file rewrite "@@ -1 +1 @@". Fun.
        # "@@ -1 +1,N @@" is also valid where N is the length of the new file.
        if not match:
          self._fail('Hunk header is unparsable')
        count = match.group(1).count(',')
        if not count:
          start_src = int(match.group(1))
          lines_src = 1
        elif count == 1:
          start_src, lines_src = map(int, match.group(1).split(',', 1))
        else:
          self._fail('Hunk header is malformed')

        count = match.group(2).count(',')
        if not count:
          start_dst = int(match.group(2))
          lines_dst = 1
        elif count == 1:
          start_dst, lines_dst = map(int, match.group(2).split(',', 1))
        else:
          self._fail('Hunk header is malformed')
        new_hunk = Hunk(start_src, lines_src, start_dst, lines_dst)
        if hunks:
          if new_hunk.start_src <= hunks[-1].start_src:
            self._fail('Hunks source lines are not ordered')
          if new_hunk.start_dst <= hunks[-1].start_dst:
            self._fail('Hunks destination lines are not ordered')
        hunks.append(new_hunk)
        continue
      hunks[-1].text.append(line)

    if len(hunks) == 1:
      if hunks[0].start_src == 0 and hunks[0].lines_src == 0:
        self.is_new = True
      if hunks[0].start_dst == 0 and hunks[0].lines_dst == 0:
        self.is_delete = True

    if self.is_new and self.is_delete:
      self._fail('Hunk header is all 0')

    if not self.is_new and not self.is_delete:
      for hunk in hunks:
        variation = (
            len([1 for i in hunk.text if i.startswith('+')]) -
            len([1 for i in hunk.text if i.startswith('-')]))
        if variation != hunk.variation:
          self._fail(
              'Hunk header is incorrect: %d vs %d; %r' % (
                variation, hunk.variation, hunk))
        if not hunk.start_src:
          self._fail(
              'Hunk header start line is incorrect: %d' % hunk.start_src)
        if not hunk.start_dst:
          self._fail(
              'Hunk header start line is incorrect: %d' % hunk.start_dst)
        hunk.start_src -= 1
        hunk.start_dst -= 1
    if self.is_new and hunks:
      hunks[0].start_dst -= 1
    if self.is_delete and hunks:
      hunks[0].start_src -= 1
    return hunks

  def mangle(self, string):
    """Mangle a file path."""
    return '/'.join(string.replace('\\', '/').split('/')[self.patchlevel:])

  def _verify_git_header(self):
    """Sanity checks the header.

    Expects the following format:

    <garbage>
    diff --git (|a/)<filename> (|b/)<filename>
    <similarity>
    <filemode changes>
    <index>
    <copy|rename from>
    <copy|rename to>
    --- <filename>
    +++ <filename>

    Everything is optional except the diff --git line.
    """
    lines = self.diff_header.splitlines()

    # Verify the diff --git line.
    old = None
    new = None
    while lines:
      match = re.match(r'^diff \-\-git (.*?) (.*)$', lines.pop(0))
      if not match:
        continue
      if match.group(1).startswith('a/') and match.group(2).startswith('b/'):
        self.patchlevel = 1
      old = self.mangle(match.group(1))
      new = self.mangle(match.group(2))

      # The rename is about the new file so the old file can be anything.
      if new not in (self.filename_utf8, 'dev/null'):
        self._fail('Unexpected git diff output name %s.' % new)
      if old == 'dev/null' and new == 'dev/null':
        self._fail('Unexpected /dev/null git diff.')
      break

    if not old or not new:
      self._fail('Unexpected git diff; couldn\'t find git header.')

    if old not in (self.filename_utf8, 'dev/null'):
      # Copy or rename.
      self.source_filename = old.decode('utf-8')
      self.is_new = True

    last_line = ''

    while lines:
      line = lines.pop(0)
      self._verify_git_header_process_line(lines, line, last_line)
      last_line = line

    # Cheap check to make sure the file name is at least mentioned in the
    # 'diff' header. That the only remaining invariant.
    if not self.filename_utf8 in self.diff_header:
      self._fail('Diff seems corrupted.')

  def _verify_git_header_process_line(self, lines, line, last_line):
    """Processes a single line of the header.

    Returns True if it should continue looping.

    Format is described to
    http://www.kernel.org/pub/software/scm/git/docs/git-diff.html
    """
    match = re.match(r'^(rename|copy) from (.+)$', line)
    old = self.source_filename_utf8 or self.filename_utf8
    if match:
      if old != match.group(2):
        self._fail('Unexpected git diff input name for line %s.' % line)
      if not lines or not lines[0].startswith('%s to ' % match.group(1)):
        self._fail(
            'Confused %s from/to git diff for line %s.' %
                (match.group(1), line))
      return

    match = re.match(r'^(rename|copy) to (.+)$', line)
    if match:
      if self.filename_utf8 != match.group(2):
        self._fail('Unexpected git diff output name for line %s.' % line)
      if not last_line.startswith('%s from ' % match.group(1)):
        self._fail(
            'Confused %s from/to git diff for line %s.' %
                (match.group(1), line))
      return

    match = re.match(r'^deleted file mode (\d{6})$', line)
    if match:
      # It is necessary to parse it because there may be no hunk, like when the
      # file was empty.
      self.is_delete = True
      return

    match = re.match(r'^new(| file) mode (\d{6})$', line)
    if match:
      mode = match.group(2)
      # Only look at owner ACL for executable.
      if bool(int(mode[4]) & 1):
        self.svn_properties.append(('svn:executable', '.'))
      elif not self.source_filename and self.is_new:
        # It's a new file, not from a rename/copy, then there's no property to
        # delete.
        self.svn_properties.append(('svn:executable', None))
      return

    match = re.match(r'^--- (.*)$', line)
    if match:
      if last_line[:3] in ('---', '+++'):
        self._fail('--- and +++ are reversed')
      if match.group(1) == '/dev/null':
        self.is_new = True
      elif self.mangle(match.group(1)) != old:
        # git patches are always well formatted, do not allow random filenames.
        self._fail('Unexpected git diff: %s != %s.' % (old, match.group(1)))
      if not lines or not lines[0].startswith('+++'):
        self._fail('Missing git diff output name.')
      return

    match = re.match(r'^\+\+\+ (.*)$', line)
    if match:
      if not last_line.startswith('---'):
        self._fail('Unexpected git diff: --- not following +++.')
      if '/dev/null' == match.group(1):
        self.is_delete = True
      elif self.filename_utf8 != self.mangle(match.group(1)):
        self._fail(
            'Unexpected git diff: %s != %s.' % (self.filename, match.group(1)))
      if lines:
        self._fail('Crap after +++')
      # We're done.
      return

  def _verify_svn_header(self):
    """Sanity checks the header.

    A svn diff can contain only property changes, in that case there will be no
    proper header. To make things worse, this property change header is
    localized.
    """
    lines = self.diff_header.splitlines()
    last_line = ''

    while lines:
      line = lines.pop(0)
      self._verify_svn_header_process_line(lines, line, last_line)
      last_line = line

    # Cheap check to make sure the file name is at least mentioned in the
    # 'diff' header. That the only remaining invariant.
    if not self.filename_utf8 in self.diff_header:
      self._fail('Diff seems corrupted.')

  def _verify_svn_header_process_line(self, lines, line, last_line):
    """Processes a single line of the header.

    Returns True if it should continue looping.
    """
    match = re.match(r'^--- ([^\t]+).*$', line)
    if match:
      if last_line[:3] in ('---', '+++'):
        self._fail('--- and +++ are reversed')
      if match.group(1) == '/dev/null':
        self.is_new = True
      elif self.mangle(match.group(1)) != self.filename_utf8:
        # guess the source filename.
        self.source_filename = match.group(1).decode('utf-8')
        self.is_new = True
      if not lines or not lines[0].startswith('+++'):
        self._fail('Nothing after header.')
      return

    match = re.match(r'^\+\+\+ ([^\t]+).*$', line)
    if match:
      if not last_line.startswith('---'):
        self._fail('Unexpected diff: --- not following +++.')
      if match.group(1) == '/dev/null':
        self.is_delete = True
      elif self.mangle(match.group(1)) != self.filename_utf8:
        self._fail('Unexpected diff: %s.' % match.group(1))
      if lines:
        self._fail('Crap after +++')
      # We're done.
      return

  def dump(self):
    """Dumps itself in a verbose way to help diagnosing."""
    return str(self) + '\n' + self.get(True)


class PatchSet(object):
  """A list of FilePatch* objects."""

  def __init__(self, patches):
    for p in patches:
      assert isinstance(p, FilePatchBase)

    def key(p):
      """Sort by ordering of application.

      File move are first.
      Deletes are last.
      """
      # The bool is necessary because None < 'string' but the reverse is needed.
      return (
          p.is_delete,
          # False is before True, so files *with* a source file will be first.
          not bool(p.source_filename),
          p.source_filename_utf8,
          p.filename_utf8)

    self.patches = sorted(patches, key=key)

  def set_relpath(self, relpath):
    """Used to offset the patch into a subdirectory."""
    for patch in self.patches:
      patch.set_relpath(relpath)

  def __iter__(self):
    for patch in self.patches:
      yield patch

  def __getitem__(self, key):
    return self.patches[key]

  @property
  def filenames(self):
    return [p.filename for p in self.patches]
