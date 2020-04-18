# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Simplify unit tests based on pymox."""

import os
import random
import shutil
import string
import StringIO
import subprocess
import sys

sys.path.append(os.path.dirname(os.path.dirname(__file__)))
from third_party.pymox import mox


class IsOneOf(mox.Comparator):
  def __init__(self, keys):
    self._keys = keys

  def equals(self, rhs):
    return rhs in self._keys

  def __repr__(self):
    return '<sequence or map containing \'%s\'>' % str(self._keys)


class TestCaseUtils(object):
  """Base class with some additional functionalities. People will usually want
  to use SuperMoxTestBase instead."""
  # Backup the separator in case it gets mocked
  _OS_SEP = os.sep
  _RANDOM_CHOICE = random.choice
  _RANDOM_RANDINT = random.randint
  _STRING_LETTERS = string.letters

  ## Some utilities for generating arbitrary arguments.
  def String(self, max_length):
    return ''.join([self._RANDOM_CHOICE(self._STRING_LETTERS)
                    for _ in xrange(self._RANDOM_RANDINT(1, max_length))])

  def Strings(self, max_arg_count, max_arg_length):
    return [self.String(max_arg_length) for _ in xrange(max_arg_count)]

  def Args(self, max_arg_count=8, max_arg_length=16):
    return self.Strings(max_arg_count,
                        self._RANDOM_RANDINT(1, max_arg_length))

  def _DirElts(self, max_elt_count=4, max_elt_length=8):
    return self._OS_SEP.join(self.Strings(max_elt_count, max_elt_length))

  def Dir(self, max_elt_count=4, max_elt_length=8):
    return (self._RANDOM_CHOICE((self._OS_SEP, '')) +
            self._DirElts(max_elt_count, max_elt_length))

  def RootDir(self, max_elt_count=4, max_elt_length=8):
    return self._OS_SEP + self._DirElts(max_elt_count, max_elt_length)

  def compareMembers(self, obj, members):
    """If you add a member, be sure to add the relevant test!"""
    # Skip over members starting with '_' since they are usually not meant to
    # be for public use.
    actual_members = [x for x in sorted(dir(obj))
                      if not x.startswith('_')]
    expected_members = sorted(members)
    if actual_members != expected_members:
      diff = ([i for i in actual_members if i not in expected_members] +
              [i for i in expected_members if i not in actual_members])
      print >> sys.stderr, diff
    # pylint: disable=no-member
    self.assertEqual(actual_members, expected_members)

  def setUp(self):
    self.root_dir = self.Dir()
    self.args = self.Args()
    self.relpath = self.String(200)

  def tearDown(self):
    pass


class StdoutCheck(object):
  def setUp(self):
    # Override the mock with a StringIO, it's much less painful to test.
    self._old_stdout = sys.stdout
    stdout = StringIO.StringIO()
    stdout.flush = lambda: None
    sys.stdout = stdout

  def tearDown(self):
    try:
      # If sys.stdout was used, self.checkstdout() must be called.
      # pylint: disable=no-member
      if not sys.stdout.closed:
        self.assertEquals('', sys.stdout.getvalue())
    except AttributeError:
      pass
    sys.stdout = self._old_stdout

  def checkstdout(self, expected):
    value = sys.stdout.getvalue()
    sys.stdout.close()
    # pylint: disable=no-member
    self.assertEquals(expected, value)


class SuperMoxTestBase(TestCaseUtils, StdoutCheck, mox.MoxTestBase):
  def setUp(self):
    """Patch a few functions with know side-effects."""
    TestCaseUtils.setUp(self)
    mox.MoxTestBase.setUp(self)
    os_to_mock = ('chdir', 'chown', 'close', 'closerange', 'dup', 'dup2',
      'fchdir', 'fchmod', 'fchown', 'fdopen', 'getcwd', 'listdir', 'lseek',
      'makedirs', 'mkdir', 'open', 'popen', 'popen2', 'popen3', 'popen4',
      'read', 'remove', 'removedirs', 'rename', 'renames', 'rmdir', 'symlink',
      'system', 'tmpfile', 'walk', 'write')
    self.MockList(os, os_to_mock)
    os_path_to_mock = ('abspath', 'exists', 'getsize', 'isdir', 'isfile',
      'islink', 'ismount', 'lexists', 'realpath', 'samefile', 'walk')
    self.MockList(os.path, os_path_to_mock)
    self.MockList(shutil, ('rmtree'))
    self.MockList(subprocess, ('call', 'Popen'))
    # Don't mock stderr since it confuses unittests.
    self.MockList(sys, ('stdin'))
    StdoutCheck.setUp(self)

  def tearDown(self):
    StdoutCheck.tearDown(self)
    TestCaseUtils.tearDown(self)
    mox.MoxTestBase.tearDown(self)

  def MockList(self, parent, items_to_mock):
    for item in items_to_mock:
      # Skip over items not present because of OS-specific implementation,
      # implemented only in later python version, etc.
      if hasattr(parent, item):
        try:
          self.mox.StubOutWithMock(parent, item)
        except TypeError, e:
          raise TypeError(
              'Couldn\'t mock %s in %s: %s' % (item, parent.__name__, e))

  def UnMock(self, obj, name):
    """Restore an object inside a test."""
    for (parent, old_child, child_name) in self.mox.stubs.cache:
      if parent == obj and child_name == name:
        setattr(parent, child_name, old_child)
        break
