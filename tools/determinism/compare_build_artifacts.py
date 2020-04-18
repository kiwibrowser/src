#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Compare the artifacts from two builds."""

import ast
import difflib
import json
import optparse
import os
import re
import shutil
import struct
import subprocess
import sys
import time

BASE_DIR = os.path.dirname(os.path.abspath(__file__))


def get_files_to_compare(build_dir, recursive=False):
  """Get the list of files to compare."""
  # TODO(maruel): Add '.pdb'.
  allowed = frozenset(
      ('', '.apk', '.app', '.dll', '.dylib', '.exe', '.nexe', '.so'))
  non_x_ok_exts = frozenset(('.apk', '.isolated'))
  def check(f):
    if not os.path.isfile(f):
      return False
    if os.path.basename(f).startswith('.'):
      return False
    ext = os.path.splitext(f)[1]
    if ext in non_x_ok_exts:
      return True
    return ext in allowed and os.access(f, os.X_OK)

  ret_files = set()
  for root, dirs, files in os.walk(build_dir):
    if not recursive:
      dirs[:] = [d for d in dirs if d.endswith('_apk')]
    for f in (f for f in files if check(os.path.join(root, f))):
      ret_files.add(os.path.relpath(os.path.join(root, f), build_dir))
  return ret_files


def diff_dict(a, b):
  """Returns a yaml-like textural diff of two dict.

  It is currently optimized for the .isolated format.
  """
  out = ''
  for key in set(a) | set(b):
    va = a.get(key)
    vb = b.get(key)
    if va.__class__ != vb.__class__:
      out += '- %s:  %r != %r\n' % (key, va, vb)
    elif isinstance(va, dict):
      c = diff_dict(va, vb)
      if c:
        out += '- %s:\n%s\n' % (
            key, '\n'.join('  ' + l for l in c.splitlines()))
    elif va != vb:
      out += '- %s:  %s != %s\n' % (key, va, vb)
  return out.rstrip()


def diff_binary(first_filepath, second_filepath, file_len):
  """Returns a compact binary diff if the diff is small enough."""
  BLOCK_SIZE = 8192
  CHUNK_SIZE = 32
  NUM_CHUNKS_IN_BLOCK = BLOCK_SIZE / CHUNK_SIZE
  MAX_STREAMS = 10
  diffs = 0
  streams = []
  offset = 0
  with open(first_filepath, 'rb') as lhs:
    with open(second_filepath, 'rb') as rhs:
      # Skip part of Win32 COFF header if timestamps are different.
      #
      # COFF header:
      #   0 -  1: magic.
      #   2 -  3: # sections.
      #   4 -  7: timestamp.
      #   ....
      #
      # COFF BigObj header:
      #   0 -  3: signature (0000 FFFF)
      #   4 -  5: version
      #   6 -  7: machine
      #   8 - 11: timestamp.
      COFF_HEADER_TO_COMPARE_SIZE = 12
      if (sys.platform == 'win32'
          and os.path.splitext(first_filepath)[1] in ('.o', '.obj')
          and file_len > COFF_HEADER_TO_COMPARE_SIZE):
        rhs_data = rhs.read(COFF_HEADER_TO_COMPARE_SIZE)
        lhs_data = lhs.read(COFF_HEADER_TO_COMPARE_SIZE)
        if (lhs_data[0:4] == rhs_data[0:4] and lhs_data[4:8] != rhs_data[4:8]
            and lhs_data[8:12] == rhs_data[8:12]):
          offset += COFF_HEADER_TO_COMPARE_SIZE
        elif (lhs_data[0:4] == '\x00\x00\xff\xff' and
              lhs_data[0:8] == rhs_data[0:8] and
              lhs_data[8:12] != rhs_data[8:12]):
          offset += COFF_HEADER_TO_COMPARE_SIZE
        else:
          lhs.seek(0)
          rhs.seek(0)

      while True:
        lhs_data = lhs.read(BLOCK_SIZE)
        rhs_data = rhs.read(BLOCK_SIZE)
        if not lhs_data:
          break
        if lhs_data != rhs_data:
          diffs += sum(l != r for l, r in zip(lhs_data, rhs_data))
          for idx in xrange(NUM_CHUNKS_IN_BLOCK):
            lhs_chunk = lhs_data[idx * CHUNK_SIZE:(idx + 1) * CHUNK_SIZE]
            rhs_chunk = rhs_data[idx * CHUNK_SIZE:(idx + 1) * CHUNK_SIZE]
            if streams is not None and lhs_chunk != rhs_chunk:
              if len(streams) < MAX_STREAMS:
                streams.append((offset + CHUNK_SIZE * idx,
                                lhs_chunk, rhs_chunk))
              else:
                streams = None
        offset += len(lhs_data)
        del lhs_data
        del rhs_data
  if not diffs:
    return None
  result = '%d out of %d bytes are different (%.2f%%)' % (
        diffs, file_len, 100.0 * diffs / file_len)
  if streams:
    encode = lambda text: ''.join(i if 31 < ord(i) < 127 else '.' for i in text)
    for offset, lhs_data, rhs_data in streams:
      lhs_line = '%s \'%s\'' % (lhs_data.encode('hex'), encode(lhs_data))
      rhs_line = '%s \'%s\'' % (rhs_data.encode('hex'), encode(rhs_data))
      diff = list(difflib.Differ().compare([lhs_line], [rhs_line]))[-1][2:-1]
      result += '\n  0x%-8x: %s\n              %s\n              %s' % (
            offset, lhs_line, rhs_line, diff)
  return result


def compare_files(first_filepath, second_filepath):
  """Compares two binaries and return the number of differences between them.

  Returns None if the files are equal, a string otherwise.
  """
  if first_filepath.endswith('.isolated'):
    with open(first_filepath, 'rb') as f:
      lhs = json.load(f)
    with open(second_filepath, 'rb') as f:
      rhs = json.load(f)
    diff = diff_dict(lhs, rhs)
    if diff:
      return '\n' + '\n'.join('  ' + line for line in diff.splitlines())
    # else, falls through binary comparison, it must be binary equal too.

  file_len = os.stat(first_filepath).st_size
  if file_len != os.stat(second_filepath).st_size:
    return 'different size: %d != %d' % (
        file_len, os.stat(second_filepath).st_size)

  return diff_binary(first_filepath, second_filepath, file_len)


def get_deps(build_dir, target):
  """Returns list of object files needed to build target."""
  NODE_PATTERN = re.compile(r'label="([a-zA-Z0-9_\\/.-]+)"')
  CHECK_EXTS = ('.o', '.obj')

  # Rename to possibly original directory name if possible.
  fixed_build_dir = build_dir
  if build_dir.endswith('.1') or build_dir.endswith('.2'):
    fixed_build_dir = build_dir[:-2]
    if os.path.exists(fixed_build_dir):
      print >> sys.stderr, ('fixed_build_dir %s exists.'
                            ' will try to use orig dir.' % fixed_build_dir)
      fixed_build_dir = build_dir
    else:
      shutil.move(build_dir, fixed_build_dir)

  try:
    out = subprocess.check_output(['ninja', '-C', fixed_build_dir,
                                   '-t', 'graph', target])
  except subprocess.CalledProcessError as e:
    print >> sys.stderr, 'error to get graph for %s: %s' % (target, e)
    return []

  finally:
    # Rename again if we renamed before.
    if fixed_build_dir != build_dir:
      shutil.move(fixed_build_dir, build_dir)

  files = []
  for line in out.splitlines():
    matched = NODE_PATTERN.search(line)
    if matched:
      path = matched.group(1)
      if not os.path.splitext(path)[1] in CHECK_EXTS:
        continue
      if os.path.isabs(path):
        print >> sys.stderr, ('not support abs path %s used for target %s'
                              % (path, target))
        continue
      files.append(path)
  return files


def compare_deps(first_dir, second_dir, targets):
  """Print difference of dependent files."""
  diffs = set()
  for target in targets:
    first_deps = get_deps(first_dir, target)
    second_deps = get_deps(second_dir, target)
    print 'Checking %s difference: (%s deps)' % (target, len(first_deps))
    if set(first_deps) != set(second_deps):
      # Since we do not thiks this case occur, we do not do anything special
      # for this case.
      print 'deps on %s are different: %s' % (
          target, set(first_deps).symmetric_difference(set(second_deps)))
      continue
    max_filepath_len = max(len(n) for n in first_deps)
    for d in first_deps:
      first_file = os.path.join(first_dir, d)
      second_file = os.path.join(second_dir, d)
      result = compare_files(first_file, second_file)
      if result:
        print('  %-*s: %s' % (max_filepath_len, d, result))
        diffs.add(d)
  return list(diffs)


def compare_build_artifacts(first_dir, second_dir, target_platform,
                            json_output, recursive=False):
  """Compares the artifacts from two distinct builds."""
  if not os.path.isdir(first_dir):
    print >> sys.stderr, '%s isn\'t a valid directory.' % first_dir
    return 1
  if not os.path.isdir(second_dir):
    print >> sys.stderr, '%s isn\'t a valid directory.' % second_dir
    return 1

  epoch_hex = struct.pack('<I', int(time.time())).encode('hex')
  print('Epoch: %s' %
      ' '.join(epoch_hex[i:i+2] for i in xrange(0, len(epoch_hex), 2)))

  with open(os.path.join(BASE_DIR, 'deterministic_build_blacklist.json')) as f:
    blacklist = frozenset(json.load(f))
  with open(os.path.join(BASE_DIR, 'deterministic_build_whitelist.pyl')) as f:
    whitelist = frozenset(ast.literal_eval(f.read())[target_platform])

  # The two directories.
  first_list = get_files_to_compare(first_dir, recursive) - blacklist
  second_list = get_files_to_compare(second_dir, recursive) - blacklist

  equals = []
  expected_diffs = []
  unexpected_diffs = []
  unexpected_equals = []
  all_files = sorted(first_list & second_list)
  missing_files = sorted(first_list.symmetric_difference(second_list))
  if missing_files:
    print >> sys.stderr, 'Different list of files in both directories:'
    print >> sys.stderr, '\n'.join('  ' + i for i in missing_files)
    unexpected_diffs.extend(missing_files)

  max_filepath_len = max(len(n) for n in all_files)
  for f in all_files:
    first_file = os.path.join(first_dir, f)
    second_file = os.path.join(second_dir, f)
    result = compare_files(first_file, second_file)
    if not result:
      tag = 'equal'
      equals.append(f)
      if f in whitelist:
        unexpected_equals.append(f)
    else:
      if f in whitelist:
        expected_diffs.append(f)
        tag = 'expected'
      else:
        unexpected_diffs.append(f)
        tag = 'unexpected'
      result = 'DIFFERENT (%s): %s' % (tag, result)
    print('%-*s: %s' % (max_filepath_len, f, result))
  unexpected_diffs.sort()

  print('Equals:           %d' % len(equals))
  print('Expected diffs:   %d' % len(expected_diffs))
  print('Unexpected diffs: %d' % len(unexpected_diffs))
  if unexpected_diffs:
    print('Unexpected files with diffs:\n')
    for u in unexpected_diffs:
      print('  %s' % u)
  if unexpected_equals:
    print('Unexpected files with no diffs:\n')
    for u in unexpected_equals:
      print('  %s' % u)

  all_diffs = expected_diffs + unexpected_diffs
  diffs_to_investigate = sorted(set(all_diffs).difference(missing_files))
  deps_diff = compare_deps(first_dir, second_dir, diffs_to_investigate)

  if json_output:
    try:
      out = {
          'expected_diffs': expected_diffs,
          'unexpected_diffs': unexpected_diffs,
          'deps_diff': deps_diff,
      }
      with open(json_output, 'w') as f:
        json.dump(out, f)
    except Exception as e:
      print('failed to write json output: %s' % e)

  return int(bool(unexpected_diffs))


def main():
  parser = optparse.OptionParser(usage='%prog [options]')
  parser.add_option(
      '-f', '--first-build-dir', help='The first build directory.')
  parser.add_option(
      '-s', '--second-build-dir', help='The second build directory.')
  parser.add_option('-r', '--recursive', action='store_true', default=False,
                    help='Indicates if the comparison should be recursive.')
  parser.add_option('--json-output', help='JSON file to output differences')
  target = {
      'darwin': 'mac', 'linux2': 'linux', 'win32': 'win'
  }.get(sys.platform, sys.platform)
  parser.add_option('-t', '--target-platform', help='The target platform.',
                    default=target, choices=('android', 'mac', 'linux', 'win'))
  options, _ = parser.parse_args()

  if not options.first_build_dir:
    parser.error('--first-build-dir is required')
  if not options.second_build_dir:
    parser.error('--second-build-dir is required')
  if not options.target_platform:
    parser.error('--target-platform is required')

  return compare_build_artifacts(os.path.abspath(options.first_build_dir),
                                 os.path.abspath(options.second_build_dir),
                                 options.target_platform,
                                 options.json_output,
                                 options.recursive)


if __name__ == '__main__':
  sys.exit(main())
