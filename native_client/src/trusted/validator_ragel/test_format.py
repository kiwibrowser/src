# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import optparse
import re


def ParseTest(lines):
  r"""Parses section-based test.

  Args:
    lines: list of \n-terminated strings.

  Returns:
    List of string pairs (field name, field content) in order. Field content is
    concatenation of \n-terminated lines, so it's either empty or ends with \n.
  """
  fields = []
  field_data = {}
  current_field = None

  for line in lines:
    if line.startswith('  '):
      assert current_field is not None, line
      field_data[current_field].append(line[2:])
    else:
      match = re.match('@(\S+):$', line)
      if match is None:
        raise Exception('Bad line: %r' % line)
      current_field = match.group(1)
      assert current_field not in field_data, current_field
      field_data[current_field] = []
      fields.append(current_field)

  return [(field, ''.join(field_data[field])) for field in fields]


def SplitLines(lines, separator_regex):
  """Split sequence of lines into sequence of list of lines.

  Args:
    lines: sequence of strings.
    separator_regex: separator regex.

  Yields:
    Nonempty sequence of (possibly empty) lists of strings. Separator lines
    are not included.
  """
  part = []
  for line in lines:
    if re.match(separator_regex, line):
      yield part
      part = []
    else:
      part.append(line)
  yield part


def LoadTestFile(filename):
  r"""Loads and parses .test file.

  Args:
    filename: filename.

  Returns:
    List of tests (see ParseTest).
  """
  with open(filename) as file_in:
    return map(ParseTest, SplitLines(file_in, r'-{3,}\s*$'))


def UnparseTest(items_list):
  """Convert test to sequence of \n-terminated strings

  Args:
    items_list: list of string pairs (see ParseTest).

  Yields:
    Sequence of \n-terminated strings.
  """
  for field, content in items_list:
    yield '@%s:\n' % field
    if content == '':
      continue

    assert content.endswith('\n')
    content = content[:-1]

    for line in content.split('\n'):
      yield '  %s\n' % line


def SaveTestFile(tests, filename):
  r"""Saves .test file

  Args:
    tests: list of tests (see ParseTest).
    filename: filename.
  Returns:
    None.
  """
  with open(filename, 'w') as file_out:
    first = True
    for test in tests:
      if not first:
        file_out.write('-' * 70 + '\n')
      first = False
      for line in UnparseTest(test):
        file_out.write(line)


def ParseHex(hex_content):
  """Parse content of @hex section and return binary data

  Args:
    hex_content: Content of @hex section as a string.

  Yields:
    Chunks of binary data corresponding to lines of given @hex section (as
    strings). If line ends with r'\\', chunk is continued on the following line.
  """

  bytes = []
  for line in hex_content.split('\n'):
    line, sep, comment = line.partition('#')
    line = line.strip()
    if line == '':
      continue

    if line.endswith(r'\\'):
      line = line[:-2]
      continuation = True
    else:
      continuation = False

    for byte in line.split():
      assert len(byte) == 2
      bytes.append(chr(int(byte, 16)))

    if not continuation:
      assert len(bytes) > 0
      yield ''.join(bytes)
      bytes = []

  assert bytes == [], r'r"\\" should not appear on the last line'


def AssertEquals(actual, expected):
  if actual != expected:
    raise AssertionError('\nEXPECTED:\n%r\n\nACTUAL:\n%r\n'
                         % (expected, actual))


class TestRunner(object):

  SECTION_NAME = None

  def CommandLineOptions(self, parser):
    pass

  def GetSectionContent(self, options, sections):
    raise NotImplementedError()

  def Test(self, options, items_list):
    info = dict(items_list)
    assert self.SECTION_NAME in info

    content = self.GetSectionContent(options, info)
    print '  Checking %s field...' % self.SECTION_NAME
    if options.update:
      if content != info[self.SECTION_NAME]:
        print '  Updating %s field...' % self.SECTION_NAME
        info[self.SECTION_NAME] = content
    else:
      AssertEquals(content, info[self.SECTION_NAME])

    # Update field values, but preserve their order.
    items_list = [(field, info[field]) for field, _ in items_list]

    return items_list

  def Run(self, argv):
    parser = optparse.OptionParser()
    parser.add_option('--bits',
                      type=int,
                      help='The subarchitecture to run tests against: 32 or 64')
    parser.add_option('--update',
                      default=False,
                      action='store_true',
                      help='Regenerate golden fields instead of testing')
    self.CommandLineOptions(parser)

    options, args = parser.parse_args(argv)

    if options.bits not in [32, 64]:
      parser.error('specify --bits 32 or --bits 64')

    if len(args) == 0:
      parser.error('No test files specified')
    processed = 0
    for glob_expr in args:
      test_files = sorted(glob.glob(glob_expr))
      if len(test_files) == 0:
        raise AssertionError(
            '%r matched no files, which was probably not intended' % glob_expr)
      for test_file in test_files:
        print 'Testing %s...' % test_file
        tests = LoadTestFile(test_file)
        tests = [self.Test(options, test) for test in tests]
        if options.update:
          SaveTestFile(tests, test_file)
        processed += 1
    print '%s test files were processed.' % processed
