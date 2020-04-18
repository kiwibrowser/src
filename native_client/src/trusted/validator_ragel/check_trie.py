#!/usr/bin/python
# Copyright (c) 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Verifies that a trie is equal to the latest snapshotted one."""

import argparse
import os
import sys

# Add location of latest_trie module to sys.path:
RAGEL_DIR = os.path.dirname(os.path.abspath(__file__))
NACL_DIR = os.path.dirname(os.path.dirname(os.path.dirname(RAGEL_DIR)))
SNAPSHOTS_DIR = os.path.join(os.path.dirname(NACL_DIR), 'validator_snapshots')
if os.path.isdir(SNAPSHOTS_DIR):
  sys.path.append(SNAPSHOTS_DIR)
else:
  print "couldn't find: ", SNAPSHOTS_DIR
  sys.exit(1)

import latest_trie
import trie


def ParseArgs():
  parser = argparse.ArgumentParser(description='Compare to latest trie.')
  parser.add_argument('trie', metavar='t', nargs=1,
                      help='path of the trie to check.')
  parser.add_argument('--bitness', type=int, required=True,
                      choices=[32, 64])
  return parser.parse_args()


def main():
  args = ParseArgs()
  golden = latest_trie.LatestRagelTriePath(SNAPSHOTS_DIR, args.bitness)
  diff_list = [diff for diff in trie.DiffTrieFiles(args.trie[0], golden)]
  if diff_list:
    print 'tries differ: ', diff_list
    sys.exit(1)


if __name__ == '__main__':
  main()
