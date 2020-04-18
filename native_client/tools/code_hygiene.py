#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Simple Code Hygiene tool meant to be run as a presubmit test or standalone

Usage:
./code_hygiene.py <file1> <file2> ...

For the styleguides see:
http://code.google.com/p/soc/wiki/PythonStyleGuide
http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml
"""

import os
import re
import subprocess
import sys

# ======================================================================
VERBOSE = 0
LIMIT = 5

# These programs are likely not available/working on windows.

# http://tidy.sourceforge.net/
HTML_CHECKER = ['tidy', '-errors']

# From depot_tools:
# To see a list of all filters run: 'depot_tools/cpplint.py --filter='
CPP_CHECKER = ['cpplint.py',
               '--filter='
               '-build/header_guard'
               ',-build/class'
               ',-build/include_order'
               ',-readability/casting'
               ',-readability/check'
               ',-readability/multiline_comment'
               ',-readability/streams'
               ',-runtime/sizeof'
               ',-runtime/threadsafe_fn'
               ',-whitespace/newline'
               ]

# From depot_tools (currently not used -- too many false positives).
# To see a list of all filters run: 'depot_tools/cpplint.py --filter='
C_CHECKER = ['cpplint.py',
             '--filter='
             '-build/header_guard'
             ',-build/class'
             ',-build/include_order'
             ',-readability/casting'
             ',-readability/check'
             ',-readability/multiline_comment'
             ',-runtime/sizeof'
             ',-runtime/threadsafe_fn'
             ',-whitespace/newline'
             ]

# http://pychecker.sourceforge.net/
PYTHON_CHECKER = ['pychecker', '-stdlib']

RE_IGNORE = re.compile(r'@IGNORE_LINES_FOR_CODE_HYGIENE\[([0-9]+)\]')

# ======================================================================
def Debug(s):
  if VERBOSE:
    print s


def RunCommand(cmd):
  """Run a shell command given an argv style vector."""
  Debug(str(cmd))

  p = subprocess.Popen(cmd,
                       bufsize=1000*1000,
                       stderr=subprocess.PIPE,
                       stdout=subprocess.PIPE)
  stdout, stderr = p.communicate()
  retcode = p.wait()

  return retcode, stdout, stderr


# ======================================================================
class ExternalChecker(object):
  """Base Class"""
  def __init__(self, commandline, use_stderr=False):
    self._commandline = commandline
    self._use_stderr = use_stderr

  def FindProblems(self, props, unused_data):
    filename = props['name']
    try:
      retcode, stdout, stderr = RunCommand(self._commandline + [filename])
    except Exception, err:
      print 'Error processing %s:' % filename
      print '  Cannot execute command %s. failed reason: %s ' % (
          str(self._commandline), str(err))
      return []
    if retcode == 0:
      return []
    if self._use_stderr:
      return [stderr]
    else:
      return [stdout]

  def FileFilter(self, unused_props):
    raise NotImplementedError()


class TidyChecker(ExternalChecker):
  """Invokes tidy tool on html files."""
  def __init__(self):
    ExternalChecker.__init__(self, HTML_CHECKER, use_stderr=True)

  def FileFilter(self, props):
    return '.html' in props


class PyChecker(ExternalChecker):
  """Invokes pychecker tool on python files."""
  def __init__(self):
    ExternalChecker.__init__(self, PYTHON_CHECKER)

  def FileFilter(self, props):
    return '.py' in props


class CppLintChecker(ExternalChecker):
  """Invokes Google's cpplint tool on c++ files."""
  def __init__(self):
    ExternalChecker.__init__(self, CPP_CHECKER, use_stderr=True)

  def FileFilter(self, props):
    return '.cc' in props or '.h' in props


class NotForCommitChecker(object):
  def FindProblems(self, unused_props, data):
    return ['This file should not be committed!']

  def FileFilter(self, props):
    return 'not_for_commit' in props['name']


# ======================================================================
class GenericRegexChecker(object):
  """Base Class"""
  def __init__(self, content_regex, line_regex=None,
               analyze_match=False, message=''):
    self._content_regex = re.compile(content_regex, re.MULTILINE)
    if line_regex:
      self._line_regex = re.compile(line_regex)
    else:
      self._line_regex = re.compile(content_regex)
    self._analyze_match = analyze_match
    self._message = message

  def FindProblems(self, unused_props, data):
    """Looks for presubmit issues in the data from a file."""
    # Speedy early out.
    if not self._content_regex.search(data):
      return []

    lines = data.split('\n')
    problem = []
    ignore_line_count = 0
    for no, line in enumerate(lines):
      if ignore_line_count > 0:
        ignore_line_count -= 1
        continue
      match = RE_IGNORE.search(line)
      if match:
        ignore_line_count = int(match.group(1))
        continue
      match = self._line_regex.search(line)
      if not match:
        continue
      if self._analyze_match:
        if not self.IsProblemMatch(match):
          continue
      # 'no' is zero-based, use 'no + 1' to get the actual line numbers.
      problem.append(self._RenderItem(no + 1, line))
    return problem

  def _RenderItem(self, no, line):
    """Returns a formatted message."""
    return 'line %d: [%s] %s' % (no, repr(line), self._message)

  def FileFilter(self, unused_props):
    """Returns true if the file should be checked by the checker."""
    # NOTE: All subclasses need to implement this and return True if the
    # input Properties show that this file should be checked.
    raise NotImplementedError()

  def IsProblemMatch(self, unused_match):
    """Returns true if the file contains a presubmit problem."""
    # NOTE: Not all subclasses need to implement this.
    pass


class TrailingWhiteSpaceChecker(GenericRegexChecker):
  """No trailing whitespaces in code we control."""
  def __init__(self):
    GenericRegexChecker.__init__(self, r' $')

  def FileFilter(self, props):
    return ('.patch' not in props and
            '.stdout' not in props and
            '.table' not in props)


class CarriageReturnChecker(GenericRegexChecker):
  """Abolish windows style line terminators."""
  def __init__(self):
    GenericRegexChecker.__init__(self, r'\r')

  def FileFilter(self, props):
    return '.h' in props or '.cc' in props or '.c' in props or '.py' in props


class CppCommentChecker(GenericRegexChecker):
  """No cpp comments in regular c files."""
  def __init__(self):
    # We tolerate http:// etc in comments
    GenericRegexChecker.__init__(self, r'(^|[^:])//')

  def FileFilter(self, props):
    return '.c' in props


class TabsChecker(GenericRegexChecker):
  """No tabs except for makefiles."""
  def __init__(self):
    GenericRegexChecker.__init__(self, r'\t')

  def FileFilter(self, props):
    return ('is_makefile' not in props and
            '.patch' not in props and
            '.valerr' not in props and
            '.dis' not in props)


class FixmeChecker(GenericRegexChecker):
  """No FIXMEs in code we control."""
  def __init__(self):
    GenericRegexChecker.__init__(self, r'\bFIXME\b')

  def FileFilter(self, props):
    return '.patch' not in props


class RewriteChecker(GenericRegexChecker):
  """No rewrite markers allowed (probaly not an issue anymore)."""
  def __init__(self):
    GenericRegexChecker.__init__(self, r'[@][@]REWRITE')

  def FileFilter(self, unused_props):
    return 1


VALID_INCLUDE_PREFIX = [
    'base/',
    'breakpad/',
    'chrome/common/',
    'chrome/renderer/',
    'gen/native_client/',
    'gpu/',
    'gtest/',
    'native_client/',
    'ppapi/',
    'srpcgen/',
    'third_party/',
    'trusted/srpcgen/',
    'untrusted/srpcgen/',
    ]


class IncludeChecker(GenericRegexChecker):
  """Enforce some base 'include' rules. Cpplint does more of these."""
  def __init__(self):
    GenericRegexChecker.__init__(self, r'#include',
                                 line_regex=r'^(#include.*|})',
                                 analyze_match=True)

  def FindProblems(self, props, data):
    self._seen_code = False
    return GenericRegexChecker.FindProblems(self, props, data)

  def IsProblemMatch(self, match):
    if match.group(1) == '}':
      # This test is our hacked indicator signaling that no includes
      # directives should follow after we have seen actual code.
      self._seen_code = True
      return False

    if self._seen_code:
      return True
    token = match.group(1).split()
    if len(token) < 2:
      return False
    path = token[1][1:]
    if '..' in path:
      return True
    if not token[1].startswith('<'):
      for prefix in VALID_INCLUDE_PREFIX:
        if path.startswith(prefix):
          break
      else:
        return True
    return False

  def FileFilter(self, props):
    return '.cc' in props or '.c' in props


class BuildConfigChecker(GenericRegexChecker):
  """Enforce that build_config.h is include when some macros are present.

  build_config.h is required as a header in many sources to provide macro
  definitions for the operating system and architecture. This presubmit check
  exists to ensure that it's never accidentally omitted in source files.

  This is implemented as a presubmit as there is no usable mechanism to warn on
  undefined macro usage on Windows.
  """

  def __init__(self):
    GenericRegexChecker.__init__(
        self,
        'include "native_client/src/include/build_config.h"|' +
        'NACL_ANDROID|NACL_LINUX|NACL_OSX|NACL_WINDOWS|' +
        'NACL_BUILD_ARCH|NACL_BUILD_SUBARCH',
        analyze_match=True,
        message='must include "native_client/src/include/build_config.h"')

  def FindProblems(self, props, data):
    # Don't check build_config.h itself. The path is shortened here to make
    # this work on trybots.
    if 'src/include/build_config.h' in props['name']:
      return False
    self._seen_include = False
    return GenericRegexChecker.FindProblems(self, props, data)

  def IsProblemMatch(self, match):
    if 'include "native_client/src/include/build_config.h"' in match.group():
      self._seen_include = True
    return not self._seen_include

  def FileFilter(self, props):
    return '.c' in props or '.cc' in props or '.h' in props or '.S' in props


# ======================================================================
# 'props' (short for properties) are tags associated with checkable files.
#
# Except for the 'name', props do not carry any associated value.
#
# Props are used to make decisions on whether (and how) a certain check is
# applicable.
#
# Props are usually derived from the pathname but can also be forced using
# the magic string '@PROPS[prop1,prop2,...]' anywhere in the first 2k of
# data.

VALID_PROPS = {
    'name': True,                # Filename, only prop with a value.
    'is_makefile': True,
    'is_trusted': True,          # Is trusted code.
    'is_untrusted': True,        # Is untrusted code.
    'is_shared': True,           # Is shared code.
    'is_scons': True,            # Is scons file (not one generated by gyp).
}


def IsValidProp(prop):
  """Returns true if the property name is valid.

  Valid property names start with '.' or are listed in VALID_PROPS.

  Args:
    prop: a property name.
  Returns:
    True if the property name is valid.
  """
  if prop.startswith('.'):
    return True
  return prop in VALID_PROPS


def FindProperties(data):
  """Finds extra properties for a file.

  Finds strings of the form '@PROPS[name name name]'.

  Args:
    data: The contents of the file being checked.
  Returns:
    A list of properties from the PROPS line in the file.
  """
  match = re.search(r'@PROPS\[([^\]]*)\]', data)
  if match:
    return match.group(1).split(',')
  return []


def FileProperties(filename, data):
  """Finds all properties associated with this file.

  Valid properties are based on the file's name and on the '@PROPS' line in
  the file.

  Args:
    filename: the filename.
    data: The contents of the file being checked.
  Returns:
    A map of property to value for each property associated with this file.
  """
  d = {}
  d['name'] = filename
  _, ext = os.path.splitext(filename)
  if ext:
    d[ext] = True

  if 'akefile' in filename:
    d['is_makefile'] = True

  if 'src/trusted/' in filename:
    d['is_trusted'] = True
  if 'src/untrusted/' in filename:
    d['is_untrusted'] = True
  if 'src/shared/' in filename:
    d['is_shared'] = True

  if (filename.endswith('nacl.scons') or
      filename.endswith('build.scons') or
      filename.endswith('SConstruct')):
      d['is_scons'] = True

  # NOTE: Look in a somewhat arbitrary region at the file beginning for
  #       a special marker advertising addtional properties.
  for p in FindProperties(data[:2048]):
    d[p] = True

  for p in d:
    assert IsValidProp(p)
  return d


# ======================================================================
def IsBinary(data):
  """Checks for a 0x00 byte as a proxy for 'binary-ness'."""
  return '\x00' in data


# ======================================================================
# CHECK entry is: is_fatal, name, check_function.
CHECKS = [# fatal checks
          (True, 'not_for_commit_files', NotForCommitChecker()),
          (True, 'tabs', TabsChecker()),
          (True, 'trailing_space', TrailingWhiteSpaceChecker()),
          (True, 'cpp_comment', CppCommentChecker()),
          (True, 'fixme', FixmeChecker()),
          (True, 'include', IncludeChecker()),
          (True, 'build_config', BuildConfigChecker()),
          # Non fatal checks
          (False, 'carriage_return', CarriageReturnChecker()),
          (False, 'rewrite', RewriteChecker()),
          (False, 'tidy', TidyChecker()),
          (False, 'pychecker', PyChecker()),
          (False, 'cpplint', CppLintChecker()),
          ]


def EmitStatus(filename, status, details=[]):
  """Prints the status and all supporting details for a file."""
  print '%s: %s' % (filename, status)
  for no, line in details[:LIMIT]:
    print '  line %d: [%s]' % (no, repr(line))


def FindExemptions(data):
  """Looks for and returns EXEMPTION items from the data.

  Finds strings of the form '@EXEMPTION[name name name]' where
  'name' is one of the names in the CHECKS table.

  Args:
    data: The contents of the file being checked.
  """
  match = re.search(r'@EXEMPTION\[([^\]]*)\]', data)
  if match:
    return match.group(1).split(',')
  return []


def CheckFile(filename, report):
  """Runs all possible presubmit checks against the filename."""
  Debug('\n' + filename)
  errors = {}
  warnings = {}

  data = open(filename).read()

  if IsBinary(data):
    EmitStatus(filename, 'binary')
    return errors, warnings

  exemptions = FindExemptions(data)
  props = FileProperties(filename, data)
  Debug('PROPS: ' + repr(props.keys()))

  for fatal, name, check in CHECKS:
    Debug('trying %s' % name)

    if not check.FileFilter(props):
      Debug('skipping')
      continue

    if name in exemptions:
      Debug('exempt')
      continue

    problems = check.FindProblems(props, data)
    if problems:
      info = '%s: %s' % (filename, name)
      items = problems[:LIMIT]
      if report:
        print info
        for i in items:
          print i
      if fatal:
        errors[info] = items
        break
      else:
        warnings[info] = items
  return errors, warnings


def main(argv):
  """Runs all required presubmit checks for all listed files."""
  num_error = 0
  for filename in argv:
    if os.path.isdir(filename):
      EmitStatus(filename, 'directory')
      continue

    if '-' in os.path.basename(filename):
      print 'hyphen in:', filename
      num_error += 1
      continue

    errors, warnings = CheckFile(filename, True)
    if errors:
      num_error += 1
  return num_error

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
