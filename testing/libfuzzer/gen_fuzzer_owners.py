#!/usr/bin/env python2.7
#
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Generate owners (.owners file) by looking at commit author for
libfuzzer test.

Invoked by GN from fuzzer_test.gni.
"""

import argparse
import os
import re
import subprocess
import sys

AUTHOR_REGEX = re.compile('author-mail <(.+)>')
CHROMIUM_SRC_DIR = os.path.dirname(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OWNERS_FILENAME = 'OWNERS'
THIRD_PARTY_SEARCH_STRING = 'third_party' + os.sep


def GetAuthorFromGitBlame(blame_output):
  """Return author from git blame output."""
  for line in blame_output.splitlines():
    m = AUTHOR_REGEX.match(line)
    if m:
      return m.group(1)

  return None


def GetOwnersIfThirdParty(source):
  """Return owners using OWNERS file if in third_party."""
  match_index = source.find(THIRD_PARTY_SEARCH_STRING)
  if match_index == -1:
    # Not in third_party, skip.
    return None

  match_index_with_library = source.find(
      os.sep, match_index + len(THIRD_PARTY_SEARCH_STRING))
  if match_index_with_library == -1:
    # Unable to determine library name, skip.
    return None

  owners_file_path = os.path.join(source[:match_index_with_library],
                                  OWNERS_FILENAME)
  if not os.path.exists(owners_file_path):
    return None

  return open(owners_file_path).read()


def GetOwnersForFuzzer(sources):
  """Return owners given a list of sources as input."""
  if not sources:
    return

  for source in sources:
    if not os.path.exists(source):
      continue

    with open(source, 'r') as source_file_handle:
      source_content = source_file_handle.read()

    if SubStringExistsIn(
        ['FuzzOneInput', 'LLVMFuzzerTestOneInput', 'PROTO_FUZZER'],
        source_content):
      # Found the fuzzer source (and not dependency of fuzzer).

      is_git_file = bool(subprocess.check_output(['git', 'ls-files', source]))
      if not is_git_file:
        # File is not in working tree. Return owners for third_party.
        return GetOwnersIfThirdParty(source)

      # git log --follow and --reverse don't work together and using just
      # --follow is too slow. Make a best estimate with an assumption that
      # the original author has authored line 1 which is usually the
      # copyright line and does not change even with file rename / move.
      blame_output = subprocess.check_output(
          ['git', 'blame', '--porcelain', '-L1,1', source])
      return GetAuthorFromGitBlame(blame_output)

  return None


def GetSourcesFromDeps(deps_list, build_dir):
  """Return list of sources from parsing deps."""
  if not deps_list:
    return None

  all_sources = []
  for deps in deps_list:
    output = subprocess.check_output(
        [GNPath(), 'desc', build_dir, deps, 'sources'])
    for source in output.splitlines():
      if source.startswith('//'):
        source = source[2:]
      actual_source = os.path.join(CHROMIUM_SRC_DIR, source)
      all_sources.append(actual_source)

  return all_sources


def GNPath():
  if sys.platform.startswith('linux'):
    subdir, exe = 'linux64', 'gn'
  elif sys.platform == 'darwin':
    subdir, exe = 'mac', 'gn'
  else:
    subdir, exe = 'win', 'gn.exe'

  return os.path.join(CHROMIUM_SRC_DIR, 'buildtools', subdir, exe)


def SubStringExistsIn(substring_list, string):
  """Return true if one of the substring in the list is found in |string|."""
  for substring in substring_list:
    if substring in string:
      return True

  return False


def main():
  parser = argparse.ArgumentParser(description='Generate fuzzer owners file.')
  parser.add_argument('--owners', required=True)
  parser.add_argument('--build-dir')
  parser.add_argument('--deps', nargs='+')
  parser.add_argument('--sources', nargs='+')
  args = parser.parse_args()

  # Generate owners file.
  with open(args.owners, 'w') as owners_file:
    # If we found an owner, then write it to file.
    # Otherwise, leave empty file to keep ninja happy.
    owners = GetOwnersForFuzzer(args.sources)
    if owners:
      owners_file.write(owners)
      return

    # Could not determine owners from |args.sources|.
    # So, try parsing sources from |args.deps|.
    deps_sources = GetSourcesFromDeps(args.deps, args.build_dir)
    owners = GetOwnersForFuzzer(deps_sources)
    if owners:
      owners_file.write(owners)


if __name__ == '__main__':
  main()
